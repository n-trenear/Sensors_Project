/*****************************************************/
/*
Serial_LCD_Test.c
Program for writing to Newhaven Display Serial LCDs

(c)2010 Curt Lagerstam - Newhaven Display International, LLC. 

 	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
*/
/*****************************************************/

#include "e:\2321_tester\tester_init.h"

/********************************************************************
*                                                                   *
*                    MAIN PROGRAM                                   *
*                                                                   *
********************************************************************/
void main(void)
{
	OSCCON |= 0x73;			// internal RC, 8MHz

	PORTA = 0x00;
	TRISA = 0xFF;			// B'1111 1111'
	PORTB = 0x00;
	TRISB = 0xFF;			// B'1111 1111'
	PORTC = 0x75;			// B'0111 0101'
	TRISC = 0x00;			// B'0000 0000'
	GIE = 0;
	INTCON = 0x00;		    // disable all interrupt
	INTCON2 = 0x00;			// enable port B pull-up
	INTCON3 = 0x00;

/********************************************************************
 set up adc for analog inputs
 ADC clock = Fosc/8 (1 uSecond), adc off, 
 AN0 is contrast input, AN1 is backlight brightness input
********************************************************************/
	ADCON0 = 0x00;
	ADCON1 = 0x0D;			// RA0, RA1 are analog inputs
	ADCON2 = 0x15;			// left just, 4TAD, Fosc/16, 0001 0101

/********************************************************************
 set up timer 0 for 10 millisecond interrupt
 timer 0 is a general purpose timer
********************************************************************/
	T0CON = 0x46;			// timer 0 control register
	T1CON = 0xB0;			// timer 1 off
	
	spi_ss = 1;
	
	if (!i2c_sel) {
		com_mode = 3;		// I2C
		init_i2c();
	}
	else {
		if (!spi_sel) {
			com_mode = 2;	// SPI
			init_spi();
		}
		else {
			com_mode = 1;	// RS232
			init_rs232();
		}
	}

/* END OF SPECIAL REGISTER INITIALIZATION */

	tick_100m = 9;			// 100 millisceond counter
	tick_1s = 9;			// 1 second counter

	TMR0L = 100;			// initialize to 100 counts
	TMR0ON = 1;
	TMR0IF = 0;
	TMR0IE = 1;
	
	contrast = 0;
	contrast_f = 0;
	bright = 0;
	bright_f = 0;
	serial_busy = 0;
	
	TXREG = 0;
	PEIE = 1;
	GIE = 1;				// enable global interrupt

	delay_ms(100);			// waiting for LCD to initialize
		
	lcd_clear();
// Set Contrast
	tx_packet[0] = 0xFE;
	tx_packet[1] = 0x52;
	tx_packet[2] = 40;		// contrast 1 - 50
	send_packet(3);

// Set Backlight Level
	tx_packet[1] = 0x53;
	tx_packet[2] = 15;		// backlight 1 - 15
	send_packet(3);

	tx_packet[1] = 0x48;	// underline cursor off
	send_packet(2);
	
	delay_ms(1000);

//*********************************************************
	for (i = 0; i < 16; i++) {	// "NewHaven Display"
		tx_packet[i] = text6[i];
	}
	send_packet(16);
		
	lcd_cursor(0x40);
	delay_ms(100);
	
	for (i = 0; i < 16; i++) {	//"Serial LCD Demo"
		tx_packet[i] = text7[i];
	}
	
	send_packet(16);
	
	
	temp = 0;
	
	for (;;) {

// Contrast Control
		if (contrast_f && !serial_busy ) {
			tx_packet[0] = 0xFE;
			tx_packet[1] = 0x52;
			tx_packet[2] = contrast_save;
			send_packet(3);
			
			contrast_f = 0;
		}

// Backlight Brightness
		if (bright_f && !serial_busy) {
			tx_packet[0] = 0xFE;
			tx_packet[1] = 0x53;
			tx_packet[2] = bright_save;
			send_packet(3);
			
			bright_f = 0;
		}
	}
}	/* end main	*/

/********************************************************************
 send 'x' byte of the tx_packet[] to the serial port
********************************************************************/
void send_packet(unsigned int x)
{
	unsigned int ix;
	
	led = 1;
	serial_busy = 1;				// set busy flag
	switch (com_mode) {
		case 1:						// RS232 mode
			for (ix = 0; ix < x; ix++) {
				while (!TXIF);
				TXREG = tx_packet[ix];
			}
			while (!TRMT);
			break;
			
		case 2:						// SPI
			spi_ss = 0;
			delay_cycles(5);
			for (ix = 0; ix < x; ix++) {
				SSPBUF = tx_packet[ix];
				while(!BF);
				temp = SSPBUF;
				delay_us(5);		// reduce effective clock rate
			}
			delay_cycles(5);
			spi_ss = 1;
			break;
			
		case 3:						// I2C
			i2c_busy(0x50);			// send address
			
			for (ix = 0; ix < x; ix++) {
				SSPBUF = tx_packet[ix];
				wait_sspif();
			}
			
			PEN = 1;				// send stop bit
			while (PEN == 1);		// wait until stop bit finished

			break;
			
		default:
			break;
	}
	led = 0;
	serial_busy = 0;				// clear busy flag
}

