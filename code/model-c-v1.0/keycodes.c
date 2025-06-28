#include "keycodes.h"

#include "pico/stdlib.h"

const uint8_t keymap[5][4] = {
   {NUM,   DIV,   MULT,  BACKSPACE},
   {SEVEN, EIGHT, NINE,  MINUS},
   {FOUR,  FIVE,  SIX,   PLUS},
   {ONE,   TWO,   THREE, ENTER},
   {ZERO,  DOT,   SIGN,  NONE}
};