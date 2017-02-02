#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

static volatile uint16_t value_to_print = 1000;
static volatile uint16_t current_digit  = 0;
static volatile uint16_t current_value  = 0;
static volatile uint8_t  digits[4]      = {0,0,0,0};
static volatile uint8_t  zero_mask      = 0b00000000;
static volatile uint8_t  custom_mask    = 0b00000000;
static volatile uint8_t  custom_mask_on = 0b00000000;

static void reset_display() {

    current_value = 0;
    PORTC ^= 0b00100000;
    _delay_us(0.25);
    PORTC ^= 0b00100000;
    _delay_us(0.25);

}

static void set_digit(int value) {
    while (current_value != value) {
        PORTC ^= 0b00010000;
        _delay_us(0.25);
        PORTC ^= 0b00010000;
        _delay_us(0.25);
        current_value = (current_value + 1) % 10;
    }
}

static void set_mask() {
    zero_mask = 0b00000000;
    uint16_t value = value_to_print;
    if (value > 99) {
        if (value > 999)
            zero_mask = 0b00001111;
        else
            zero_mask = 0b00000111;
    } else {
        if (value > 9)
            zero_mask = 0b00000011;
        else
            zero_mask = 0b00000001;
    }
}

void set_value(uint16_t value) {

    value_to_print = value;
    int digit = 0;
    while (digit < 4) {
        digits[digit] = value % 10;
        value /= 10;
        digit++;
    }
    set_mask();
}

void set_digits(uint8_t value[4]) {

    digits[0] = value[0];
    digits[1] = value[1];
    digits[2] = value[2];
    digits[3] = value[3];
    value_to_print = (uint16_t)(digits[0]) + 
                     (uint16_t)(digits[1])*10 + 
                     (uint16_t)(digits[2])*100 + 
                     (uint16_t)(digits[3])*1000;
    set_mask();

}

void setup_display(uint8_t value) {
    DDRC  |= 0b00111111;  //Set PortC Pin0-5 as an output
    PORTC &= 0b11000000;  //Set PortC Pin0-5 low to turn on LED

    reset_display();
    set_value(value);
}

void increment_display() {

    current_digit = (current_digit + 1) % 4;
    PORTC &= 0b11110000;
    set_digit(digits[current_digit]);
    uint8_t digit_mask = (~custom_mask_on & zero_mask) | (custom_mask_on & custom_mask);
    PORTC |= (0b00000001 << current_digit) & digit_mask;

}

void enable_custom_mask() {
    custom_mask_on = 0b11111111;
}
void disable_custom_mask() {
    custom_mask_on = 0b00000000;
}
void set_custom_mask(uint8_t mask) {
    custom_mask = mask & 0b00001111;
}
