#include <stdint.h>
#include <string.h>
#include "loramac.h"
#include "loramac_crypto.h"
#include "loramac_handler.h"
#include "loramac_message.h"
#include "loramac_serializer.h"
#include "esp_timer.h"
#include "esp_log.h"

#define TAG "loramac"
static loramac_ctx_t context;

// DEV_EUI: 0059AC00001B2EB8
// APP_EUI: 0059AC0000010D19
// APP_KEY: b5a69b6df41f53d39108917bdcdf72e5

#define LORAMAC_RX_TIMEOUT 3000
#define LORAMAC_DEFAULT_RX1_DELAY 1000
#define LORAMAC_DEFAULT_RX2_DELAY 2000
#define LORAMAC_RX1_JOIN_DELAY 4900
#define LORAMAC_RX2_JOIN_DELAY 5900

static void loramac_init_ctx(loramac_ctx_t *ctx, const loramac_init_config_t *config);
static void loramac_init_radio(const loramac_ctx_t *ctx);
static void loramac_rx_timer_callback(void *arg);
static void loramac_rx_timeout_callback(void);
static void loramac_tx_done_callback(void);
static void loramac_rx_done_callback(uint8_t *payload, uint8_t payload_length, uint16_t rssi, uint8_t snr);

loramac_err_t loramac_init(const loramac_init_config_t *config)
{
    loramac_init_ctx(&context, config);
    loramac_init_radio(&context);
    loramac_crypto_init();

    return LORAMAC_OK;
}

loramac_err_t loramac_join(bool force, uint32_t timeout)
{
    if (context.joined && !force)
    {
        ESP_LOGW(TAG, "Already joined to network, ignoring join request.");
        return LORAMAC_OK;
    }

    context.joined = false;

    lora_radio_set_public_network(true);

    uint8_t buff[255];

    loramac_message_join_request_t msg;
    msg.buffer = &buff;
    msg.buffer_size = sizeof(buff);
    msg.mhdr.frame_type = LORAMAC_FRAME_JOIN_REQUEST;
    msg.mhdr.major = 0x00;
    memcpy(&msg.join_eui, &join_eui, sizeof(join_eui));
    memcpy(&msg.dev_eui, &dev_eui, sizeof(dev_eui));

    loramac_crypto_prepare_join_request(&msg);
    lora_radio_set_tx_params(LORA_RADIO_LORA, context.tx_config.tx_power,
                             context.tx_config.bandwidth,
                             context.tx_config.spreading_factor);

    lora_radio_set_channel(context.tx_config.frequency_hz);
    lora_radio_send(msg.buffer, msg.buffer_size);

    return LORAMAC_OK;
}


loramac_err_t loramac_send(void *payload, uint16_t payload_length)
{
    static uint8_t buff[255];
    memset(&buff, 0, sizeof(255));

    loramac_message_mac_t msg;
    msg.buffer = &buff;
    msg.mhdr.frame_type = LORAMAC_FRAME_DATA_UPLINK_UNCONFIRMED;
    msg.mhdr.major = 0x00;

    msg.direction = 0;
    msg.fhdr.frame_ctrl.uplink.ack = 0;
    msg.fhdr.frame_ctrl.uplink.adr = 0;
    msg.fhdr.frame_ctrl.uplink.adr_ack_req = 0;
    msg.fhdr.frame_ctrl.uplink.frame_options_len = 0;
    msg.port = 1;
    msg.mic = 0;

    memcpy(msg.payload, payload, payload_length);
    msg.payload_length = payload_length;

    loramac_err_t err;
    err = loramac_crypto_prepare_mac_message(&msg);

    if (err != 0)
    {
        ESP_LOGE(TAG, "Couldn't prepare message");
        return -1;
    }

    context.rx1_delay = LORAMAC_DEFAULT_RX1_DELAY;
    context.rx2_delay = LORAMAC_DEFAULT_RX2_DELAY;

    lora_radio_set_tx_params(LORA_RADIO_LORA, context.tx_config.tx_power,
                             context.tx_config.bandwidth,
                             context.tx_config.spreading_factor);

    lora_radio_set_channel(context.tx_config.frequency_hz);
    lora_radio_send(msg.buffer, msg.buffer_size);

    ESP_LOGI(TAG, "Transmitting %d bytes @ %lu Hz, BW: %d, SF: %d", msg.buffer_size, context.tx_config.frequency_hz, context.tx_config.bandwidth, context.tx_config.spreading_factor);

    return LORAMAC_OK;
}

