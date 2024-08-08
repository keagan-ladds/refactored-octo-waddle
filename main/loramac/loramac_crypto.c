#include "loramac_crypto.h"
#include "mbedtls/cmac.h"
#include "mbedtls/aes.h"
#include "mbedtls/cipher.h"
#include "esp_log.h"
#include <stdio.h>

#define LORAMAC_CRYPTO_TAG "loramac_crypto"

loramac_crypto_err_t loramac_crypto_aes_cmac(uint8_t *key, uint8_t *buffer, uint16_t buffer_length, uint32_t *cmac)
{
    unsigned char output[16];
    int ret;

    // Initialize AES context
    mbedtls_cipher_context_t ctx;
    mbedtls_cipher_init(&ctx);

    // Set up the AES CMAC context
    const mbedtls_cipher_info_t *cipher_info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB);
    if (cipher_info == NULL)
    {
        ESP_LOGE(LORAMAC_CRYPTO_TAG, "Failed to get cipher info\n");
        return 1;
    }

    ret = mbedtls_cipher_setup(&ctx, cipher_info);
    if (ret != 0)
    {
        ESP_LOGE(LORAMAC_CRYPTO_TAG, "Failed to set up cipher: %d\n", ret);
        return 1;
    }

    // Set the CMAC key
    ret = mbedtls_cipher_cmac_starts(&ctx, key, 128);
    if (ret != 0)
    {
        ESP_LOGE(LORAMAC_CRYPTO_TAG, "Failed to set CMAC key: %d\n", ret);
        return 1;
    }

    // Update the CMAC with the message
    ret = mbedtls_cipher_cmac_update(&ctx, buffer, buffer_length);
    if (ret != 0)
    {
        ESP_LOGE(LORAMAC_CRYPTO_TAG, "Failed to update CMAC: %d\n", ret);
        return 1;
    }

    // Finish the CMAC calculation
    ret = mbedtls_cipher_cmac_finish(&ctx, output);
    if (ret != 0)
    {
        ESP_LOGE(LORAMAC_CRYPTO_TAG, "Failed to finish CMAC: %d\n", ret);
        return 1;
    }

    // Free the AES context
    mbedtls_cipher_free(&ctx);

    uint32_t t = (uint32_t)output[3] << 24;
    t |= (uint32_t)output[2] << 16;
    t |= (uint32_t)output[1] << 8;
    t |= (uint32_t)output[0];

    *cmac = t;

    return 0;
}

loramac_crypto_err_t loramac_crypto_aes_encrypt(uint8_t *key, uint8_t *buffer, uint16_t buffer_length, uint8_t *encrypt_buffer)
{

    if (buffer_length % 16 != 0)
    {
        fprintf(stderr, "Input length must be a multiple of 16 bytes\n");
        return 1;
    }

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // Set the encryption key
    int ret = mbedtls_aes_setkey_enc(&aes, key, 128);

    for (size_t i = 0; i < buffer_length; i += 16)
    {
        ret = mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, &buffer[i], &encrypt_buffer[i]);
        if (ret != 0)
        {
            fprintf(stderr, "Failed to encrypt block %zu: %d\n", i / 16, ret);
            return 1;
        }
    }

    // Free the AES context
    mbedtls_aes_free(&aes);

    return 0;
}