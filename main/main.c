/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "nmea_parser.h"
#include "esp_log.h"
#include "app_ui.h"
#include "adxl345.h"
#include "lora_radio.h"

static const char *TAG = "gps_demo";

#define TIME_ZONE (+8)   // Beijing Time
#define YEAR_BASE (2000) // date in GPS starts from 2000

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

    lora_radio_init();
    lora_radio_set_channel(869525000);
    lora_radio_set_rx_params(LORA_RADIO_LORA, LORA_BW_125, LORA_SF_9);
    lora_radio_receive(0xFFFFFF);
    //adxl345_init();

    uint8_t data[6];

    /*nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
    nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);
    app_ui_init();*/
    while (true)
    {

        /*adxl345_read_val(&data);

        const int x = (((int)data[1]) << 8) | data[0];
        const int y = (((int)data[3]) << 8) | data[2];
        const int z = (((int)data[5]) << 8) | data[4];

        ESP_LOGI(TAG, "X: %d, Y: %d, Z: %d", x, y, z);
        */
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
