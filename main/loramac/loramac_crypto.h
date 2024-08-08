#ifndef __LORAMAC_CRYPTO_H__
#define __LORAMAC_CRYPTO_H__
#include <stdint.h>
#define LORAMAC_CRYPTO_KEY_LENGTH 128



typedef int loramac_crypto_err_t;

loramac_crypto_err_t loramac_crypto_aes_encrypt(uint8_t *key, uint8_t *buffer, uint16_t buffer_length, uint8_t *encrypt_buffer);
loramac_crypto_err_t loramac_crypto_aes_cmac(uint8_t *key, uint8_t *buffer, uint16_t buffer_length, uint32_t *cmac);

#endif