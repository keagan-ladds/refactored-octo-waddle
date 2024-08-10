#include "loramac_crypto.h"
#include "mbedtls/cmac.h"
#include "mbedtls/aes.h"
#include "mbedtls/cipher.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "loramac_serializer.h"
#include "loramac_parser.h"
#include <stdlib.h>

#define LORAMAC_CRYPTO_TAG "loramac_crypto"

uint8_t app_key[] = {0x27, 0xE8, 0xC6, 0x0A, 0xAA, 0x4A, 0x86, 0x8A, 0xF0, 0xA8, 0x85, 0xD8, 0x0B, 0x65, 0x17, 0x57};

loramac_crypto_err_t loramac_crypto_nvm_init(void);
loramac_crypto_err_t loramac_crypto_aes_verify_cmac(uint8_t *key, uint8_t *buffer, uint16_t buffer_length, uint32_t cmac);
loramac_crypto_err_t loramac_crypto_derive_key(uint8_t *app_key, uint8_t key_type, uint8_t *app_nonce, uint8_t *net_id, uint16_t dev_nonce);
loramac_crypto_err_t loramac_crypto_nvm_get_key(uint8_t key_type, uint8_t *key);
loramac_crypto_err_t loramac_crypto_nvm_set_key(uint8_t key_type, uint8_t *key);
loramac_crypto_err_t loramac_crypto_nvm_get_dev_nonce(uint16_t *nonce);
loramac_crypto_err_t loramac_crypto_nvm_set_dev_nonce(uint16_t nonce);
loramac_crypto_err_t loramac_crypto_nvm_set_dev_addr(uint32_t addr);
loramac_crypto_err_t loramac_crypto_nvm_get_dev_addr(uint32_t *addr);

loramac_crypto_err_t loramac_crypto_nvm_set_frame_counter(uint8_t direction, uint32_t addr);
loramac_crypto_err_t loramac_crypto_nvm_get_frame_counter(uint8_t direction, uint32_t *addr);

void loramac_crypto_prepare_b0(uint8_t dir, uint32_t dev_addr, uint32_t frame_cnt, uint8_t msg_length, uint8_t *b0);

loramac_crypto_err_t loramac_crypto_init(void)
{
    loramac_crypto_err_t err = loramac_crypto_nvm_init();

    uint8_t nwk_s_key[16];
    loramac_crypto_nvm_get_key(LORAMAC_CRYPTO_KEY_APP_SESS, nwk_s_key);

    ESP_LOGI(LORAMAC_CRYPTO_TAG, "APP_S_KEY");
    ESP_LOG_BUFFER_HEX(LORAMAC_CRYPTO_TAG, &nwk_s_key, 16);

    return err;
}

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

