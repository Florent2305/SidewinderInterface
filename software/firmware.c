#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>

#include "bits.h"
#include "callbacks.h"

#include "ks0108.c"
#include "uart.c"
#include "sidewinder.c"

#define INDI_DDR DDRB
#define INDI_PORT PORTB
#define INDI_P PB7

volatile uint8_t is_data_valid = 0;

void sw_data_is_now_invalid(void)
{
	is_data_valid = 0;
}

void sw_data_is_now_valid(void)
{
	if(sw_dta.btn_fire)
		CLEARBIT(INDI_PORT, INDI_P);
	else
		SETBIT(INDI_PORT, INDI_P);

	is_data_valid = 1;
}


#define XY_SZ 5


void ks0108DrawPixmap8P(uint8_t x, const uint8_t y, uint8_t count, const uint8_t *pixels)
{
	pixels += count;
	while(count-- > 0)
	{
		ks0108GotoXY(x++, y);
		ks0108WriteData(pgm_read_byte(--pixels));
	}
}

int __attribute__((OS_main))
main(void)
{
	// setup sidewinder device communication
	sw_setup();

	// initialize display
	ks0108Init(0);
	_delay_ms(1000);

	static const PROGMEM uint8_t lArrow[3] = {0b00100, 0b01010, 0b10001};
	static const PROGMEM uint8_t rArrow[3] = {0b10001, 0b01010, 0b00100};
	static const PROGMEM uint8_t tArrow[5] = {0b001, 0b010, 0b100, 0b010, 0b001};
	static const PROGMEM uint8_t bArrow[5] = {0b100, 0b010, 0b001, 0b010, 0b100};

	static const PROGMEM uint8_t lArrowOn[3] = {0b00100, 0b01110, 0b11111};
	static const PROGMEM uint8_t rArrowOn[3] = {0b11111, 0b01110, 0b00100};
	static const PROGMEM uint8_t tArrowOn[5] = {0b001, 0b011, 0b111, 0b011, 0b001};
	static const PROGMEM uint8_t bArrowOn[5] = {0b100, 0b110, 0b111, 0b110, 0b100};


	ks0108DrawRect( 0,  0, 63, 63, BLACK); // xy-area
	ks0108DrawRect( 67, 0,  5, 63, BLACK); // m-bar
	ks0108DrawRect( 76, 0, 51,  5, BLACK); // r-bar


	ks0108DrawRect( 76, 54, 51,  9, BLACK); // fire-btn

	ks0108DrawRect( 76, 43,  9,  9, BLACK); // head-up-btn
	ks0108DrawRect( 76, 32,  9,  9, BLACK); // head-down-btn
	ks0108DrawRect(118, 32,  9, 20, BLACK); // head-btn

	ks0108DrawRect( 76,  8,  9, 20, BLACK); // shift-btn

	ks0108DrawRect(107,  8,  9,  9, BLACK); // a-btn
	ks0108DrawRect(101, 19,  9,  9, BLACK); // b-btn
	ks0108DrawRect(112, 19,  9,  9, BLACK); // c-btn
	ks0108DrawRect(118,  8,  9,  9, BLACK); // d-btn

	// arrows
	ks0108DrawPixmap8P( 92, 40, 3, rArrow);
	ks0108DrawPixmap8P(108, 40, 3, lArrow);
	ks0108DrawPixmap8P( 99, 49, 5, tArrow);
	ks0108DrawPixmap8P( 99, 33, 5, bArrow);

	// led connected to that indicator as output
	SETBIT(INDI_DDR, INDI_P);

	// enable interrupts
	sei();

	while(1)
	{
		if(is_data_valid)
		{
			uint8_t sreg_tmp = SREG;
			cli();

			sw_data_t c_dta = sw_dta;

			SREG = sreg_tmp;

			// x & y display coords
			uint8_t
				x = (((1024 - (uint16_t)c_dta.x) * (62-XY_SZ)) / 1024) + 1,
				y = (((1024 - (uint16_t)c_dta.y) * (62-XY_SZ)) / 1024) + 1;

			// clear right half of screen
			if(x > 1)
				ks0108FillRect(1, 1, x-2, 61, WHITE);

			// clear left half of screen
			if(x < 58)
				ks0108FillRect(x + XY_SZ, 1, 62 - x - XY_SZ, 61, WHITE);

			// clear part below the dot
			if(y > 1)
				ks0108FillRect(x, 1, XY_SZ - 1, y - 2, WHITE);

			// clear part above the dot
			if(y < 58)
				ks0108FillRect(x, y + XY_SZ, XY_SZ - 1, 57 - y, WHITE);


			// paint the dot
			ks0108FillRect(
				x, y,
				XY_SZ-1, XY_SZ-1,
				BLACK
			);





			// movable-member
			uint8_t
				m = (((128 - (uint16_t)c_dta.m) * 62) / 128);

			// clear movable-member bar
			if(m < 62)
				ks0108FillRect(68, m + 1, 3, 61 - m, WHITE);

			// fill movable-member bar
			if(m > 0)
				ks0108FillRect(68, 1, 3, m - 1, BLACK);





			// rotation
			uint8_t
				r = ((((uint16_t)c_dta.r) * 49) / 64);

			// clear rotation bar
			ks0108FillRect(77, 1, 49, 3, WHITE);
			// fill movable-member bar
			if(r > 24)
				ks0108FillRect(77+25, 1, r-24, 3, BLACK);
			else
				ks0108FillRect(77+r,  1, 24-r, 3, BLACK);


			// fire btn
			ks0108FillRect( 77, 55, 49,  7, c_dta.btn_fire ? WHITE : BLACK);


			// head-up-btn
			ks0108FillRect( 77, 44,  7,  7, c_dta.btn_top_up ? WHITE : BLACK);

			// head-down-btn
			ks0108FillRect( 77, 33,  7,  7, c_dta.btn_top_down ? WHITE : BLACK);

			// head-up-btn
			ks0108FillRect(119, 33,  7, 18, c_dta.btn_top ? WHITE : BLACK);

			// shift-btn
			ks0108FillRect( 77,  9,  7, 18, c_dta.btn_shift ? WHITE : BLACK);

			// [a-d]-btn
			ks0108FillRect(108,  9,  7,  7, c_dta.btn_a ? WHITE : BLACK);
			ks0108FillRect(102, 20,  7,  7, c_dta.btn_b ? WHITE : BLACK);
			ks0108FillRect(113, 20,  7,  7, c_dta.btn_c ? WHITE : BLACK);
			ks0108FillRect(119,  9,  7,  7, c_dta.btn_d ? WHITE : BLACK);

			// arrows
			ks0108DrawPixmap8P( 92, 40, 5, (c_dta.head >= 2 && c_dta.head <= 4) ? bArrowOn : bArrow);
			ks0108DrawPixmap8P(108, 40, 3, (c_dta.head >= 6 && c_dta.head <= 8) ? lArrowOn : lArrow);
			ks0108DrawPixmap8P( 99, 49, 5, ((c_dta.head >= 1 && c_dta.head <= 2) || c_dta.head == 1) ? tArrowOn : tArrow);
			ks0108DrawPixmap8P( 99, 33, 5, (c_dta.head >= 4 && c_dta.head <= 6) ? bArrowOn : bArrow);
		}
	}
	return 0;
}
