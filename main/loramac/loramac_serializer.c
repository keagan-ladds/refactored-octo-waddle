#include "loramac_serializer.h"
#include <string.h>
#include "esp_log.h"

#define TAG "loramac_serializer"

loramac_serializer_result_t loramac_serialize_join_request(loramac_message_join_request_t *msg)
{
    if ((msg == 0) || (msg->buffer == 0))
    {
        return LORAMAC_SERIALIZER_ERROR_NPE;
    }

    uint16_t bufItr = 0;

    // Check macMsg->BufSize
    if (msg->buffer_size < LORAMAC_JOIN_REQ_MSG_SIZE)
    {
        return LORAMAC_SERIALIZER_ERROR_BUF_SIZE;
    }

    msg->buffer[bufItr] = (msg->mhdr.frame_type << 5) & 0xE0;
    msg->buffer[bufItr++] |= (msg->mhdr.major) & 0x03;

    memcpy(&msg->buffer[bufItr], msg->join_eui, LORAMAC_JOIN_EUI_FIELD_SIZE);
    bufItr += LORAMAC_JOIN_EUI_FIELD_SIZE;

    memcpy(&msg->buffer[bufItr], msg->dev_eui, LORAMAC_DEV_EUI_FIELD_SIZE);
    bufItr += LORAMAC_DEV_EUI_FIELD_SIZE;

    msg->buffer[bufItr++] = msg->dev_nonce & 0xFF;
    msg->buffer[bufItr++] = (msg->dev_nonce >> 8) & 0xFF;

    msg->buffer[bufItr++] = msg->mic & 0xFF;
    msg->buffer[bufItr++] = (msg->mic >> 8) & 0xFF;
    msg->buffer[bufItr++] = (msg->mic >> 16) & 0xFF;
    msg->buffer[bufItr++] = (msg->mic >> 24) & 0xFF;

    msg->buffer_size = bufItr;

    return 0;
}

loramac_err_t loramac_serialize_mac_message(loramac_message_mac_t *msg)
{
    if ((msg == 0) || (msg->buffer == 0))
    {
        return LORAMAC_SERIALIZER_ERROR_NPE;
    }

    uint16_t buff_ptr = 0;

    msg->buffer[buff_ptr] = (msg->mhdr.frame_type << 5) & 0xE0;
    msg->buffer[buff_ptr++] |= (msg->mhdr.major) & 0x03;

    msg->buffer[buff_ptr++] = msg->fhdr.dev_addr & 0xFF;
    msg->buffer[buff_ptr++] = (msg->fhdr.dev_addr >> 8) & 0xFF;
    msg->buffer[buff_ptr++] = (msg->fhdr.dev_addr >> 16) & 0xFF;
    msg->buffer[buff_ptr++] = (msg->fhdr.dev_addr >> 24) & 0xFF;

    if (msg->direction == 0)
    {
        msg->buffer[buff_ptr] = msg->fhdr.frame_ctrl.uplink.frame_options_len & 0x0F;
        msg->buffer[buff_ptr] |= (msg->fhdr.frame_ctrl.uplink.ack << 5) & 0x20;
        msg->buffer[buff_ptr] |= (msg->fhdr.frame_ctrl.uplink.adr_ack_req << 6) & 0x40;
        msg->buffer[buff_ptr++] |= (msg->fhdr.frame_ctrl.uplink.adr << 7) & 0x80;
    }
    else
    {
        ESP_LOGE(TAG, "Unsupported frame direction %d.", msg->direction);
        return -1;
    }

    msg->buffer[buff_ptr++] = msg->fhdr.frame_counter & 0xFF;
    msg->buffer[buff_ptr++] = (msg->fhdr.frame_counter >> 8) & 0xFF;

    for (int i = 0; i < msg->fhdr.frame_ctrl.uplink.frame_options_len; i++)
    {
        msg->buffer[buff_ptr++] = msg->fhdr.frame_options[i];
    }

    msg->buffer[buff_ptr++] = msg->port;

    for (int i = 0; i < msg->payload_length; i++)
    {
        msg->buffer[buff_ptr++] = msg->payload[i];
    }

    msg->buffer[buff_ptr++] = msg->mic & 0xFF;
    msg->buffer[buff_ptr++] = (msg->mic >> 8) & 0xFF;
    msg->buffer[buff_ptr++] = (msg->mic >> 16) & 0xFF;
    msg->buffer[buff_ptr++] = (msg->mic >> 24) & 0xFF;

    msg->buffer_size = buff_ptr;

    return 0;
}