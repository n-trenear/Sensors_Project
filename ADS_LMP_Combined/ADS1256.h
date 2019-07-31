/*
 * ADS1256.h
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

//#define  DRDY  RPI_V2_GPIO_P1_11 //P0
#define  DRDY  17
//#define  RST  RPI_V2_GPIO_P1_12  //P1
#define  RST  18
//#define	SPICS	RPI_V2_GPIO_P1_15	 //P3
#define  SPICS  22

#define CS_1() bcm2835_gpio_write(SPICS,HIGH)
#define CS_0()  bcm2835_gpio_write(SPICS,LOW)

#define DRDY_IS_LOW()	((bcm2835_gpio_lev(DRDY)==0))

/* gain channel� */
typedef enum{
	ADS1256_GAIN_1			= (0),	/* GAIN   1 */
	ADS1256_GAIN_2			= (1),	/*GAIN   2 */
	ADS1256_GAIN_4			= (2),	/*GAIN   4 */
	ADS1256_GAIN_8			= (3),	/*GAIN   8 */
	ADS1256_GAIN_16			= (4),	/* GAIN  16 */
	ADS1256_GAIN_32			= (5),	/*GAIN    32 */
	ADS1256_GAIN_64			= (6),	/*GAIN    64 */
}ADS1256_GAIN_E;

typedef enum{
	ADS1256_30000SPS = 0,
	ADS1256_15000SPS,
	ADS1256_7500SPS,
	ADS1256_3750SPS,
	ADS1256_2000SPS,
	ADS1256_1000SPS,
	ADS1256_500SPS,
	ADS1256_100SPS,
	ADS1256_60SPS,
	ADS1256_50SPS,
	ADS1256_30SPS,
	ADS1256_25SPS,
	ADS1256_15SPS,
	ADS1256_10SPS,
	ADS1256_5SPS,
	ADS1256_2d5SPS,

	ADS1256_DRATE_MAX
}ADS1256_DRATE_E;

void ADS1256_StartScan(uint8_t _ucScanMode);
static void ADS1256_Send8Bit(uint8_t _data);
static void ADS1256_WaitDRDY(void);
void ADS1256_CfgADC(ADS1256_GAIN_E _gain, ADS1256_DRATE_E _drate);
static uint8_t ADS1256_Recive8Bit(void);
static void ADS1256_WriteReg(uint8_t _RegID, uint8_t _RegValue);
static void ADS1256_WriteCmd(uint8_t _cmd);
static void ADS1256_SetDiffChannel(uint8_t _ch);
static int32_t ADS1256_ReadData(void);
int32_t ADS1256_GetAdc(uint8_t _ch);
void ADS1256_ISR(void);
uint8_t ADS1256_Scan(void);
uint16_t Voltage_Convert(float Vref, float voltage);
