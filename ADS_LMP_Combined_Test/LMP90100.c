/*
 * LMP90100.c
 *
 * Created on: 28/05/2019
 * Author: Nathan Trenear
*/

#include <bcm2835.h>
#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <modbus.h>
#include "LMP90100.h"

#define MISO_AUX  19
#define MOSI_AUX  20
#define SPICS_AUX 26
#define CS_AUX_1() bcm2835_gpio_write(SPICS_AUX,HIGH)
#define CS_AUX_0() bcm2835_gpio_write(SPICS_AUX,LOW)
#define CS_AUX_IS_LOW()  (bcm2835_gpio_lev(SPICS_AUX) == 0)
#define CS_AUX_IS_HIGH() (bcm2835_gpio_lev(SPICS_AUX) == 1)
#define DRDY_AUX_IS_LOW()  (bcm2835_gpio_lev(MISO_AUX)==0)
#define DRDY_AUX_IS_HIGH() (bcm2835_gpio_lev(MISO_AUX)==1)

/*
*********************************************************************************************************
*	name: LMP90100_ReadChannel
*	function: Write the corresponding register
*	parameter: NULL
*
*	The return value: ADC channel scanned
*********************************************************************************************************
*/
static int  LMP90100_ReadChannel(void)
{
	uint8_t buf[4];
	uint8_t ans;

	buf[0] = 0x10;  // Set Upper nibble of register to read
	buf[1] = 0x01;  // Upper nibble is 1
	buf[2] = 0x89;  // Read 0x19 register
	buf[3] = 0x00;  // Transmit zero.. Response will be written to this value
	bcm2835_aux_spi_transfern(buf,4);  // Transmit set value of buf, write response into buf for each byte sent.

	ans = buf[3] & 0x07;  // Read 3 LSBs of 0x19 register
	return ans;
}

/*
*********************************************************************************************************
*	name: LMP90100_ReadADC
*	function:  Read the ADC registers 1A,1B,1C (24bit) and calculate temperature
*	parameter: NULL
*
*	The return value: Temperature
*********************************************************************************************************
*/
static float LMP90100_ReadADC(void)
{
	float temp_calc;
	uint8_t buf[6];
	int32_t adc;

	buf[0] = 0x10;
	buf[1] = 0x01;
	buf[2] = 0xCA;
	buf[3] = 0x00;
	buf[4] = 0x00;
	buf[5] = 0x00;
	bcm2835_aux_spi_transfern(buf,6);

	adc =  (((uint32_t) buf[3]) << 16);
	adc += (((uint32_t) buf[4]) <<  8);
	adc += (((uint32_t) buf[5]) <<  0);

	temp_calc = (((float)adc) /16777216*4015 - 100)/0.3925;

  return temp_calc;
}

/*
*********************************************************************************************************
*	name: LMP90100_DRDY
*	function: Detect ADC ready pulse on MISO line and initate ADC read and ADC channel number read
*	parameter: NULL
*
*	The return value: 1 on success else return 0
*********************************************************************************************************
*/
unsigned int LMP90100_DRDY (modbus_t *ctx)
{
	int Channel;
	float Temp_Reading;
	int result = 0;
	static int ctr = 0;
	uint16_t tab_reg[100];
	int modbus_register = 3;
	int rc;

	while(!result){
	if (DRDY_AUX_IS_LOW() && CS_AUX_IS_LOW())
	{
  	if (ctr > 1)
    {
		Temp_Reading = LMP90100_ReadADC();
		Channel = LMP90100_ReadChannel();
		printf("Ch:%d Temp: %3.1f \n",Channel,Temp_Reading);
		Temp_Reading = Temp_Reading * 10;
		tab_reg[0] = (uint16_t) Temp_Reading;
		rc = modbus_write_registers(ctx, 1000+modbus_register, 1, tab_reg);
      ctr = 0;
      result = 1;
    }
    else
    {
    	ctr = 0;
      result = 0;
    }
	}

	if (DRDY_AUX_IS_HIGH() && CS_AUX_IS_LOW())
	{
		ctr++;
	}
	if (DRDY_AUX_IS_HIGH() && CS_AUX_IS_HIGH() && (ctr >= 1))
	{
		ctr = 0;
	}
}

	return result;
}

/*
*********************************************************************************************************
*	name: LMP90100_Setup
*	function: Detect ADC ready pulse on MISO line and initate ADC read and ADC channel number read
*	parameter: NULL
*
*	The return value: 1 on success else return 0
*********************************************************************************************************
*/
void LMP90100_Setup(void){
	uint8_t setup_buf[16];

  CS_AUX_0();
  setup_buf[0] = 0x10;
  setup_buf[1] = 0x01;
  setup_buf[2] = 0x02;
  setup_buf[3] = 0x0A;   //Set current source to 1mA
  setup_buf[4] = 0x0F;
  setup_buf[5] = 0x98;   //Set continuous scan on CH0 - CH3 only.
  setup_buf[6] = 0x10;
  setup_buf[7] = 0x02;
  setup_buf[8] = 0x01;
  setup_buf[9] = 0x60;
  setup_buf[10] = 0x03;
  setup_buf[11] = 0x60;
  setup_buf[12] = 0x05;
  setup_buf[13] = 0x60;
  setup_buf[14] = 0x07;
  setup_buf[15] = 0x60;

  bcm2835_aux_spi_transfern(setup_buf,16);
  CS_AUX_1();
}
