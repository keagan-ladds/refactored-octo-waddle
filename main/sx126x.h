#ifndef __SX126X_H__
#define __SX126X_H__

#include <inttypes.h>
#include <stdbool.h>
#include "loramac/lora_radio.h"

#define LORA_SYNCWORD_PUBLIC 0x3444
#define LORA_SYNCWORD_PRIVATE 0x1424

#define SX126X_REG_WHITENING_INIT_VAL 0x06B8
#define SX126X_REG_CRC_INIT_VAL 0x06BC
#define SX126X_REG_CRC_POLY_VAL 0x06BE
#define SX126X_REG_SYNCWORD 0x06C0
#define SX126X_REG_NODE_ADDR 0x06CD
#define SX126X_REG_BROADCAST_ADDR 0x06CE
#define SX126X_REG_LORA_SYNCWORD 0x0740
#define SX126X_REG_RND_NUM_GEN 0x0819
#define SX126X_REG_RX_GAIN 0x08AC
#define SX126X_REG_OCP_CONFIG 0x08E7
#define SX126X_REG_XTA_TRIM 0x0911
#define SX126X_REG_XTB_TRIM 0x0912
#define SX126X_REG_TX_CLAMP_CONFIG 0x08D8
#define SX126X_REG_IQ_POLARITY_CONFIG 0x0736

#define SX126X_IRQ_TX_DONE (1 << 0)
#define SX126X_IRQ_RX_DONE (1 << 2)
#define SX126X_IRQ_TIMEOUT (1 << 9)

typedef enum
{
    SX162X_GET_STATUS = 0xC0,
    SX162X_WRITE_REGISTER = 0x0D,
    SX162X_READ_REGISTER = 0x1D,
    SX162X_WRITE_BUFFER = 0x0E,
    SX162X_READ_BUFFER = 0x1E,
    SX162X_SET_SLEEP = 0x84,
    SX162X_SET_STANDBY = 0x80,
    SX162X_SET_FS = 0xC1,
    SX162X_SET_TX = 0x83,
    SX162X_SET_RX = 0x82,
    SX162X_SET_RXDUTYCYCLE = 0x94,
    SX162X_SET_CAD = 0xC5,
    SX162X_SET_TXCONTINUOUSWAVE = 0xD1,
    SX162X_SET_TXCONTINUOUSPREAMBLE = 0xD2,
    SX162X_SET_PACKETTYPE = 0x8A,
    SX162X_GET_PACKETTYPE = 0x11,
    SX162X_SET_RFFREQUENCY = 0x86,
    SX162X_SET_TXPARAMS = 0x8E,
    SX162X_SET_PACONFIG = 0x95,
    SX162X_SET_CADPARAMS = 0x88,
    SX162X_SET_BUFFERBASEADDRESS = 0x8F,
    SX162X_SET_MODULATIONPARAMS = 0x8B,
    SX162X_SET_PACKETPARAMS = 0x8C,
    SX162X_GET_RXBUFFERSTATUS = 0x13,
    SX162X_GET_PACKETSTATUS = 0x14,
    SX162X_GET_RSSIINST = 0x15,
    SX162X_GET_STATS = 0x10,
    SX162X_RESET_STATS = 0x00,
    SX162X_CFG_DIOIRQ = 0x08,
    SX162X_GET_IRQSTATUS = 0x12,
    SX162X_CLR_IRQSTATUS = 0x02,
    SX162X_CALIBRATE = 0x89,
    SX162X_CALIBRATEIMAGE = 0x98,
    SX162X_SET_REGULATORMODE = 0x96,
    SX162X_GET_ERROR = 0x17,
    SX162X_CLR_ERROR = 0x07,
    SX162X_SET_TCXOMODE = 0x97,
    SX162X_SET_TXFALLBACKMODE = 0x93,
    SX162X_SET_RFSWITCHMODE = 0x9D,
    SX162X_SET_STOPRXTIMERONPREAMBLE = 0x9F,
    SX162X_SET_LORASYMBTIMEOUT = 0xA0,
} sx162x_command_t;

typedef enum
{
    PACKET_TYPE_GFSK = 0x00,
    PACKET_TYPE_LORA = 0x01
} sx162x_packet_type_t;

typedef struct
{
    uint8_t WakeUpRTC : 1;
    uint8_t Reset : 1;
    uint8_t WarmStart : 1;
    uint8_t Reserved : 5;
} sx162x_sleep_config_t;

