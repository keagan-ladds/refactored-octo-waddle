#ifndef __LORAMAC_PROTOCOL_H__
#define __LORAMAC_PROTOCOL_H__

#include <inttypes.h>

#define LORAMAC_FRAME_JOIN_REQUEST 0x00
#define LORAMAC_FRAME_JOIN_ACCEPT 0x01
#define LORAMAC_FRAME_DATA_UPLINK_UNCONFIRMED 0x2
#define LORAMAC_FRAME_DATA_DOWNLINK_UNCONFIRMED 0x3
#define LORAMAC_FRAME_DATA_UPLINK_CONFIRMED 0x4
#define LORAMAC_FRAME_DATA_DOWNLINK_CONFIRMED 0x5

#define LORAMAC_FRAME_DIRECTION_UP 0
#define LORAMAC_FRAME_DIRECTION_DOWN 1

typedef struct
{
    uint8_t frame_type : 3;
    uint8_t reserved : 3;
    uint8_t major : 2;
} loramac_mac_header_t;

typedef struct
{
    uint8_t adr : 1;
    uint8_t reserved : 1;
    uint8_t ack : 1;
    uint8_t frame_pending : 1;
    uint8_t frame_options_len : 4;
} loramac_downlink_frame_fctrl_t;

typedef struct
{
    uint8_t adr : 1;
    uint8_t adr_ack_req : 1;
    uint8_t ack : 1;
    uint8_t frame_options_len : 4;
} loramac_uplink_frame_fctrl_t;

typedef union
{
    loramac_uplink_frame_fctrl_t uplink;
    loramac_downlink_frame_fctrl_t downlink;
} loramac_frame_fctrl_t;

typedef struct
{
    uint32_t dev_addr;
    loramac_frame_fctrl_t frame_ctrl;
    uint32_t frame_counter;
    uint8_t frame_options[15];
} loramac_frame_header_t;

#endif