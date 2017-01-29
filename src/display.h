#ifndef DISPLAY
#define DISPLAY

void increment_display( void );
void set_value( uint16_t );
void set_digits( uint8_t[4] );
void setup_display( uint16_t );

void enable_custom_mask();
void disable_custom_mask();
void set_custom_mask(uint8_t mask);

#endif
