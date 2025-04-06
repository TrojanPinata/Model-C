#include "calculator.h"
#include "keycodes.h"
#include "display.h"

#include <stdio.h>
#include "pico/stdlib.h"

#define NUM_DIGITS 8

#define COMP_A 0
#define COMP_B 2
#define COMP_C 1
#define RESULT 3
#define ERROR  4

char operator[1] = " ";    // COMP_C
char operandA[8] = {" "};  // COMP_A
char operandB[8] = {" "};  // COMP_B
char result[8] = {" "};  // RESULT

#define POSITIVE true
#define NEGATIVE false

bool operandA_sign = POSITIVE;
bool operandB_sign = POSITIVE;

bool error_flag = false;

uint8_t operator_length = 0;
uint8_t operandA_length = 0;
uint8_t operandB_length = 0;
uint8_t result_length = 0;

const uint8_t operator_max_length = 1;
const uint8_t operandA_max_length = NUM_DIGITS;
const uint8_t operandB_max_length = NUM_DIGITS;
const uint8_t result_max_length = NUM_DIGITS;

uint8_t modifying = COMP_A;

// prototypes
uint32_t calculate();
void clear_operation_component(uint8_t component);
void clear_operation();
void append_component(char key, uint8_t component);
void pop_component(uint8_t component);
void use_key(uint8_t* keys);
char* int_to_char(uint32_t val);
char parse_keycode(uint8_t keycode);
uint8_t fix_length();

// clear specific field of operation
void clear_operation_component(uint8_t component) { 
   switch (component) {
      case COMP_A:
         operandA_length = 0;
         for (uint8_t i = 0; i < operandA_max_length; i++) {
            operandA[i] = " ";
         }
         break;

      case COMP_B:
         operandB_length = 0;
         for (uint8_t i = 0; i < operandB_max_length; i++) {
            operandB[i] = " ";
         }
         break;

      case COMP_C:
         operator_length = 0;
         operator[0] = " ";
         break;

      case RESULT:
         result_length = 0;
         for (uint8_t i = 0; i < result_max_length; i++) {
            result[i] = " ";
         }
         break;
   }
}

// clear all fields of operation
void clear_operation() {
   operator_length = 0;
   operandA_length = 0;
   operandB_length = 0;

   operator[0] = " ";
   for (uint8_t i = 0; i < NUM_DIGITS; i++) {
      operandA[i] = " ";
      operandB[i] = " ";
      result[i] = " ";
   }
   error_flag = false;

   modifying = COMP_A;
}

// append character to operation component field
void append_component(char key, uint8_t component) {
   switch (component) {
      case COMP_A:
         if (operandA_length < operandA_max_length) {
            operandA[operandA_length] = key;
            operandA_length++;
         }
         break;

      case COMP_B:
         if (operandB_length < operandB_max_length) {
            operandB[operandB_length] = key;
            operandB_length++;
         }
         break;

      case COMP_C:
         if (operator_length < operator_max_length) {
            operator[operator_length] = key;
            operator_length++;
         }
         break;

      case RESULT:
         if (result_length < result_max_length) {
            result[result_length] = key;
            result_length++;
         }
         break;
   }
}

// remove character from operation component field
void pop_component(uint8_t component) {
   switch (component) {
      case COMP_A:
         if (operandA_length > 0) {
            operandA[operandA_length] = " ";
            operandA_length--;
         }
         break;

      case COMP_B:
         if (operandB_length > 0) {
            operandB[operandB_length] = " ";
            operandB_length--;
         }
         break;
   }
}

