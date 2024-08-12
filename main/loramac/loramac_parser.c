#include "loramac_parser.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

#define TAG "loramac_parser"

loramac_err_t loramac_parse_frame(loramac_frame_t *frame)
{
    if (frame->buffer == NULL || frame->buffer_size == 0)
    {
        ESP_LOGE(TAG, "Failed to parse frame, null or zero length input buffer.");
        return -1;
    }

    uint16_t buff_ptr = 0;
    frame->mhdr.frame_type = (frame->buffer[buff_ptr] & 0xE0) >> 5;
    frame->mhdr.major = (frame->buffer[buff_ptr++] & 0x03);

    return LORAMAC_OK;
}

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

    // Check that we can atleast still read the MIC
    if (msg->buffer_size - buff_ptr < 4)
    {
        ESP_LOGE(TAG, "Failed to parse join-accept message, unexpected end of frame.");
        return -1;
    }

    msg->cf_list_length = msg->buffer_size - buff_ptr - LORAMAC_MIC_FIELD_SIZE;

    for (int i = 0; i < msg->cf_list_length; i++)
    {
        msg->cf_list[i] = msg->buffer[buff_ptr++];
    }

    msg->mic = msg->buffer[buff_ptr++];
    msg->mic |= msg->buffer[buff_ptr++] << 8;
    msg->mic |= msg->buffer[buff_ptr++] << 16;
    msg->mic |= msg->buffer[buff_ptr++] << 24;

    return 0;
}