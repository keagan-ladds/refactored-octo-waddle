#include "loramac/lora_radio.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sx126x.h"
#include <string.h>

#define LORA_RADIO_TAG "LORA_RADIO"
#define SX126X_GPIO_INT1 33
#define SX126X_GPIO_TXEN 12
#define SX126X_GPIO_RXEN 14

#define ESP_INTR_FLAG_DEFAULT 0

static sx162x_packet_params_t packet_params;
static sx162x_modulation_params_t modulation_params;
static sx126x_pa_config_t pa_config;
static sx126x_tx_params_t tx_params;
static sx162x_irq_params_t irq_params;
static lora_radio_modem_t modem_mode;

void sx126x_read_buffer(uint8_t addr, void *buffer, uint8_t buffer_len);
void sx126x_write_buffer(uint8_t offset, void *buffer, uint8_t buffer_len);

static QueueHandle_t gpio_evt_queue = NULL;

spi_device_handle_t spi;
static void gpio_task_example(void *arg);
static void IRAM_ATTR gpio_isr_handler(void *arg);
static lora_radio_config_t radio_config;
static lora_radio_state_t radio_state = LORA_RADIO_STATE_IDLE;

void lora_radio_spi_init();
void lora_radio_gpio_init();

void lora_radio_init(lora_radio_config_t config)
{
    radio_config = config;

    lora_radio_spi_init();
    lora_radio_gpio_init();
    sx162x_init();

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048 * 2, NULL, 10, NULL);

    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(SX126X_GPIO_INT1, gpio_isr_handler, (void *)SX126X_GPIO_INT1);
}

void lora_radio_receive(uint32_t timeout)
{
    irq_params.irq_mask = 0xFFF;
    irq_params.dio_1_mask = 0xFFF;
    irq_params.dio_2_mask = 0x00;
    irq_params.dio_3_mask = 0x00;

    gpio_set_level(SX126X_GPIO_TXEN, 0);
    gpio_set_level(SX126X_GPIO_RXEN, 1);

    sx162x_set_dio_irq_params(irq_params);
    sx162x_set_buffer_base_address(0x00, 0x100);
    sx162x_set_rx(timeout);
    radio_state = LORA_RADIO_STATE_RX;
}

void lora_radio_set_channel(uint32_t freq)
{
    sx162x_set_rf_frequency(freq);
}

void lora_radio_set_rx_params(lora_radio_modem_t modem, uint8_t bandwidth, uint8_t spreading_factor)
{
    switch (modem)
    {
    case LORA_RADIO_LORA:
        modem_mode = LORA_RADIO_LORA;
        sx162x_set_standby(STDBY_RC);
        sx162x_set_packet_type(PACKET_TYPE_LORA);

        packet_params.lora.header_type = LORA_HEADER_TYPE_VARIABLE;
        packet_params.lora.preamble_length = 8;
        packet_params.lora.payload_length = 0xFF;
        packet_params.lora.inverted_iq = LORA_INVERT_IQ_INVERTED;
        packet_params.lora.crc_type = LORA_CRC_TYPE_OFF;

        modulation_params.lora.bandwidth = bandwidth;
        modulation_params.lora.speading_factor = spreading_factor;
        modulation_params.lora.coding_rate = LORA_CR_4_5;
        modulation_params.lora.low_data_rate_optimize = false;

        sx162x_set_modulation_params(modulation_params);
        sx162x_set_packet_params(packet_params);
        ESP_LOGI(LORA_RADIO_TAG, "Configuring RX params for Lora mode.");
        break;

    default:
        ESP_LOGE(LORA_RADIO_TAG, "Unable to configure RX params, unsupported modem type '0x%X'.", modem);
        return;
    }
}

