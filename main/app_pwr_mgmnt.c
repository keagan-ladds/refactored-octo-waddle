#include "app_pwr_mgmnt.h"
#include <stdint.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_types.h"
#include <stdatomic.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "esp_adc_cal.h"

#define TAG "app_power_management"
#define PWR_EXT_WAKE_PIN 34
#define PWR_MGMT_GET_VOLTAGE_PERIOD_MS 1000 * 5
#define DEFAULT_VREF 1100

static pwr_mgmnt_ctx_t pwr_mgmnt_ctx;
static uint32_t sleeplock_count = 0;
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_3;
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;
static esp_timer_handle_t get_state_timer;
static esp_timer_handle_t get_battery_voltage_timer;

static prw_mgmnt_sleeplock_t sleeplock_config[PWR_MGMNT_MAX_SLEEPLOCKS];

static void pwr_mgmnt_init_ctx(pwr_mgmnt_ctx_t *ctx, const pwr_mgmnt_init_config_t *config);
static void pwr_mgmnt_set_power_on_reason(pwr_mgmnt_ctx_t *ctx);
static void pwr_mgmnt_init_sleeplock(pwr_mgmnt_ctx_t *ctx);
static void pwr_mgmnt_init_sleeplock_timers(pwr_mgmnt_ctx_t *ctx);
static void pwr_mgmt_init_batt_adc();

static void batt_voltage_timer_callback(void *arg);

void pwr_mgmnt_init(const pwr_mgmnt_init_config_t *config)
{
    pwr_mgmnt_init_ctx(&pwr_mgmnt_ctx, config);
    pwr_mgmnt_init_sleeplock(&pwr_mgmnt_ctx);
    pwr_mgmt_init_batt_adc();
}

void app_pwr_sleep(void)
{
    /*const int wakeup_time_sec = 20;
    printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);*/

    const uint64_t ext_wakeup_pin_1_mask = 1ULL << PWR_EXT_WAKE_PIN;
    ESP_LOGI(TAG, "Enabling EXT1 wakeup on pins GPIO%d", PWR_EXT_WAKE_PIN);
    esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ANY_HIGH);

    // st7789_sleep();
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    ESP_LOGI(TAG, "Entering Deep Sleep");
    esp_deep_sleep_start();
}

void pwr_mgmnt_register_sleeplock(const pwr_mgmnt_sleeplock_config_t *config, pwr_mgmnt_sleeplock_handle_t *handle)
{
    assert(sleeplock_count < PWR_MGMNT_MAX_SLEEPLOCKS);

    ESP_LOGI(TAG, "Registering sleep-lock '%s'.", config->name);

    sleeplock_config[sleeplock_count].index = sleeplock_count;
    sleeplock_config[sleeplock_count].name = config->name;

    *handle = &sleeplock_config;
    sleeplock_count++;
}

void pwr_mgmnt_set_sleeplock(pwr_mgmnt_sleeplock_handle_t handle)
{
    const prw_mgmnt_sleeplock_t *sleeplock = (prw_mgmnt_sleeplock_t *)handle;
    ESP_LOGI(TAG, "Setting sleep-lock bit sleep-lock '%s'.", sleeplock->name);

    esp_timer_stop(pwr_mgmnt_ctx.light_sleep_timer_handle);
    esp_timer_stop(pwr_mgmnt_ctx.deep_sleep_timer_handle);
}
void pwr_mgmnt_release_sleeplock(pwr_mgmnt_sleeplock_handle_t handle)
{
    const prw_mgmnt_sleeplock_t *sleeplock = (prw_mgmnt_sleeplock_t *)handle;
    ESP_LOGI(TAG, "Releasing sleep-lock bit sleep-lock '%s'.", sleeplock->name);
}

static void pwr_mgmnt_set_power_on_reason(pwr_mgmnt_ctx_t *ctx)
{
    assert(ctx != NULL);

    esp_sleep_wakeup_cause_t wake_reason = esp_sleep_get_wakeup_cause();
    if (wake_reason == ESP_SLEEP_WAKEUP_UNDEFINED)
    {
        ESP_LOGI(TAG, "Power-on reason: Power-On-Reset");
        ctx->power_on_reason.wake_reason = WAKE_REASON_RESET;
        ctx->power_on_reason.wake_from_sleep = false;
    }
    else if (wake_reason == ESP_SLEEP_WAKEUP_EXT1)
    {
        ESP_LOGI(TAG, "Power-on reason: Power Button Pressed");
        ctx->power_on_reason.wake_reason = WAKE_REASON_EXT1;
        ctx->power_on_reason.wake_from_sleep = true;
    }
    else if (wake_reason == ESP_SLEEP_WAKEUP_TIMER)
    {
        ESP_LOGI(TAG, "Power-on reason: Timer");
        ctx->power_on_reason.wake_reason = WAKE_REASON_TIMER;
        ctx->power_on_reason.wake_from_sleep = true;
    }
}

static void pwr_mgmnt_init_ctx(pwr_mgmnt_ctx_t *ctx, const pwr_mgmnt_init_config_t *config)
{
    ctx->auto_light_sleep_timeout = config->auto_light_sleep_timeout;
    ctx->auto_deep_sleep_timeout = config->auto_light_sleep_timeout;

    pwr_mgmnt_set_power_on_reason(&pwr_mgmnt_ctx);
}

static void light_sleep_timer_callback(void) {}
static void deep_sleep_timer_callback(void) {}

static void pwr_mgmnt_init_sleeplock(pwr_mgmnt_ctx_t *ctx)
{
    ctx->sleeplock_event_group_handle = xEventGroupCreate();
    pwr_mgmnt_init_sleeplock_timers(&pwr_mgmnt_ctx);
}

static void pwr_mgmnt_init_sleeplock_timers(pwr_mgmnt_ctx_t *ctx)
{
    if (ctx->auto_light_sleep_timeout > 0)
    {
        const esp_timer_create_args_t light_sleep_timer_args = {
            .callback = &light_sleep_timer_callback};

        ESP_ERROR_CHECK(esp_timer_create(&light_sleep_timer_args, &ctx->light_sleep_timer_handle));
    }

    if (ctx->auto_deep_sleep_timeout > 0)
    {
        const esp_timer_create_args_t deep_sleep_timer_args = {
            .callback = &deep_sleep_timer_callback};

        ESP_ERROR_CHECK(esp_timer_create(&deep_sleep_timer_args, &ctx->deep_sleep_timer_handle));
    }
}

static void pwr_mgmt_init_batt_adc()
{
    const esp_timer_create_args_t get_voltage_timer_args = {
        .callback = &batt_voltage_timer_callback,
        .name = "pwr_mgmt_get_voltage"};

    ESP_ERROR_CHECK(esp_timer_create(&get_voltage_timer_args, &get_battery_voltage_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(get_battery_voltage_timer, PWR_MGMT_GET_VOLTAGE_PERIOD_MS * 1000));

    if (unit == ADC_UNIT_1)
    {
        adc1_config_width(width);
        adc1_config_channel_atten(channel, atten);
    }

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
}

static void batt_voltage_timer_callback(void *arg)
{
    uint32_t adc_reading = 0;
    for (int x = 0; x < 10; x++)
    {
        adc_reading += adc1_get_raw((adc1_channel_t)channel);
        vTaskDelay(pdMS_TO_TICKS(25));
    }

    adc_reading /= 10;

    uint32_t voltage = 2 * esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    //ESP_LOGI(TAG, "Battery Voltage: %f", voltage * 1.0f);
}