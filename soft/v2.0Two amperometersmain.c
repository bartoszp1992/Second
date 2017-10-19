/*
 * main.c
 *
 *	Bart's SECOND
 *
 *	for ATmega 8A
 *	Created on: 31 sty 2017
 *	Author: Bart
 *
 *	Ver 2.0
 *	-under construction- two amperometers
 */


// #define F_CPU 1000000UL (no use in eclipse- go to properties)
#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#define fire (1<<PC2)
#define plus (1<<PC1)
#define minus (1<<PC0)
#define pwm PB1
#define stb PC5
#define adcin PC3
#define adcin2 PC4
#define ind4 PD0
#define ind3 PD1
#define ind2 PD2
#define ind1 PD3
#define buz PD4
#define warm PD7
#define bl PD6

//Pins description
// 25(PC2)button fire
// 24(PC1)button plus
// 23(PC0)button minus

// 15(PB1) PWM Out(mosfet gate)

// 28(PC5) standby(green)
// 26(PC3) ADC3 negative input
// 27(PC4) ADC4 negative input(amperometer)

// 02(PD0) ind4 (blue)
// 03(PD1) ind3 (blue)
// 04(PD2) ind2 (blue)
// 05(PD3) ind1 (blue)
// 13(PD7) warming(red)
// 06(PD4) buzzer
// 12(PD6) backlight

//beeps description
// 2- locked
// 3- unlocked
// 1-menu enter/exit/set/check battery
// 4- no atomizer
// 5- low battery
//long- short circuit

//add bytes for save values
EEMEM unsigned char d; //for duty
EEMEM unsigned char p; // for preheat duty
EEMEM unsigned char t; // for preheat time

