#ifndef __LORA_RADIO_H__
#define __LORA_RADIO_H__

#include <inttypes.h>
#include <stdbool.h>

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 5
#define SX126X_SPI_HOST SPI2_HOST

typedef enum
{
    LORA_SF_5 = 0x05,
    LORA_SF_6 = 0x06,
    LORA_SF_7 = 0x07,
    LORA_SF_8 = 0x08,
    LORA_SF_9 = 0x09,
    LORA_SF_10 = 0x0A,
    LORA_SF_11 = 0x0B,
    LORA_SF_12 = 0x0C,
} lora_spreading_factor_t;

typedef enum
{
    LORA_BW_7 = 0x00,
    LORA_BW_10 = 0x08,
    LORA_BW_15 = 0x01,
    LORA_BW_20 = 0x09,
    LORA_BW_31 = 0x02,
    LORA_BW_41 = 0x0A,
    LORA_BW_62 = 0x03,
    LORA_BW_125 = 0x04,
    LORA_BW_250 = 0x05,
    LORA_BW_500 = 0x06
} lora_bandwidth_t;

typedef enum
{
    LORA_CR_4_5 = 0x01,
    LORA_CR_4_6 = 0x02,
    LORA_CR_4_7 = 0x03,
    LORA_CR_4_8 = 0x04
} lora_coding_rate_t;

typedef enum {
    LORA_RADIO_GFSK = 0x00,
    LORA_RADIO_LORA = 0x01
} lora_radio_modem_t;

void lora_radio_init(void);
void lora_radio_set_channel(uint32_t freq);
void lora_radio_set_public_network(bool public);
void lora_radio_receive(uint32_t timeout);
void lora_radio_send(void* buffer, uint8_t len);
void lora_radio_set_rx_params(lora_radio_modem_t modem, uint8_t bandwidth, uint8_t spreading_factor);
void lora_radio_set_tx_params(lora_radio_modem_t modem, uint8_t power, uint8_t bandwidth, uint8_t spreading_factor);



#endif