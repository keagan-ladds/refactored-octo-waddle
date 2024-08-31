#ifndef __LORAMAC_CRYPTO_H__
#define __LORAMAC_CRYPTO_H__
#include <stdint.h>
#include "loramac_message.h"
#include "loramac.h"

#define LORAMAC_CRYPTO_KEY_LENGTH 128
#define LORAMAC_CRYPTO_KEY_APP_ROOT 0x00
#define LORAMAC_CRYPTO_KEY_NWK_SESS 0x01
#define LORAMAC_CRYPTO_KEY_APP_SESS 0x02

typedef int loramac_crypto_err_t;

loramac_crypto_err_t loramac_crypto_init(loramac_ctx_t *ctx);
loramac_crypto_err_t loramac_crypto_aes_encrypt(uint8_t *key, uint8_t *buffer, uint16_t buffer_length, uint8_t *encrypt_buffer);
loramac_crypto_err_t loramac_crypto_aes_cmac(uint8_t *key, uint8_t *b0_buffer, uint8_t *buffer, uint16_t buffer_length, uint32_t *cmac);
loramac_crypto_err_t loramac_crypto_handle_join_accept(loramac_message_join_accept_t *msg);
loramac_crypto_err_t loramac_crypto_prepare_join_request(loramac_message_join_request_t *msg);
loramac_crypto_err_t loramac_crypto_prepare_mac_message(loramac_message_mac_t *msg);
#endif