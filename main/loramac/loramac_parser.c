#include "loramac_parser.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

#define TAG "loramac_parser"

loramac_parser_error_t loramac_parse_join_accept(loramac_message_join_accept_t *msg)
{
    if (msg->buffer == NULL || msg->buffer_size == 0)
    {
        ESP_LOGE(TAG, "Failed to parse join-accept message, null or zero length input buffer.");
        return -1;
    }

    uint16_t buff_ptr = 0;
    msg->mhdr.frame_type = (msg->buffer[buff_ptr] & 0xE0) >> 5;
    msg->mhdr.major = (msg->buffer[buff_ptr++] & 0x03);

    if (msg->mhdr.frame_type != LORAMAC_FRAME_JOIN_ACCEPT)
    {
        ESP_LOGE(TAG, "Failed to parse join-accept message, unexpected frame type '0x%X'.", msg->mhdr.frame_type);
        return -1;
    }

    memcpy(&msg->app_nonce, &msg->buffer[buff_ptr], 3);
    buff_ptr = buff_ptr + 3;

    memcpy(&msg->net_id, &msg->buffer[buff_ptr], 3);
    buff_ptr = buff_ptr + 3;

    msg->dev_addr = (uint32_t)msg->buffer[buff_ptr++];
    msg->dev_addr |= ((uint32_t)msg->buffer[buff_ptr++] << 8);
    msg->dev_addr |= ((uint32_t)msg->buffer[buff_ptr++] << 16);
    msg->dev_addr |= ((uint32_t)msg->buffer[buff_ptr++] << 24);

    msg->dl_settings.rx1_dr_offset = (msg->buffer[buff_ptr] & 0x70) >> 4;
    msg->dl_settings.rx2_dr = (msg->buffer[buff_ptr++] & 0x0F);

    msg->rx_delay = msg->buffer[buff_ptr++] & 0x0F;
    return 0;
}