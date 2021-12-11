/*
 * Relay_Test.c
 *
 * Created: 30. 11. 2021
 * Author : xsedla1n
 */ 

/* Defines -----------------------------------------------------------*/
#define RELAY_PIN2 PB1 // Arduino PIN ~9
#define RELAY_PIN1 PB0 // Arduino PIN 8
#define BUTTON_PIN PD5 // Arduino PIN 5

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
	
	// Configure output PINs
	GPIO_config_output(&DDRB, RELAY_PIN1);
	// GPIO_config_output(&DDRB, RELAY_PIN2);
	GPIO_write_high(&PORTB, RELAY_PIN1);
	// GPIO_write_low(&PORTB, RELAY_PIN2);
	
	// Configure input PIN
	GPIO_config_input_pullup(&DDRD, BUTTON_PIN);
	
	// Input start string
	uart_puts("RELAY STATUS:\r\n");
	
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
	if (GPIO_read(&PIND, BUTTON_PIN) == 0) {
		uart_puts("Relay_1: ON; Relay_2: ON");
		uart_puts("\r\n");
		GPIO_write_low(&PORTB, RELAY_PIN1);
		// GPIO_write_high(&PORTB, RELAY_PIN2);
	}
	else {
		uart_puts("Relay_1: ON;	Relay_2: ON");
		uart_puts("\r\n");
		GPIO_write_high(&PORTB, RELAY_PIN1);
		// GPIO_write_low(&PORTB, RELAY_PIN2);
	}
}
