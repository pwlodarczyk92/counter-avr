avr-gcc -mmcu=atmega168p -o main.o src/main.c src/display.c -Os -Wall -DF_CPU=1000000UL
avr-objcopy -O ihex main.o main.hex