// primary handler for key logic
void use_key(uint8_t* keys) {
   char key_char = parse_keycode(keys[0]);
   if (key_char == 'B') {
      if (modifying == COMP_A) {
         pop_component(COMP_A);
      }
      else if (modifying == COMP_B) {
         if (operandB_length == 0) {
            clear_operation_component(COMP_C);
            modifying = COMP_A;
         }
         else {
            pop_component(COMP_B);
         }
      }
      else if (modifying == RESULT) {
         clear_operation();
      }
   }

   else if (key_char == 'S') {
      if (modifying == COMP_A) {
         operandA_sign = !operandA_sign;
      }
      else if (modifying == COMP_B) {
         operandB_sign = !operandB_sign;
      }
   }

   else if (key_char == 'C') {
      clear_operation();
   }

   else if (key_char == "=") {
      char* temp = int_to_char(calculate());
      for (uint8_t i = 0; i < len(temp); i++) {
         append_component(temp[i], RESULT);
      }
      fix_length();

      modifying = RESULT;  // this is the only time this state should be used
      clear_operation();
   }

   else if (key_char == '+' || key_char == '-' || key_char == '*' || key_char == '/') {
      // pressing a operator when on operandA screen, but no characters there
      if (modifying == COMP_A) {
         if (operandA_length == 0) {
            append_component('0', COMP_A);
         }
         // pressing operator while on operandA screen
         else {
            append_component(key_char, COMP_C);
            // call display here
            modifying = COMP_B;
         }
         
      }
      else if (modifying == COMP_B) {
         // pressing a operator when on operatorB scren (technically impossible)
         if (operandA_length == 0) {   // replace operator if nothing typed in operandB yet
            clear_operation_component(COMP_C);
            append_component(key_char, COMP_C);
         }

         // pressing a operator when on operandB
         else {
            char* temp = int_to_char(calculate());

            // remove previous operandA and replace with result
            clear_operation_component(COMP_A);
            for (uint8_t i = 0; i < len(temp); i++) {
               append_component(temp[i], COMP_C);
            }

            clear_operation_component(COMP_B);
            fix_length();
            append_component(key_char, COMP_C);

            modifying = COMP_B;
         }
      }

      // when you press a operator on the results screen
      else if (modifying == RESULT) {
         for (uint8_t i = 0; i < NUM_DIGITS; i++) {
            append_component(result[i], COMP_A);
         }
         fix_length();
         append_component(key_char, COMP_C);
         modifying = COMP_B;
      }
   }

   else {
      if (modifying == COMP_A) {
         if (operandA_length < operandA_max_length) {
            append_component(key_char, COMP_A);
         }
         // any characters past limit will bounce and the next character must be a operator
      }
      else if (modifying == COMP_B) {
         if (operandB_length < operandB_max_length) {
            append_component(key_char, COMP_B);
         }
      }
      else {
         // do nothing if putting a number where a operator should be
      }
   }

}

// change sign of operand to facillitate using negative numbers
void change_sign(bool sign, uint8_t component) {
   if (component == COMP_A) {
      if (sign) {
         operandA_sign = POSITIVE;
      }
      else {
         operandA_sign = NEGATIVE;
      }
   }
   else if (component == COMP_B) {
      if (sign) {
         operandB_sign = POSITIVE;
      }
      else {
         operandB_sign = NEGATIVE;
      }
   }
}

// calculate result using operation components specified
uint32_t calculate() {
   uint32_t operandA_int = atoi(operandA);
   uint32_t operandB_int = atoi(operandB);

   uint8_t operandA_int_sign = 1;
   uint8_t operandB_int_sign = 1;

   if (operandA_sign == NEGATIVE) {
      operandA_int_sign = -1;
      operandA_int = operandA_int * operandA_int_sign;
   }
   if (operandB_sign == NEGATIVE) {
      operandB_int_sign = -1;
      operandB_int = operandB_int * operandB_int_sign;
   }

   if (operator[0] == "+") {
      return operandA_int + operandB_int;
   }
   else if (operator[0] == "-") {
      return operandA_int - operandB_int;
   }
   else if (operator[0] == "*") {
      return operandA_int * operandB_int;
   }
   else if (operator[0] == "/") {
      if (operandB == 0) {
         error_flag = true;
         modifying = ERROR;
         return 0;
      }
      else {
         return operandA_int / operandB_int;
      }
   }

}

// convert integer to character array
char* int_to_char(uint32_t val) {
   uint8_t digit = 0;
   uint32_t num = val;
   while (num) {
      digit++;
      num /= 10;
   }
   char* array;
   char arr1[digit];

   int index = 0;
   while (val) {
      arr1[++index] = val % 10 + '0';
      val /= 10;
   }

   int i;
   for (i = 0; i < index; i++) {
      array[i] = arr1[index - i];
   }

   array[i] = '\0';
   return (char*)array;
}

// ensure the length of the character arrays match the number of used slots
uint8_t fix_length() {
   for (uint8_t i = 0; i < operandA_max_length; i++) {
      if (operandA[i] != " ") {
         operandA_length++;
      }
   }

   for (uint8_t i = 0; i < operandB_max_length; i++) {
      if (operandB[i] != " ") {
         operandB_length++;
      }
   }

   for (uint8_t i = 0; i < operator_max_length; i++) {
      if (operator[i] != " ") {
         operator_length++;
      }
   }

   for (uint8_t i = 0; i < result_max_length; i++) {
      if (result[i] != " ") {
         result_length++;
      }
   }
}

// parse keycodes to characters
char parse_keycode(uint8_t keycode) {
   switch (keycode) {
      // operators and modifiers
      case SIGN:
         return 'S';

      case NUM:
         return 'C';

      case DIV:
         return '/';
      
      case MULT:
         return '*';

      case MINUS:
         return '-';

      case PLUS:
         return '+';

      case DOT:
         return '.';

      case BACKSPACE:
         return 'B';

      case ENTER:
         return '=';

      // numbers
      case ONE:
         return '1';

      case TWO:
         return '2';
      
      case THREE:
         return '3';

      case FOUR:
         return '4';

      case FIVE:
         return '5';

      case SIX:
         return '6';

      case SEVEN:
         return '7';

      case EIGHT:
         return '8';
      
      case NINE:
         return '9';

      case ZERO:
         return '0';

      case NONE:
         return '_';
   }
}