int main(void)
{

//Variables and constants
//main
int duty, step, ds1, ds2, ds3, ds4, adj_counter, counter, phstep, phd;
step = 1; //step for duty change
duty = 30; //default duty
//duty indicator steps
ds1 = 60;
ds2 = 120;
ds3 = 180;
ds4 = 240;
adj_counter = 20;//after which steps adjust set to fast mode
counter = 1;

//preheat time inters
phstep = 1; //preheat adjust step
phd = 255; //default ph duty

//times
int t_warm, t_blink, t_splash, t_press, t_pause, v_check, t_adj_s, t_adj_f, v_display, t_unlock, tph, tph1, tph2, tph3, tph4;

t_warm = 40; //warming stipe speed
t_blink = 250; // blinking at notifications
t_splash = 100; // time for splash and other transitions/ 1 state lock blinking
t_press = 150;// waiting after press(menu)
v_display = 1500; // display battery state time/0 state lock blinking
t_pause = 100; // pause after display battery state/menu blinking if selected
v_check = 350; //2000 single, 350 for serial.checking load voltages- time for capacitor
t_adj_s = 35; //slow power adjust time
t_adj_f = 12; //fast power adjust time/preheat adjust time
t_unlock = 800; //unlocking time
//preheat times
tph1 = 100;
tph2 = 200;
tph3 = 500;
tph4 = 800;
tph = 1;//default preheat time

//voltages
int voltage, low_voltage, min_l_v, vs1, vs2, vs3, vs4, l_v, fl_v, v_dcr, max_v_dcr, i_v, min_v_dcr;

low_voltage = 2550;
min_l_v = 2600; // minimum load voltage

max_v_dcr = 1250;//default: 650 for short protection- max voltage decrease
//@up
// 650 for serial, min 0.35 Ohm
// 650 for single, min 0.2 Ohm
// 750- ?


min_v_dcr = 50; //10- for single, 50 for serial//no atomizer- min voltage decrease// 2 not tested yet

//battery indicator steps
vs1 = 3600;
vs2 = 3700;
vs3 = 3800;
vs4 = 4000;

/*
//amperometer - under construction
int b_voltage, t_warm2, p_voltage, decrease, amperage, amp1, amp2, amp3, amp4, t_ampcheck, ampcount, ampchecks;

amp1 = 5;
amp2 = 10;
amp3 = 20;
amp4 = 30;
t_ampcheck = 200;// time od display single amp valuse
ampchecks = 500; // number of checks in one warming cycle
t_warm2 = 10; //warming time unit
*/

//amperometer2 - under construction

uint16_t multiplier, max_amp, amperage;
int t_warm2, amp1, amp2, amp3, amp4, amp5, amp6, amp7, amp8, amp9, amp10, amp11, amp12, amp13, amp14, amp15;
amp1 = 2000;
amp2 = 4000;
amp3 = 6000;
amp4 = 8000;
amp5 = 10000;
amp6 = 12000;
amp7 = 14000;
amp8 = 16000;
amp9 = 18000;
amp10 = 20000;
amp11 = 22000;
amp12 = 24000;
amp13 = 26000;
amp14 = 28000;
amp15 = 30000;
t_warm2 = 100;

//modes
int menu_toggle, opt, mode, lock, pha, tpha;
menu_toggle = 0;
opt = 4; //default optin after enter menu(4- lock)
mode = 0; //default mode(normal)
lock = 1; //lock on by default
pha = 0; //preheat adjust mode
tpha = 0; //preheat time adjust mode


//define ports
//B- PWM PB1(pin15)- OC1A
DDRB = 0x3F;  // 111 111
PORTB = 0x00; // 000 000

//D- indicator, warming led.
DDRD = 0xFF;  // 1111 1111 all D as Out
PORTD = 0x00; // 0000 0000

//C- voltage meter, standby led(PC5, pin28), (PC6 reset- dont use), buttons
DDRC = 0x60;  // 110 0 000 -PC6 reset, PC5 standby led, others for voltage meter
PORTC = 0x07; // 000 0 111 - buttons high

//define hardware PWM

	//mode3, fast PWM
TCCR1A |= (1<<WGM10);
TCCR1B |= (1<<WGM12);

	//compare output mode(set for pin15)-- moved to warming
//TCCR1A |= (1<<COM1A1);

	//prescaler/freq(freq= clock/prescaler/256)
TCCR1B |= (1<<CS10); // no prescaler
// TCCR1B |= (1<<CS11);// /8
// TCCR1B |= (1<<CS10) | (1<<CS11); // /64

OCR1A = 0; //PWM out as low

//define ADC
	//prescaler- set for 16
// ADCSRA |= (1<<ADPS0);
// ADCSRA |= (1<<ADPS1);
ADCSRA |= (1<<ADPS2);
ADCSRA |= (1<<ADEN);

	//ref select- internal
SFIOR |= (1<<ACME);

//ADMUX ^= (1<<MUX2);
//ADMUX |= (1<<MUX1);
//ADMUX |= (1<<MUX0); //adc3

//ADMUX |= (1<<MUX2);
ADMUX ^= (1<<MUX1);
ADMUX ^= (1<<MUX0); //adc4(conn pin)

	//set internal vref(2.56V)
ADMUX |= (1<<REFS1);
ADMUX |= (1<<REFS0);

//read last values from eeprom
			duty = eeprom_read_byte(&d); //duty
			phd = eeprom_read_byte(&p); // preheat duty
			tph = eeprom_read_byte(&t); // preheat time

//splash
PORTC |= (1<<stb);
PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
_delay_ms(t_splash);
PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
_delay_ms(t_splash);
PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
_delay_ms(t_splash);
PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
_delay_ms(t_splash);
PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
_delay_ms(t_splash);
PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
_delay_ms(t_splash);
PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
_delay_ms(t_splash);
PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
_delay_ms(t_splash);
PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
_delay_ms(t_splash);
PORTC = 0x07;

while(1)
{
// standby light on
PORTC |= (1<<stb);

//backlight on
PORTD |= (1<<bl);

//low battery check
ADMUX ^= (1<<MUX2);
ADMUX |= (1<<MUX1);
ADMUX |= (1<<MUX0); //adc3
ADCSRA |= (1<<ADSC); //start
while(ADCSRA & (1<<ADSC));
voltage = ADC * 5;
while(voltage < low_voltage )
	{
	OCR1A = 0;
	//low battery blinking
	PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
	_delay_ms(t_blink);
	PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);
	_delay_ms(t_blink);
	PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
	_delay_ms(t_blink);
	PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);
	_delay_ms(t_blink);
	PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
	_delay_ms(t_blink);
	PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);
	_delay_ms(t_blink);

	//check again
	ADMUX ^= (1<<MUX2);
