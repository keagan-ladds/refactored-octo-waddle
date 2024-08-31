#include "loramac_region.h"

void loramac_init_phy_config(loramac_phy_config_t *config)
{
    config->rx1_delay = LORAMAC_DEFAULT_RX1_DELAY;
    config->rx2_delay = LORAMAC_DEFAULT_RX2_DELAY;
    config->dr_offset = 0;
    config->rx2_dr = DR_0;
}