/********************************************************************
  wait for SSPIF set routine, add a software counter to ensure the
  routine does not hang.
********************************************************************/
void wait_sspif(void)
{
	loop_hi = 9;
	loop_lo = 0xC4;
	SSPIF = 0;
	while (!SSPIF && loop_hi) {
		loop_lo--;
		if(!loop_lo)
			loop_hi--;
	}
	SSPIF = 0;
}

/********************************************************************
  i2c_busy routine poll the ACK status bit to see if device is busy
********************************************************************/
void i2c_busy(unsigned char x)
{
	SEN = 1;				// start bit
	while(SEN);				// wait until start bit finished

	while (1) {
		SSPIF = 0;
		SSPBUF = x;			// send write command
		wait_sspif();		// wait until command finished

		if (ACKSTAT) {		// check ACK, if 1, ACK not received
			RSEN = 1;		// then sent start bit again
			while (RSEN);	// wait until start bit finished
		}
		else
			break;
	}
} 
	
/********************************************************************
 set lcd cursor
********************************************************************/
void lcd_cursor(unsigned int x)
{
	tx_packet[0] = 0xFE;
	tx_packet[1] = 0x45;
	tx_packet[2] = x;
	send_packet(3);	
	delay_ms(10);
}

/********************************************************************
 clear one line of display
********************************************************************/
void clear_line(unsigned int x)
{
	unsigned int ij;
	
	for (ij = 0; ij < x; ij++) {
		tx_packet[ij] = 0x20;
	}
	send_packet(x);
}

/********************************************************************
 lcd clear
********************************************************************/
void lcd_clear(void)
{
	tx_packet[0] = 0xFE;
	tx_packet[1] = 0x51;
	send_packet(2);
	delay_ms(10);
}

/********************************************************************
  initialize for USART interface
********************************************************************/
void init_rs232(void)
{
	TXSTA = 0x26;			// rs232 transmit status register
	RCSTA = 0x80;			// serial port enable
	BAUDCTL = 0x08;			// BRG16 = 1;
	SPBRG = 207;
	SPBRGH = 0;
}

/********************************************************************
 initialize for spi interface
********************************************************************/
void init_spi(void)
{
	TRISC |= 0x10;
	SSPCON1 = 0x32;			// master, 125K CLK, CKP = 1
	
	temp = SSPBUF;
	SSPOV = 0;
	SSPIF = 0;
}

/********************************************************************
 initialize for i2c interface
********************************************************************/
void init_i2c(void)
{
	TRISC |= 0x18;			// RC3, RC4 as inputs
	SSPSTAT = 0x00;
	SSPADD = 19;			// 100K Hz
	SSPCON1 = 0x28;			// i2c master, enable SSP

	temp = SSPBUF;
	SSPOV = 0;
	SSPIF = 0;
	SSPIE = 0;				// enable I2C interrupts
}

/********************************************************************
  ADC read routine, read the selected adc channel and put the result
  in adc_result.
********************************************************************/
void adc(unsigned char n)
{
	switch (n) {
		case 0:				// adc channel 0, contrast
			ADCON0 = 0x03;
			break;
			
		case 1:				// adc channel 1, backlight
			ADCON0 = 0x07;
			break;
			
		default:
			break;
	}
					
	while (GODONE);				// wait for end of convert
	adc_result = ADRESH;
	if (adc_result > 250)
		adc_result = 250;
}

/*****************************************************************************/
/* interrupt routines                                                        */
/* timer 0 is set to interrupt at 100 Hz rate.                               */
/*****************************************************************************/
#int_timer0
void timer0_isr(void)	/*interrupt handler routine*/
{
	restart_wdt();		// reset watchdog timer
	TMR0L = 100;
			   		
/********************************************************************
                 10 millisecond functions 
********************************************************************/
	if (tick_wait10)
		tick_wait10--;
	   		
	if (tick_100m) {
   		tick_100m--;
   	} 

/********************************************************************
                 100 millisecond functions 
********************************************************************/
   	else {				// 100 millisecond function
   		tick_100m = 9;	// reload 100 millisecond timer
   			
		if (tick_wait100)		// general purpose 100 mSecond timer
			tick_wait100--;
			
			adc(0);					// read contrast setting
			contrast = adc_result / 5;
			if (!contrast)
				contrast = 1;
				
			if (contrast != contrast_save) {
				contrast_save = contrast;
				contrast_f = 1;
			}

			adc(1);					// read backlight setting
			bright = adc_result / 15;
			if (!bright)
				bright = 1;
				
			if (bright != bright_save) {
				bright_save = bright;
				bright_f = 1;
			}
	
		if (tick_1s) {
			tick_1s--;
		}
/********************************************************************
                    1 second functions 
********************************************************************/
		else {
			tick_1s = 9;		// reload 1 second timer
		}
	}
}	/* end of timer 0 interrupt routine */

