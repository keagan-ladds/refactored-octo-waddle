#include "loramac_debug.h"
#include "esp_log.h"

#define TAG_1 "loramac"
void loramac_debug_dump_join_accept(loramac_message_join_accept_t *msg)
{
    ESP_LOG_BUFFER_HEX(TAG_1, msg->buffer, msg->buffer_size);
    ESP_LOGI(TAG_1, "AppNonce: {0x%X, 0x%X, 0x%X}", msg->app_nonce[0], msg->app_nonce[1], msg->app_nonce[2]);
    ESP_LOGI(TAG_1, "NetID: {0x%X, 0x%X, 0x%X}", msg->net_id[0], msg->net_id[1], msg->net_id[2]);
    ESP_LOGI(TAG_1, "DevAddr: {0x%08lX}", msg->dev_addr);
    ESP_LOGI(TAG_1, "CFList Length: {0x%X}", msg->cf_list_length);
}

void loramac_debug_dump_join_request(loramac_message_join_request_t *msg)
{
    ESP_LOG_BUFFER_HEX(TAG_1, msg->buffer, msg->buffer_size);
    ESP_LOGI(TAG_1, "DevNonce: 0x%X", msg->dev_nonce);
}