all:
	avr-g++ -std=c++14 -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -Werror scorgasmatron.cpp --output scorgasmatron.elf
	avr-objcopy -O ihex scorgasmatron.elf scorgasmatron.hex

program:
	avrdude -p atmega328p -c usbtiny -U flash:w:scorgasmatron.hex

clean:
	rm scorgasmatron.hex scorgasmatron.elf
