#ifndef KEYCODES_H
#define KEYCODES_H

#include "pico/stdlib.h"

// scancodes for each key
#define NUM 69
#define BACKSPACE 14
#define ENTER 156

#define DOT 83
#define MINUS 74
#define PLUS 78
#define MULT 55
#define DIV 53

#define ONE 79
#define TWO 80
#define THREE 81
#define FOUR 75
#define FIVE 76
#define SIX 77
#define SEVEN 71
#define EIGHT 72
#define NINE 73
#define ZERO 82

#define SIGN 1

#define NONE 0

extern const uint8_t keymap[5][4];

#endif // KEYCODES_H