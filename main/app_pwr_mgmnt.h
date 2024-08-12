#ifndef __APP_PWR_MGMNT_H__
#define __APP_PWR_MGMNT_H__

#include <stdbool.h>
#include <stdint.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define PWR_MGMNT_MAX_SLEEPLOCKS 8

typedef enum
{
    WAKE_REASON_RESET,
    WAKE_REASON_TIMER,
    WAKE_REASON_EXT1
} pwr_mgmnt_wake_reason_t;

typedef struct
{
    bool wake_from_sleep;
    pwr_mgmnt_wake_reason_t wake_reason;
} pwr_mgmnt_power_on_reason_t;

typedef struct
{
    uint32_t auto_light_sleep_timeout;
    uint32_t auto_deep_sleep_timeout;
    esp_timer_handle_t light_sleep_timer_handle;
    esp_timer_handle_t deep_sleep_timer_handle;
    pwr_mgmnt_power_on_reason_t power_on_reason;
    EventGroupHandle_t sleeplock_event_group_handle;
} pwr_mgmnt_ctx_t;

typedef struct
{
    uint32_t auto_light_sleep_timeout;
    uint32_t auto_deep_sleep_timeout;
} pwr_mgmnt_init_config_t;

typedef struct
{
    const char *name;

} pwr_mgmnt_sleeplock_config_t;

typedef void *pwr_mgmnt_sleeplock_handle_t;

typedef struct
{
    uint16_t index;
    const char *name;
} prw_mgmnt_sleeplock_t;

void pwr_mgmnt_init(const pwr_mgmnt_init_config_t *config);
void pwr_mgmnt_register_sleeplock(const pwr_mgmnt_sleeplock_config_t *config, pwr_mgmnt_sleeplock_handle_t *handle);
void pwr_mgmnt_set_sleeplock(pwr_mgmnt_sleeplock_handle_t handle);
void pwr_mgmnt_release_sleeplock(pwr_mgmnt_sleeplock_handle_t handle);

void app_pwr_sleep(void);

#endif