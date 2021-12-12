/*
 * GreenHouse.c
 *
 * Created: 5. 12. 2021 0:47:37
 * Author : xsedla1n
 */ 

/* Defines -----------------------------------------------------------*/
#define CSMS_PIN PC0		// Capacitive soil moisture sensor pin
#define VENT_PIN PD3		// Ventilation relay pin
#define SPRNKL_PIN PD2		// Sprinkler relay pin
#define BULB_PIN PB2		// Light relay pin

#ifndef F_CPU
# define F_CPU 16000000  // CPU frequency in Hz required for UART_BAUD_SELECT
#endif

/* Includes ----------------------------------------------------------*/
#include <avr/io.h>         // AVR device-specific IO definitions
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
#include <stdlib.h>         // C library. Needed for conversion function
#include <string.h>			// C library. Needed for string connections
#include <util/delay.h>     // Functions for busy-wait delay loops
#include "gpio.h"			// GPIO library for AVR-GCC
#include "uart.h"           // Peter Fleury's UART library
#include "timer.h"          // Timer library for AVR-GCC
#include "twi.h"            // TWI library for AVR-GCC
#include "lcd.h"            // Peter Fleury's LCD library

/* Variables ---------------------------------------------------------*/
typedef enum {              // FSM declaration
	STATE_IDLE = 1,
	STATE_GET_TEMP,
	STATE_TOGGLE_VENT,
	STATE_GET_MOIST,
	STATE_TOGGLE_SPRNKL,
	STATE_GET_TIME,
	STATE_GET_LIGHT,
	STATE_TOGGLE_BULB
} state_t;

uint16_t adc_moist = 0;		// ADC Soil Moisture level value
uint16_t adc_light = 400;		// ADC Light level value

// Custom character definition
uint32_t customChar[24] = {
	0b00000, // First line of the humidity character
	0b00100,
	0b01110,
	0b11111,
	0b11111,
	0b11111,
	0b01110,
	0b00000, // Last line of the humidity character
	0b00100, // First line of the temperature character
	0b01010,
	0b01110,
	0b01110,
	0b01110,
	0b10101,
	0b11111,
	0b01110,  // Last line of the temperature character
	0b01110,  // First line of the light character
	0b10001,
	0b10001,
	0b10101,
	0b10101,
	0b01110,
	0b01110,
	0b00100   // Last line of the light character
};

int main(void)
{	
	// Configure pins
	GPIO_config_output(&DDRD, VENT_PIN);
	GPIO_config_output(&DDRD, SPRNKL_PIN);
	GPIO_config_output(&DDRB, BULB_PIN);
	GPIO_write_low(&PORTD, VENT_PIN);
	GPIO_write_low(&PORTD, SPRNKL_PIN);
	GPIO_write_low(&PORTB, BULB_PIN);
	
	// LCD Initialization
	lcd_init(LCD_DISP_ON);
	
    // Initialize I2C (TWI)
    twi_init();

	// Initialize RTC time
	if (twi_start((0x68<<1) + TWI_WRITE) == 0) {	// device is accessible
		twi_write(0x00);							// sending message to slave
		twi_write(0b00000000);
		twi_write(0b01000101);
		twi_write(0b00000010);
		twi_write(0b00000000);
		twi_write(0b00000000);
		twi_write(0b00000000);
		twi_write(0b00000000);
		twi_write(0b00000000);
	}

	// Import customChar matrix into a LCD memory
    lcd_command(1 << LCD_CGRAM); // Set pointer to beginning of CGRAM memory
    for (uint8_t i = 0; i < 24; i++)
    {
	    // Store all new chars to memory line by line
	    lcd_data(customChar[i]);
    }
    lcd_command(1 << LCD_DDRAM); // Set DDRAM address
	
	// Create basic layout on the LCD screen
    lcd_gotoxy(0, 0);
    lcd_puts("00:00:00");
    lcd_gotoxy(10, 0);
    lcd_putc(1);		// Display thermometer character
	lcd_putc('0');
	lcd_putc(0xdf);
	lcd_putc('C');
    lcd_gotoxy(0, 1);
    lcd_putc(0);		// Display moisture character
    lcd_gotoxy(10, 1);
    lcd_putc(2);		// Display light level character
	lcd_gotoxy(14, 1);
	
	// Setup an ADC conversion
	ADCSRA |= (1<<ADEN);	// Enable ADC module
	ADCSRA |= (1<<ADPS0); ADCSRA |= (1<<ADPS1); ADCSRA |= (1<<ADPS2);	// Set clock prescaler to 128
	
	// Initialize UART to asynchronous, 8N1, 9600
	uart_init(UART_BAUD_SELECT(9600, F_CPU));
	uart_puts("UART Enabled.\r\n");
	
    // Configure 8-bit Timer/Counter0 for Scan cycle
    // Set the overflow prescaler to 4 sec and enable interrupt
    TIM1_overflow_33ms();
    TIM1_overflow_interrupt_enable();
	
	// Enable interrupts by setting the global interrupt mask
	sei();

    // Infinite loop
    while (1) 
    {
        /* Empty loop. All subsequent operations are performed exclusively 
         * inside interrupt service routines ISRs */
    }
	
    // Function will never reach this point
    return 0;
}

