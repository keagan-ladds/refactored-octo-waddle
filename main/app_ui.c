#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/xtensa_api.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "app_ui.h"
#include "esp_timer.h"

#include "lvgl.h"
#include "lvgl_helpers.h"
#include "i2c_manager.h"
#include "lvgl_tft/ssd1306.h"

#define LV_TICK_PERIOD_MS 1
#define APP_UI_TAG "APP_UI"

static SemaphoreHandle_t ui_semaphore_handle = NULL;
static void ui_task_handler(void *arg);
static void lv_tick_task(void *arg);

lv_obj_t *label1 = NULL;

void app_ui_demo_ui(void)
{
    /* Get the current screen  */
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);

    /*Create a Label on the currently active screen*/
    label1 = lv_label_create(scr, NULL);

    lv_anim_t a;
    lv_anim_init(&a);

    /* MANDATORY SETTINGS
     *------------------*/

    /*Set the "animator" function*/
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_local_text_opa);
    lv_anim_set_var(&a, label1);
    lv_anim_set_time(&a, 200);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_100);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);

    /*Modify the Label's text*/
    lv_label_set_text(label1, LV_SYMBOL_WIFI);
    lv_label_set_align(label1, LV_LABEL_ALIGN_CENTER);
    /* Align the Label to the center
     * NULL means align on parent (which is the screen now)
     * 0, 0 at the end means an x, y offset after alignment*/
    lv_obj_align(label1, NULL, LV_ALIGN_CENTER, 0, 0);
}

void app_ui_init(void)
{
    ui_semaphore_handle = xSemaphoreCreateMutex();

    lvgl_i2c_locking(i2c_manager_locking());
    lv_init();

    /* Initialize SPI or I2C bus used by the drivers */
    lvgl_driver_init();

    lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);

    static lv_disp_buf_t disp_buf;

    uint32_t size_in_px = DISP_BUF_SIZE;

#if defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820 || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306

    /* Actual size in pixels, not bytes. */

#endif

    size_in_px *= 8;

    /* Initialize the working buffer depending on the selected display.
     * NOTE: buf2 == NULL when using monochrome displays. */
    lv_disp_buf_init(&disp_buf, buf1, NULL, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;

    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    ESP_LOGI(APP_UI_TAG, "Starting UI Task");

    app_ui_demo_ui();
    xTaskCreatePinnedToCore(ui_task_handler, "gui", 4096 * 2, NULL, 0, NULL, 1);
}

void app_ui_sleep()
{
    lv_label_set_text(label1, "Going to Sleep");
    lv_label_set_align(label1, LV_LABEL_ALIGN_CENTER);
    /* Align the Label to the center
     * NULL means align on parent (which is the screen now)
     * 0, 0 at the end means an x, y offset after alignment*/
    lv_obj_align(label1, NULL, LV_ALIGN_CENTER, 0, 0);

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    ssd1306_sleep_in();
}

static void ui_task_handler(void *arg)
{
    (void)arg;

    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(ui_semaphore_handle, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(ui_semaphore_handle);
        }
    }

    // TODO: Free display buffer
    vTaskDelete(NULL);
}

static void lv_tick_task(void *arg)
{
    (void)arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}