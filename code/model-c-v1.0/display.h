#ifndef DISPLAY_H
#define DISPLAY_H

#include "pico/stdlib.h"

void ht16k33_init();
void ht16k33_clear();
void ht16k33_write_digit_16bit(uint8_t digit, uint16_t segments);
void display_start();
void display_number_all(uint8_t num);
void test_display_1();
void display_abstraction(uint8_t position, uint8_t number);
void display_numpad();
void display_number(int32_t num);
void test_display_2();
void display_error();
uint8_t count_digits(int32_t num);
uint8_t check_negative(int32_t num);
void display_float(float num);
uint8_t count_digits_float(float num);

#endif // DISPLAY_H