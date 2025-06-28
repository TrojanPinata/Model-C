#ifndef DISPLAY_H
#define DISPLAY_H

#include "pico/stdlib.h"

void ht16k33_init();
void ht16k33_clear();
void ht16k33_write_digit_raw(uint8_t digit_pos, uint8_t segments);
void display_start();
void test_display_1();
void test_display_2();
void display_number_all(uint8_t num);
void display_numpad();
void display_error();
void display_number(int32_t num);

#endif // DISPLAY_H