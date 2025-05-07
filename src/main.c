/* See LICENSE for license details. */

#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/delay.h>

#include "oled.h"

#define DEBOUNCE while((PINB & (1 << PINB0)) || (PINB & (1 << PINB1)) || (PINB & (1 << PINB2))) {}

#define MOSFET_ON PORTA |= (1 << PORTA7)
#define MOSFET_OFF PORTA &= ~(1 << PORTA7)

#define TEMP_ADDR 0x00
#define PREHEAT_ADDR 0x01
#define REFLOW_ADDR 0x02
#define SOAK_ADDR 0x03
#define LIQUID_ADDR 0x04

/* min and max temperatures */
#define MAX_TEMP 180
#define MIN_TEMP 50

/* min and max times for soak and reflow */
#define MAX_SOAK 250
#define MIN_SOAK 10
#define MAX_REFLOW 60
#define MIN_REFLOW 1
/* 2000 mV on ADC is around 6.5 V */
#define MIN_MV 2000

#define VERSION "Version 2.0"

static void setup(void);
static uint8_t analog_read(uint8_t pin);
static void int_to_str(char *s, uint16_t n);
static uint16_t read_temp(void);
static uint16_t read_volts(void);
static void cool(void);
static void heat(const char *msg, uint8_t set_temp, uint8_t stop);
static void hold(const char *msg, uint8_t set_temp, uint8_t set_sec);
static void set_var(const char *msg1, const char *msg2, uint8_t min, uint8_t max, uint8_t *var);
static void normal_mode(void);
static void curve_mode(void);

/* normal mode temperature */
static uint8_t temp = MIN_TEMP;
/* curve mode temperatures */
static uint8_t preheat = MIN_TEMP;
static uint8_t reflow = 100;
/* curve mode times */
static uint8_t soak = 30;
static uint8_t liquid = 10;

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

	/* pull normal mode temp from eeprom */
	__EEGET(eeprom, TEMP_ADDR);
	if (eeprom >= MIN_TEMP && eeprom <= MAX_TEMP) {
		temp = eeprom;
	}

	/* pull heat curve mode variables */
	eeprom_busy_wait();
	__EEGET(eeprom, PREHEAT_ADDR);
	if (eeprom >= MIN_TEMP && eeprom <= MAX_TEMP) {
		preheat = eeprom;
	}

	eeprom_busy_wait();
	__EEGET(eeprom, REFLOW_ADDR);
	if (eeprom >= MIN_TEMP && eeprom <= MAX_TEMP) {
		reflow = eeprom;
	}

	eeprom_busy_wait();
	__EEGET(eeprom, SOAK_ADDR);
	if (eeprom >= MIN_SOAK && eeprom <= MAX_SOAK) {
		soak = eeprom;
	}

	eeprom_busy_wait();
	__EEGET(eeprom, LIQUID_ADDR);
	if (eeprom >= MIN_REFLOW && eeprom <= MAX_REFLOW) {
		liquid = eeprom;
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

	DEBOUNCE;

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

		_delay_ms(10);
	}
}

void
heat(const char *msg, uint8_t set_temp, uint8_t stop)
{
	uint16_t rtemp;
	char buf[8];

	oled_clear();

	DEBOUNCE;

	oled_setpos(0, 0);
	oled_print(msg);

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

		if (rtemp < set_temp) {
			MOSFET_ON;
		} else {
			MOSFET_OFF;
		}

		if (PINB & (1 << PINB2)) {
			break;
		}

		/* if stop is set and target temp is reached, return */
		if (stop && rtemp >= set_temp) {
			return;
		}

		_delay_ms(10);
	}

	MOSFET_OFF;

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
}

