#ifndef __LORAMAC_REGION_H__
#define __LORAMAC_REGION_H__
#include "loramac.h"

#define LORAMAC_DEFAULT_RX1_DELAY 1000
#define LORAMAC_DEFAULT_RX2_DELAY 2000
#define LORAMAC_RX1_JOIN_DELAY 4900
#define LORAMAC_RX2_JOIN_DELAY 5900
#define LORAMAC_RX2_FREQ 869525000

static loramac_channel_t loramac_eu868_channels[] =
    {{868100000, {DR_0, DR_5}},
     {868300000, {DR_0, DR_5}},
     {868500000, {DR_0, DR_5}}};

void loramac_init_phy_config(loramac_phy_config_t *config);

#endif