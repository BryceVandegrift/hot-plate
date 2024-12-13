#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/delay.h>

#include "oled.h"

#define MOSFET_ON PORTA |= (1 << PORTA7)
#define MOSFET_OFF PORTA &= ~(1 << PORTA7)

#define EEPROM_ADDR 0x00

#define MAX_TEMP 180
/* 2000 mV on ADC is around 6.5 V */
#define MIN_MV 2000

#define VERSION "Version 1.0"

static void setup(void);
static uint8_t analog_read(uint8_t pin);
static void int_to_str(char *s, uint16_t n);
static uint16_t read_temp(void);
static uint16_t read_volts(void);
static void cool(void);
static void heat(void);

static uint8_t temp = 50;

void
setup()
{
	uint8_t eeprom;

	/* Pinouts:
	 * PA0: Not connected
	 * PA1: Not connected
	 * PA2: Voltage monitor
	 * PA3: Temperature input
	 * PA4: Slave clock
	 * PA5: Not used (MISO)
	 * PA6: Slave data
	 * PA7: MOSFET output
	 *
	 * PB0: UP button input
	 * PB1: DOWN button input
	 * PB2: SELECT button input
	 * PB3: Not used (RESET)
	 */
	DDRA = 0xd0; /* 11010000 */
	DDRB = 0x00;

	PORTA = 0x50; /* 01010000 */
	PORTB = 0x00;

	/* enable power to ADC */
	PRR &= ~(1 << PRADC);

	/* use VCC as AREF */
	ADMUX &= ~((1 << REFS1) | (1 << REFS0));

	/* left justify ADC bits for clean 8-bit value */
	ADCSRB |= (1 << ADLAR);

	/* enable ADC on port A */
	ADCSRA |= (1 << ADEN);

	/* pull temp from eeprom */
	eeprom = eeprom_read_byte(EEPROM_ADDR);
	if (eeprom >= 50 && eeprom <= MAX_TEMP) {
		temp = eeprom;
	}
}

uint8_t
analog_read(uint8_t pin)
{
	ADMUX = ((ADMUX & 0xf0) | pin);

	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADCH;
}

void
int_to_str(char *s, uint16_t n)
{
	uint16_t i = 10000;
 
	while (i > n) i /= 10;

	do {
		*s++ = '0' + (((n - n % i) / i) % 10);
	} while (i /= 10);

	if (n < 100) {
		*s = ' ';
		s++;
	}

	*s = 0;
}

uint16_t
read_temp()
{
	uint8_t i;
	float data = 0;

	for (i = 0; i < 100; i++) {
		data += analog_read(PINA3);
	}

	data /= 100;
	data *= 19.53125; /* (5.0 / 256) * 1000 */
	return (uint16_t)((data - 2637) / -13.6);
}

uint16_t
read_volts()
{
	uint8_t i;
	float data = 0;

	for (i = 0; i < 20; i++) {
		data += analog_read(PINA2);
	}

	data /= 20;
	data *= 19.53125; /* (5.0 / 256) * 1000 */
	return (uint16_t)data;
}

void
cool()
{
	uint16_t rtemp;
	char buf[8];

	oled_clear();

	oled_setpos(0, 0);
	oled_print("Cooling...");

	oled_setpos(0, 3);
	oled_print("Temp: ");

	for (;;) {
		oled_setpos(32, 3);
		rtemp = read_temp();
		int_to_str(buf, rtemp);
		oled_print(buf);

		if (PINB & (1 << PINB2)) {
			break;
		}

		_delay_ms(100);
	}

	return;
}

void
heat()
{
	uint16_t rtemp;
	char buf[8];

	oled_clear();

	/* write temp to eeprom */
	eeprom_write_byte(EEPROM_ADDR, temp);

	_delay_ms(1000);

	oled_setpos(0, 0);
	oled_print("Heating...");

	oled_setpos(0, 3);
	oled_print("Temp: ");

	for (;;) {
		oled_setpos(32, 3);
		rtemp = read_temp();
		int_to_str(buf, rtemp);
		oled_print(buf);

		if (read_volts() < MIN_MV) {
			goto undervolt;
		}

		if (rtemp < temp) {
			MOSFET_ON;
		} else {
			MOSFET_OFF;
		}

		if (PINB & (1 << PINB2)) {
			break;
		}

		_delay_ms(100);
	}

	MOSFET_OFF;

	_delay_ms(1000);

	/* cool down plate */
	cool();

	_delay_ms(1000);

	return;

undervolt:

	MOSFET_OFF;

	oled_clear();
	oled_setpos(0, 0);
	oled_print("Voltage is too low!");

	for (;;) {
		if (PINB & (1 << PINB2)) {
			break;
		}
	}

	return;
}

int
main()
{
	char buf[8];

	setup();

	_delay_ms(1000);

	oled_init();
	oled_clear();
	oled_setpos(0, 0);
	oled_print("PCB Hot Plate");
	oled_setpos(0, 3);
	oled_print(VERSION);

reset:
	_delay_ms(3000);
	oled_clear();

	oled_setpos(0, 0);
	oled_print("Set temperature");

	oled_setpos(0, 3);
	oled_print("Set Temp: ");

	for (;;) {
		oled_setpos(64, 3);
		int_to_str(buf, temp);
		oled_print(buf);

		if (PINB & (1 << PINB0) && temp < MAX_TEMP) {
			temp++;
		} else if (PINB & (1 << PINB1) && temp > 50) {
			temp--;
		} else if (PINB & (1 << PINB2)) {
			heat();
			goto reset;
		}

		_delay_ms(100);
	}

	return 0;
}
