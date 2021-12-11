/*
 * GreenHouse.c
 *
 * Created: 5. 12. 2021 0:47:37
 * Author : xsedla1n
 */ 

/* Defines -----------------------------------------------------------*/
#define CSMS_PIN PC0		// Capacitive soil moisture sensor pin

#ifndef F_CPU
# define F_CPU 16000000  // CPU frequency in Hz required for UART_BAUD_SELECT
#endif

/* Includes ----------------------------------------------------------*/
#include <avr/io.h>         // AVR device-specific IO definitions
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
#include <stdlib.h>         // C library. Needed for conversion function
#include "uart.h"           // Peter Fleury's UART library
#include "timer.h"          // Timer library for AVR-GCC
#include "twi.h"            // TWI library for AVR-GCC
#include "lcd.h"            // Peter Fleury's LCD library

/* Variables ---------------------------------------------------------*/
typedef enum {              // FSM declaration
	STATE_IDLE,
	STATE_CHECK_TEMP,
	STATE_TOGGLE_VENT,
	STATE_CHECK_MOIST,
	STATE_TOGGLE_SPRINKLER,
	STATE_CHECK_TIME,
	STATE_CHECK_LIGHT,
	STATE_TOGGLE_LIGHTS
} state_t;

// Custom character definition
uint16_t customChar[16] = {
	00000, // First line of the humidity character
	00100,
	01110,
	11111,
	11111,
	11111,
	01110,
	00000, // Last line of the humidity character
	00100, // First line of the temperature character
	01010,
	01110,
	01110,
	01110,
	10101,
	11111,
	01110  // Last line of the temperature character
};

int main(void)
{
	// LCD Initialization
	lcd_init(LCD_DISP_ON);

	// Import customChar matrix into a LCD memory
    lcd_command(1 << LCD_CGRAM); // Set pointer to beginning of CGRAM memory
    for (uint8_t i = 0; i < 16; i++)
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
	lcd_puts("0°C");
    lcd_gotoxy(0, 1);
    lcd_putc(0);		// Display moisture character
    lcd_gotoxy(10, 1);
    lcd_putc('x');		// Display light level character
	
	// Setup an ADC conversion
	// Configure ADC to convert PC0[A0] analog value
	ADMUX |= (1<<REFS0); ADMUX &= ~(1<<REFS1);	// Set ADC reference to AVcc
	ADMUX &= ~(1<<MUX0); ADMUX &= ~(1<<MUX1); ADMUX &= ~(1<<MUX2); ADMUX &= ~(1<<MUX3); // Set input channel to ADC0
	ADCSRA |= (1<<ADEN);	// Enable ADC module
	ADCSRA |= (1<<ADIE);	// Enable conversion complete interrupt
	ADCSRA |= (1<<ADPS0); ADCSRA |= (1<<ADPS1); ADCSRA |= (1<<ADPS2);	// Set clock prescaler to 128
	
    // Configure 8-bit Timer/Counter2 for Stopwatch
    // Set the overflow prescaler to 16 ms and enable interrupt
    TIM2_overflow_16ms();
    TIM2_overflow_interrupt_enable();
	
    // Configure 8-bit Timer/Counter0 for Scan cycle
    // Set the overflow prescaler to 4 sec and enable interrupt
    TIM0_overflow_4s();
    TIM0_overflow_interrupt_enable();
		
    // Infinite loop
    while (1) 
    {
        /* Empty loop. All subsequent operations are performed exclusively 
         * inside interrupt service routines ISRs */
    }
	
    // Function will never reach this point
    return 0;
}

ISR(TIMER2_OVF_vect)
{
	static uint8_t number_of_overflows = 0;
	static uint8_t seconds = 0;
	static uint8_t minutes = 0;
	static uint8_t hours = 0;
	char lcd_string[2] = "  ";      // String for converting numbers by itoa()

	number_of_overflows++;
	if (number_of_overflows >= 31)
	{
		// Do this every 31 x 16 ms = 496 ms (approx. 500 ms)
		number_of_overflows = 0;
		// Access the RTC and get: seconds, minutes & hours
		// Display acquired values
	}
	// Else do nothing and exit the ISR
}

