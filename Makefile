all:
	avr-g++ -std=c++14 -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -Werror scornado.cpp --output scornado.elf
	avr-objcopy -O ihex scornado.elf scornado.hex

program:
	avrdude -p atmega328p -c usbtiny -U flash:w:scornado.hex

clean:
	rm scornado.hex scornado.elf
