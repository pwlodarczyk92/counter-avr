avr-gcc -mmcu=atmega168p -o main.o src/main.c src/display.c src/clocks.c -O2 -Wall -DF_CPU=1000000ULL
avr-objcopy -O ihex main.o main.hex
