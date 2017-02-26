avr-gcc -mmcu=atmega168p -o main.o src/main.c src/display.c src/clocks.c -O2 -Wall -DF_CPU=1000000ULL
avr-objcopy -O ihex main.o main.hex
sudo avrdude -P /dev/ttyACM0 -c stk500v2 -p m168p -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xf9:m -U flash:w:main.hex
