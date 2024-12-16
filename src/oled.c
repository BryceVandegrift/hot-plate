/* See LICENSE for license details. */

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "font.h"
#include "oled.h"

#define SDA_ON PORTA |= (1 << PORTA6)
#define SDA_OFF PORTA &= ~(1 << PORTA6)
#define SCL_ON PORTA |= (1 << PORTA4)
#define SCL_OFF PORTA &= ~(1 << PORTA4)

#define READ_SDA PINA & (1 << PINA6)
#define READ_SCL PINA & (1 << PINA4)

#define OLED_ADDRESS 0x3c
#define OLED_COMMAND 0x00
#define OLED_DATA 0x40

static void i2c_tx(uint8_t data);
static void i2c_start(void);
static void i2c_stop(void);

static const uint8_t init_bytes[] PROGMEM = {
	0xae,
	0xd5,
	0x80,
	0xa8,
	0x1f,
	0xd3,
	0x00,
	0x40,
	0x8d,
	0x14,
	0x20,
	0x00,
	0xa0,
	0xc8,
	0xda,
	0x02,
	0x81,
	0x8f,
	0xd9,
	0xc2,
	0xdb,
	0x40,
	0xa4,
	0xa6,
	0x2e,
	0xa1,
	0xaf
};

void
i2c_tx(uint8_t data)
{
	uint8_t i;

	for (i = 0; i < 8; i++) {
		if (data & 0x80) {
			SDA_ON;
		} else {
			SDA_OFF;
		}

		SCL_ON;
		SCL_OFF;
		data <<= 1;
	}

	SDA_ON;
	SCL_ON;
	SCL_OFF;
}

void
i2c_start()
{
	SDA_ON;
	SCL_ON;
	SDA_OFF;
	SCL_OFF;

	i2c_tx(OLED_ADDRESS << 1);
}

void
i2c_stop()
{
	SDA_OFF;
	SCL_ON;
	SDA_ON;
}

void
oled_init()
{
	uint8_t i;

	i2c_start();
	/* send commands */
	i2c_tx(OLED_COMMAND);

	for (i = 0; i < sizeof(init_bytes); i++) {
		i2c_tx(pgm_read_byte(&init_bytes[i]));
	}

	i2c_stop();
}

void
oled_clear()
{
	uint8_t i, j;

	for (i = 0; i < 8; i++) {
		i2c_start();
		i2c_tx(OLED_COMMAND);
		
		i2c_tx(0xb0 + i);
		i2c_tx(0x00);
		i2c_tx(0x10);

		i2c_stop();

		i2c_start();
		i2c_tx(OLED_DATA);

		for (j = 0; j < 128; j++) {
			i2c_tx(0x00);
		}

		i2c_stop();
	}
}

void
oled_setpos(uint8_t x, uint8_t y)
{
	i2c_start();
	i2c_tx(OLED_COMMAND);

	i2c_tx(0xb0 + y);
	i2c_tx(((x & 0xf0) >> 4) | 0x10);
	i2c_tx((x & 0x0f) | 0x01);

	i2c_stop();
}

void
oled_putchar(char c)
{
	uint8_t i;

	c -= 32;

	i2c_start();
	i2c_tx(OLED_DATA);

	for (i = 0; i < 6; i++) {
		i2c_tx(pgm_read_byte(&font[c * 6 + i]));
	}

	i2c_stop();
}

void
oled_print(const char *s)
{
	while (*s) {
		oled_putchar(*s++);
	}
}
