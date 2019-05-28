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

#define MISO_AUX  19
#define MOSI_AUX  20
#define SPICS_AUX 26
#define CS_AUX_1() bcm2835_gpio_write(SPICS_AUX,HIGH)
#define CS_AUX_0() bcm2835_gpio_write(SPICS_AUX,LOW)
#define CS_AUX_IS_LOW()  (bcm2835_gpio_lev(SPICS_AUX) == 0)
#define CS_AUX_IS_HIGH() (bcm2835_gpio_lev(SPICS_AUX) == 1)
#define DRDY_AUX_IS_LOW()  (bcm2835_gpio_lev(MISO_AUX)==0)
#define DRDY_AUX_IS_HIGH() (bcm2835_gpio_lev(MISO_AUX)==1)

static int  LMP90100_ReadChannel(void);
static float LMP90100_ReadADC(void);
unsigned int LMP90100_DRDY (modbus_t *ctx);
void LMP90100_Setup(void);
