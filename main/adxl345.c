#include "adxl345.h"
#include "driver/i2c.h"
#include "assert.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define TAG "ADXL345"

static uint8_t send_data(void *bytes, size_t bytes_len);
static uint8_t recieve_data(uint8_t reg_addr, void *bytes, size_t bytes_len);

static uint8_t adxl345_reg_write(uint8_t reg, uint8_t val);
static uint8_t adxl345_reg_read(uint8_t reg, uint8_t *data);

bool lvgl_i2c_driver_init(int port, int sda_pin, int scl_pin, int speed_hz);
void adxl345_init_gpio(void);
#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            uint8_t reg;
            adxl345_reg_read(ADXL345_INT_SOURCE, &reg);
            printf("No Touchy Touch!\n");
        }
    }
}

void adxl345_init(void)
{

    lvgl_i2c_driver_init(ADXL345_I2C_PORT,
                         ADXL345_I2C_SDA, ADXL345_I2C_SCL,
                         ADXL345_I2C_SPEED_HZ);

    adxl345_init_gpio();

    uint8_t reg;

    adxl345_reg_read(ADXL345_INT_SOURCE, &reg);

    adxl345_reg_write(ADXL345_DATA_FORMAT, 0x0B);
    adxl345_reg_write(ADXL345_BW_RATE, 0x09);
    adxl345_reg_write(ADXL345_ACT_INACT_CTL, 0xF << 4);
    adxl345_reg_write(ADXL345_INT_ENABLE, 1 << 4);
    adxl345_reg_write(ADXL345_THRESH_ACT, 0x0A);
    adxl345_reg_write(ADXL345_POWER_CTL, 1 << 3);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(ADXL345_GPIO_INT1, gpio_isr_handler, (void *)ADXL345_GPIO_INT1);
}

void adxl345_init_gpio(void)
{
    gpio_config_t io_conf = {};
    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    // bit mask of the pins
    io_conf.pin_bit_mask = (1ULL << ADXL345_GPIO_INT1);
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

bool lvgl_i2c_driver_init(int port, int sda_pin, int scl_pin, int speed_hz)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing I2C master port %d...", port);
    ESP_LOGI(TAG, "SDA pin: %d, SCL pin: %d, Speed: %d (Hz)",
             sda_pin, scl_pin, speed_hz);

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl_pin,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = speed_hz,
    };

    ESP_LOGI(TAG, "Setting I2C master configuration...");
    err = i2c_param_config(port, &conf);
    assert(ESP_OK == err);

    ESP_LOGI(TAG, "Installing I2C master driver...");
    err = i2c_driver_install(port,
                             I2C_MODE_MASTER,
                             0, 0 /*I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE */,
                             0 /* intr_alloc_flags */);
    assert(ESP_OK == err);

    return ESP_OK != err;
}

static uint8_t adxl345_reg_write(uint8_t reg, uint8_t val)
{
    uint8_t cmd[] = {
        reg,
        val};

    return send_data(cmd, sizeof(cmd));
}

static uint8_t adxl345_reg_read(uint8_t reg, uint8_t *data)
{
    return recieve_data(reg, data, 1);
}

void adxl345_read_val(uint8_t *data)
{
    recieve_data(ADXL345_DATAX0, data, 6);
}

static uint8_t send_data(void *bytes, size_t bytes_len)
{
    esp_err_t err;

    uint8_t *data = (uint8_t *)bytes;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADXL345_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);

    for (size_t idx = 0; idx < bytes_len; idx++)
    {
        i2c_master_write_byte(cmd, data[idx], true);
    }

    i2c_master_stop(cmd);

    /* Send queued commands */
    err = i2c_master_cmd_begin(ADXL345_I2C_PORT, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return ESP_OK == err ? 0 : 1;
}

static uint8_t recieve_data(uint8_t reg_addr, void *bytes, size_t bytes_len)
{
    esp_err_t err;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADXL345_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(ADXL345_I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        return ESP_OK == err ? 0 : 1;
    }

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADXL345_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, bytes, bytes_len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(ADXL345_I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ESP_OK == err ? 0 : 1;
}