ADMUX |= (1<<MUX1);
ADMUX |= (1<<MUX0); //adc3
	ADCSRA |= (1<<ADSC); //start
	while(ADCSRA & (1<<ADSC));
	voltage = ADC * 5;
	}

// lock
while(lock == 1)
	{
	PORTC |= (1<<stb);
	_delay_ms(t_splash);
	PORTC ^= (1<<stb);
	_delay_ms(v_display);
	PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<bl);

	if (!(PINC & fire) && !(PINC & minus) && (PINC & plus))
		{
		_delay_ms(t_unlock);
		if(!(PINC & fire) && !(PINC & minus) && (PINC & plus))
			{
			menu_toggle = 0;
			lock = 0;
			PORTC |= (1<<stb);
			PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTC = 0x07;
			}
		}
	}

// reset counter
counter = 1;

// menu

if (!(PINC & fire) && !(PINC & minus) && (PINC & plus) && (menu_toggle == 0))
	{
	menu_toggle = 1;
	opt = 4;
	PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (0<<ind1) | (0<<ind2) | (1<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (0<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	}

while(menu_toggle == 1)
	{
	if (!(PINC & plus) && (opt < 4))
		{
		opt = opt +1;
		PORTD |= (1<<buz);
		_delay_ms(t_press);
		PORTD ^= (1<<buz);
		}

	if (!(PINC & minus) && (opt > 1))
		{
		opt = opt -1;
		PORTD |= (1<<buz);
		_delay_ms(t_press);
		PORTD ^= (1<<buz);
		}

	if ( opt == 1 ) PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (1<<bl);
	if ( opt == 2 ) PORTD = (0<<ind1) | (1<<ind2) | (0<<ind3) | (0<<ind4) | (1<<bl);
	if ( opt == 3 ) PORTD = (0<<ind1) | (0<<ind2) | (1<<ind3) | (0<<ind4) | (1<<bl);
	if ( opt == 4 ) PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (1<<ind4) | (1<<bl);

	//opt1- mechanic
	if ((opt == 1) && !(PINC & fire) && (PINC & minus))
		{
		mode = 2;
		PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (1<<bl);
		_delay_ms(t_pause);
		PORTD ^= (1<<ind1);
		PORTD |= (1<<buz);
		_delay_ms(t_pause);
		PORTD |= (1<<ind1);
		PORTD ^= (1<<buz);
		_delay_ms(t_pause);
		PORTD ^= (1<<ind1);
		_delay_ms(t_pause);
		PORTD |= (1<<ind1);
		_delay_ms(t_pause);
		PORTD ^= (1<<ind1);
		_delay_ms(t_pause);
		PORTD |= (1<<ind1);
		menu_toggle = 0;
		}


	//opt2- preheat
	if ((opt == 2) && !(PINC & fire) && (PINC & minus))
		{
		mode = 1;
		pha = 1;
		PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (1<<bl);
		_delay_ms(t_pause);
		PORTD ^= (1<<ind2);
		PORTD |= (1<<buz);
		_delay_ms(t_pause);
		PORTD |= (1<<ind2);
		PORTD ^= (1<<buz);
		_delay_ms(t_pause);
		PORTD ^= (1<<ind2);
		_delay_ms(t_pause);
		PORTD |= (1<<ind2);
		_delay_ms(t_pause);
		PORTD ^= (1<<ind2);
		_delay_ms(t_pause);
		PORTD |= (1<<ind2);

		//adjust ph power
		while(pha == 1)
			{
			if ( phd < ds1 ) PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
			if ( phd >= ds1 ) PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
			if ( phd >= ds2 ) PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
			if ( phd >= ds3 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (1<<bl);
			if ( phd >= ds4 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);

			if(!(PINC & fire))
				{
				pha = 0;
				tpha = 1;
				PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (1<<bl);
				_delay_ms(t_pause);
				PORTD ^= (1<<ind2);
				PORTD |= (1<<buz);
				_delay_ms(t_pause);
				PORTD |= (1<<ind2);
				PORTD ^= (1<<buz);
				_delay_ms(t_pause);
				PORTD ^= (1<<ind2);
				_delay_ms(t_pause);
				PORTD |= (1<<ind2);
				_delay_ms(t_pause);
				PORTD ^= (1<<ind2);
				_delay_ms(t_pause);
				PORTD |= (1<<ind2);
				}

			if(!(PINC & plus) && (phd < 255 - phstep))
				{
				phd = phd + step;
				_delay_ms(t_adj_f);
				}

			if(!(PINC & minus) && (phd > 0 + phstep))
				{
				phd = phd - step;
				_delay_ms(t_adj_f);
				}
			}
		//adjust preheat time
		while(tpha == 1)
			{
			if ( tph > 4 ) tph = 1;

			if ( tph == 1 ) PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
			if ( tph == 2 ) PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
			if ( tph == 3 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (1<<bl);
			if ( tph == 4 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);

			if(!(PINC & fire))
				{
				tpha = 0;
				menu_toggle = 0;
				PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (1<<bl);
				_delay_ms(t_pause);
				PORTD ^= (1<<ind2);
				PORTD |= (1<<buz);
				_delay_ms(t_pause);
				PORTD |= (1<<ind2);
				PORTD ^= (1<<buz);
				_delay_ms(t_pause);
				PORTD ^= (1<<ind2);
				_delay_ms(t_pause);
				PORTD |= (1<<ind2);
				_delay_ms(t_pause);
				PORTD ^= (1<<ind2);
				_delay_ms(t_pause);
				PORTD |= (1<<ind2);
				}

			if(!(PINC & plus) && (tph < 4 ))
				{
				tph = tph + 1;
				_delay_ms(t_press);
				}

			if(!(PINC & minus) && (tph > 1 ))
				{
				tph = tph - 1;
				_delay_ms(t_press);
				}
			}
		}

//opt3 normal
	if ((opt == 3) && !(PINC & fire) && (PINC & minus))
		{
		mode = 0;
		PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (1<<bl);
		_delay_ms(t_pause);
		PORTD ^= (1<<ind3);
		PORTD |= (1<<buz);
		_delay_ms(t_pause);
		PORTD |= (1<<ind3);
		PORTD ^= (1<<buz);
		_delay_ms(t_pause);
		PORTD ^= (1<<ind3);
		_delay_ms(t_pause);
		PORTD |= (1<<ind3);
		_delay_ms(t_pause);
		PORTD ^= (1<<ind3);
		_delay_ms(t_pause);
		PORTD |= (1<<ind3);
		menu_toggle = 0;
		}

	//opt4 lock
	if ((opt == 4) && !(PINC & fire) && (PINC & minus) && (lock == 0))
		{
		lock = 1;
		if (lock == 1)
			{
			PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
			_delay_ms(t_splash);
			PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
			}
		menu_toggle = 0;
		}
//menu exit
	if (!(PINC & fire) && !(PINC & minus) && (PINC & plus) && (menu_toggle == 1))
		{
		menu_toggle = 0;
		PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_splash);
		PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_splash);
		PORTD = (0<<ind1) | (0<<ind2) | (1<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_splash);
		PORTD = (0<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_splash);
		PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_splash);
		PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_splash);
		PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_splash);
		PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_splash);
		PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_splash);
		}
	}

