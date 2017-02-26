#ifndef F_CPU
#error frequency is not set. (eg. -DF_CPU=1000000ULL) // set the CPU clock
#endif

#define PERIOD 500  //ms_period
#define FPM    120  //120//PERIOD * FPM = 60 000ms
#define DELAY  5    //10//number of periods - delay between simultaneous clocks countdowns
#define FPS    160  //display PWM frequency

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
volatile uint16_t timer_counter = 0;          // Seconds counter
volatile uint8_t  parity        = 0;          // Parity for current digit blink
volatile uint8_t  current_digit = 0;          // Currently edited digit

/* timer mode variables */
volatile uint8_t clock_num          = 0;
volatile uint8_t current_sleep_mode = SLEEP_MODE_IDLE;
volatile uint8_t orders[CLKNUM]     = {0};    // Unexecuted orders

void static inline setup_main_timer(int period_ms) {
    TCCR1B |= 1<<CS11 | 1<<CS10;              //Divide by 64 the main clock
    OCR1A   = (F_CPU*period_ms)/(64U*1000U);  //ms_period <= 4096 for freq=1MHz
    TCCR1B |= 1<<WGM12;                       //Put Timer/Counter1 in CTC timer_mode
    TIMSK1 |= 1<<OCIE1A;                      //Enable timer compare interrupt
}

void static inline setup_display_pwm_timer(int cps) {
    TCCR0B |= 1<<CS02;           //Divide by 256 the main clock
    OCR0A   = (F_CPU/256)/cps;   //16 <= cps <= 1024 for freq=1MHz
    TCCR0A |= 1<<WGM01;          //Put Timer/Counter0 in CTC timer_mode
    TIMSK0 |= 1<<OCIE0A;         //Enable timer compare interrupt
}
void static inline disable_display_pwm_timer() {
    TCCR0B &= ~(1<<CS02);
}
void static inline enable_display_pwm_timer() {
    TCCR0B |= 1<<CS02;
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
    EICRA  |= (1 << ISC01); //Interrupt executed on falling edge (reverse logic!).
    EIMSK  |= (1 << INT0);
}
void static inline disable_int0() {
    EIMSK  &= ~(1 << INT0);
    EICRA  &= ~(1 << ISC01);
}

int main(void)
{

    cli();                      //Disable global interrupts
    setup_main_timer(PERIOD);
    setup_display_pwm_timer(FPS);

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

void static inline trigger_motor(uint8_t clk) {
    DDRD  |=  0b00100000 << clk;
    PORTD |=  0b00100000 << clk;
    _delay_ms(100);
    PORTD &= ~0b00100000 << clk;
    DDRD  &= ~0b00100000 << clk;
}

void static inline swich_power_off() {
    EIFR |= 1<<INTF0; //clear int0 flag so uC wont wake up immediately
    enable_int0();
    current_sleep_mode = SLEEP_MODE_PWR_DOWN;
}

ISR(TIMER1_COMPA_vect)
{
    timer_counter = (timer_counter + 1) % FPM;

    if (clock_num < CLKNUM) {

        parity = !parity; //blink currently edited digit every half second
        set_custom_mask(~(parity << current_digit));

    } else if (timer_counter == 0) {

        uint8_t clk = 0;
        for(; clk < CLKNUM; clk++) {
            if (is_zero(clk))
                continue;
            decrement(clk);
            if (is_zero(clk))
                orders[clk] = 1;
        }

    } else if (timer_counter == DELAY) {
        
        uint8_t clk = 0;
        uint8_t all_zero = 1;
        for(; clk < CLKNUM; clk++) {
            if (orders[clk]) {
                orders[clk] = 0;
                trigger_motor(clk);
                timer_counter = 0; // delay next timers checks
                return;
            }
            all_zero &= is_zero(clk);
        }
        if (all_zero) //all timers are inactive
            swich_power_off();
    }

}

ISR(TIMER0_COMPA_vect)
{    
    increment_display();
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
        if (clock_num < CLKNUM) { // edit <clock_num> timer
            set_custom_mask(0b11111111);
            set_clock(clock_num);
            current_digit = 0;
            enable_display_pwm_timer();
        } else { /*if (timer_mode == CLKNUM) - start counting */
            timer_counter = 0;
            disable_display_pwm_timer();
            set_custom_mask(0b00000000);
            increment_display();  //necessary, because in saving mode display is no more incremented
        }
    }
    
    if ((input_diff & CHANGE_DIGIT) && (clock_num < CLKNUM)) /* PD3 - change currently edited digit */
        current_digit = (current_digit + 1) % 4;

    if ((input_diff & INCR_DIGIT) && (clock_num < CLKNUM)) /* PD2 - increment digit */
        rot_digit(clock_num, current_digit);
}