#include <stdio.h>
#include "display.h"
#include "clocks.h"

volatile uint8_t  clock            = 0;
volatile uint8_t  zeros[CLKNUM]    = {0};  // Cached logic values
volatile uint8_t  digits[4*CLKNUM] = {0};  // Displayed digits

void static inline set_display_digits() {
    uint8_t offset = 4*clock;
    uint8_t copy[4] = {digits[offset+0], digits[offset+1], digits[offset+2], digits[offset+3]};
    set_digits(copy);
}

void static inline update(uint8_t num) {
    uint8_t offset = 4*num;
    zeros[num] = !(digits[offset+0] | digits[offset+1] | digits[offset+2] | digits[offset+3]);
    
    if (num == clock) 
        set_display_digits();
}

void decrement(uint8_t num) {
    uint8_t offset = 4*num;
    int digit = 0;
    while (digit < 4 && digits[offset+digit] == 0)
        digit++;
    if (digit == 4)
        return;
    digits[offset+digit--]--;
    while(digit>=0)
        digits[offset+digit--] = 9;
    update(num);
}

void rot_digit(uint8_t num, uint8_t position) {
    uint8_t offset = 4*num;
    digits[offset+position] = (digits[offset+position] + 1) % 10;
    update(num);
}


uint8_t is_zero(uint8_t num) {
    return zeros[num];
}

void set_clock(uint8_t num) {
    clock = num;
    set_display_digits();
}