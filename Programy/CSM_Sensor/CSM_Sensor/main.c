/*
 * CSM_Sensor.c
 *
 * Created: 30. 11. 2021 13:27:56
 * Author : xsedla1n
 */ 

/* Defines -----------------------------------------------------------*/
#define SENSOR_PIN PC0

#ifndef F_CPU
# define F_CPU 16000000  // CPU frequency in Hz required for UART_BAUD_SELECT
#endif

/* Includes ----------------------------------------------------------*/
#include <avr/io.h>         // AVR device-specific IO definitions
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
#include <stdlib.h>         // C library. Needed for conversion function
#include "gpio.h"			// GPIO library for AVR-GCC
#include "uart.h"           // Peter Fleury's UART library
#include "timer.h"          // Timer library for AVR-GCC


int main(void)
{
	// Initialize UART to asynchronous, 8N1, 9600
	uart_init(UART_BAUD_SELECT(9600, F_CPU));
	
	// Configure ADC to convert PC0[A0] analog value
    // Set ADC reference to AVcc
	ADMUX |= (1<<REFS0); ADMUX &= ~(1<<REFS1); 

    // Set input channel to ADC0
	ADMUX &= ~(1<<MUX0); ADMUX &= ~(1<<MUX1); ADMUX &= ~(1<<MUX2); ADMUX &= ~(1<<MUX3); 
	
    // Enable ADC module
	ADCSRA |= (1<<ADEN);

    // Enable conversion complete interrupt
	ADCSRA |= (1<<ADIE);

    // Set clock prescaler to 128
	ADCSRA |= (1<<ADPS0); ADCSRA |= (1<<ADPS1); ADCSRA |= (1<<ADPS2);
	
	// Input start string
	uart_puts("Soil moisture:\r\n");
	
    // Configure 16-bit Timer/Counter1 to update FSM
    // Set prescaler to 33 ms and enable interrupt
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

    // Will never reach this
    return 0;
}

ISR(TIMER1_OVF_vect) {
	
	// Start ADC conversion
	ADCSRA |= (1<<ADSC);
	
}

ISR(ADC_vect)
{
	static uint8_t air_val = 920;
	static uint8_t water_val = 760;
	static uint16_t value = 0;
	char uart_str[3] = "";
	
	value = ADC;
	itoa(value, uart_str, 10);
	uart_puts(uart_str);
	uart_puts("\r\n");
}
