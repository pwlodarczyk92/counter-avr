#ifndef F_CPU
#error frequency is not set. (eg. -DF_CPU=1000000UL) // set the CPU clock
#endif
#define FPS 160

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h> 
#include "display.h"
#include "clocks.h"

/* button pins */
uint8_t last_input = 0b11111111;              // Last input pin state. Reverse logic, 1 = off! 
static const uint8_t SWITCH_BTN   = 1 << PD2;
static const uint8_t CHANGE_DIGIT = 1 << PD3;
static const uint8_t INCR_DIGIT   = 1 << PD4;

/* display timer variables */
volatile uint16_t timer_counter = 0;          // Cyclic counter, one cycle per second.
volatile uint8_t  current_digit = 0;          // Currently edited digit
    
/* timer mode variables */
volatile uint8_t clock_num = 0;
volatile uint8_t current_sleep_mode = SLEEP_MODE_IDLE;

void static inline setup_clock(int cps) {
    TCCR1B |= 1<<CS11 | 1<<CS10;   //Divide by 64 the main clock
    OCR1A   = (F_CPU/64)/cps;      //8MHz/64 = 125000Hz
    TCCR1B |= 1<<WGM12;            //Put Timer/Counter1 in CTC timer_mode
    TIMSK1 |= 1<<OCIE1A;           //enable timer compare interrupt
}
void static inline setup_input() {
    /* enable PD2, PD3 and PD4 inputs and it's pull-up resistors */
    DDRD   &= ~0b00011100;
    PORTD  |=  0b00011100;
    /* enable all 3 inputs triggers */
    PCICR  |= (1 << PCIE2); 
    PCMSK2 |= (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20); 
}
void static inline enable_int0() {
    EICRA  |= (1 << ISC01);         //Interrupt executed on falling edge (reverse logic!).
    EIMSK  |= (1 << INT0);
}
void static inline disable_int0() {
    EIMSK  &= ~(1 << INT0);
    EICRA  &= ~(1 << ISC01);
}

int main(void)
{

    cli();                      //Disable global interrupts
    setup_clock(FPS);

    setup_display(0);           //Setup display output pins and reset decade counter.
    set_custom_mask(0b11111111);//Show all digits, even leading zeros.
    enable_custom_mask();

    setup_input();              //Enable input pins for buttons
    sei();                      //Enable global interrupts

    while(1) {
        cli();
        set_sleep_mode(current_sleep_mode);
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();
    };
}

void static inline execute_order_66(uint8_t clk) {
    DDRD  |= 0b00100000 << clk;
    PORTD |= 0b00100000 << clk;
    _delay_ms(100);
    PORTD &= ~0b00100000 << clk;
    DDRD  &= ~0b00100000 << clk;
}

void static inline swich_power_off() {
    enable_int0();
    current_sleep_mode = SLEEP_MODE_PWR_DOWN;
}

ISR(TIMER1_COMPA_vect)   //Interrupt Service Routine for timer
{

    timer_counter = (timer_counter + 1) % FPS;
    uint8_t full_sec = timer_counter   == 0;

    if (clock_num < CLKNUM) {

        uint8_t half_sec = timer_counter*2 == FPS + (FPS%2);
        if (full_sec || half_sec) // blink!
            set_custom_mask(~(half_sec << current_digit));
        increment_display();

    } else if (!full_sec) {
        return;
    } else {      

        uint8_t any_nonzero = 0;
        uint8_t clk = 0;

        for(; clk < CLKNUM ; clk++) {

            if (is_zero(clk))
                continue;
            decrement(clk);
            if (!is_zero(clk)) {
                any_nonzero |= 0b11111111; 
                continue;
            }
            
            execute_order_66(clk); // was nonzero and became zero
            timer_counter = 1;     // execute orders one by one
            return;                // delay sleep
        }

        if (!any_nonzero)
            swich_power_off();
    }
}

ISR(INT0_vect) {
    disable_int0();
    current_sleep_mode = SLEEP_MODE_IDLE;
}

ISR(PCINT2_vect)
{
    /* reverse logic! */
    uint8_t input_now = PIND;
    uint8_t input_diff = ~input_now & last_input;
    last_input = input_now;
    
    if (input_diff & SWITCH_BTN) {   /* PD4 - switch mode*/
        clock_num = (clock_num + 1) % (CLKNUM + 1);
        if (clock_num < CLKNUM) {
            set_custom_mask(0b11111111);
            set_clock(clock_num);
            current_digit = 0;
            timer_counter = 1;
        } else { /*if (timer_mode == CLKNUM)*/
            set_custom_mask(0b00000000);
            increment_display();  //necessary, because in saving mode display is no more incremented
        }
    }
    
    if ((input_diff & CHANGE_DIGIT) && (clock_num < CLKNUM)) /* change currently edited digit */
        current_digit = (current_digit + 1) % 4;

    if ((input_diff & INCR_DIGIT) && (clock_num < CLKNUM)) /* increment digit */
        rot_digit(clock_num, current_digit);

}