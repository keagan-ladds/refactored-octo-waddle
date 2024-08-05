#include "sx126x.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define TAG "SX126X"

#define SX126X_XTAL_FREQ 32000000UL
#define SX126X_PLL_STEP_SHIFT_AMOUNT (14)
#define SX126X_PLL_STEP_SCALED (SX126X_XTAL_FREQ >> (25 - SX126X_PLL_STEP_SHIFT_AMOUNT))

static uint32_t SX126xConvertFreqInHzToPllStep(uint32_t freqInHz);
static uint8_t packet_type = 0;

void sx126x_write_register(uint16_t addr, void *buffer, uint16_t buffer_len);
void sx126x_read_register(uint16_t addr, void *buffer, uint16_t buffer_len);
void sx126x_write_buffer(uint8_t offset, void *buffer, uint8_t buffer_len);
void sx126x_read_buffer(uint8_t addr, void *buffer, uint8_t buffer_len);
void sx126x_write_command(sx162x_command_t cmd, void *buffer, uint16_t buffer_len);
void sx126x_read_command(sx162x_command_t cmd, void *buffer, uint16_t buffer_len);

void sx162x_set_lora_sync_word(uint16_t sync_word)
{
    uint8_t data[2];
    data[0] = (sync_word >> 8) & 0xFF;
    data[1] = (sync_word) & 0xFF;

    sx126x_write_register(SX126X_REG_LORA_SYNCWORD, &data, 2);
}

void sx162x_calibrate(uint8_t calibrate_params)
{
    uint8_t data[1];
    data[0] = calibrate_params;

    sx126x_write_command(SX162X_CALIBRATE, &data, 1);
}

void sx126x_set_pa_config(sx126x_pa_config_t pa_config)
{
    if (pa_config.duty_cycle > 0x06)
        pa_config.duty_cycle = 0x06;

    if (pa_config.hp_max > 0x07)
        pa_config.hp_max = 0x07;

    uint8_t data[4];
    data[0] = pa_config.duty_cycle;
    data[1] = pa_config.hp_max;
    data[2] = pa_config.device_sel;
    data[3] = 0x01;

    sx126x_write_command(SX162X_SET_PACONFIG, &data, 4);
}

void sx126x_set_tx_params(sx126x_tx_params_t tx_params)
{
    uint8_t data[2];
    data[0] = tx_params.power;
    data[1] = tx_params.ramp_time;

    sx126x_write_command(SX162X_SET_TXPARAMS, &data, 4);
}

void sx162x_set_dio3_as_tcxo_ctrl(uint8_t voltage, uint32_t timeout)
{
    uint8_t data[4];
    data[0] = voltage & 0x07;
    data[1] = (timeout >> 16) & 0xFF;
    data[2] = (timeout >> 8) & 0xFF;
    data[3] = (timeout >> 0) & 0xFF;

    sx126x_write_command(SX162X_SET_TCXOMODE, &data, 4);
}
#define TCXO_CTRL_1_7V 0x01
#define TCX0_TIMEOUT 5 << 6

void sx162x_init(void)
{
    sx162x_set_standby(STDBY_RC);

    sx162x_set_dio2_as_rf_ctrl(true);

    sx162x_clear_irq_status(0xFFF);

    sx162x_set_dio3_as_tcxo_ctrl(TCXO_CTRL_1_7V, TCX0_TIMEOUT);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    sx162x_calibrate(0x7F);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    sx162x_calibrate_image(0xD7, 0xDB);
    vTaskDelay(100 / portTICK_PERIOD_MS);


    uint8_t reg = sx162x_get_packet_type();
    printf("SX126X_PACKET_TYPE = 0x%X", reg);

    sx162x_clear_device_errors();

    /*sx162x_irq_params_t irq_params = {
        .irq_mask = 0xFFF,
        .dio_1_mask = 0xFFF,
        .dio_2_mask = 0x00};

    sx162x_set_dio_irq_params(irq_params);*/
    sx162x_set_lora_sync_word(0x3444);
}

