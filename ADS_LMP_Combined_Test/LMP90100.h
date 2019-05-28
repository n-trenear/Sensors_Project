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

static int  LMP90100_ReadChannel(void);
static float LMP90100_ReadADC(void);
static unsigned int LMP90100_DRDY (modbus_t *ctx);
static void LMP90100_Setup(void);
