#include "calculator.h"
#include "keycodes.h"
#include "display.h"

#include <stdio.h>
#include "pico/stdlib.h"

#define NUM_DIGITS 7

#define COMP_A 0
#define COMP_B 2
#define COMP_C 1
#define RESULT 3
#define ERROR  4

#define PLUS_OP   0
#define MINUS_OP  1
#define MULT_OP   2
#define DIV_OP    3

uint8_t operator = PLUS_OP;
int32_t operandA = 0;
int32_t operandB = 0;
float result     = 0.0;

bool error_flag = false;

uint8_t operator_length = 0;
uint8_t operandA_length = 0;
uint8_t operandB_length = 0;
uint8_t result_length = 0;

int32_t result_int = 0;

uint8_t modifying = COMP_A;

// prototypes
float calculate();
void clear_operation_component(uint8_t component);
void clear_operation();
void change_sign(uint8_t component);
void update_operator(uint8_t new_op);
void append_component(uint8_t digit, uint8_t component);
void pop_component(uint8_t component);
uint8_t count_digits(int32_t num);
void use_key(uint8_t* keys);
char parse_keycode(uint8_t keycode);

// clear specific field of operation
void clear_operation_component(uint8_t component) { 
   switch (component) {
      case COMP_A:
         operandA = 0;
         break;

      case COMP_B:
         operandB = 0;
         break;

      case COMP_C:
         operator = PLUS_OP;
         break;

      case RESULT:
         result = 0;
         break;
   }
}

// clear all fields of operation
void clear_operation() {
   operandA = 0;
   operandB = 0;
   operator = PLUS_OP;
   result = 0;
   error_flag = false;
   modifying = COMP_A;
}

// change sign of operand to facillitate using negative numbers
void change_sign(uint8_t component) {
   if (component == COMP_A) {
      operandA = -operandA;
   }
   else if (component == COMP_B) {
      operandB = -operandB;
      printf("changing sign\n");
   }
}

// append character to operation component field
void append_component(uint8_t digit, uint8_t component) {
   switch (component) {
      case COMP_A:
         operandA = (operandA * 10) + digit;
         break;

      case COMP_B:
         operandB = (operandB * 10) + digit;
         break;
   }
}

// function for changing operator
void update_operator(uint8_t new_op) {
   if (new_op == '+') {
      operator = PLUS_OP;
   }
   else if (new_op == '-') {
      operator = MINUS_OP;
   }
   else if (new_op == '*') {
      operator = MULT_OP;
   }
   else if (new_op == '/') {
      operator = DIV_OP;
   }
}

// remove character from operation component field
void pop_component(uint8_t component) {
   switch (component) {
      case COMP_A:
         operandA = operandA / 10;
         break;

      case COMP_B:
         operandB = operandB / 10;
         break;
   }
}

// count number of digits in a int
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

// primary handler for key logic
void use_key(uint8_t* keys) {
   char key_char = parse_keycode(keys[0]);
   printf("char %c", key_char);
   printf("\n");
   printf("mod %d", modifying);
   printf("\n");
   printf("operandA %d", operandA);
   printf("\n");
   printf("operandB %d", operandB);
   printf("\n");
   printf("operator %d", operator);
   printf("\n");
   printf("result %f", result);
   printf("\n");
   if (key_char == 'B') {
      if (modifying == COMP_A) {
         pop_component(COMP_A);
      }
      else if (modifying == COMP_B) {
         if (operandB == 0) {
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
         change_sign(COMP_A);
      }
      else if (modifying == COMP_B) {
         change_sign(COMP_B);
      }
   }

   else if (key_char == 'C') {
      clear_operation();
   }

   else if (key_char == 'E') {
      result = calculate();
      modifying = RESULT;  // this is the only time this state should be used
   }

   else if (key_char == '+' || key_char == '-' || key_char == '*' || key_char == '/') {
      // pressing a operator when on operandA screen, but no characters there
      if (modifying == COMP_A) {
         if (operandA == 0) {
            append_component(0, COMP_A);
         }
         // pressing operator while on operandA screen
         update_operator(key_char);
         modifying = COMP_C;
      }
      else if (modifying == COMP_B) {
         // pressing a operator when on operatorB scren (technically impossible)
         if (operandA == 0) {   // replace operator if nothing typed in operandB yet
            update_operator(key_char);
         }

         // pressing a operator when on operandB
         else {
            operandA = calculate();
            update_operator(key_char);
            clear_operation_component(COMP_B);
            modifying = COMP_C;
         }
      }

      // when you press a operator on the results screen
      else if (modifying == RESULT) {
         operandA = result;
         update_operator(key_char);
         modifying = COMP_C;
      }
   }

   // number
   else {
      uint8_t length = 0;
      if (modifying == COMP_A) {
         length = count_digits(operandA);
         if (length < 7) {
            append_component(atoi(&key_char), COMP_A);
         }
         // any characters past limit will bounce and the next character must be a operator
      }
      else if (modifying == COMP_B) {
         length = count_digits(operandB);
         if (length < 7) {
            append_component(atoi(&key_char), COMP_B);
         }
      }
      else if (modifying == COMP_C) {
         modifying = COMP_B;
         length = count_digits(operandB);
         if (length < 7) {
            append_component(atoi(&key_char), COMP_B);
         }
      }
   }

   // display processing
   if (modifying == COMP_A) {
      display_number(operandA);
   }
   else if (modifying == COMP_B) {
      display_number(operandB);
   }
   else if (modifying == RESULT) {
      display_float(result);
   }
}

// calculate result using operation components specified
float calculate() {
   float operandA_working = (float)operandA;
   float operandB_working = (float)operandB;

   printf("calculating \n");
   printf("%f", operandA_working);
   printf("\n");
   printf("%f", operandB_working);
   printf("\n");
   printf("%f", operandA_working + operandB_working);
   printf("\n");

   if (operator == PLUS_OP) {
      return operandA_working + operandB_working;
   }
   else if (operator == MINUS_OP) {
      return operandA_working - operandB_working;
   }
   else if (operator == MULT_OP) {
      return operandA_working * operandB_working;
   }
   else if (operator == DIV_OP) {
      if (operandB == 0) {
         error_flag = true;
         modifying = ERROR;
         return 0.0;
      }
      else {
         return operandA_working / operandB_working;
      }
   }
   return 0.0;
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
         return 'E';

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