void lora_radio_send(void *buffer, uint8_t len)
{
    switch (modem_mode)
    {
    case LORA_RADIO_LORA:
        packet_params.lora.payload_length = len;
        sx162x_set_packet_params(packet_params);
        ESP_LOGI(LORA_RADIO_TAG, "Setting TX payload length to %d bytes.", len);
        break;
    default:
        return;
    }

    irq_params.dio_1_mask = 0xFFF;
    irq_params.irq_mask = 0xFFF;

    gpio_set_level(SX126X_GPIO_TXEN, 1);
    gpio_set_level(SX126X_GPIO_RXEN, 0);
    sx162x_set_dio_irq_params(irq_params);

    sx162x_set_buffer_base_address(0x00, 0x00);
    sx126x_write_buffer(0x00, buffer, len);

    vTaskDelay(5 / portTICK_PERIOD_MS);
    sx162x_set_tx(0xFFFFFF);
    radio_state = LORA_RADIO_STATE_TX;
    ESP_LOGI(LORA_RADIO_TAG, "Transmitting %d bytes.", len);
    ESP_LOG_BUFFER_HEX(LORA_RADIO_TAG, buffer, len);
}

void lora_radio_set_tx_params(lora_radio_modem_t modem, uint8_t power, uint8_t bandwidth, uint8_t spreading_factor)
{
    switch (modem)
    {
    case LORA_RADIO_LORA:
        modem_mode = LORA_RADIO_LORA;
        sx162x_set_standby(STDBY_RC);
        sx162x_set_packet_type(PACKET_TYPE_LORA);

        packet_params.lora.header_type = LORA_HEADER_TYPE_VARIABLE;
        packet_params.lora.preamble_length = 8;
        packet_params.lora.payload_length = 0xFF;
        packet_params.lora.inverted_iq = LORA_INVERT_IQ_STANDARD;
        packet_params.lora.crc_type = LORA_CRC_TYPE_ON;

        modulation_params.lora.bandwidth = bandwidth;
        modulation_params.lora.speading_factor = spreading_factor;
        modulation_params.lora.coding_rate = LORA_CR_4_5;
        modulation_params.lora.low_data_rate_optimize = false;

        pa_config.duty_cycle = 0x04;
        pa_config.hp_max = 0x07;
        pa_config.device_sel = 0x00;

        tx_params.power = power;
        tx_params.ramp_time = 0x00;

        sx162x_set_modulation_params(modulation_params);
        sx162x_set_packet_params(packet_params);
        sx126x_set_tx_params(tx_params);
        sx126x_set_pa_config(pa_config);
        ESP_LOGI(LORA_RADIO_TAG, "Configuring TX params for Lora mode.");
        break;

    default:
        ESP_LOGE(LORA_RADIO_TAG, "Unable to configure TX params, unsupported modem type '0x%X'.", modem);
        return;
    }
}

void lora_radio_set_public_network(bool public)
{
    if (public)
    {
        sx162x_set_lora_sync_word(LORA_SYNCWORD_PUBLIC);
    }
    else
    {
        sx162x_set_lora_sync_word(LORA_SYNCWORD_PRIVATE);
    }
}

void lora_radio_gpio_init()
{
    gpio_config_t io_conf = {};
    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    // bit mask of the pins
    io_conf.pin_bit_mask = (1ULL << SX126X_GPIO_INT1);
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << SX126X_GPIO_TXEN) | (1ULL << SX126X_GPIO_RXEN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
}

