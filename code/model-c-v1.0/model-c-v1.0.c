#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "keycodes.h"

// states
#define START 0
#define NUMPAD 1
#define CALC 2
#define ERROR 3
uint8_t state = START;

// display setup
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define I2C_TOGGLE 0

// mode switch
#define MODE_PIN 14

// matrix and key setup
#define ROWS 5
#define COLS 4
#define DEBOUNCE 5  // ms delay
#define CONFIRM 3   // number of consecutive reads the key was confirmed for

const uint8_t row_pins[ROWS] = {3, 4, 5, 6, 7};
const uint8_t col_pins[COLS] = {8, 9, 10, 11};

uint8_t states[ROWS][COLS] = {0}; // state array for switches


int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}

void init_matrix() {
    // initialize rows as inputs pulled up
    for (uint8_t i = ROWS; i >= 0; --i) {
        gpio_init(row_pins[i]);
        gpio_set_dir(row_pins[i], GPIO_IN);
        gpio_pull_up(row_pins[i]);  // make rows inputs being pulled up - when column set to low
    }

    // initialize columns as outputs which turn off when scanning each column - pulls row down to indicate hit
    for (uint8_t i = COLS; i >= 0; --i) {
        gpio_init(col_pins[i]);
        gpio_set_dir(col_pins[i], GPIO_OUT);
        gpio_put(col_pins[i], 1);   // make constantly high - make low when checking column
    }
}

void init_state() {
    gpio_init(MODE_PIN);
    gpio_set_dir(MODE_PIN, GPIO_IN);
    gpio_pull_up(MODE_PIN);

    if (gpio_get(MODE_PIN) == 0) { state = CALC; }
    else { state = NUMPAD; }
}

int main() {
    stdio_init_all();

    init_matrix();
    init_state();
    
    if (I2C_TOGGLE) {
        i2c_init(I2C_PORT, 400*1000);
        gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
        gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
        gpio_pull_up(I2C_SDA);
        gpio_pull_up(I2C_SCL);
    }
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    

    // Timer example code - This example fires off the callback after 2000ms
    add_alarm_in_ms(100, alarm_callback, NULL, false);
    // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer


    while (true) {
        printf("System Clock Frequency is %d Hz\n", clock_get_hz(clk_sys));
        printf("USB Clock Frequency is %d Hz\n", clock_get_hz(clk_usb));
        sleep_ms(1000);
    }
}
