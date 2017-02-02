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

/* button pins */
static const uint8_t SWITCH_MODE  = 0b00000100;
static const uint8_t CHANGE_DIGIT = 0b00001000;
static const uint8_t INCR_DIGIT   = 0b00010000;

/* display timer variables */
volatile uint8_t current_digit  = 0;          // Currently edited digit
volatile uint8_t digits[4]      = {0,0,0,0};
volatile uint16_t timer_counter = 0;         // Cyclic counter, one cycle per second.

/* timer mode variables */
volatile uint8_t     timer_mode    = 1;   // in setting mode you can edit digits and counting stops.
static const uint8_t SETTING_MODE  = 1;
static const uint8_t TIMER_MODE  = 0;

/* clock timer variables */
volatile uint16_t    clock_counter   = 0;         // Cyclic counter, one cycle per second.
static const uint8_t CYCLES_PER_TICK = 252;
static const uint8_t TICKS_PER_SECOND = 31;       //31 * 252 * 1024 ~ 8M

void static setup_clock_t1(int cps) {

    TCCR1B |= 1<<CS11 | 1<<CS10;   //Divide by 64 the main clock
    OCR1A   = (F_CPU/64)/cps;      //8MHz/64 = 125000Hz
    TCCR1B |= 1<<WGM12;            //Put Timer/Counter1 in CTC timer_mode
    TIMSK1 |= 1<<OCIE1A;           //enable timer compare interrupt

}

void static setup_clock_t2(int cps) {

    TCCR2B |= 1<<CS22 | 1<<CS21 | 1<<CS20;   //Divide by 1024 the main clock
    OCR2A   = (F_CPU/1024)/cps;              //8MHz/64 = 125000Hz
    TCCR2B |= 1<<WGM22;            //Put Timer/Counter1 in CTC timer_mode
    TIMSK2 |= 1<<OCIE2A;           //enable timer compare interrupt

}

void static setup_input() {
    DDRD   &= ~0b00011100;
    PORTD  |=  0b00011100;
    /* enable all 3 inputs triggers */
    PCICR  |= (1 << PCIE2); 
    PCMSK2 |= (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20); 
}

int main(void)
{

    cli();                      //Disable global interrupts
    setup_clock_t1(FPS);        //
    setup_clock_t2(CYCLES_PER_TICK);         //31 * 252 * 1024 ~ 8M    
    setup_display(0);           //Setup display output pins and reset decade counter. 0 = initial displayed value.
    set_custom_mask(0b11111111);//Show all digits, even leading zeros.
    enable_custom_mask();
    setup_input();              //Enable input pins for buttons
    sei();                      //Enable global interrupts

    while(1) {};
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
}

ISR(TIMER1_COMPA_vect)   //Interrupt Service Routine for timer
{
    increment_display();
    timer_counter = (timer_counter + 1) % FPS;

    if (timer_mode == SETTING_MODE) {
        //blink currently edited digit
        if ((timer_counter*2)/FPS == 0)
            set_custom_mask(~(0b00000001 << current_digit));
        else
            set_custom_mask(0b11111111);

    }
}

ISR(TIMER2_COMPA_vect)   //Interrupt Service Routine for timer
{

    clock_counter = (clock_counter + 1) % TICKS_PER_SECOND;
    if ((timer_mode == TIMER_MODE) && (clock_counter == 0)) {
        decrement();
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
    
    if (input_diff & SWITCH_MODE) {   /* PD4 - change mode*/
        timer_mode = 1-timer_mode;
        if (timer_mode == SETTING_MODE) {
            enable_custom_mask();
            current_digit = 0;
        }
        else {
            disable_custom_mask();
            timer_counter = 1;
            clock_counter = 1;
        }
    }
    
    if ((timer_mode == SETTING_MODE) && (input_diff & CHANGE_DIGIT)) { /* PD3 - change currently edited digit */
        current_digit = (current_digit + 1) % 4;
    }

    if ((timer_mode == SETTING_MODE) && (input_diff & INCR_DIGIT)) { /* PD2 - increment digit */
        digits[current_digit] = (digits[current_digit] + 1) % 10;
        uint8_t copy[4] = {digits[0], digits[1], digits[2], digits[3]};
        set_digits(copy);
    }

}