void lora_radio_spi_init()
{
    esp_err_t ret;

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1};

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // Clock out at 10 MHz
        .mode = 0,                          // SPI mode 0
        .spics_io_num = PIN_NUM_CS,         // CS pin
        .queue_size = 7,                    // We want to be able to queue 7 transactions at a time
        .address_bits = 0,
        .command_bits = 0,
        .dummy_bits = 0,
        .duty_cycle_pos = 128};
    // Initialize the SPI bus
    ret = spi_bus_initialize(SX126X_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    // Attach the LCD to the SPI bus
    ret = spi_bus_add_device(SX126X_SPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
}

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void *arg)
{
    uint32_t io_num;
    static uint8_t buff[255];
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            uint16_t reg;
            uint16_t irq_status = sx162x_get_irq_status();
            sx162x_clear_irq_status(irq_status);
            printf("SX162X IRQ - 0x%X\n", irq_status);

            if ((irq_status & SX126X_IRQ_RX_DONE) == SX126X_IRQ_RX_DONE)
            {
                vTaskDelay(100 / portTICK_PERIOD_MS);
                sx162x_rx_buffer_status_t buff_status = sx162x_get_rx_buffer_status();
                sx126x_packet_status_t packet_status = sx162x_get_packet_status();
                printf("PKT RCVD Size: 0x%X, Rssi: %d dBm\n", buff_status.payload_length, packet_status.lora.rssi_pkt);

                if (buff_status.payload_length > 0)
                {
                    memset(&buff, 0, sizeof(buff));
                    sx126x_read_buffer(buff_status.buffer_start_offset, &buff, buff_status.payload_length);
                    ESP_LOG_BUFFER_HEX(LORA_RADIO_TAG, &buff, buff_status.payload_length);

                    if (radio_config.rx_done != NULL)
                    {
                        radio_config.rx_done(&buff, buff_status.payload_length, packet_status.lora.rssi_pkt, packet_status.lora.snr_pkt);
                    }
                }

                radio_state = LORA_RADIO_STATE_IDLE;
            }

            if ((irq_status & SX126X_IRQ_TX_DONE) == SX126X_IRQ_TX_DONE)
            {
                ESP_LOGI(LORA_RADIO_TAG, "TX DONE");
                radio_state = LORA_RADIO_STATE_IDLE;
                if (radio_config.tx_done != NULL)
                {
                    radio_config.tx_done();
                }
            }

            if ((irq_status & SX126X_IRQ_TIMEOUT) == SX126X_IRQ_TIMEOUT)
            {
                switch (radio_state)
                {
                case LORA_RADIO_STATE_RX:
                    ESP_LOGI(LORA_RADIO_TAG, "RX TIMEOUT");
                    if (radio_config.rx_timeout != NULL)
                    {
                        radio_config.rx_timeout();
                    }
                    break;

                case LORA_RADIO_STATE_TX:
                    ESP_LOGI(LORA_RADIO_TAG, "TX TIMEOUT");
                    if (radio_config.tx_timeout != NULL)
                    {
                        radio_config.tx_timeout();
                    }
                    break;

                default:
                    break;
                }
            }
        }
    }
}

void sx126x_write_command(sx162x_command_t cmd, void *buffer, uint16_t buffer_len)
{
    esp_err_t ret;

    ret = spi_device_acquire_bus(spi, portMAX_DELAY);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "Could not aquire SPI bus lock.\n");
    }

    uint8_t instruction_to_read_data[1] = {
        cmd};

    spi_transaction_t trans_desc = {
        .flags = SPI_TRANS_CS_KEEP_ACTIVE,
        .tx_buffer = &instruction_to_read_data,
        .length = 8};

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI write operation failed\n");
    }

    trans_desc.flags = 0;
    trans_desc.tx_buffer = buffer;
    trans_desc.rx_buffer = NULL;
    trans_desc.rxlength = 0;
    trans_desc.length = buffer_len * 8;

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI write operation failed\n");
    }

    spi_device_release_bus(spi);
}

void sx126x_read_command(sx162x_command_t cmd, void *buffer, uint16_t buffer_len)
{
    esp_err_t ret;

    ret = spi_device_acquire_bus(spi, portMAX_DELAY);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "Could not aquire SPI bus lock.\n");
    }

    uint8_t instruction_to_read_data[2] = {
        cmd,
        0x00};

    spi_transaction_t trans_desc = {
        .flags = SPI_TRANS_CS_KEEP_ACTIVE,
        .tx_buffer = &instruction_to_read_data,
        .length = (2) * 8};

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI write operation failed\n");
    }

    trans_desc.flags = 0;
    trans_desc.tx_buffer = NULL;
    trans_desc.rx_buffer = buffer;
    trans_desc.rxlength = buffer_len * 8;
    trans_desc.length = buffer_len * 8;

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI read operation failed\n");
    }

    spi_device_release_bus(spi);
}

