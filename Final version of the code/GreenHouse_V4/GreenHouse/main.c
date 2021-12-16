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
	if (twi_start((0x68<<1) + TWI_WRITE) == 0) {	// Device is accessible
		twi_write(0x00);							// Sending message to slave
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
	ADCSRA |= (1 << ADSC); // Start conversion
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

unsigned int readADC1()
{
	ADMUX |= (1<<REFS0); ADMUX &= ~(1<<REFS1);	// Set ADC reference to AVcc
	ADMUX |= (1<<MUX0); ADMUX &= ~(1<<MUX1); ADMUX &= ~(1<<MUX2); ADMUX &= ~(1<<MUX3); // Set input channel to ADC1
	ADMUX |= (0 << ADLAR);
	ADCSRA |= (1 << ADSC); // Start conversion
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
	static uint8_t seconds_1 = 0;	// First digit of seconds
	static uint8_t seconds_2 = 0;	// Second digit of seconds
	static uint8_t minutes = 0;
	static uint8_t minutes_1 = 0;	// First digit of minutes
	static uint8_t minutes_2 = 0;	// Second digit of minutes
	static uint8_t hours = 0;
	static uint8_t hours_1 = 0;		// First digit of hours
	static uint8_t hours_2 = 0;		// Second digit of hours
	char lcd_string[8] = " ";      // String for converting numbers by itoa()
	
	// ADC variables
	uint16_t air_val = 920;	// Needs to be calibrated with each device
	uint16_t water_val = 760;	// Needs to be calibrated with each device
	uint16_t day_val = 100;	// Needs to be calibrated with each device
	static uint16_t raw_value = 0;
	char temp_str[3] = "";
	
	/**********************************************************************
	 * Switch statement
	 * Purpose: Functions as a state machine. FSM has 8 states in total. 
	 * STATE_IDLE: Add counter 
	 * STATE_GET_TEMP: Measures temperature and updates LCD display.
	 * STATE_STATE_GET_MOIST: Measures soil humidity and updates LCD display.
	 * STATE_STATE_GET_TIME: Takes time from clock and displays it on LCD.
	 * STATE_GET_LIGHT: Measures luminance and updates it on LCD display.
	 * STATE_TOGGLE_BULB: Turns on lights when it's too dark.
	 * STATE_TOGGLE_SPRNKL: Turns on watering when the soil moisture is too low.
	 * STATE_TOGGLE_VENT: Turn on ventilator when temperature is too high.
	 **********************************************************************/
	switch(state) {
	
	case STATE_IDLE:
		if (counter >= 455) { // 33ms x 455 = 15s (approx.)
			counter = 0;
			state = STATE_GET_TEMP;
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
		err = twi_start((0x5c<<1) + TWI_WRITE);	// Send address byte to slave device
		if (err == 0) {
			twi_write(0x2); // Sending the data
		}
		else {
			// Debug check
			uart_puts("Device not found.\r\n"); 
		}
				
		err = twi_start((0x5c<<1) + TWI_READ); // Send address byte to master
		if (err == 0) {
			// Create a temperature string
			// Get first part of the temperature information (integer part)
			temperature = twi_read_ack();	   // 
			itoa(temperature, temperature1, 10);		// Convert integer temperature to decimal values
			
			// Get second part of the temperature information (fractional part)
			temperature = twi_read_nack();
			itoa(temperature, temperature2, 10);		// Convert fractional temperature to decimal values
			
			// Display temperature via UART
			// uart_puts("Temperature: ");
			// uart_puts(temperature1);
			// uart_puts(".");
			// uart_puts(temperature2);
			// uart_puts("\r\n");
			
			// Update LCD display ("xx,x°C")
			lcd_gotoxy(11, 0);
			lcd_puts(temperature1);
			lcd_puts(".");
			lcd_puts(temperature2);
			lcd_gotoxy(15, 0);
			lcd_putc(0xdf);
			lcd_putc('C');
			
			// Get integer from a string
			temperature = atoi(temperature1);
		}
		else {
			// Debug check
			// uart_puts("Temperature unavailable.\r\n");
		}
		
		state = STATE_GET_MOIST;
		break;
		
	case STATE_TOGGLE_VENT:
		if (temperature > 28) {					// After 28°C turn on the ventilator
			GPIO_write_high(&PORTD, VENT_PIN);	// Ventilator ON
			// Debug check
			uart_puts("Ventilation ON\r\n");	
		}
		else {
			GPIO_write_low(&PORTD, VENT_PIN);	// Ventilator OFF
		}
		
		state = STATE_TOGGLE_SPRNKL;			
		break;
		
	case STATE_GET_MOIST:
		raw_value = readADC0();

		// Get moisture value in %
		if (raw_value > air_val) {
			raw_value = air_val;		// 960 is the lowest moisture value from the device
		}
		else if (raw_value < water_val) {
			raw_value = water_val;		// 720 is the highest moisture value from the device
		}
		
		raw_value = round(100 * (1 - (float)(raw_value-water_val)/(float)(air_val-water_val))); // Getting moisture percentage value
		itoa(raw_value, temp_str, 10);
		adc_moist = raw_value;
		
		// Debug check
		uart_puts("Moisture value: ");
		uart_puts(temp_str);
		uart_puts("\r\n");
		
		// Update the moisture value on LCD
		lcd_gotoxy(1, 1);
		lcd_puts("   ");
		lcd_gotoxy(1, 1);
		lcd_puts(temp_str);
		lcd_gotoxy(4, 1);
		lcd_puts("%");
		
		state = STATE_GET_TIME;
		break;
		
	case STATE_TOGGLE_SPRNKL:
		if (adc_moist < 80) {			// Start watering when soil moisture is below 80 %
			GPIO_write_high(&PORTD, SPRNKL_PIN);	// Watering ON
			// Debug check
			uart_puts("Water ON\r\n");
		}
		else {
			GPIO_write_low(&PORTD, SPRNKL_PIN);		// Watering OFF
		}
		state = STATE_IDLE;
		break;
	
	case STATE_GET_TIME:
		err = twi_start((0x68<<1) + TWI_WRITE);		// Starting communication with slave
		if (err == 0){								// Device is accessible
			twi_write(0x00);						// Sending message to slave
		}
	
		err = twi_start((0x68<<1) + TWI_READ);		// Starting communication with master
		if (err == 0){
			lcd_gotoxy(0, 0);
			lcd_puts("00:00:00");
		
			seconds = twi_read_ack();
			seconds_1 = (seconds & 0b00001111);				// Getting first digit of seconds
			seconds_2 = ((seconds >> 4) & 0b00000111);		// Getting second digit of seconds
			itoa(seconds_1, lcd_string, 10);				// Converting to decimal values
			
			// Update seconds on LCD
			lcd_gotoxy(7, 0);
			lcd_puts(lcd_string);
			itoa(seconds_2, lcd_string, 10);
			lcd_gotoxy(6, 0);
			lcd_puts(lcd_string);
		
			minutes = twi_read_ack();
			minutes_1 = minutes & 0b00001111;				// Getting first digit of minutes
			minutes_2 = minutes >> 4 & 0b00000111;			// Getting second digit of minutes
			itoa(minutes_1, lcd_string, 10);				// Converting to decimal values
			
			// Update minutes on LCD
			lcd_gotoxy(4, 0);
			lcd_puts(lcd_string);
			itoa(minutes_2, lcd_string, 10);
			lcd_gotoxy(3, 0);
			lcd_puts(lcd_string);
		
			hours = twi_read_ack();
			hours_1 = hours & 0b00001111;					// Getting first digit of hours
			hours_2 = hours >> 4 & 0b00000011;				// Getting second digit of hours
			itoa(hours_1, lcd_string, 10);					// Converting to decimal values
			
			// Update hours on LCD
			lcd_gotoxy(1, 0);
			lcd_puts(lcd_string);
			itoa(hours_2, lcd_string, 10);
			lcd_gotoxy(0, 0);
			lcd_puts(lcd_string);
		}
		else {
			// Debug check
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
		// Debug check
		uart_puts("Light value: ");
		uart_puts(temp_str);
		uart_puts("\r\n");
		
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
			GPIO_write_high(&PORTB, BULB_PIN); // Turn lights ON
			// Debug check
			uart_puts("Light ON\r\n");
		}
		else {
			GPIO_write_low(&PORTB, BULB_PIN); // Turn lights OFF
		}
		state = STATE_TOGGLE_VENT;
		break;
	
	default:
		state = STATE_IDLE;
		break;
	}
	// Else do nothing and exit the ISR
}