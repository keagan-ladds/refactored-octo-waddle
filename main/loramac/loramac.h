#ifndef __LORAMAC_H__
#define __LORAMAC_H__

#include <stdint.h>
#include <stdbool.h>
#include "esp_timer.h"
#include "lora_radio.h"

typedef int loramac_err_t;

typedef enum
{
    DR_0,
    DR_1,
    DR_2,
    DR_3,
    DR_4,
    DR_5
} lora_data_rate_t;

#define LORAMAC_OK 0

typedef struct
{
    uint32_t frequency_hz;
    lora_bandwidth_t bandwidth;
    lora_spreading_factor_t spreading_factor;
} rx_window_config_t;

typedef struct
{
    uint8_t dr;
    uint8_t tx_power;
    uint32_t frequency_hz;
    lora_bandwidth_t bandwidth;
    lora_spreading_factor_t spreading_factor;
} tx_window_config_t;

typedef struct
{
    uint32_t frequency_hz;
} channel_config_t;

typedef struct
{
    esp_timer_handle_t rx1_timer_handle;
    esp_timer_handle_t rx2_timer_handle;

    uint8_t channel;
    uint32_t rx1_delay;
    uint32_t rx2_delay;

    tx_window_config_t tx_window_config;
    rx_window_config_t rx1_window_config;
    rx_window_config_t rx2_window_config;

    bool joined;
} loramac_ctx_t;

typedef struct
{
    uint8_t channel;
    uint8_t dr;
} loramac_init_config_t;

const static uint8_t data_rate_spreading_factors[] = {LORA_SF_12, LORA_SF_11, LORA_SF_10, LORA_SF_9, LORA_SF_8, LORA_SF_7};
const static uint32_t data_rate_bandwidth[] = {LORA_BW_125, LORA_BW_125, LORA_BW_125, LORA_BW_125, LORA_BW_125, LORA_BW_125};
const static channel_config_t channels[] = {{.frequency_hz = 868100000}, {.frequency_hz = 868300000}, {.frequency_hz = 868500000}};

// DEV_EUI: 0059AC00001B2EB8
// APP_EUI: 0059AC0000010D19
// APP_KEY: 

// KPN

//const static uint8_t join_eui[] = {0xB8, 0x2E, 0x1B, 0x00, 0x00, 0xAC, 0x59, 0x00};
//const static uint8_t dev_eui[] = {0x19, 0x0D, 0x01, 0x00, 0x00, 0xAC, 0x59, 0x00};

//const static uint8_t join_eui[] = {0x00, 0x59, 0xAC, 0x00, 0x00, 0x1B, 0x2E, 0xB8};
//const static uint8_t dev_eui[] = {0x00, 0x59, 0xAC, 0x00, 0x00, 0x01, 0x0D, 0x19};
//const static uint8_t app_key[] = {0xb5, 0xa6, 0x9b, 0x6d, 0xf4, 0x1f, 0x53, 0xd3, 0x91, 0x08, 0x91, 0x7b, 0xdc, 0xdf, 0x72, 0xe5};

// KPN
const static uint8_t join_eui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const static uint8_t dev_eui[] = {0xF5, 0x99, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70};
const static uint8_t app_key[] = {0x27, 0xE8, 0xC6, 0x0A, 0xAA, 0x4A, 0x86, 0x8A, 0xF0, 0xA8, 0x85, 0xD8, 0x0B, 0x65, 0x17, 0x57};

//TTN
//const static uint8_t join_eui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//const static uint8_t dev_eui[] = {0x5F, 0x99, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70};
//const static uint8_t app_key[] = {0x27, 0xE8, 0xC6, 0x0A, 0xAA, 0x4A, 0x86, 0x8A, 0xF0, 0xA8, 0x85, 0xD8, 0x0B, 0x65, 0x17, 0x57};


loramac_err_t loramac_init(const loramac_init_config_t *config);
loramac_err_t loramac_join(bool force, uint32_t timeout);
loramac_err_t loramac_send(void *payload, uint16_t payload_length);

#endif