typedef struct
{
    uint8_t chip_mode : 3;
    uint8_t command_mode : 3;
} sx162x_status_t;

typedef struct
{

} sx162x_gfsk_modulation_params_t;

typedef enum
{
    SET_RAMP_200U = 0x04

} tx_ramp_time_t;

typedef struct
{
    lora_spreading_factor_t speading_factor;
    lora_bandwidth_t bandwidth;
    lora_coding_rate_t coding_rate;
    bool low_data_rate_optimize;
} sx162x_lora_modulation_params_t;

typedef union
{
    sx162x_gfsk_modulation_params_t gfsk;
    sx162x_lora_modulation_params_t lora;
} sx162x_modulation_params_t;

typedef struct
{

} sx162x_gfsk_packet_params_t;

typedef enum
{
    LORA_HEADER_TYPE_VARIABLE = 0x00,
    LORA_HEADER_TYPE_FIXED = 0x01
} lora_header_type_t;

typedef enum
{
    LORA_CRC_TYPE_OFF = 0x00,
    LORA_CRC_TYPE_ON = 0x01
} lora_crc_type_t;

typedef enum
{
    LORA_INVERT_IQ_STANDARD = 0x00,
    LORA_INVERT_IQ_INVERTED = 0x01
} lora_invert_iq_setup_t;

typedef struct
{
    uint16_t preamble_length;
    lora_header_type_t header_type;
    uint8_t payload_length;
    lora_crc_type_t crc_type;
    lora_invert_iq_setup_t inverted_iq;
} sx162x_lora_packet_params_t;

typedef union
{
    sx162x_gfsk_packet_params_t gfsk;
    sx162x_lora_packet_params_t lora;
} sx162x_packet_params_t;

typedef enum
{
    STDBY_RC = 0x00,
    STDBY_XOSC
} sx162x_standby_mode_t;

typedef struct
{
    uint8_t duty_cycle;
    uint8_t hp_max;
    uint8_t device_sel;
} sx126x_pa_config_t;

typedef struct
{
    uint8_t power;
    tx_ramp_time_t ramp_time;
} sx126x_tx_params_t;

typedef struct
{
    uint8_t payload_length;
    uint8_t buffer_start_offset;
} sx162x_rx_buffer_status_t;

typedef struct
{
    uint16_t irq_mask;
    uint16_t dio_1_mask;
    uint16_t dio_2_mask;
    uint16_t dio_3_mask;
} sx162x_irq_params_t;

typedef struct
{
    uint8_t rssi_pkt;
    uint8_t snr_pkt;
    uint8_t signal_rssi;
} sx126x_lora_packet_status_t;

typedef union
{
    sx126x_lora_packet_status_t lora;
} sx126x_packet_status_t;

void sx162x_init(void);
void sx162x_set_tx(uint32_t timeout);
void sx162x_set_rx(uint32_t timeout);
sx162x_status_t sx126x_get_status();
sx162x_packet_type_t sx162x_get_packet_type(void);
void sx162x_set_packet_type(sx162x_packet_type_t packet_type);
void sx162x_set_standby(sx162x_standby_mode_t standby_mode);
void sx162x_set_rf_frequency(uint32_t frequency);
void sx162x_set_buffer_base_address(uint8_t tx_base_address, uint8_t rx_base_address);
void sx162x_set_modulation_params(sx162x_modulation_params_t modulation_params);
void sx162x_set_packet_params(sx162x_packet_params_t packet_params);
void sx162x_set_dio_irq_params(sx162x_irq_params_t irq_params);
void sx162x_set_lora_sync_word(uint16_t sync_word);
uint16_t sx162x_get_irq_status(void);
uint16_t sx162x_get_device_errors(void);
void sx162x_clear_device_errors(void);
void sx162x_clear_irq_status(uint16_t params);
void sx162x_set_dio3_as_tcxo_ctrl(uint8_t voltage, uint32_t timeout);
void sx162x_set_dio2_as_rf_ctrl(bool enable);
void sx162x_calibrate(uint8_t calibrate_params);
void sx162x_calibrate_image(uint8_t freq_1, uint8_t freq_2);
sx162x_rx_buffer_status_t sx162x_get_rx_buffer_status(void);
sx126x_packet_status_t sx162x_get_packet_status(void);

void sx126x_set_pa_config(sx126x_pa_config_t pa_config);
void sx126x_set_tx_params(sx126x_tx_params_t tx_params);
#endif