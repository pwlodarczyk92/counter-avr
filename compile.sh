avr-gcc -mmcu=atmega168p -o main.o src/main.c src/display.c -O2 -Wall -DF_CPU=1000000UL
avr-objcopy -O ihex main.o main.hex
