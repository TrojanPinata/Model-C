#include "display.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define SDA_PIN 0
#define SCL_PIN 1
#define HT16K33_ADDR 0x70
#define HT16K33_ADDR_ALT 0x71

#define DASH 10

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

// 00000pgfedcba000
const uint16_t digits[11] = {
   0b0000000111111000, // 0
   0b0000000000110000, // 1
   0b0000001011011000, // 2
   0b0000001001111000, // 3
   0b0000001100110000, // 4
   0b0000001101101000, // 5
   0b0000001111101000, // 6
   0b0000000000111000, // 7
   0b0000001111111000, // 8
   0b0000001101111000, // 9
   0b0000001000000000  // -
};

const uint16_t dec_mask = 0b000001000000000;

const uint16_t error_letters[5] = {
   0b0000001111001000, // E
   0b0000001010000000, // r
   0b0000001010000000, // r
   0b0000001011100000, // o
   0b0000001010000000  // r
};

const uint8_t init[] = {
   0x21,       
   0x81,       
   0xE0 | 0x0F 
};

void ht16k33_init() {
   for (int i = 0; i < 3; i++) {
      i2c_write_blocking(I2C_PORT, HT16K33_ADDR, &init[i], 1, false);
      sleep_ms(10);
   }
   for (int i = 0; i < 3; i++) {
      i2c_write_blocking(I2C_PORT, HT16K33_ADDR_ALT, &init[i], 1, false);
      sleep_ms(10);
   }
}

void ht16k33_clear() {
   uint8_t buffer[17] = {0};
   i2c_write_blocking(I2C_PORT, HT16K33_ADDR, buffer, 17, false);
   i2c_write_blocking(I2C_PORT, HT16K33_ADDR_ALT, buffer, 17, false);
}

void ht16k33_write_digit_16bit(uint8_t digit, uint16_t segments) {
   uint8_t pos = digit;
   if (digit >= 4) {
      pos = digit - 4;
   }

   uint8_t buffer[3] = {
      (uint8_t)(pos * 2), 
      (uint8_t)(segments & 0xFF), 
      (uint8_t)((segments >> 8) & 0xFF) 
   };
   
   if (digit >= 4) {
      i2c_write_blocking(I2C_PORT, HT16K33_ADDR_ALT, buffer, 3, false);
   }
   else {
      i2c_write_blocking(I2C_PORT, HT16K33_ADDR, buffer, 3, false);
   }
}

void display_start() {
   i2c_init(I2C_PORT, 100 * 1000); // 100kHz
   gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
   gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
   gpio_pull_up(SDA_PIN);
   gpio_pull_up(SCL_PIN);

   ht16k33_init();
   ht16k33_clear();
}

void display_number_all(uint8_t num) {
   for (uint8_t i = 0; i < 8; i++) {
      ht16k33_write_digit_16bit(i, digits[num]);
   }
}

void test_display_1() {
   while (1) {
      for (uint8_t j = 0; j < 10; j++) {
         display_number_all(j);
         sleep_ms(250);
      }
      sleep_ms(1000);
   }
}

void test_display_2() {
   int32_t test_nums[10] = {19, 100, 88888888, 987654321, -7654321, 87654321, 0, 4, -1};
   while (1) {
      for (uint8_t j = 0; j < 9; j++) {
         display_number(test_nums[j]);
         sleep_ms(2000);
      }
   }
}

// can abstract the display so 0 is the rightmost position and number does not need to be input to digits array
void display_abstraction(uint8_t position, uint8_t number) {
   if (number < 11 && position < 8 && position >= 0 && number >= 0) {
      uint8_t pos = 7 - position;
      uint16_t code = digits[number];
      ht16k33_write_digit_16bit(pos, code);
   }
}

void display_numpad() {
   uint8_t letters[5] = {5, 8, 0, 0, 8};
   for (uint8_t i = 0; i < 5; i++) {
      display_abstraction(i, letters[i]);
   }
}

uint8_t count_digits(int32_t num) {
   if (num < 0) {
      num = -num;
   }

   if (num == 0) {
      return 1;
   }

   uint8_t digits = 0;
   while (num > 0) {
      num /= 10;
      digits++;
   }
   return digits;
}

uint8_t check_negative(int32_t num) {
   if (num < 0) {
      return 0;
   }
   return 1;
}

void display_error() {
   for (uint8_t i = 0; i < 5; i++) {
      ht16k33_write_digit_16bit(7 - i, error_letters[4 - i]);
   }
}

void display_number(int32_t num) {
   uint8_t sign = check_negative(num);
   uint8_t length = 0;

   if (sign == 1) {
      length = count_digits(num);
   }
   else {
      length = count_digits(num);
      num = num * -1;
   }

   ht16k33_clear();
   if ((sign == 1 && length > 8) || (sign == 0 && length > 7)) {
      display_error();
   }
   else {
      for (uint8_t i = 0; i < length; i++) {
         display_abstraction(i, num % 10);
         num = num / 10;
      }
      if (sign == 0) {
         display_abstraction(length, DASH);
      }
   }
}