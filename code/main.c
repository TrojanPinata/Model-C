#include <avr/io.h>
#include <LedControl.h>

#define COL_PORT PORTD
#define COL_PIN PIND
#define COL_DDR DDRD

#define ROW_PORT PORTA
#define ROW_PIN PINA
#define ROW_DDR DDRA

#define MAX7219_PORT PORTB
#define MAX7219_DDR DDRB

#define MAX7219_SS 4 
#define MAX7219_MISO 6 
#define MAX7219_MOSI 5 
#define MAX7219_SCK 7 

// define ports
DDRA = 0x1F;
DDRB = 0x00;
DDRC = 0x00;
DDRD = 0x0F;

// define pins
int col0 = PIND0;
int col1 = PIND1;
int col2 = PIND2;
int col3 = PIND3;
int row0 = PINA0;
int row1 = PINA1;
int row2 = PINA2;
int row3 = PINA3;
int row4 = PINA4;

// define variables
double input1 = 0;
double input2 = 0;
int operator = 0;
int dotstate = 0;
double result = 0;
int decDivider = 1;

// prototypes
void display(double num);
int checkKeys();
double calc();
int isOperator();
void updateOperator();
void clear();
int stage0();
int stage1();
int stage2();
int stageSelect(int stage);
void initMCU();
void initSSG();

// main
int main() {
   int stage = 0;
   while (1) {
      stage = stageSelect(stage);
   }
}

void initMCU() {
   COL_DDR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
   ROW_DDR &= ~((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4)); // Use pull-up resistors

   // Set up SPI for communication with MAX7219
   MAX7219_DDR |= (1 << MAX7219_SS) | (1 << MAX7219_MOSI) | (1 << MAX7219_SCK);
   SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

void initSSG() {
   MAX7219_PORT |= (1 << MAX7219_SS);
}

int stageSelect(int stage) {
   if (stage == 0) {
      stage = stage0();
   }
   if (stage == 1) {
      stage = stage1();
   }
   if (stage == 2) {
      stage = stage2();
   }
   return stage;
}

int stage0() {
   int key = checkKeys();
   if (dotstate == 0 && key != -1) {   // default
      input1 = (input1 * 10) + key;
      display(input1);
   }
   else if (dotstate == 1 && key != -1) { // decimal place
      decDivider = decDivider / 10;
      input1 = input1 + (key / decDivider);
   }
   if (isOperator()) {  // operator has been selected, move to second input
      updateOperator();
      dotstate = 0;
      return 1;
   }
   if (col3 && row4) {  // enter key pressed
      operator = 0;
      dotstate = 0;
      return 2;
   }
   if (col0 && row0) {  // clear
      clear();
      return 1;
   }
   return 0
}

int stage1() {
   int key = checkKeys();
   if (dotstate == 0 && key != -1) {         // default
      input2 = (input2 * 10) + key;
      display(input2);
   }
   else if (dotstate == 1 && key != -1) {    // decimal place
      decDivider = decDivider / 10;
      input2 = input2 + (key / decDivider);
   }
   if (isOperator()) {  // do operation on current input and do another one with a new number
      input1 = calc();
      updateOperator();
      dotstate = 0;
      input2 = 0;
   }
   if (col3 && row4) {  // enter - move to stage 2
      operator = 0;
      dotstate = 0;
      return 2;
   }
   if (col0 && row0) {  // clear 
      clear();
      return 0;
   }
   return 1;
}

int stage2() {
   input1 = calc();
   if (isOperator) { // do operation on result
      updateOperator();
      return 1;
   }
   if (col3 && row4) {  // repeat operation
      input1 = calc();
   }
   int key = checkKeys();
   if (key != -1) {    // new key is pressed - delete previous result and reset
      input1 = key;
      input2 = 0;
      operator = 0;
      return 0;
   }
   dotstate = 0;
   return 2;
}

// display number
void display(double num) {
   // contact MAX7219 via SPI
}

// check if key is pressed
int checkKeys() {
   if (col0 && row3) {
      return 1;
   }
   if (col1 && row3) {
      return 2;
   }
   if (col2 && row3) {
      return 3;
   }
   if (col0 && row2) {
      return 4;
   }  
   if (col1 && row2) {
      return 5;
   }
   if (col2 && row2) {
      return 6;
   }
   if (col0 && row1) {
      return 7;
   }
   if (col1 && row1) {
      return 8;
   }
   if (col2 && row1) {
      return 9;
   }
   if (col0 && row4) {
      return 0;
   }
   if (col3 && row4) {
      dotstate = 1;
   }
   return -1;
}

// do calculation on inputs and display
double calc() {
   if (operator == 0) {
      result = input1 + input2;
   }
   if (operator == 1) {
      result = input1 - input2;
   }
   if (operator == 2) {
      result = input1 * input2;
   }
   if (operator == 3) {
      result = input1 / input2;
   }
   input1 = result;
   display(result);
   return result;
}

// check if operator key is pressed
int isOperator() {
   if ((col3 && row1) || (col3 && row0) || (col2 && row0) || (col1 && row0)) {
      return 1;
   }
   return 0;
}

// change operator state
void updateOperator() {
   if (col3 && row1) {
      operator = 0;
   }
   if (col3 && row0) {
      operator = 1;
   }
   if (col2 && row0) {
      operator = 2;
   }
   if (col1 && row0) {
      operator = 3;
   }
}

// clear all registers
void clear() {
   input1 = 0;
   operator = 0;
   result = 0;
   input2 = 0;
   display(0);
}
