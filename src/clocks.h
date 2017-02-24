#ifndef CLOCKS
#define CLOCKS

#define CLKNUM 3

void set_clock( uint8_t );
void decrement( uint8_t );
void rot_digit( uint8_t, uint8_t );
uint8_t is_zero( uint8_t );

#endif
