#ifndef __LORAMAC_MESSAGE_H__
#define __LORAMAC_MESSAGE_H__
#include "loramac_protocol.h"

#define LORAMAC_MHDR_FIELD_SIZE 1
#define LORAMAC_JOIN_EUI_FIELD_SIZE 8
#define LORAMAC_DEV_EUI_FIELD_SIZE 8
#define LORAMAC_DEV_NONCE_FIELD_SIZE 2
#define LORAMAC_MIC_FIELD_SIZE 4

/*!
 * JoinRequest frame size
 *
 * MHDR(1) + JoinEUI(8) + DevEUI(8) + DevNonce(2) + MIC(4)
 */
#define LORAMAC_JOIN_REQ_MSG_SIZE (LORAMAC_MHDR_FIELD_SIZE + LORAMAC_JOIN_EUI_FIELD_SIZE +     \
                                   LORAMAC_DEV_EUI_FIELD_SIZE + LORAMAC_DEV_NONCE_FIELD_SIZE + \
                                   LORAMAC_MIC_FIELD_SIZE)

typedef struct
{
    uint8_t *buffer;
    uint16_t buffer_size;

    loramac_mac_header_t mhdr;
    uint8_t join_eui[LORAMAC_JOIN_EUI_FIELD_SIZE];
    uint8_t dev_eui[LORAMAC_DEV_EUI_FIELD_SIZE];
    uint16_t dev_nonce;
    uint32_t mic;
} loramac_message_join_request_t;

typedef struct
{
    uint8_t reserved : 1;
    uint8_t rx1_dr_offset : 3;
    uint8_t rx2_dr : 4;
} loramac_dl_settings_t;

typedef struct
{
    uint8_t *buffer;
    uint16_t buffer_size;

    loramac_mac_header_t mhdr;
    uint8_t app_nonce[3];
    uint8_t net_id[3];
    uint32_t dev_addr;
    loramac_dl_settings_t dl_settings;
    uint8_t rx_delay;
    uint8_t cf_list[16];
    uint32_t mic;
} loramac_message_join_accept_t;

#endif