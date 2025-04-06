#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"

#include "keycodes.h"
#include "calculator.h"


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
#define DEBOUNCE 20000  // 50 ms delay
#define CONFIRM 3   // number of consecutive reads the key was confirmed for
#define HID_DELAY 5 // ms delay between checking/sending next keypress

const uint8_t row_pins[ROWS] = {3, 4, 5, 6, 7};
const uint8_t col_pins[COLS] = {8, 9, 10, 11};

uint64_t last_keypress_time[ROWS] = {0};

volatile bool key_pressed = false;
uint8_t keys_pressed[6] = {0};
uint8_t prev_state[6] = {0}; // state array for switches 
// Oh? You wanted NKRO? Tell me how you are planning to fit 6 of your greasy fingers on a numpad thats smaller than your hand.
// This isn't a gaming keyboard or something where theres any benefit at all. You must have thought I would take the time to
// make a custom descriptor before. Suprise, I have - and this is not the project for that.

void scan_matrix_irq(int dir);
void scan_matrix(void);
void init_matrix(void);

void keypress_callback(uint8_t gpio, uint32_t events) {
    uint64_t now = time_us_64();
    int index = 0;

    if (gpio == MODE_PIN) {
        irq_set_enabled(IO_IRQ_BANK0, false);
        state = (gpio_get(MODE_PIN) == 0) ? CALC : NUMPAD;
        init_matrix();
        irq_set_enabled(IO_IRQ_BANK0, true);
    }

    for (uint8_t i = 0; i < ROWS; i++) {
        if (row_pins[i] == gpio) {
            index = i;
        }
    }

    if (now - last_keypress_time[index] < DEBOUNCE) { return; }
    last_keypress_time[index] = now;

    if (events & GPIO_IRQ_EDGE_FALL) {
        scan_matrix_irq(GPIO_IRQ_EDGE_FALL);
    }
    if (events & GPIO_IRQ_EDGE_RISE) {
        scan_matrix_irq(GPIO_IRQ_EDGE_RISE);
    }
}

void keypress_handler(void) {
    printf("keypress handler\n");

    //irq_set_enabled(IO_IRQ_BANK0, false); // turn off interrputs

    for (uint8_t i = 0; i < ROWS; i++) {
        uint gpio = row_pins[i];
        uint32_t events = gpio_get_irq_event_mask(gpio);

        if (gpio_get_irq_event_mask(MODE_PIN)) {
            keypress_callback(MODE_PIN, gpio_get_irq_event_mask(MODE_PIN));
            break;
        }
        else if (events) {
            keypress_callback(gpio, events);
            //gpio_acknowledge_irq(row_pins[i], events);
        }
    }
}

void clear_IRQ_flags() {
    gpio_acknowledge_irq(MODE_PIN, gpio_get_irq_event_mask(MODE_PIN));
    for (uint8_t i = 0; i < ROWS; i++) {
        uint32_t events = gpio_get_irq_event_mask(row_pins[i]);
        if (events) {
            gpio_acknowledge_irq(row_pins[i], events);
        }
    }
}

