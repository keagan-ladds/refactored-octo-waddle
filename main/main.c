/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nmea_parser.h"
#include "esp_log.h"
#include "app_ui.h"
#include "adxl345.h"
#include "loramac/lora_radio.h"
#include "loramac/loramac_serializer.h"
#include "loramac/loramac_message.h"
#include "loramac/loramac_crypto.h"
#include "loramac/loramac_parser.h"
#include "loramac/loramac_debug.c"
#include "app_pwr_mgmnt.h"

static const char *TAG = "gps_demo";

void lorawan_join();
void lorawan_rx_done_callback(uint8_t *payload, uint8_t payload_length, uint16_t rssi, uint8_t snr);
void lorawan_tx_done_callback(void);

#define TIME_ZONE (+8)   // Beijing Time
#define YEAR_BASE (2000) // date in GPS starts from 2000

#define RX1_DELAY 4900
#define RX2_DELAY 6000

static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gps_t *gps = NULL;
    switch (event_id)
    {
    case GPS_UPDATE:
        gps = (gps_t *)event_data;
        /* print information parsed from GPS statements */
        ESP_LOGI(TAG, "%d/%d/%d %d:%d:%d => \r\n"
                      "\t\t\t\t\t\tlatitude   = %.05f°N\r\n"
                      "\t\t\t\t\t\tlongitude = %.05f°E\r\n"
                      "\t\t\t\t\t\taltitude   = %.02fm\r\n"
                      "\t\t\t\t\t\tspeed      = %fm/s",
                 gps->date.year + YEAR_BASE, gps->date.month, gps->date.day,
                 gps->tim.hour + TIME_ZONE, gps->tim.minute, gps->tim.second,
                 gps->latitude, gps->longitude, gps->altitude, gps->speed);
        break;
    case GPS_UNKNOWN:
        /* print unknown statements */
        ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
        break;
    default:
        break;
    }
}


void app_main(void)
{
    printf("Hello world!\n");

    const pwr_mgmnt_init_config_t pwr_mgmnt_config = {
        .auto_deep_sleep_timeout = 5000,
        .auto_light_sleep_timeout = 10};

    const pwr_mgmnt_sleeplock_config_t sleep_config = {
        .name = "main app"
    };

    const loramac_init_config_t lora_config = {
        .channel = 0,
        .dr = DR_3
    };

    const pwr_mgmnt_sleeplock_handle_t hdl;
    
    pwr_mgmnt_init(&pwr_mgmnt_config);

    pwr_mgmnt_register_sleeplock(&sleep_config, &hdl);
    pwr_mgmnt_set_sleeplock(hdl);
    pwr_mgmnt_release_sleeplock(hdl);

    loramac_init(&lora_config);

    app_ui_init();
    adxl345_init();

    const char* str = "Hello World!";
    //loramac_send(str, strlen(str));

    loramac_join(false, -1);


    vTaskDelay(15000 / portTICK_PERIOD_MS);
    // app_ui_sleep();
    // app_pwr_sleep();

    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}







