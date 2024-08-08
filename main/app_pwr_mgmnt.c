#include "app_pwr_mgmnt.h"
#include <stdint.h>
#include "esp_log.h"
#include "esp_sleep.h"

#define TAG "app_power_management"
#define PWR_EXT_WAKE_PIN 34
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