void sx162x_calibrate_image(uint8_t freq_1, uint8_t freq_2)
{
    uint8_t data[2];
    data[0] = freq_1;
    data[1] = freq_2;

    sx126x_write_command(SX162X_CALIBRATEIMAGE, &data, 2);
}

void sx162x_set_dio2_as_rf_ctrl(bool enable)
{
    uint8_t data[1];
    data[0] = enable ? 1 : 0;

    sx126x_write_command(SX162X_SET_RFSWITCHMODE, &data, 1);
}

sx162x_rx_buffer_status_t sx162x_get_rx_buffer_status(void)
{
    uint8_t data[2];
    sx126x_read_command(SX162X_GET_RXBUFFERSTATUS, &data, 2);

    sx162x_rx_buffer_status_t status = {
        .payload_length = data[0],
        .buffer_start_offset = data[1]};

    return status;
}

void sx162x_clear_device_errors(void)
{
    uint8_t data[0];
    data[0] = 0x00;
    sx126x_write_command(SX162X_CLR_ERROR, &data, 1);
}

void sx162x_clear_irq_status(uint16_t params)
{
    uint8_t data[2];
    data[0] = (params >> 8) & 0xFF;
    data[1] = (params >> 0) & 0xFF;

    sx126x_write_command(SX162X_CLR_IRQSTATUS, &data, 2);
}

uint16_t sx162x_get_device_errors(void)
{
    uint8_t data[2];
    sx126x_read_command(SX162X_GET_ERROR, &data, 2);

    return ((data[0] << 8) | data[1]) & 0x1FF;
}

uint16_t sx162x_get_irq_status(void)
{
    uint8_t data[2];
    sx126x_read_command(SX162X_GET_IRQSTATUS, &data, 2);

    return (data[0] << 8) | data[1];
}

void sx162x_set_dio_irq_params(sx162x_irq_params_t irq_params)
{
    uint8_t data[9];

    data[0] = (irq_params.irq_mask >> 8) & 0xFF;
    data[1] = (irq_params.irq_mask) & 0xFF;

    data[2] = (irq_params.dio_1_mask >> 8) & 0xFF;
    data[3] = (irq_params.dio_1_mask) & 0xFF;

    data[4] = (irq_params.dio_2_mask >> 8) & 0xFF;
    data[5] = (irq_params.dio_2_mask) & 0xFF;

    data[6] = (irq_params.dio_3_mask >> 8) & 0xFF;
    data[7] = (irq_params.dio_3_mask) & 0xFF;

    sx126x_write_command(SX162X_CFG_DIOIRQ, &data, 9);
}

void sx162x_set_packet_params(sx162x_packet_params_t packet_params)
{
    uint8_t data[8];
    uint8_t packet_type = sx162x_get_packet_type();
    switch (packet_type)
    {
    case PACKET_TYPE_LORA:
        data[0] = (packet_params.lora.preamble_length >> 8) & 0xFF;
        data[1] = (packet_params.lora.preamble_length) & 0xFF;
        data[2] = packet_params.lora.header_type;
        data[3] = packet_params.lora.payload_length;
        data[4] = packet_params.lora.crc_type;
        data[5] = packet_params.lora.inverted_iq;
        sx126x_write_command(SX162X_SET_PACKETPARAMS, &data, 6);
        break;

    default:
        ESP_LOGE(TAG, "Unsupported packet type 0x%X", packet_type);
        return;
    }
}

void sx162x_set_modulation_params(sx162x_modulation_params_t modulation_params)
{
    uint8_t data[8];

    switch (sx162x_get_packet_type())
    {
    case PACKET_TYPE_LORA:
        data[0] = modulation_params.lora.speading_factor;
        data[1] = modulation_params.lora.bandwidth;
        data[2] = modulation_params.lora.coding_rate;
        data[3] = modulation_params.lora.low_data_rate_optimize;
        sx126x_write_command(SX162X_SET_MODULATIONPARAMS, &data, 4);
        break;

    default:
        ESP_LOGE(TAG, "Unsupported packet type.");
        return;
    }
}

