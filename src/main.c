#ifndef F_CPU
#define F_CPU 8000000UL                          // set the CPU clock
#endif
#define FPS 400

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h> 
#include "display.h"

uint8_t last_input = 0b11111111; // Last input pin state. Reverse logic, 1 = off! 

volatile uint8_t current_digit  = 0;          // Currently edited digit
volatile uint8_t digits[4]      = {0,0,0,0};
volatile uint16_t timer_counter = 0;         // Cyclic counter, one cycle per second.
volatile uint8_t setting_mode   = 0b11111111; // in setting mode you can edit digits and counting stops.

void static setup_clock(int cps) {

    TCCR1B |= 1<<CS11 | 1<<CS10;   //Divide by 64 the main clock
    OCR1A = (8000000/64)/cps;          //8MHz/64 = 125000Hz
    TCCR1B |= 1<<WGM12;            //Put Timer/Counter1 in CTC setting_mode
    TIMSK1 |= 1<<OCIE1A;           //enable timer compare interrupt

}

void static setup_input() {
    DDRD   &= 0b11100011;
    PORTD  |= 0b00011100;
    /* enable all 3 inputs triggers */
    PCICR  |= (1 << PCIE2); 
    PCMSK2 |= (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20); 
}

int main(void)
{

    cli();                      //Disable global interrupts
    setup_clock(FPS);    
    setup_display(0);           //Setup display output pins and reset decade counter. 0 = initial displayed value.
    set_custom_mask(0b11111111);//Show all digits, even leading zeros.
    enable_custom_mask();
    setup_input();              //Enable input pins for buttons
    sei();                      //Enable global interrupts

    while(1) {};
}

ISR(TIMER1_COMPA_vect)   //Interrupt Service Routine for timer
{
    increment_display();
    timer_counter = (timer_counter + 1) % FPS;

    if (setting_mode) {
        //blink currently edited digit
        if ((timer_counter*2)/FPS == 0)
            set_custom_mask(~(0b00000001 << current_digit));
        else
            set_custom_mask(0b11111111);

    } else if (timer_counter == 0) {

        int digit = 0;
        while (digit < 4 && digits[digit] == 0)
            digit++;
        if (digit == 4)
            return;
        digits[digit--]--;
        while(digit>=0)
            digits[digit--] = 9;

        uint8_t copy[4] = {digits[0], digits[1], digits[2], digits[3]};
        set_digits(copy);
    }
}

ISR(PCINT2_vect)
{
    /* reverse logic! */
    uint8_t input_now = PIND;
    uint8_t input_diff = ~input_now & last_input;
    last_input = input_now;
    
    if (input_diff & 0b00010000) {   /* PD4 - change mode*/
        setting_mode = ~setting_mode;
        if (setting_mode)
            enable_custom_mask();
        else {
            disable_custom_mask();
            timer_counter = 1;
            current_digit = 0;
        }
    }
    
    if (setting_mode && (input_diff & 0b00001000)) { /* PD3 - change currently edited digit */
        current_digit = (current_digit + 1) % 4;
    }

    if (setting_mode && (input_diff & 0b00000100)) { /* PD2 - increment digit */
        digits[current_digit] = (digits[current_digit] + 1) % 10;
        uint8_t copy[4] = {digits[0], digits[1], digits[2], digits[3]};
        set_digits(copy);
    }

}