ISR(TIMER0_OVF_vect)
{
	// Base Variables
	static uint8_t state = STATE_IDLE;
	static uint8_t response = 0;				// Response variable for I2C protocol
	static uint8_t number_of_overflows = 0;		// Counter divider integer
	char temp_string = "";						// Temporary string for converting numbers by itoa()
	
	// DHT12 Variables
	static uint8_t temperature = 0;
	char temperature_str = "";
	
	// Photosensor Variables
	static uint8_t light_level = 0;
	
	// Soil moisture sensor Variables
	static uint8_t soil_humidity = 0;

	number_of_overflows++;
	// Do this every 4 x 4 sec = 16 sec
	if (number_of_overflows >= 4)
	{
		number_of_overflows = 0;
		// Access the other components and get: air temperature, soil moisture, light level, time of the day.
		switch(state) {
			case STATE_IDLE:
			state = STATE_CHECK_TEMP;
			break;
			
			case STATE_CHECK_TEMP:
			response = twi_start((0x5c<<1) + TWI_WRITE);
			if (response == 0) {
				uart_puts("DHT12 is responding for writing.\r\n");
				twi_write(0x02);
			}
			response = twi_start((0x5c<<1) + TWI_READ);
			if (response == 0) {
				// UART response message
				uart_puts("DHT12 is responding for reading temperature.\r\n");
				
				// Create a temperature string
				// Get first part of the temperature information
				temperature = twi_read_ack();
				itoa(temperature, temp_string, 10);
				temperature_str = temperature;
				// Get second part of the temperature information
				temperature = twi_read_nack();
				itoa(temperature, temp_string, 10);
				temperature_str = temperature_str + (".%s", temperature);
				
				// Display temperature via UART
				uart_puts("Temperature: ");
				uart_puts(temperature_str);
				uart_puts("\r\n");
				
				// Update LCD display ("xx,x°C")
				lcd_gotoxy(11, 0);
				lcd_puts(temperature_str);
				lcd_gotoxy(15, 0);
				lcd_puts("°C");
				
				// Continue to another state
				temperature = atoi(temperature_str);
				if (temperature > 25) {
					state = STATE_TOGGLE_VENT;
				}
				else {
					state = STATE_CHECK_MOIST;
				}
			}
			else {
				uart_puts("DHT12 is not responding.\r\n");
				uart_puts("Moving onto checking soil moisture.\r\n");
				state = STATE_CHECK_MOIST;
			}
			break;
			
			case STATE_CHECK_MOIST:
				uart_puts("Checking soil moisture.\r\n");
				// Start ADC conversion
				ADCSRA |= (1<<ADSC);
			break;
			
			default:
			state = STATE_IDLE;
			break;
		}
		

	}
	// Else do nothing and exit the ISR
}

ISR(ADC_vect)
{
	static uint16_t air_val = 920;	// Needs to be calibrated
	static uint16_t water_val = 760;	// Needs to be calibrated
	static uint8_t raw_value = 0;
	static uint8_t value = 0;
	char temp_str[3] = "";
	
	// Get raw value and display it on UART
	raw_value = ADC;
	itoa(raw_value, temp_str, 10);
	uart_puts("ADC raw value: ");
	uart_puts(temp_str);
	uart_puts("\r\n");
	
	// Get moisture value in %
	value = atof(temp_str);
	if (value > air_val) {
		value = air_val;
	}
	else if (value < water_val) {
		value = water_val;
	}
	value = (100/(air_val-water_val)) * (value-water_val);
	itoa(value, temp_str, 10);
	
	// Update the LCD
	lcd_gotoxy(1, 1);
	lcd_puts("   ");
	lcd_gotoxy(1, 1);
	lcd_puts(temp_str);
	lcd_gotoxy(4, 1);
	lcd_puts("%");
}