void
hold(const char *msg, uint8_t set_temp, uint8_t set_sec)
{
	uint16_t rtemp;
	char buf[8];
	uint8_t i;

	oled_clear();

	DEBOUNCE;

	oled_setpos(0, 0);
	oled_print(msg);

	oled_setpos(0, 2);
	oled_print("Time: ");

	oled_setpos(0, 3);
	oled_print("Temp: ");

	/*
	 * a simple (but inaccurate) alternative to using timers.
	 * works well enough I guess... 
	 */
	for (i = set_sec; i > 0; i--) {
		oled_setpos(32, 3);
		rtemp = read_temp();
		int_to_str(buf, rtemp);
		oled_print(buf);

		oled_setpos(32, 2);
		int_to_str(buf, i);
		oled_print(buf);

		if (read_volts() < MIN_MV) {
			goto undervolt;
		}

		if (rtemp < set_temp) {
			MOSFET_ON;
		} else {
			MOSFET_OFF;
		}

		if (PINB & (1 << PINB2)) {
			break;
		}

		_delay_ms(1000);
	}

	MOSFET_OFF;

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
}

void
set_var(const char *msg1, const char *msg2, uint8_t min, uint8_t max, uint8_t *var)
{
	char buf[8];

	oled_clear();

	oled_setpos(0, 0);
	oled_print(msg1);

	oled_setpos(0, 3);
	oled_print(msg2);

	for (;;) {
		oled_setpos(64, 3);
		int_to_str(buf, *var);
		oled_print(buf);

		if (PINB & (1 << PINB0) && *var < max) {
			(*var)++;
		} else if (PINB & (1 << PINB1) && *var > min) {
			(*var)--;
		} else if (PINB & (1 << PINB2)) {
			return;
		}

		_delay_ms(100);
	}
}

void
normal_mode()
{
	oled_clear();

	DEBOUNCE;

	set_var("Set temperature", "Set temp:", MIN_TEMP, MAX_TEMP, &temp);

	/* write normal mode temp to eeprom */
	__EEPUT(TEMP_ADDR, temp);

	heat("Heating...", temp, 0);
	cool();
}

void
curve_mode()
{
	oled_clear();

	DEBOUNCE;

	set_var("Set preheat temp", "Set temp:", MIN_TEMP, MAX_TEMP, &preheat);
	set_var("Set soak time", "Set time:", MIN_SOAK, MAX_SOAK, &soak);
	set_var("Set reflow temp", "Set temp:", MIN_TEMP, MAX_TEMP, &reflow);
	set_var("Set reflow time", "Set time:", MIN_REFLOW, MAX_REFLOW, &liquid);

	/* write heat curve values to eeprom */
	__EEPUT(PREHEAT_ADDR, preheat);
	eeprom_busy_wait();
	__EEPUT(SOAK_ADDR, soak);
	eeprom_busy_wait();
	__EEPUT(REFLOW_ADDR, reflow);
	eeprom_busy_wait();
	__EEPUT(LIQUID_ADDR, liquid);

	heat("Preheating...", preheat, 1);
	hold("Soaking...", preheat, soak);
	heat("Heating to reflow...", reflow, 1);
	hold("Reflowing...", reflow, liquid);
	cool();
}

int
main()
{
	uint8_t cursor = 0;

	setup();

	_delay_ms(1000);

	oled_init();
	oled_clear();
	oled_setpos(0, 0);
	oled_print("PCB Hot Plate");
	oled_setpos(0, 3);
	oled_print(VERSION);
	_delay_ms(3000);

reset:
	oled_clear();

	oled_setpos(0, 0);
	oled_print("Select mode:");

	oled_setpos(8, 2);
	oled_print("Normal heating");

	oled_setpos(8, 3);
	oled_print("Heat curve");

	for (;;) {
		if (!cursor) {
			oled_setpos(0, 2);
			oled_print(">");
			oled_setpos(0, 3);
			oled_print(" ");
		} else {
			oled_setpos(0, 2);
			oled_print(" ");
			oled_setpos(0, 3);
			oled_print(">");
		}

		if (PINB & (1 << PINB0) || PINB & (1 << PINB1)) {
			cursor = !cursor;
		} else if (PINB & (1 << PINB2)) {
			if (cursor) {
				curve_mode();
			} else {
				normal_mode();
			}

			break;
		}

		_delay_ms(100);
	}

	goto reset;

	return 0;
}
