#include "loramac_handler.h"
#include "loramac_parser.h"
#include "loramac_crypto.h"
#include "esp_log.h"

#define TAG "loramac"

static loramac_err_t loramac_handle_join_accept(loramac_ctx_t *ctx, uint8_t *payload, uint8_t payload_length, uint16_t rssi, uint8_t snr);
static loramac_err_t loramac_handle_downlink(loramac_ctx_t *ctx, uint8_t *payload, uint8_t payload_length, uint16_t rssi, uint8_t snr);

loramac_err_t loramac_handle(loramac_ctx_t *ctx, uint8_t *payload, uint8_t payload_length, uint16_t rssi, uint8_t snr)
{
    loramac_err_t err;
    loramac_frame_t frame = {.buffer = payload, .buffer_size = payload_length};

    err = loramac_parse_frame(&frame);

    if (err != LORAMAC_OK)
    {
        return err;
    }

    switch (frame.mhdr.frame_type)
    {
    case LORAMAC_FRAME_JOIN_ACCEPT:
        return loramac_handle_join_accept(ctx, payload, payload_length, rssi, snr);

    case LORAMAC_FRAME_DATA_DOWNLINK_CONFIRMED:
    case LORAMAC_FRAME_DATA_DOWNLINK_UNCONFIRMED:
        return loramac_handle_downlink(ctx, payload, payload_length, rssi, snr);

    default:
        ESP_LOGW(TAG, "Unhandled frame type 0x%X.", frame.mhdr.frame_type);
        break;
    }

    return LORAMAC_OK;
}

static loramac_err_t loramac_handle_join_accept(loramac_ctx_t *ctx, uint8_t *payload, uint8_t payload_length, uint16_t rssi, uint8_t snr)
{
    ESP_LOGI(TAG, "Handling join-accept frame.");

    loramac_err_t err;
    loramac_message_join_accept_t msg = {
        .buffer = payload,
        .buffer_size = payload_length};

    err = loramac_crypto_handle_join_accept(&msg);

    if (err != LORAMAC_OK)
    {
        return err;
    }

    ctx->joined = true;
    return LORAMAC_OK;
}

static loramac_err_t loramac_handle_downlink(loramac_ctx_t *ctx, uint8_t *payload, uint8_t payload_length, uint16_t rssi, uint8_t snr)
{
    ESP_LOGI(TAG, "Handling downlink frame.");
    return LORAMAC_OK;
}