void sx162x_set_rf_frequency(uint32_t frequency)
{
    uint8_t data[4];

    uint32_t freqInPllSteps = SX126xConvertFreqInHzToPllStep(frequency);

    data[0] = (freqInPllSteps >> 24) & 0xFF;
    data[1] = (freqInPllSteps >> 16) & 0xFF;
    data[2] = (freqInPllSteps >> 8) & 0xFF;
    data[3] = (freqInPllSteps) & 0xFF;

    sx126x_write_command(SX162X_SET_RFFREQUENCY, &data, 4);
}

void sx162x_set_buffer_base_address(uint8_t tx_base_address, uint8_t rx_base_address)
{
    uint8_t data[2];

    data[0] = tx_base_address;
    data[1] = rx_base_address;

    sx126x_write_command(SX162X_SET_BUFFERBASEADDRESS, &data, sizeof(data));
}

void sx162x_set_standby(sx162x_standby_mode_t standby_mode)
{
    sx126x_write_command(SX162X_SET_STANDBY, &standby_mode, 1);
}

sx162x_status_t sx126x_get_status()
{
    uint8_t status_cmd;
    sx162x_status_t status;

    sx126x_read_command(SX162X_GET_STATUS, &status_cmd, 1);

    status.chip_mode = (status_cmd & 0x70) >> 4;
    status.command_mode = (status_cmd & 0x0E) >> 1;

    return status;
}

sx162x_packet_type_t sx162x_get_packet_type(void)
{
    //return packet_type;

    uint8_t data[1];
    sx126x_read_command(SX162X_GET_PACKETTYPE, &data, 1);

    return data[0];
}

void sx162x_set_packet_type(sx162x_packet_type_t type)
{
    uint8_t data[1];
    data[0] = type;

    ESP_LOGI(TAG, "Setting SX126X packet type to 0x%X.", type);
    sx126x_write_command(SX162X_SET_PACKETTYPE, &data, 1);

    packet_type = type;
}

void sx162x_set_rx(uint32_t timeout)
{
    uint8_t data[3];
    data[0] = (timeout >> 16) & 0xFF;
    data[1] = (timeout >> 8) & 0xFF;
    data[2] = (timeout >> 0) & 0xFF;

    sx126x_write_command(SX162X_SET_RX, &data, 3);
}

void sx162x_set_tx(uint32_t timeout)
{
    uint8_t data[3];
    data[0] = (timeout >> 16) & 0xFF;
    data[1] = (timeout >> 8) & 0xFF;
    data[2] = (timeout >> 0) & 0xFF;

    sx126x_write_command(SX162X_SET_TX, &data, 3);
}

static uint32_t SX126xConvertFreqInHzToPllStep(uint32_t freqInHz)
{
    float steps = ((1 << 25) / 32000000.0f) * freqInHz;
    ESP_LOGI(TAG, "STEPS %f.", steps);
    uint32_t stepsInt;
    uint32_t stepsFrac;

    // pllSteps = freqInHz / (SX126X_XTAL_FREQ / 2^19 )
    // Get integer and fractional parts of the frequency computed with a PLL step scaled value
    stepsInt = freqInHz / SX126X_PLL_STEP_SCALED;
    stepsFrac = freqInHz - (stepsInt * SX126X_PLL_STEP_SCALED);

    // Apply the scaling factor to retrieve a frequency in Hz (+ ceiling)
    uint32_t steps_1 = (stepsInt << SX126X_PLL_STEP_SHIFT_AMOUNT) +
                       (((stepsFrac << SX126X_PLL_STEP_SHIFT_AMOUNT) + (SX126X_PLL_STEP_SCALED >> 1)) /
                        SX126X_PLL_STEP_SCALED);

    ESP_LOGI(TAG, "STEPS 1 %lu.", steps_1);
    return steps_1;
}