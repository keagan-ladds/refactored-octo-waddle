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
#include "nmea_parser.h"
#include "esp_log.h"
#include "app_ui.h"
#include "adxl345.h"
#include "lora_radio.h"
#include "loramac/loramac_serializer.h"
#include "loramac/loramac_message.h"
#include "loramac/loramac_crypto.h"
#include "loramac/loramac_parser.h"
#include "loramac/loramac_debug.c"
#include "app_pwr_mgmnt.h"

static const char *TAG = "gps_demo";

void lorawan_broadcast();
void lorawan_rx_done_callback(uint8_t *payload, uint8_t payload_length, uint16_t rssi, uint8_t snr);
void lorawan_tx_done_callback(void);

#define TIME_ZONE (+8)   // Beijing Time
#define YEAR_BASE (2000) // date in GPS starts from 2000

#define RX1_DELAY 900
#define RX2_DELAY 2000

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

uint8_t join_eui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t dev_eui[] = {0x5F, 0x99, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70};
uint8_t app_key[] = {0x27, 0xE8, 0xC6, 0x0A, 0xAA, 0x4A, 0x86, 0x8A, 0xF0, 0xA8, 0x85, 0xD8, 0x0B, 0x65, 0x17, 0x57};

uint8_t join_accept[] = {0x20, 0x73, 0xf7, 0x97, 0x38, 0x2b, 0x32, 0x8a, 0x03, 0x7b, 0x60, 0x0f, 0xe7, 0xea, 0x21, 0x10, 0xcb, 0x81, 0x84, 0x36, 0xb4, 0xa9, 0xb8, 0xf4, 0x9b, 0x57, 0x8b, 0x1f, 0x9d, 0x0b, 0xd7, 0x63, 0x59};
void app_main(void)
{
    printf("Hello world!\n");

    lora_radio_config_t lora_radio_config = {
        .rx_done = lorawan_rx_done_callback,
        .tx_done = lorawan_tx_done_callback};
    lora_radio_init(lora_radio_config);

    adxl345_init();

    lorawan_rx_done_callback(&join_accept, sizeof(join_accept), 0, 0);

    app_ui_init();
    // lorawan_broadcast();

    vTaskDelay(15000 / portTICK_PERIOD_MS);
    // app_ui_sleep();
    // app_pwr_sleep();

    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void lorawan_tx_done_callback(void)
{
    vTaskDelay(RX1_DELAY / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Listening in RX1 Window");

    lora_radio_set_rx_params(LORA_RADIO_LORA, LORA_BW_125, LORA_SF_7);
    lora_radio_set_channel(868100000);
    lora_radio_receive(0xFFFFFF); /*

     vTaskDelay((RX2_DELAY - RX1_DELAY) / portTICK_PERIOD_MS);
     ESP_LOGI(TAG, "Listening in RX2 Window");

     lora_radio_set_rx_params(LORA_RADIO_LORA, LORA_BW_125, LORA_SF_9);
     lora_radio_set_channel(869525000);
     lora_radio_receive(0xFFFFFF);*/
}

void lorawan_rx_done_callback(uint8_t *payload, uint8_t payload_length, uint16_t rssi, uint8_t snr)
{
    ESP_LOGI(TAG, "Received %d bytes, RSSI: %d, SNR: %d.", payload_length, rssi, snr);
    loramac_message_join_accept_t msg = {
        .buffer = payload,
        .buffer_size = payload_length};

    loramac_parser_error_t err = loramac_parse_join_accept(&msg);
    if (err == 0)
    {
        ESP_LOGI(TAG, "Successfully parsed join-accept message.");
        loramac_debug_dump_join_accept(&msg);

        loramac_crypto_aes_encrypt(&app_key, payload + LORAMAC_MHDR_FIELD_SIZE, payload_length - LORAMAC_MHDR_FIELD_SIZE, payload + LORAMAC_MHDR_FIELD_SIZE);
        loramac_parse_join_accept(&msg);
        
        loramac_debug_dump_join_accept(&msg);
    }
}

void lorawan_broadcast()
{
    uint8_t buff[255];

    loramac_message_join_request_t join_request_msg;
    join_request_msg.buffer = &buff;
    join_request_msg.buffer_size = sizeof(buff);
    join_request_msg.mhdr.frame_type = LORAMAC_FRAME_JOIN_REQUEST;
    join_request_msg.mhdr.major = 0x00;
    memcpy(&join_request_msg.join_eui, &join_eui, sizeof(join_eui));
    memcpy(&join_request_msg.dev_eui, &dev_eui, sizeof(dev_eui));
    join_request_msg.dev_nonce = 16;

    loramac_serialize_join_request(&join_request_msg);
    loramac_crypto_aes_cmac(&app_key, join_request_msg.buffer, join_request_msg.buffer_size - 4, &join_request_msg.mic);
    loramac_serialize_join_request(&join_request_msg);

    ESP_LOG_BUFFER_HEX(TAG, join_request_msg.buffer, join_request_msg.buffer_size);

    lora_radio_set_public_network(true);

    lora_radio_set_tx_params(LORA_RADIO_LORA, 0x16, LORA_BW_125, LORA_SF_7);
    lora_radio_set_channel(868100000);

    lora_radio_send(join_request_msg.buffer, join_request_msg.buffer_size);
}

/*

    */
