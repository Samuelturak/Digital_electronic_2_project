/*
 * CSM_Sensor.c
 *
 * Created: 30. 11. 2021 13:27:56
 * Author : xsedla1n
 */ 

/* Defines -----------------------------------------------------------*/
#define SENSOR_OUT PC0

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
	
	// Configure A0 PIN as input
	GPIO_config_input_nopull(&DDRC, SENSOR_OUT);
	
	// Input start string
	uart_puts("Soil moisture:\r\n");
	
    // Configure 16-bit Timer/Counter1 to update FSM
    // Set prescaler to 33 ms and enable interrupt
    TIM1_overflow_1s();
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
	static uint8_t humid = 0;
	char uart_str[4] = "";
	
	humid = GPIO_read(&PINC, SENSOR_OUT);
	itoa(humid, uart_str, 10);
	uart_puts(uart_str);
	uart_puts("\r\n");
}