// setting duty

//plus
while(!(PINC & plus) && (PINC & minus) && (PINC & fire) && (duty < 255 - step) && (mode != 2))
	{
	PORTD |= (1<<warm);
	duty = duty + step;
	//slow mode
	if(counter < adj_counter)
		{
		_delay_ms(t_adj_s);
		PORTD ^= (1<<warm);
		_delay_ms(t_adj_s);

		if ( duty < ds1 ) PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds1 ) PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds2 ) PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds3 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds4 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);
		}
	//fast mode
	if(counter >= adj_counter)
		{
		_delay_ms(t_adj_f);
		PORTD |= (1<<warm);

		if ( duty < ds1 ) PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds1 ) PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds2 ) PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds3 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds4 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);
		}

	counter = counter + 1;
	}

//minus
while(!(PINC & minus) && (PINC & plus) && (PINC & fire) && (duty > 0 + step) && (mode != 2))
	{
	PORTD |= (1<<warm);
	duty = duty - step;


	if(counter < adj_counter)
		{
		_delay_ms(t_adj_s);
		PORTD ^= (1<<warm);
		_delay_ms(t_adj_s);

		if ( duty < ds1 ) PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds1 ) PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds2 ) PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds3 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds4 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);
		}

	if(counter >= adj_counter)
		{
		_delay_ms(t_adj_f);
		PORTD |= (1<<warm);

		if ( duty < ds1 ) PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds1 ) PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds2 ) PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds3 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (1<<bl);
		if ( duty >= ds4 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);
		}

	counter = counter + 1;
}

