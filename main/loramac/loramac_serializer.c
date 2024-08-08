#include "loramac_serializer.h"
#include <string.h>

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

    msg->buffer[bufItr] = (msg->mhdr.frame_type << 4) & 0xE0;
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