static void loramac_init_tx_config(tx_window_config_t *config, uint8_t channel, uint8_t dr)
{
    config->frequency_hz = channels[channel].frequency_hz;
    config->bandwidth = data_rate_bandwidth[dr];
    config->spreading_factor = data_rate_spreading_factors[dr];
    config->tx_power = 0x16;
    config->dr = dr;
}

static void loramac_init_rx_config(rx_window_config_t *config, uint32_t freq_hz, uint8_t dr)
{
    config->frequency_hz = freq_hz;
    config->bandwidth = data_rate_bandwidth[dr];
    config->spreading_factor = data_rate_spreading_factors[dr];
}

static void loramac_init_ctx(loramac_ctx_t *ctx, const loramac_init_config_t *config)
{
    const esp_timer_create_args_t rx1_timer_args = {.callback = &loramac_rx_timer_callback, .arg = (void *)&ctx->rx1_window_config};
    const esp_timer_create_args_t rx2_timer_args = {.callback = &loramac_rx_timer_callback, .arg = (void *)&ctx->rx2_window_config};
    const esp_timer_create_args_t rx_timeout_timer_args = {.callback = &loramac_rx_timeout_callback};

    ESP_ERROR_CHECK(esp_timer_create(&rx1_timer_args, &ctx->rx1_timer_handle));
    ESP_ERROR_CHECK(esp_timer_create(&rx2_timer_args, &ctx->rx2_timer_handle));
    ESP_ERROR_CHECK(esp_timer_create(&rx_timeout_timer_args, &ctx->rx_timeout_timer_handle));

    ctx->channel = config->channel;

    loramac_init_tx_config(&ctx->tx_config, config->channel, config->dr);

    ctx->rx1_delay = 4900;
    ctx->rx2_delay = 5900;


    ctx->rx2_window_config.frequency_hz = 869525000;
    ctx->rx2_window_config.bandwidth = LORA_BW_125;
    ctx->rx2_window_config.spreading_factor = LORA_SF_12;
}

static void loramac_init_radio(const loramac_ctx_t *ctx)
{
    lora_radio_config_t lora_radio_config = {
        .rx_done = &loramac_rx_done_callback,
        .tx_done = &loramac_tx_done_callback};
    lora_radio_init(lora_radio_config);
}

static void loramac_rx_timer_callback(void *arg)
{
    rx_window_config_t *config = (rx_window_config_t *)arg;

    ESP_LOGI(TAG, "Listening for RX @ %lu Hz, BW: %d, SF: %d", config->frequency_hz, config->bandwidth, config->spreading_factor);

    lora_radio_set_rx_params(LORA_RADIO_LORA, config->bandwidth, config->spreading_factor);
    lora_radio_set_channel(config->frequency_hz);
    lora_radio_receive(0xFFFFFF);

    //esp_timer_stop(context.rx_timeout_timer_handle);
    esp_timer_start_once(context.rx_timeout_timer_handle, LORAMAC_RX_TIMEOUT * 1000);
}

static void loramac_rx_timeout_callback(void)
{
    ESP_LOGI(TAG, "Recieve timeout, putting radio in standby.");
    lora_radio_standby();
}

static void loramac_tx_done_callback(void)
{
    // Configure RX1 based on TX
    loramac_init_rx_config(&context.rx1_window_config, context.tx_config.frequency_hz, context.tx_config.dr);

    esp_timer_start_once(context.rx1_timer_handle, context.rx1_delay * 1000);
    esp_timer_start_once(context.rx2_timer_handle, context.rx2_delay * 1000);
}

static void loramac_rx_done_callback(uint8_t *payload, uint8_t payload_length, uint16_t rssi, uint8_t snr)
{
    loramac_err_t err;
    ESP_LOGI(TAG, "Received %d bytes, RSSI: %d, SNR: %d.", payload_length, rssi, snr);

    err = loramac_handle(&context, payload, payload_length, rssi, snr);

    // We have succesfully handled a frame, stop timers.
    if (err == 0)
    {
        esp_timer_stop(context.rx1_timer_handle);
        esp_timer_stop(context.rx2_timer_handle);
        esp_timer_stop(context.rx_timeout_timer_handle);
    }
}
