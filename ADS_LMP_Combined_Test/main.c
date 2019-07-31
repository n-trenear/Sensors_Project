/*
 * main.c
 *
 * Created on: 27/03/2019
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
#include "ADS1256.h"
#include "LMP90100.h"

#define SW1 4
#define SW2 27
#define SW3 18
#define SW4 5
#define FB1A 25

#define SW1_1() bcm2835_gpio_write(SW1, HIGH)
#define SW1_0() bcm2835_gpio_write(SW1, LOW)

#define SW2_1() bcm2835_gpio_write(SW2, HIGH)
#define SW2_0() bcm2835_gpio_write(SW2, LOW)

#define SW3_1() bcm2835_gpio_write(SW3, HIGH)
#define SW3_0() bcm2835_gpio_write(SW3, LOW)

#define SW4_1() bcm2835_gpio_write(SW4, HIGH)
#define SW4_0() bcm2835_gpio_write(SW4, LOW)
/*
*********************************************************************************************************
*	name: LMP90100_DispTemp
*	function: Detect ADC ready pulse on MISO line and initate ADC read and ADC channel number read
*	parameter: NULL
*
*	The return value: 1 on success else return 0
*********************************************************************************************************
*/
static void LMP90100_DispTemp(modbus_t *ctx){
		unsigned int cs_state = 1;

		//if (LMP90100_DRDY(ctx))
		if (cs_state == 1)
		{
			CS_AUX_0();
			cs_state = 0;
		}

		if (LMP90100_DRDY(ctx))
		{
			if (cs_state == 0)
			{
				CS_AUX_1();
				cs_state = 1;
			}
			bcm2835_delayMicroseconds(3000);
		}
}



/*
*********************************************************************************************************
*	name: ADS1256_DispVoltage
*	function:  display voltage to terminal
*	parameter: NULL
*
*	The return value:  NULL
*********************************************************************************************************
*/
static void ADS1256_DispVoltage(modbus_t *ctx){
	int32_t Vin;
	int32_t numChannels = 1;
	int32_t adc[numChannels];
	int32_t volt[numChannels];
	uint16_t tab_reg[100];
	int rc;

	uint8_t buf[3];

	for(int i = 0; i <= numChannels; i++){
		while((ADS1256_Scan() == 0));
	}

	for(int i = 0; i < 1; i++){
		adc[i] = ADS1256_GetAdc(i);
		volt[i] = (adc[i] * 100) / 167;

		Vin = volt[i] / 8 * ((1500 + 100000) / 1500); /* uV */
		Vin = Vin * 1.0425; //multiply by error factor

		if (Vin < 0){
			SW1_0(); //Disconnect the contactor.
			
			Vin = -Vin;
			printf("-%ld.%03ld %03ld V \n", Vin / 1000000, (Vin%1000000)/1000, Vin%1000);
			
			Vin = -Vin/100000;
			tab_reg[0] = (uint16_t) Vin;
			rc = modbus_write_registers(ctx, 1005, 1, tab_reg);
			if(rc == -1){
				fprintf(stderr,"%s\n", modbus_strerror(errno));
			}
		}
		else{
			SW1_1(); //Connect the contactor.
			printf("%ld.%03ld %03ld V \n", Vin / 1000000, (Vin%1000000)/1000, Vin%1000);
			Vin = Vin/100000;
			tab_reg[0] = (uint16_t) Vin;
			rc = modbus_write_registers(ctx, 1005, 1, tab_reg);
		}
	}

}

/*
*********************************************************************************************************
*	name:  main
*	Setup SPI device then collect Temperature and ADC channel and print values on screen.
*	parameter: NULL
*
*	The return value: 0
*********************************************************************************************************
*/
int  main()
{
	int scanMode = 1; // 0 single ended mode : 1 differential mode
	clock_t start_t, end_t, total_t;
	modbus_t *ctx;
	uint16_t tab_reg[100];
	int rc;
	int i;
	int ctr = -1;

  if (!bcm2835_init())
  	return 1;

	bcm2835_spi_begin();
	bcm2835_aux_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);   //default
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE1);
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8192);//default
	bcm2835_aux_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_2048);//default

	bcm2835_gpio_fsel(SPICS_AUX, BCM2835_GPIO_FSEL_OUTP);//
	bcm2835_gpio_write(SPICS_AUX, HIGH);

	LMP90100_Setup();

	bcm2835_gpio_fsel(SPICS, BCM2835_GPIO_FSEL_OUTP);//
	bcm2835_gpio_write(SPICS, HIGH);
	bcm2835_gpio_fsel(DRDY, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(DRDY, BCM2835_GPIO_PUD_UP);

	ADS1256_CfgADC(ADS1256_GAIN_1, ADS1256_15SPS);
	ADS1256_StartScan(scanMode);

	bcm2835_gpio_fsel(SW1, BCM2835_GPIO_FSEL_OUTP);	
	bcm2835_gpio_fsel(SW2, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(SW3, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(SW4, BCM2835_GPIO_FSEL_OUTP);

	bcm2835_gpio_fsel(FB1A, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(FB1A, BCM2835_GPIO_PUD_UP);

	ctx = modbus_new_tcp("192.168.1.218", 502); //connect to dev board ip
	if (ctx == NULL){
		fprintf(stderr,"Unable to allocate libmodbus context\n");
		return -1;
	}

	if(modbus_connect(ctx) == -1){
		fprintf(stderr,"Connection failed: %s\n",modbus_strerror(errno));
		modbus_free(ctx);
		return -1;
	}

	tab_reg[0] = (uint16_t) 123;
	rc = modbus_write_registers(ctx, 1000, 1, tab_reg);
	if(rc == -1){
		fprintf(stderr,"%s\n", modbus_strerror(errno));
		return -1;
	}

	while(1)
	{		
		if(ctr == 10 || ctr == -1){
			ctr = 0;
			ADS1256_DispVoltage(ctx);
			LMP90100_DispTemp(ctx);
			printf("\33[%dA", 5);
		}		
		
		else{
			ctr++;
			ADS1256_DispVoltage(ctx);
			printf("\33[%dA", 1);
		}
	}

	bcm2835_spi_end();
	bcm2835_aux_spi_end();
	bcm2835_close();
	modbus_close(ctx);
	modbus_free(ctx);

	return 0;
}