loramac_crypto_err_t loramac_crypto_aes_verify_cmac(uint8_t *key, uint8_t *buffer, uint16_t buffer_length, uint32_t cmac_verify)
{
    uint32_t cmac = 0;
    loramac_crypto_err_t err;

    // Compute CMAC
    err = loramac_crypto_aes_cmac(key, buffer, buffer_length, &cmac);

    if (err != 0)
    {
        return err;
    }

    if (cmac != cmac_verify)
    {
        ESP_LOGE(LORAMAC_CRYPTO_TAG, "Message CMAC mismatch! Expected 0x%lX but got 0x%lX.", cmac_verify, cmac);
        return -1;
    }

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

loramac_crypto_err_t loramac_crypto_derive_key(uint8_t *app_key, uint8_t key_type, uint8_t *app_nonce, uint8_t *net_id, uint16_t dev_nonce)
{
    uint8_t buff_ptr = 0;
    uint8_t buff[16];
    memset(&buff, 0, sizeof(buff));

    buff[buff_ptr++] = key_type;

    buff[buff_ptr++] = app_nonce[0];
    buff[buff_ptr++] = app_nonce[1];
    buff[buff_ptr++] = app_nonce[2];

    buff[buff_ptr++] = net_id[0];
    buff[buff_ptr++] = net_id[1];
    buff[buff_ptr++] = net_id[2];

    buff[buff_ptr++] = dev_nonce & 0xFF;
    buff[buff_ptr++] = (dev_nonce >> 8) & 0xFF;

    uint8_t key[16];
    loramac_crypto_err_t err = loramac_crypto_aes_encrypt(app_key, &buff, 16, &key);

    if (err != 0)
    {
        ESP_LOGE(LORAMAC_CRYPTO_TAG, "Failed to derive key type 0x%X.", key_type);
        return -1;
    }

    loramac_crypto_nvm_set_key(key_type, &key);

    ESP_LOGI(LORAMAC_CRYPTO_TAG, "Derived Key Type: 0x%X.", key_type);
    ESP_LOG_BUFFER_HEX(LORAMAC_CRYPTO_TAG, &key, 16);

    return 0;
}

loramac_crypto_err_t loramac_crypto_prepare_join_request(loramac_message_join_request_t *msg)
{
    uint16_t dev_nonce;
    if (loramac_crypto_nvm_get_dev_nonce(&dev_nonce) == 0)
    {
        dev_nonce++;
        loramac_crypto_nvm_set_dev_nonce(dev_nonce);
    }
    else
    {
        ESP_LOGW(LORAMAC_CRYPTO_TAG, "Failed to get dev_nonce, defaulting to 0");
        dev_nonce = 0;
    }

    msg->dev_nonce = dev_nonce;

    loramac_serialize_join_request(msg);
    loramac_crypto_aes_cmac(&app_key, msg->buffer, msg->buffer_size - LORAMAC_MIC_FIELD_SIZE, &msg->mic);
    loramac_serialize_join_request(msg);

    return 0;
}

loramac_crypto_err_t loramac_crypto_handle_join_accept(loramac_message_join_accept_t *msg)
{
    uint16_t dev_nonce;
    loramac_crypto_err_t err;

    err = loramac_crypto_aes_encrypt(&app_key, msg->buffer + LORAMAC_MHDR_FIELD_SIZE, msg->buffer_size - LORAMAC_MHDR_FIELD_SIZE, msg->buffer + LORAMAC_MHDR_FIELD_SIZE);
    err = loramac_parse_join_accept(msg);

    if (err != 0)
    {
        ESP_LOGE(LORAMAC_CRYPTO_TAG, "Error while parsing join-accept message.");
        return -1;
    }

    err = loramac_crypto_aes_verify_cmac(&app_key, msg->buffer, msg->buffer_size - LORAMAC_MIC_FIELD_SIZE, msg->mic);

    if (err != 0)
    {
        ESP_LOGE(LORAMAC_CRYPTO_TAG, "Could to verify CMAC of join-accept.");
        return -1;
    }

    err = loramac_crypto_nvm_get_dev_nonce(&dev_nonce);
    if (err != 0)
    {
        ESP_LOGE(LORAMAC_CRYPTO_TAG, "Could to get dev_nonce from nvm, unable to derive session keys.");
        return -1;
    }

    err = loramac_crypto_derive_key(&app_key, LORAMAC_CRYPTO_KEY_NWK_SESS, msg->app_nonce, msg->net_id, dev_nonce);
    err = loramac_crypto_derive_key(&app_key, LORAMAC_CRYPTO_KEY_APP_SESS, msg->app_nonce, msg->net_id, dev_nonce);
    err = loramac_crypto_nvm_set_dev_addr(msg->dev_addr);

    // Reset frame counters
    loramac_crypto_nvm_set_frame_counter(LORAMAC_FRAME_DIRECTION_UP, 0);
    loramac_crypto_nvm_set_frame_counter(LORAMAC_FRAME_DIRECTION_DOWN, 0);

    return err;
}

void loramac_crypto_prepare_b0(uint8_t dir, uint32_t dev_addr, uint32_t frame_cnt, uint8_t msg_length, uint8_t *b0)
{
    b0[0] = 0x49;

    b0[1] = 0x00;
    b0[2] = 0x00;
    b0[3] = 0x00;
    b0[4] = 0x00;

    b0[5] = dir & 0x0F;

    b0[6] = dev_addr & 0xFF;
    b0[7] = (dev_addr >> 8) & 0xFF;
    b0[8] = (dev_addr >> 16) & 0xFF;
    b0[9] = (dev_addr >> 24) & 0xFF;

    b0[10] = frame_cnt & 0xFF;
    b0[11] = (frame_cnt >> 8) & 0xFF;
    b0[12] = (frame_cnt >> 16) & 0xFF;
    b0[13] = (frame_cnt >> 24) & 0xFF;

    b0[14] = 0;

    b0[15] = msg_length;
}

loramac_err_t loramac_crypto_secure_mac_message(loramac_message_mac_t *msg)
{
    uint8_t nwk_s_key[16];
    loramac_crypto_nvm_get_key(LORAMAC_CRYPTO_KEY_NWK_SESS, &nwk_s_key);

    uint8_t *buff = (uint8_t *)malloc(16 + msg->buffer_size - 4);
    loramac_crypto_prepare_b0(msg->direction, msg->fhdr.dev_addr, msg->fhdr.frame_counter, msg->buffer_size - 4, buff);
    memcpy(buff + 16, msg->buffer, msg->buffer_size - 4);
    loramac_crypto_aes_cmac(&nwk_s_key, msg->buffer, msg->buffer_size - LORAMAC_MIC_FIELD_SIZE, &msg->mic);
    loramac_serialize_mac_message(msg);
    loramac_crypto_nvm_set_frame_counter(msg->direction, msg->fhdr.frame_counter + 1);

    free(buff);
    return 0;
}

loramac_crypto_err_t loramac_crypto_prepare_mac_message(loramac_message_mac_t *msg)
{
    loramac_err_t err;
    uint32_t dev_addr;
    uint32_t frame_cnt;

    err = loramac_crypto_nvm_get_dev_addr(&dev_addr);
    err = loramac_crypto_nvm_get_frame_counter(msg->direction, &frame_cnt);

    ESP_LOGI(LORAMAC_CRYPTO_TAG, "Prepare mac message, dev_addr: 0x%08lX", dev_addr);

    msg->fhdr.dev_addr = dev_addr;
    msg->fhdr.frame_counter = frame_cnt;
    err = loramac_serialize_mac_message(msg);
    err = loramac_crypto_secure_mac_message(msg);

    return 0;
}