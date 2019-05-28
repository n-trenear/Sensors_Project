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
