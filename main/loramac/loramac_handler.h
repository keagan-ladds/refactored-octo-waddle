#ifndef __LORAMAC_HANDLER_H__
#define __LORAMAC_HANDLER_H__
#include "loramac.h"

loramac_err_t loramac_handle(loramac_ctx_t *ctx, uint8_t *payload, uint8_t payload_length, uint16_t rssi, uint8_t snr);
#endif