//Show Voltage
if(!(PINC & minus) && !(PINC & plus)) //minus and plus
	{
	ADMUX |= (0<<MUX2); ADMUX |= (1<<MUX1); ADMUX |= (1<<MUX0); //adc3
	ADCSRA |= (1<<ADSC); //start
	while(ADCSRA & (1<<ADSC));
	voltage = ADC * 5;

	//check animation
	PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (1<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (0<<ind1) | (1<<ind2) | (0<<ind3) | (0<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (0<<ind1) | (0<<ind2) | (1<<ind3) | (0<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (1<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (0<<buz) | (1<<bl);
	_delay_ms(t_splash);
	PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (0<<buz) | (1<<bl);

	_delay_ms(t_pause);

	//voltage indicator
	if (voltage < vs1) PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
	if (voltage >= vs1) PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
	if (voltage >= vs2) PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
	if (voltage >= vs3) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (1<<bl);
	if (voltage >= vs4) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);

	_delay_ms(v_display);
	PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) | (0<<ind4) | (1<<bl);
	_delay_ms(t_pause);
	}

//power indicator

if (mode == 2) PORTD = (0<<warm) | (0<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4);

if ((duty < ds1) && (mode != 2) && (lock == 0)) PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
if ((duty >= ds1) && (mode != 2) && (lock == 0)) PORTD = (0<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
if ((duty >= ds2) && (mode != 2) && (lock == 0)) PORTD = (0<<warm) | (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
if ((duty >= ds3) && (mode != 2) && (lock == 0)) PORTD = (0<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (1<<bl);
if ((duty >= ds4) && (mode != 2) && (lock == 0)) PORTD = (0<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);

//warming
while(!(PINC & fire) && (PINC & minus) && (PINC & plus))
	{
	//pwm on
	TCCR1A |= (1<<COM1A1);

	//mechanical warming
	while(!(PINC & fire) && (mode == 2) && (PINC & minus) && (PINC & plus))
		{
		OCR1A = 255;
		PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);
		}

	//save duty
	eeprom_write_byte(&d, duty);
	_delay_ms(20);

	//save preheat duty
	eeprom_write_byte(&p, phd);
	_delay_ms(20);

	//save preheat time
	eeprom_write_byte(&t, tph);
	_delay_ms(20);

	//check voltage decrease
		//idle Voltage
	OCR1A = 0;
	_delay_us(v_check);
	ADMUX ^= (1<<MUX2);
ADMUX |= (1<<MUX1);
ADMUX |= (1<<MUX0); //adc3
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
	i_v = ADC * 5;
	OCR1A = 0;

	//full load voltage
	OCR1A = 255;
	_delay_us(v_check);
	ADMUX ^= (1<<MUX2);
ADMUX |= (1<<MUX1);
ADMUX |= (1<<MUX0); //adc3
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
	fl_v = ADC * 5;
	OCR1A = 0;

		//decrease voltage
	v_dcr = i_v - fl_v;

	//actual load
	OCR1A = duty;
	_delay_us(v_check);
	ADMUX ^= (1<<MUX2);
ADMUX |= (1<<MUX1);
ADMUX |= (1<<MUX0); //adc3
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
	l_v = ADC * 5;
	OCR1A = 0;

	//anti-short protection
	while(v_dcr >= max_v_dcr && !(PINC & fire) && (PINC & minus) && (PINC & plus) && (mode != 2))
		{
		TCCR1A ^= (1<<COM1A1);
		OCR1A = 0;
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);

		OCR1A = 0;
		ADMUX ^= (1<<MUX2);
		ADMUX |= (1<<MUX1);
		ADMUX |= (1<<MUX0); //adc3
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & (1<<ADSC));
		i_v = ADC * 5;
		OCR1A = 0;

		OCR1A = 255;
		_delay_us(v_check);
		ADMUX ^= (1<<MUX2);
ADMUX |= (1<<MUX1);
ADMUX |= (1<<MUX0); //adc3
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & (1<<ADSC));
		fl_v = ADC * 5;
		OCR1A = 0;

		v_dcr = i_v - fl_v;
		TCCR1A |= (1<<COM1A1);

		}

	//no atomizer
	while(v_dcr < min_v_dcr && !(PINC & fire) && (PINC & minus) && (PINC & plus) && (mode != 2))
	 	 {
		OCR1A = 0;
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);

		OCR1A = 0;
		ADMUX ^= (1<<MUX2);
ADMUX |= (1<<MUX1);
ADMUX |= (1<<MUX0); //adc3
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & (1<<ADSC));
		i_v = ADC * 5;
		OCR1A = 0;

		OCR1A = 255;
		ADMUX ^= (1<<MUX2);
ADMUX |= (1<<MUX1);
ADMUX |= (1<<MUX0); //adc3
		_delay_us(v_check);
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & (1<<ADSC));
		fl_v = ADC * 5;
		OCR1A = 0;

		v_dcr = i_v - fl_v;
	 	 }

	//low battery
	while(l_v < min_l_v && !(PINC & fire) && (PINC & minus) && (PINC & plus) && (mode != 2))
		{
		OCR1A = 0;
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (0<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<buz) | (1<<bl);
		_delay_ms(t_blink);
		PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<buz) | (1<<bl);
		_delay_ms(t_blink);

		OCR1A = duty;
		_delay_us(v_check);
		ADMUX ^= (1<<MUX2);
