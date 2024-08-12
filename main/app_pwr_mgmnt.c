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

#define TAG "app_power_management"
#define PWR_EXT_WAKE_PIN 34

static pwr_mgmnt_ctx_t pwr_mgmnt_ctx;
static uint32_t sleeplock_count = 0;

static prw_mgmnt_sleeplock_t sleeplock_config[PWR_MGMNT_MAX_SLEEPLOCKS];

static void pwr_mgmnt_init_ctx(pwr_mgmnt_ctx_t *ctx, const pwr_mgmnt_init_config_t *config);
static void pwr_mgmnt_set_power_on_reason(pwr_mgmnt_ctx_t *ctx);
static void pwr_mgmnt_init_sleeplock(pwr_mgmnt_ctx_t *ctx);
static void pwr_mgmnt_init_sleeplock_timers(pwr_mgmnt_ctx_t *ctx);

void pwr_mgmnt_init(const pwr_mgmnt_init_config_t *config)
{
    pwr_mgmnt_init_ctx(&pwr_mgmnt_ctx, config);
    pwr_mgmnt_init_sleeplock(&pwr_mgmnt_ctx);
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