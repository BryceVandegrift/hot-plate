# PCB Hot Plate code for ATTINY84a
# See LICENSE file for copyright and license details.

CC = avr-gcc
OBJCPY = avr-objcopy
CFLAGS = -std=c89 -pedantic -Wall -Wextra -Os -DF_CPU=1000000UL -mmcu=attiny84a -fno-split-wide-types

SRC = main.c oled.c
OBJ = ${SRC:.c=.o}

all: main.hex

.c.o:
	${CC} -c ${CFLAGS} $<

main.elf: ${OBJ}
	${CC} -o $@ ${OBJ} ${CFLAGS}

main.hex: main.elf
	${OBJCPY} -O ihex main.elf $@

clean:
	rm -f main.elf main.hex ${OBJ}

#flash: all
#	doas avrdude -p t84a -P /dev/ttyACM0 -c avrisp -b 19200 -U flash:w:main.hex

.PHONY: all clean
