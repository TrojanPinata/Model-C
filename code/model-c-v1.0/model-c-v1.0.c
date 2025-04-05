#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "tusb.h"
#include "bsp/board.h"

#include "keycodes.h"
#include "tusb_config.h"
#include "usb_descriptors.h"


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
#define HID_DELAY 5 // ms delay between checking/sending next keypress

const uint8_t row_pins[ROWS] = {3, 4, 5, 6, 7};
const uint8_t col_pins[COLS] = {8, 9, 10, 11};

uint8_t states[ROWS][COLS] = {0}; // state array for switches

volatile bool key_pressed = false;
uint8_t keys_pressed[6] = {0}; 
// Oh? You wanted NKRO? Tell me how you are planning to fit 6 of your greasy fingers on a numpad thats smaller than your hand.
// This isn't a gaming keyboard or something where theres any benefit at all. You must have thought I would take the time to
// make a custom descriptor before. Suprise, I have - and this is not the project for that.

/* typedef struct {
    uint8_t modifier;   // Shift, Ctrl, etc.
    uint8_t reserved;   // Always 0
    uint8_t keycode[6]; // Up to 6 keys at once
} hid_keyboard_report_t; */

hid_keyboard_report_t keyboard_report = {0};

// avoid doing too much in callbacks like this, just set the flag
void keypress_callback(uint gpio, uint32_t events) {
    key_pressed = true;
}

void init_matrix() {
    // initialize rows as inputs pulled up
    for (uint8_t i = 0; i < COLS; i++) {
        gpio_init(row_pins[i]);
        gpio_set_dir(row_pins[i], GPIO_IN);
        gpio_pull_up(row_pins[i]);  // make rows inputs being pulled up - when column set to low

        // setup callbacks for interrupt driven key handling
        gpio_set_irq_enabled_with_callback(row_pins[i], GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &keypress_callback);
    }

    // initialize columns as outputs which turn off when scanning each column - pulls row down to indicate hit
    // start all columns off in order to allow for interrupt when key hit - not polling here
    for (uint8_t i = 0; i < COLS; i++) {
        gpio_init(col_pins[i]);
        gpio_set_dir(col_pins[i], GPIO_OUT);
        gpio_put(col_pins[i], 0); 
    }
}

void init_state() {
    gpio_init(MODE_PIN);
    gpio_set_dir(MODE_PIN, GPIO_IN);
    gpio_pull_up(MODE_PIN);

    state = (gpio_get(MODE_PIN) == 0) ? CALC : NUMPAD;
}

void send_report() {
    if (tud_hid_ready()) {
        memcpy(keyboard_report.keycode, keys_pressed, 6);
        tud_hid_keyboard_report(0, keyboard_report.modifier, keyboard_report.keycode);
    }
}

void key_update(uint8_t keycode) {
    for (uint8_t i = 0; i < 6; i++) {
        if (keys_pressed == 0) {
            keys_pressed[i] = keycode;
        }
    }
}

void clear_keys() {
    for (uint8_t i = 0; i < 6; i++) {
        keys_pressed[i] = 0;
    }
}

void scan_matrix() {
    // set all columns to high in order to start scan and isolate key
    for (uint8_t i = 0; i < COLS; i++) {
        gpio_init(col_pins[i]);
        gpio_set_dir(col_pins[i], GPIO_OUT);
        gpio_put(col_pins[i], 1); 
    }

    clear_keys();
    for (uint8_t col = 0; col < COLS; col++) {
        gpio_put(col_pins[col], 0);
        for (uint8_t row = 0; row < ROWS; row++) {
            if (gpio_get(row_pins[row]) == 0) {
                key_update(keymap[row][col]);
            }
        }
        gpio_put(col_pins[col], 1);
    }

    send_report();  // send report will all concurrent keys being pressed
}

int main() {
    stdio_init_all();
    tusb_init();

    init_matrix();
    init_state();
    
    if (I2C_TOGGLE) {
        i2c_init(I2C_PORT, 400*1000);
        gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
        gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
        gpio_pull_up(I2C_SDA);
        gpio_pull_up(I2C_SCL);
    }


    while (true) {
        tud_task();
        if (key_pressed) {
            scan_matrix();
            sleep_ms(DEBOUNCE); // debounce delay
            key_pressed = false;
        }
        sleep_ms(1);

        /* key_update(NUM);
        send_report(); */
    }
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
}