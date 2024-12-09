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

struct led_state new_state;

SemaphoreHandle_t haptic_mutex;

#define LONG_PRESS_DURATION 2000 // 2 seconds in milliseconds

// expand this to include more "apps" -- think flash light, etc
enum app_states {
    APP_SPLASH_SCREEN,
    APP_GAME,
    APP_BLUETOOTH_CONTROL,
    APP_MAX_STATES // This is to keep track of the number of states
};

// Current state of the application
enum app_states current_state = APP_SPLASH_SCREEN;

// Last state of the button
bool last_button_state = false;
uint32_t button_press_time = 0;
bool button_press_detected = false;


float get_next_hue(void) {
    static float hue = 0.0f;

    // Increment hue for next call
    hue += 0.01f; // Adjust this value to control the speed of the color change
    if (hue >= 1.0f) hue = 0.0f;

    return hue;
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


uint32_t game_delay_reset = 0;
uint32_t game_over_haptic = 0;

void game_loop(void){

    MotorUpdate update_a = {0, 100, 1};

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
}

uint8_t short_press_detect(void){
    uint8_t short_press = 0;

    if(gpio_get_level(1) == 1){
        short_press = 1;
    }

    return short_press;
}


uint8_t button_press_detect(uint32_t press_duration) {
    // Read the current state of the button
    bool current_button_state = gpio_get_level(1);

    uint8_t press = 0;

    // Check for button press start
    if (current_button_state && !last_button_state) {
        button_press_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        button_press_detected = false; // Reset detection flag
    } else if (current_button_state && last_button_state) {
        uint32_t elapsed_time = xTaskGetTickCount() * portTICK_PERIOD_MS - button_press_time;
        if (elapsed_time > press_duration && !button_press_detected) {
            press = 1;
            button_press_detected = true; // Set detection flag
        }
    } else if (!current_button_state && last_button_state) {
        // Button released, reset the button press time and detection flag
        button_press_time = 0;
        button_press_detected = false;
    }

    last_button_state = current_button_state;

    return press;
}

// this is the main logic loop
void main_loop(void *pvParameters){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20); // For 50 Hz (1000 ms / 50 = 20 ms)

    while(1){

        // a delay that allows us to precisely run at a specified frequency
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Run the current application state
        switch (current_state) {
            case APP_SPLASH_SCREEN:
                // Code to run the splash screen app
                float hue = get_next_hue();
                uint32_t next_color = hsv_to_rgb(hue, 1.0f, 0.5f);
                for(int i = 0; i < 8; i++){
                    for(int b = 0; b < 15; b++)
                        display_buffer[i][b] = next_color;
                }

                if(button_press_detect(100) == 1){
                    // Transition to the next state
                    current_state = (current_state + 1) % APP_MAX_STATES;
                }

                break;
            case APP_GAME:
                // Code to run the game app
                game_loop();

                if(button_press_detect(1000) == 1){
                    // Transition to the next state
                    current_state = (current_state + 1) % APP_MAX_STATES;

                }

                break;
            case APP_BLUETOOTH_CONTROL:
                // Code to run the Bluetooth control app
                // write the bt buffer to the main display buffer

                // check if there is an active connection
                // if there is an active connection, write the buffer
                // else write the bt logo
                if(gatt_get_num_pkgs_recieved() > 0) {
                    for (int i = 0; i < 8; i++) {
                        for (int b = 0; b < 15; b++) {
                            display_buffer[i][b] = display_bt_buffer[i][b];
                        }
                    }
                } else {
                    display_draw_bt_logo();
                    // Write a string to the display
//                    display_write_string("BLE", 0, 0);
                }

                if(button_press_detect(1000) == 1) {
                    // Transition to the next state
                    current_state = (current_state + 1) % APP_MAX_STATES;
                }
                break;

            default:
                // Handle invalid state (should not occur)
                break;
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
    nimble_port_init();
    ble_hs_cfg.sync_cb = sync_cb;
    ble_hs_cfg.reset_cb = reset_cb;
    gatt_svr_init();
    ble_svc_gap_device_name_set(device_name);
    nimble_port_freertos_init(host_task);

    // initilize interrupt and input on IO1
    configure_gpio_interrupt();
    gpio_intr_evt_queue = gpio_interrupt_get_evt_queue();
    xTaskCreate(gpio_interrupt_task, "gpio_interrupt_task", 2048, NULL, 2, NULL);

    // haptic feedback
    motor_init();

    // initialize the task for game logic loop
    xTaskCreate(main_loop, "main_loop", 4096,
                NULL, 0, NULL);



    while (1) {

        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay

    }
}
