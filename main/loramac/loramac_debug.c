#include "loramac_debug.h"
#include "esp_log.h"

#define TAG_1 "loramac"
void loramac_debug_dump_join_accept(loramac_message_join_accept_t *msg)
{
    ESP_LOGI(TAG_1, "AppNonce: {0x%X, 0x%X, 0x%X}", msg->app_nonce[0], msg->app_nonce[1], msg->app_nonce[2]);
    ESP_LOGI(TAG_1, "DevAddr: {0x%08lX}", msg->dev_addr);
}