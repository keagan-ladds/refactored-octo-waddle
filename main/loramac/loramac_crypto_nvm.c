#include "loramac_crypto.h"
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "nvs.h"

#define LORAMAC_CRYPTO_NVM_APP_ROOT_KEY "app_key"
#define LORAMAC_CRYPTO_NVM_NWK_SESS_KEY "nwk_s_key"
#define LORAMAC_CRYPTO_NVM_APP_SESS_KEY "app_s_key"
#define LORAMAC_CRYPTO_NVM_DEV_NONCE "dev_nonce"
#define LORAMAC_CRYPTO_NVM_DEV_ADDR "dev_addr"
#define LORAMAC_CRYPTO_NVM_FCNT_UP "fcnt_up"
#define LORAMAC_CRYPTO_NVM_FCNT_DOWN "fcnt_down"
#define LORAMAC_CRYPTO_NVM_DEV_NONCE_DEFAULT 0
#define TAG "loramac_crypto_nvm"

typedef struct
{
    uint8_t key_type;
    const char *key_name;
} nvm_key_t;

static nvm_key_t loramac_crypto_nvm_keys[] = {
    {LORAMAC_CRYPTO_KEY_APP_ROOT, LORAMAC_CRYPTO_NVM_APP_ROOT_KEY},
    {LORAMAC_CRYPTO_KEY_NWK_SESS, LORAMAC_CRYPTO_NVM_NWK_SESS_KEY},
    {LORAMAC_CRYPTO_KEY_APP_SESS, LORAMAC_CRYPTO_NVM_APP_SESS_KEY}};

static nvs_handle_t loramac_crypto_nvs_handle;

loramac_crypto_err_t loramac_crypto_nvm_init()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open("loramac_crypto", NVS_READWRITE, &loramac_crypto_nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }

    return err;
}

loramac_crypto_err_t loramac_crypto_nvm_get_key(uint8_t key_type, uint8_t *key)
{
    esp_err_t err;

    if (key_type > sizeof(loramac_crypto_nvm_keys) - 1)
    {
        ESP_LOGE(TAG, "Unknown key type 0x%X", key_type);
        return -1;
    }

    ESP_LOGI(TAG, "Getting key '%s' from nvm.", loramac_crypto_nvm_keys[key_type].key_name);

    size_t blob_size;

    err = nvs_get_blob(loramac_crypto_nvs_handle, loramac_crypto_nvm_keys[key_type].key_name, key, &blob_size);

    if (err != 0)
    {
        ESP_LOGE(TAG, "Unable to get key '%s' from nvm, error: 0x%X.", loramac_crypto_nvm_keys[key_type].key_name, err);
        return err;
    }

    if (blob_size != 16)
    {
        ESP_LOGW(TAG, "Unexpected key size");
    }
    return err;
}

loramac_crypto_err_t loramac_crypto_nvm_set_key(uint8_t key_type, uint8_t *key)
{
    if (key_type > sizeof(loramac_crypto_nvm_keys) - 1)
    {
        ESP_LOGE(TAG, "Unknown key type 0x%X", key_type);
        return -1;
    }

    ESP_LOGI(TAG, "Persisting key '%s' to nvm.", loramac_crypto_nvm_keys[key_type].key_name);

    esp_err_t err = nvs_set_blob(loramac_crypto_nvs_handle, loramac_crypto_nvm_keys[key_type].key_name, key, 16);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to persist key '%s'.", loramac_crypto_nvm_keys[key_type].key_name);
        return err;
    }

    return nvs_commit(loramac_crypto_nvs_handle);
}

loramac_crypto_err_t loramac_crypto_nvm_set_dev_nonce(uint16_t nonce)
{
    esp_err_t err = nvs_set_u16(loramac_crypto_nvs_handle, LORAMAC_CRYPTO_NVM_DEV_NONCE, nonce);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to persist dev_nonce '%d'.", nonce);
        return err;
    }

    return nvs_commit(loramac_crypto_nvs_handle);
}

loramac_crypto_err_t loramac_crypto_nvm_get_dev_nonce(uint16_t *nonce)
{
    esp_err_t err;
    err = nvs_get_u16(loramac_crypto_nvs_handle, LORAMAC_CRYPTO_NVM_DEV_NONCE, nonce);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        *nonce = (uint16_t)LORAMAC_CRYPTO_NVM_DEV_NONCE_DEFAULT;
        err = loramac_crypto_nvm_set_dev_nonce(*nonce);
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %X while getting dev_nonce.", err);
    }

    return err;
}

loramac_crypto_err_t loramac_crypto_nvm_set_dev_addr(uint32_t addr)
{

    ESP_LOGI(TAG, "Persisting dev_addr 0x%08lX to nvm.", addr);
    esp_err_t err = nvs_set_u32(loramac_crypto_nvs_handle, LORAMAC_CRYPTO_NVM_DEV_ADDR, addr);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to persist dev_addr '0x%08lX'.", addr);
        return err;
    }

    return nvs_commit(loramac_crypto_nvs_handle);
}

loramac_crypto_err_t loramac_crypto_nvm_get_dev_addr(uint32_t *addr)
{
    esp_err_t err;
    err = nvs_get_u32(loramac_crypto_nvs_handle, LORAMAC_CRYPTO_NVM_DEV_ADDR, addr);

    return err;
}

loramac_crypto_err_t loramac_crypto_nvm_set_frame_counter(uint8_t direction, uint32_t addr)
{
    ESP_LOGI(TAG, "Persisting frame count 0x%08lX  for direction %d to nvm.", addr, direction);
    esp_err_t err = nvs_set_u32(loramac_crypto_nvs_handle, direction == 0 ? LORAMAC_CRYPTO_NVM_FCNT_UP : LORAMAC_CRYPTO_NVM_FCNT_DOWN, addr);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to persist frame count '0x%08lX'.", addr);
        return err;
    }

    return nvs_commit(loramac_crypto_nvs_handle);
}

loramac_crypto_err_t loramac_crypto_nvm_get_frame_counter(uint8_t direction, uint32_t *addr)
{
    esp_err_t err;
    err = nvs_get_u32(loramac_crypto_nvs_handle, direction == 0 ? LORAMAC_CRYPTO_NVM_FCNT_UP : LORAMAC_CRYPTO_NVM_FCNT_DOWN, addr);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        *addr = (uint32_t)0;
        err = loramac_crypto_nvm_set_frame_counter(direction, *addr);
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %X while getting frame count for direction %d.", err, direction);
    }

    return err;
}