void sx126x_write_register(uint16_t addr, void *buffer, uint16_t buffer_len)
{
    esp_err_t ret;

    ret = spi_device_acquire_bus(spi, portMAX_DELAY);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "Could not aquire SPI bus lock.\n");
    }

    uint8_t instruction_to_read_data[3] = {
        SX162X_WRITE_REGISTER,
        ((addr & 0xFF00) >> 8),
        (addr & 0xFF)};

    spi_transaction_t trans_desc = {
        .flags = SPI_TRANS_CS_KEEP_ACTIVE,
        .tx_buffer = &instruction_to_read_data,
        .length = (3) * 8};

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI write operation failed\n");
    }

    trans_desc.flags = 0;
    trans_desc.tx_buffer = buffer;
    trans_desc.rx_buffer = NULL;
    trans_desc.rxlength = 0;
    trans_desc.length = buffer_len * 8;

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI write operation failed\n");
    }

    spi_device_release_bus(spi);
}

void sx126x_read_register(uint16_t addr, void *buffer, uint16_t buffer_len)
{
    esp_err_t ret;

    ret = spi_device_acquire_bus(spi, portMAX_DELAY);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "Could not aquire SPI bus lock.\n");
    }

    uint8_t instruction_to_read_data[4] = {
        SX162X_READ_REGISTER,
        ((addr & 0xFF00) >> 8),
        (addr & 0xFF),
        0x00};

    spi_transaction_t trans_desc = {
        .flags = SPI_TRANS_CS_KEEP_ACTIVE,
        .tx_buffer = &instruction_to_read_data,
        .length = (4) * 8};

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI write operation failed\n");
    }

    trans_desc.flags = 0;
    trans_desc.tx_buffer = NULL;
    trans_desc.rx_buffer = buffer;
    trans_desc.rxlength = buffer_len * 8;
    trans_desc.length = buffer_len * 8;

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI read operation failed\n");
    }

    spi_device_release_bus(spi);
}

void sx126x_write_buffer(uint8_t offset, void *buffer, uint8_t buffer_len)
{
    esp_err_t ret;

    ret = spi_device_acquire_bus(spi, portMAX_DELAY);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "Could not aquire SPI bus lock.\n");
    }

    uint8_t instruction_to_read_data[2] = {
        SX162X_WRITE_BUFFER,
        offset};

    spi_transaction_t trans_desc = {
        .flags = SPI_TRANS_CS_KEEP_ACTIVE,
        .tx_buffer = &instruction_to_read_data,
        .length = (2) * 8};

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI write operation failed\n");
    }

    trans_desc.flags = 0;
    trans_desc.tx_buffer = buffer;
    trans_desc.rx_buffer = NULL;
    trans_desc.rxlength = 0;
    trans_desc.length = buffer_len * 8;

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI write operation failed\n");
    }

    spi_device_release_bus(spi);
}

void sx126x_read_buffer(uint8_t addr, void *buffer, uint8_t buffer_len)
{
    esp_err_t ret;

    ret = spi_device_acquire_bus(spi, portMAX_DELAY);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "Could not aquire SPI bus lock.\n");
    }

    uint8_t instruction_to_read_data[4] = {
        SX162X_READ_BUFFER,
        addr,
        0x00};

    spi_transaction_t trans_desc = {
        .flags = SPI_TRANS_CS_KEEP_ACTIVE,
        .tx_buffer = &instruction_to_read_data,
        .length = (3) * 8};

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI write operation failed\n");
    }

    trans_desc.flags = 0;
    trans_desc.tx_buffer = NULL;
    trans_desc.rx_buffer = buffer;
    trans_desc.rxlength = buffer_len * 8;
    trans_desc.length = buffer_len * 8;

    ret = spi_device_polling_transmit(spi, &trans_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LORA_RADIO_TAG, "SPI read operation failed\n");
    }

    spi_device_release_bus(spi);
}