void init_matrix() {
    // initialize rows as inputs pulled up
    for (uint8_t i = 0; i < ROWS; i++) {
        gpio_init(row_pins[i]);
        gpio_set_dir(row_pins[i], GPIO_IN);
        gpio_pull_up(row_pins[i]);  // make rows inputs being pulled up - when column set to low

        // setup callbacks for interrupt driven key handling
        if (state == CALC) {
            gpio_set_irq_enabled(row_pins[i], GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
        }
        else {
            gpio_set_irq_enabled(row_pins[i], GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);
        }
    }

    irq_set_exclusive_handler(IO_IRQ_BANK0, keypress_handler);

    if (state == CALC) {
        // initialize columns as outputs which turn off when scanning each column - pulls row down to indicate hit
        // start all columns off in order to allow for interrupt when key hit - not polling here
        for (uint8_t i = 0; i < COLS; i++) {
            gpio_init(col_pins[i]);
            gpio_set_dir(col_pins[i], GPIO_OUT);
            gpio_put(col_pins[i], 0); 
        }
    }
    else {
        for (uint8_t i = 0; i < COLS; i++) {
            gpio_init(col_pins[i]);
            gpio_set_dir(col_pins[i], GPIO_OUT);
            gpio_put(col_pins[i], 1); 
        }
    }
    
    printf("matrix initialized\n");
    clear_IRQ_flags();
}

void init_state() {
    gpio_init(MODE_PIN);
    gpio_set_dir(MODE_PIN, GPIO_IN);
    gpio_pull_up(MODE_PIN);

    state = (gpio_get(MODE_PIN) == 0) ? CALC : NUMPAD;

    init_matrix();
    gpio_set_irq_enabled(MODE_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    printf("mode switch initialized\n");
}

void save_state() {
    for (uint8_t i = 0; i < 6; i++) {
        prev_state[i] = keys_pressed[i];
    }
}

void remove_duplicates(uint8_t keycode) {
    // check for duplicate additions
    bool check = false;
    for (uint8_t i = 0; i < 6; i++) {
        if (keys_pressed[i] == keycode && !check) {
            check = true;
        }
        else if (keys_pressed[i] == keycode && check) {
            keys_pressed[i] = 0;
        }
    }
}

// add key to array
void add_key(uint8_t keycode) {
    for (uint8_t i = 0; i < 6; i++) {
        if (keys_pressed[i] == keycode) {
            return;
        }
    }
    for (uint8_t i = 0; i < 6; i++) {
        if (keys_pressed[i] == 0) {
            keys_pressed[i] = keycode;
            remove_duplicates(keycode);
            return;
        }
    }
}

// remove key from array
void remove_key(uint8_t keycode) {
    for (uint8_t i = 0; i < 6; i++) {
        if (keys_pressed[i] == keycode) {
            keys_pressed[i] = 0;
        }
    }

    // doing it again for insurance
    for (uint8_t i = 0; i < 6; i++) {
        if (keys_pressed[i] == keycode) {
            keys_pressed[i] = 0;
        }
    }
}

void scan_matrix_irq(int dir) {
    save_state();
    
    // set all columns to high in order to start scan and isolate key
    for (uint8_t i = 0; i < COLS; i++) {
        gpio_put(col_pins[i], 1); 
    }

    for (uint8_t col = 0; col < COLS; col++) {
        gpio_put(col_pins[col], 0);

        for (uint8_t row = 0; row < ROWS; row++) {
            if (gpio_get(row_pins[row]) == 0) {
                if (dir == GPIO_IRQ_EDGE_FALL) {
                    add_key(keymap[row][col]);
                }
            }
            else {
                if (dir == GPIO_IRQ_EDGE_RISE) {
                    remove_key(keymap[row][col]);
                }
            }
        }
        gpio_put(col_pins[col], 1);
    }

    // re-enable columns so interrupt works again
    for (uint8_t i = 0; i < COLS; i++) {
        gpio_put(col_pins[i], 0); 
    }

    clear_IRQ_flags();

    printf("matrix scanned (irq)\n");
}

void scan_matrix() {
    save_state();

    for (uint8_t col = 0; col < COLS; col++) {
        gpio_put(col_pins[col], 0);

        for (uint8_t row = 0; row < ROWS; row++) {
            if (gpio_get(row_pins[row]) == 0) {
                add_key(keymap[row][col]);
            }
            else {
                remove_key(keymap[row][col]);
            }
        }
        gpio_put(col_pins[col], 1);
    }

    printf("matrix scanned\n");
}

int main() {
    stdio_init_all();

    sleep_ms(5000);

    init_state();
    
    if (I2C_TOGGLE) {
        i2c_init(I2C_PORT, 400*1000);
        gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
        gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
        gpio_pull_up(I2C_SDA);
        gpio_pull_up(I2C_SCL);
    }

    irq_set_enabled(IO_IRQ_BANK0, true);

    printf("scanning\n");
    while (true) {
        if (state == CALC) {
            sleep_ms(50);
        }
        else if (state == NUMPAD) {
            scan_matrix();
            sleep_ms(50);
        }

        for (int i = 0; i < 6; i++) {
          printf("%d ", keys_pressed[i]);
        }
        printf("\n");
    }
}