ADMUX |= (1<<MUX1);
ADMUX |= (1<<MUX0); //adc3
		ADCSRA |= (1<<ADSC); //start
		while(ADCSRA & (1<<ADSC));
		l_v = ADC * 5;
		OCR1A = 0;
		}

	//normal warming
	while(v_dcr <= max_v_dcr && l_v >= min_l_v && !(PINC & fire) && (mode == 0) && (PINC & minus) && (PINC & plus))
		{
		OCR1A = duty;

		/*
		//amperometer- under construction
		ampcount = 0;

		// check bat voltage
		while(ampcount <= ampchecks)
			{
			ADMUX ^= (1<<MUX2);
			ADMUX |= (1<<MUX1);
			ADMUX |= (1<<MUX0); //adc3
			ADCSRA |= (1<<ADSC);
			while(ADCSRA & (1<<ADSC));
			b_voltage = ADC * 5;

		// check pin voltage
			ADMUX |= (1<<MUX2);
			ADMUX ^= (1<<MUX1);
			ADMUX ^= (1<<MUX0); //adc4(conn pin)
			ADCSRA |= (1<<ADSC);
			while(ADCSRA & (1<<ADSC));
			p_voltage = ADC * 5;

			decrease = b_voltage - p_voltage;
			amperage = decrease / 5;

			if ( amperage < amp1 ) PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<bl);
			if ( amperage >= amp1 ) PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<bl);
			if ( amperage >= amp2 ) PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (0<<bl);
			if ( amperage >= amp3 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (0<<bl);
			if ( amperage >= amp4 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (0<<bl);

			_delay_us(t_ampcheck);

			ampcount = ampcount + 1;
			}

		ampcount = 0;

		*/

		//amperometer2

		max_amp = v_dcr / 40;
		multiplier = duty * 6;
		amperage = max_amp * multiplier;



		if ( amperage < amp1 ) PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<bl);
		if ( amperage >= amp1 ) PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (0<<bl);
		if ( amperage >= amp2 ) PORTD = (0<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (0<<bl);
		if ( amperage >= amp3 ) PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (0<<bl);
		if ( amperage >= amp4 ) PORTD = (0<<ind1) | (0<<ind2) | (1<<ind3) |(0<<ind4) | (0<<bl);
		if ( amperage >= amp5 ) PORTD = (1<<ind1) | (0<<ind2) | (1<<ind3) |(0<<ind4) | (0<<bl);
		if ( amperage >= amp6 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (0<<bl);
		if ( amperage >= amp7 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(0<<ind4) | (0<<bl);
		if ( amperage >= amp8 ) PORTD = (0<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (0<<bl);
		if ( amperage >= amp9 ) PORTD = (1<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (0<<bl);
		if ( amperage >= amp10 ) PORTD = (0<<ind1) | (1<<ind2) | (0<<ind3) |(1<<ind4) | (0<<bl);
		if ( amperage >= amp11 ) PORTD = (1<<ind1) | (1<<ind2) | (0<<ind3) |(1<<ind4) | (0<<bl);
		if ( amperage >= amp12 ) PORTD = (0<<ind1) | (0<<ind2) | (1<<ind3) |(1<<ind4) | (0<<bl);
		if ( amperage >= amp13 ) PORTD = (1<<ind1) | (0<<ind2) | (1<<ind3) |(1<<ind4) | (0<<bl);
		if ( amperage >= amp14 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (0<<bl);
		if ( amperage >= amp15 ) PORTD = (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (0<<bl);

		_delay_ms(t_warm2);

		ADMUX ^= (1<<MUX2);
		ADMUX |= (1<<MUX1);
		ADMUX |= (1<<MUX0); //adc3
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & (1<<ADSC));
		l_v = ADC * 5;

		OCR1A = 0;
		_delay_us(v_check);
		ADMUX ^= (1<<MUX2);
		ADMUX |= (1<<MUX1);
		ADMUX |= (1<<MUX0); //adc3
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & (1<<ADSC));
		i_v = ADC * 5;
		OCR1A = 0;

		OCR1A = 255;
		_delay_us(v_check);
		ADMUX ^= (1<<MUX2);
		ADMUX |= (1<<MUX1);
		ADMUX |= (1<<MUX0); //adc3
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & (1<<ADSC));
		fl_v = ADC * 5;
		OCR1A = 0;

		v_dcr = i_v - fl_v;
		}

	//preheat warming
	while(v_dcr <= max_v_dcr && l_v >= min_l_v && !(PINC & fire) && (mode == 1) && (PINC & minus) && (PINC & plus))
		{
		OCR1A = phd;
		PORTD = (1<<warm) | (1<<ind1) | (1<<ind2) | (1<<ind3) |(1<<ind4) | (1<<bl);
		if (tph == 1) _delay_ms(tph1);
		if (tph == 2) _delay_ms(tph2);
		if (tph == 3) _delay_ms(tph3);
		if (tph == 4) _delay_ms(tph4);


		while(v_dcr <= max_v_dcr && l_v >= min_l_v && !(PINC & fire) && (mode == 1))
			{
			OCR1A = duty;

			//warming stripe
			PORTD = (1<<warm) | (1<<ind1) | (0<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
			_delay_ms(t_warm);
			PORTD = (1<<warm) | (0<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
			_delay_ms(t_warm);
			PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (1<<ind3) |(0<<ind4) | (1<<bl);
			_delay_ms(t_warm);
			PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (0<<ind3) |(1<<ind4) | (1<<bl);
			_delay_ms(t_warm);
			PORTD = (1<<warm) | (0<<ind1) | (0<<ind2) | (1<<ind3) |(0<<ind4) | (1<<bl);
			_delay_ms(t_warm);
			PORTD = (1<<warm) | (0<<ind1) | (1<<ind2) | (0<<ind3) |(0<<ind4) | (1<<bl);
			_delay_ms(t_warm);

			ADCSRA |= (1<<ADSC);
			ADMUX ^= (1<<MUX2);
			ADMUX |= (1<<MUX1);
			ADMUX |= (1<<MUX0); //adc3
			while(ADCSRA & (1<<ADSC));
			l_v = ADC * 5;

			OCR1A = 0;
			_delay_us(v_check);
			ADMUX ^= (1<<MUX2);
			ADMUX |= (1<<MUX1);
			ADMUX |= (1<<MUX0); //adc3
			ADCSRA |= (1<<ADSC);
			while(ADCSRA & (1<<ADSC));
			i_v = ADC * 5;
			OCR1A = 0;

			OCR1A = 255;
			_delay_us(v_check);
			ADMUX ^= (1<<MUX2);
			ADMUX |= (1<<MUX1);
			ADMUX |= (1<<MUX0); //adc3
			ADCSRA |= (1<<ADSC);
			while(ADCSRA & (1<<ADSC));
			fl_v = ADC * 5;
			OCR1A = 0;

			v_dcr = i_v - fl_v;
			}// afterheat close
		}//prehat warming close

	//pwm off
	TCCR1A ^= (1<<COM1A1);

	}//warming close
	OCR1A = 0;
}//1 close
}//uC close