unsigned int readADC0()
{
	ADMUX |= (1<<REFS0); ADMUX &= ~(1<<REFS1);	// Set ADC reference to AVcc
	ADMUX &= ~(1<<MUX0); ADMUX &= ~(1<<MUX1); ADMUX &= ~(1<<MUX2); ADMUX &= ~(1<<MUX3); // Set input channel to ADC0
	ADMUX |= (0 << ADLAR);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

unsigned int readADC1()
{
	ADMUX |= (1<<REFS0); ADMUX &= ~(1<<REFS1);	// Set ADC reference to AVcc
	ADMUX |= (1<<MUX0); ADMUX &= ~(1<<MUX1); ADMUX &= ~(1<<MUX2); ADMUX &= ~(1<<MUX3); // Set input channel to ADC1
	ADMUX |= (0 << ADLAR);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

ISR(TIMER1_OVF_vect)
{
	// Base Variables
	static state_t state = STATE_IDLE;
	static uint16_t counter = 455;
	static uint8_t err = 0;
	
	// DHT12 Variables
	static uint8_t temperature = 0;
	char temperature1[] = "";
	char temperature2[] = "";
	
	// RTC DS1307 variables
	static uint8_t seconds = 0;
	static uint8_t seconds_1 = 0;	// first digit of seconds
	static uint8_t seconds_2 = 0;	// second digit of seconds
	static uint8_t minutes = 0;
	static uint8_t minutes_1 = 0;	// first digit of minutes
	static uint8_t minutes_2 = 0;	// second digit of minutes
	static uint8_t hours = 0;
	static uint8_t hours_1 = 0;		// first digit of hours
	static uint8_t hours_2 = 0;		// second digit of hours
	char lcd_string[8] = " ";      // String for converting numbers by itoa()
	
	// ADC variables
	uint16_t air_val = 920;	// Needs to be calibrated
	uint16_t water_val = 760;	// Needs to be calibrated
	uint16_t day_val = 100;	// Needs to be calibrated
	static uint16_t raw_value = 0;
	char temp_str[3] = "";
	
	switch(state) {
		
	case STATE_IDLE:
		if (counter >= 455) { // 33ms x 455 = 15s (approx.)
			counter = 0;
			state = STATE_GET_TEMP;
			uart_puts("[INFO] Get values.\r\n");
		}
		else if (counter % 6 == 0) { // 33ms x 6 = 200ms (approx.)
			counter++;
			state = STATE_GET_TIME;
		}
		else {
			counter++;
		}
		break;
		
	case STATE_GET_TEMP:
		err = twi_start((0x5c<<1) + TWI_WRITE);
		if (err == 0) {
			twi_write(0x2);
		}
				
		err = twi_start((0x5c<<1) + TWI_READ);
		if (err == 0) {
			// Create a temperature string
			// Get first part of the temperature information
			temperature = twi_read_ack();
			itoa(temperature, temperature1, 10);
			// Get second part of the temperature information
			temperature = twi_read_nack();
			itoa(temperature, temperature2, 10);
			strcat(temperature1, ".");
			strcat(temperature1, temperature2);
			
			// Display temperature via UART
			// uart_puts("Temperature: ");
			// uart_puts(temperature1);
			// uart_puts("\r\n");
			
			// Update LCD display ("xx,x°C")
			lcd_gotoxy(11, 0);
			lcd_puts(temperature1);
			// lcd_gotoxy(15, 0);
			// lcd_puts("°C");
			
			// Continue to another state
			temperature = atoi(temperature1);
		}
		else {
			uart_puts("Temperature unavailable.\r\n");
		}
		state = STATE_GET_MOIST;
		break;
		
	case STATE_TOGGLE_VENT:
		if (temperature > 25) {
			GPIO_write_high(&PORTD, VENT_PIN);
		}
		else {
			GPIO_write_low(&PORTD, VENT_PIN);
		}
		state = STATE_TOGGLE_SPRNKL;
		break;
		
	case STATE_GET_MOIST:
		raw_value = readADC0();

		// Get moisture value in %
		if (raw_value > air_val) {
			raw_value = air_val;		// 960
		}
		else if (raw_value < water_val) {
			raw_value = water_val;		// 720
		}
		
		raw_value = round((100/(float)(air_val-water_val)) * (raw_value-water_val)); // PREROBIT
		itoa(raw_value, temp_str, 10);
		adc_moist = raw_value;
		
		// Update the LCD
		lcd_gotoxy(1, 1);
		lcd_puts("   ");
		lcd_gotoxy(1, 1);
		lcd_puts(temp_str);
		lcd_gotoxy(4, 1);
		lcd_puts("%");
		state = STATE_GET_TIME;
		break;
		
	case STATE_TOGGLE_SPRNKL:
		if (adc_moist > 80) {
			GPIO_write_high(&PORTD, SPRNKL_PIN);
		}
		else {
			GPIO_write_low(&PORTD, SPRNKL_PIN);
		}
		state = STATE_IDLE;
		break;
	
	case STATE_GET_TIME:
		err = twi_start((0x68<<1) + TWI_WRITE);		// starting communication with slave
		if (err == 0){								// device is accessible
			twi_write(0x00);						// sending message to slave
		}
	
		err = twi_start((0x68<<1) + TWI_READ);		// starting communication with master
		if (err == 0){
			lcd_gotoxy(0, 0);
			lcd_puts("00:00:00");
		
			seconds = twi_read_ack();
			seconds_1 = (seconds & 0b00001111);			// getting first digit of seconds
			seconds_2 = ((seconds >> 4) & 0b00000111);		// getting second digit of seconds
			itoa(seconds_1, lcd_string, 10);
			lcd_gotoxy(7, 0);
			lcd_puts(lcd_string);
			itoa(seconds_2, lcd_string, 10);
			lcd_gotoxy(6, 0);
			lcd_puts(lcd_string);
		
			minutes = twi_read_ack();
			minutes_1 = minutes & 0b00001111;
			minutes_2 = minutes >> 4 & 0b00000111;
			itoa(minutes_1, lcd_string, 10);
			lcd_gotoxy(4, 0);
			lcd_puts(lcd_string);
			itoa(minutes_2, lcd_string, 10);
			lcd_gotoxy(3, 0);
			lcd_puts(lcd_string);
		
			hours = twi_read_ack();
			hours_1 = hours & 0b00001111;
			hours_2 = hours >> 4 & 0b00000011;
			itoa(hours_1, lcd_string, 10);
			lcd_gotoxy(1, 0);
			lcd_puts(lcd_string);
			itoa(hours_2, lcd_string, 10);
			lcd_gotoxy(0, 0);
			lcd_puts(lcd_string);
		}
		else {
			uart_puts("Device not found.\r\n");
		}
		
		if (counter == 0) {
			state = STATE_GET_LIGHT;
		}
		else {
			state = STATE_IDLE;
		}
		break;
		
	case STATE_GET_LIGHT:
		raw_value = readADC1();
		
		if (raw_value > day_val) {
			raw_value = day_val;
		}
		
		itoa(raw_value, temp_str, 10);
		adc_light = raw_value;
		
		// Update the LCD
		lcd_gotoxy(11, 1);
		lcd_puts("   ");
		lcd_gotoxy(11, 1);
		lcd_puts(temp_str);
		lcd_gotoxy(14, 1);
		lcd_puts("%");
		state = STATE_TOGGLE_BULB;
		break;
		
	case STATE_TOGGLE_BULB:
		if (adc_light < 60) {
			GPIO_write_high(&PORTB, BULB_PIN);
		}
		else {
			GPIO_write_low(&PORTB, BULB_PIN);
		}
		state = STATE_TOGGLE_VENT;
		break;
	
	default:
		state = STATE_IDLE;
		break;
	}
	// Else do nothing and exit the ISR
}

/*
ISR(ADC_vect)
{
	uint16_t air_val = 920;	// Needs to be calibrated
	uint16_t water_val = 760;	// Needs to be calibrated
	uint16_t day_val = 100;	// Needs to be calibrated
	static uint16_t raw_value = 0;
	char temp_str[3] = "";
		
	switch((ADMUX & 0b00001111)) {
		
		case (0b00000000):
			// Get raw value and display it on UART
			raw_value = ADC;

			// Get moisture value in %
			if (raw_value > air_val) {
				raw_value = air_val;
			}
			else if (raw_value < water_val) {
				raw_value = water_val;
			}
		
			raw_value = round((100/(float)(air_val-water_val)) * (raw_value-water_val));
			itoa(raw_value, temp_str, 10);
			adc_moist = raw_value;
		
			// Update the LCD
			lcd_gotoxy(1, 1);
			lcd_puts("   ");
			lcd_gotoxy(1, 1);
			lcd_puts(temp_str);
			lcd_gotoxy(4, 1);
			lcd_puts("%");
			break;
		
		case (0b00000001):
			// Get raw value and display it on UART
			raw_value = ADC;
		
			if (raw_value > day_val) {
				raw_value = day_val;
			}
		
			itoa(raw_value, temp_str, 10);
			adc_light = raw_value;
		
			// Update the LCD
			lcd_gotoxy(11, 1);
			lcd_puts("   ");
			lcd_gotoxy(11, 1);
			lcd_puts(temp_str);
			lcd_gotoxy(14, 1);
			lcd_puts("%");
			break;
	}	
}
*/