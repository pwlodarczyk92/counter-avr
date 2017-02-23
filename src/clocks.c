#include <stdio.h>
#include "display.h"
volatile uint8_t  is_zero_cache    = 1;          // Cached logic value
volatile uint8_t  digits[4]        = {0};  // Displayed digits

void static inline update() {
    uint8_t copy[4] = {digits[0], digits[1], digits[2], digits[3]};
    is_zero_cache = !(copy[0] | copy[1] | copy[2] | copy[3]);
    set_digits(copy);
}

void decrement() {
    int digit = 0;
    while (digit < 4 && digits[digit] == 0)
        digit++;
    if (digit == 4)
        return;
    digits[digit--]--;
    while(digit>=0)
        digits[digit--] = 9;
    update();
}

void rot_digit(uint8_t position) {
    digits[position] = (digits[position] + 1) % 10;
    update();
}

uint8_t is_zero() {
    return is_zero_cache;
}