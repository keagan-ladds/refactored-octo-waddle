#ifndef __LORAMAC_H__
#define __LORAMAC_H__

#include <stdint.h>
#include <stdbool.h>
#include "esp_timer.h"
#include "lora_radio.h"

typedef int loramac_err_t;

#define LORAMAC_OK 0

typedef struct
{
    uint32_t delay_ms;
    uint32_t frequency_hz;
    lora_bandwidth_t bandwidth;
    lora_spreading_factor_t spreading_factor;
} rx_window_config_t;

typedef struct
{
    uint8_t tx_power;
    uint32_t frequency_hz;
    lora_bandwidth_t bandwidth;
    lora_spreading_factor_t spreading_factor;
} tx_window_config_t;

typedef struct
{
    esp_timer_handle_t rx1_timer_handle;
    esp_timer_handle_t rx2_timer_handle;

    tx_window_config_t tx_window_config;
    rx_window_config_t rx1_window_config;
    rx_window_config_t rx2_window_config;

    bool joined;
} loramac_ctx_t;

typedef struct
{

} loramac_init_config_t;

loramac_err_t loramac_init(const loramac_init_config_t *config);
loramac_err_t loramac_join(bool force);
loramac_err_t loramac_send(void *payload, uint16_t payload_length);

#endif