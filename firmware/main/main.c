// main.c

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_cpu.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "motor.h"

// BLE
#include "gap.h"
#include "gatt_svr.h"
#include "nvs_flash.h"
#include "esp_bt.h"

// user generated
#include "ws2812_control.h"
#include "motor.h"
#include "gpio_interrupt.h"
#include "display_helpers.h"
#include "game.h"

// random
#include "esp_random.h"

// usb drivers
#include "driver/usb_serial_jtag.h"
#include "esp_vfs_dev.h"
#include "driver/usb_serial_jtag_vfs.h"

static const char *TAG = "main";

#define RED   0x001000
#define GREEN 0x100000
#define BLUE  0x000010

#include <stdint.h>
#include <math.h>

#define MAX_BRIGHTNESS 0.1f // 20% brightness

struct led_state new_state;

SemaphoreHandle_t haptic_mutex;

uint32_t get_next_color_full_spectrum(void) {
    static uint32_t hue = 0;
    float r, g, b;

    // Convert hue to RGB
    float h = hue / 65536.0f;
    float x = 1 - fabsf(fmodf(h * 6, 2) - 1);

    if (h < 1.0f/6.0f)      { r = 1; g = x; b = 0; }
    else if (h < 2.0f/6.0f) { r = x; g = 1; b = 0; }
    else if (h < 3.0f/6.0f) { r = 0; g = 1; b = x; }
    else if (h < 4.0f/6.0f) { r = 0; g = x; b = 1; }
    else if (h < 5.0f/6.0f) { r = x; g = 0; b = 1; }
    else                    { r = 1; g = 0; b = x; }

    // Apply brightness limit
    r *= MAX_BRIGHTNESS * 255;
    g *= MAX_BRIGHTNESS * 255;
    b *= MAX_BRIGHTNESS * 255;

    // Increment hue for next call
    hue++;
    if (hue >= 65536) hue = 0;

    // Convert to GRB format
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
}

QueueHandle_t gpio_intr_evt_queue = NULL;
void gpio_interrupt_task(void *pvParameters)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_intr_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGD(TAG, "GPIO[%lu] interrupt occurred!\n", io_num);
            if (gpio_get_level(io_num) == 1) {

                // this could be wrapped up into its own thing instead of dealing with semaphores
                if (xSemaphoreTake(haptic_mutex, portMAX_DELAY) == pdTRUE) {

                    MotorUpdate update_a = {0, 100, 1};
                    xQueueSend(motor_queue[0], &update_a, 0);

                    vTaskDelay(30 / portTICK_PERIOD_MS);  // Delay

                    // turn off the motor
                    update_a.speed_percent = 0;
                    xQueueSend(motor_queue[0], &update_a, 0);
                    xSemaphoreGive(haptic_mutex);
                }
            }
        }
    }
}


// count how many digits in a number
int count_digits(int number) {
    if (number == 0) return 1;
    int count = 0;
    while (number != 0) {
        number /= 10;
        count++;
    }

    return count;
}


// this is the main logic loop
void main_loop(void *pvParameters){

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20); // For 50 Hz (1000 ms / 50 = 20 ms)
    MotorUpdate update_a = {0, 100, 1};

    uint32_t game_delay_reset = 0;
    uint32_t game_over_haptic = 0;

    while(1){

        // a delay that allows us to precisely run at a specified frequency
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // if the game is over, reset if user presses the button
        if(game_get_game_over() == 1){

            // let the user see what happened for 20 frames
            if(game_delay_reset > 20){
                // get the game score
                int score = (int)game_get_score();
                if(score > 9999)
                    score = 9999; // display limit

                // write the display buffer to the new state buffer
                display_clear();

                // determine the center of our string
                int start_x = (15 - (count_digits(score) * 4 - 1)) / 2;

                // write the score to screen
                display_number(score, start_x,2);
            }

            if(game_delay_reset > 0 && game_delay_reset < 30 && game_over_haptic == 0){
                if (xSemaphoreTake(haptic_mutex, portMAX_DELAY) == pdTRUE) {
                    update_a.speed_percent = 100;
                    xQueueSend(motor_queue[0], &update_a, 0);
                    game_over_haptic = 1;
                }

            } else if (game_delay_reset > 30 && game_over_haptic == 1){
                update_a.speed_percent = 0;
                xQueueSend(motor_queue[0], &update_a, 0);
                game_over_haptic = 0;
                xSemaphoreGive(haptic_mutex);
            }

            if(game_delay_reset > 30){

                // wait for a button press to restart
                if (gpio_get_level(1) == 1) {
                    game_reset();
                    game_delay_reset = 0;
                }
            }

            // delay reset counter
            game_delay_reset++;

        } else{
            // run the game logic
            game_run();

            // write the display buffer
            game_compile_buffer(display_buffer);

        }

        display_write_buffer(&new_state);
        ws2812_write_leds(new_state);

    }

}

void app_main(void)
{
    usb_serial_jtag_driver_config_t usb_serial_jtag_config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    usb_serial_jtag_driver_install(&usb_serial_jtag_config);

    usb_serial_jtag_vfs_use_driver();

    // wait a little bit for the drivers to kick in
    vTaskDelay(1000 / portTICK_PERIOD_MS);


    ESP_LOGI(TAG, "starting up");

    haptic_mutex = xSemaphoreCreateMutex();

    // init led driver
    ws2812_control_init();

    ESP_LOGI(TAG, "ws driver initialized");

    // BLE Setup -------------------
//    nimble_port_init();
//    ble_hs_cfg.sync_cb = sync_cb;
//    ble_hs_cfg.reset_cb = reset_cb;
//    gatt_svr_init();
//    ble_svc_gap_device_name_set(device_name);
//    nimble_port_freertos_init(host_task);

    // initilize interrupt and input on IO1
    configure_gpio_interrupt();
    gpio_intr_evt_queue = gpio_interrupt_get_evt_queue();
    xTaskCreate(gpio_interrupt_task, "gpio_interrupt_task", 2048, NULL, 2, NULL);

    // haptic feedback
    motor_init();

    // send a test run
    // TODO: reuse the controller from Racer for auto timeouts on haptic feedback
    MotorUpdate update_a = {0, 100, 1};
    xQueueSend(motor_queue[0], &update_a, 0);

    vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay

    // turn off the motor
    update_a.speed_percent = 0;
    xQueueSend(motor_queue[0], &update_a, 0);


    // initialize the task for game logic loop
    xTaskCreate(main_loop, "main_loop", 4096,
                NULL, 0, NULL);



//
//    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay


    while (1) {

//        for(int i = 0; i < 120; i++)
//            new_state.leds[i] = get_next_color_full_spectrum();

        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay

    }
}
