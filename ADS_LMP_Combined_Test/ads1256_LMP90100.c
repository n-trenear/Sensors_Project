/*
 * ads1256_LMP90100_test.c
 *
 * Created on: 27/03/2019
 * Author: Nathan Trenear
*/

/*
             define from bcm2835.h
                 3.3V | | 5V               ->                 3.3V | | 5V
    RPI_V2_GPIO_P1_03 | | 5V               ->                  SDA | | 5V
    RPI_V2_GPIO_P1_05 | | GND              ->                  SCL | | GND
       RPI_GPIO_P1_07 | | RPI_GPIO_P1_08   ->                  IO7 | | TX
                  GND | | RPI_GPIO_P1_10   ->                  GND | | RX
       RPI_GPIO_P1_11 | | RPI_GPIO_P1_12   ->                  IO0 | | IO1
    RPI_V2_GPIO_P1_13 | | GND              ->                  IO2 | | GND
       RPI_GPIO_P1_15 | | RPI_GPIO_P1_16   ->                  IO3 | | IO4
                  VCC | | RPI_GPIO_P1_18   ->                  VCC | | IO5
       RPI_GPIO_P1_19 | | GND              ->                 MOSI | | GND
       RPI_GPIO_P1_21 | | RPI_GPIO_P1_22   ->                 MISO | | IO6
       RPI_GPIO_P1_23 | | RPI_GPIO_P1_24   ->                  SCK | | CE0
                  GND | | RPI_GPIO_P1_26   ->                  GND | | CE1


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
#include <time.h>
#include <modbus.h>

//CS    -----   SPICS
//DIN     -----   MOSI
//DOUT  -----   MISO
//SCLK   -----   SCLK
//DRDY  -----   ctl_IO     data  starting
//RST     -----   ctl_IO     reset

//ADS1256 macros
//#define  DRDY  RPI_V2_GPIO_P1_11         //P0
#define  DRDY  17
//#define  RST  RPI_V2_GPIO_P1_12     //P1
#define  RST  18
//#define	SPICS	RPI_V2_GPIO_P1_15	//P3
#define  SPICS  22

#define CS_1() bcm2835_gpio_write(SPICS,HIGH)
#define CS_0()  bcm2835_gpio_write(SPICS,LOW)

#define DRDY_IS_LOW()	((bcm2835_gpio_lev(DRDY)==0))

#define RST_1() 	bcm2835_gpio_write(RST,HIGH)
#define RST_0() 	bcm2835_gpio_write(RST,LOW)

//LMP90100 macros
#define MISO_AUX  19
#define MOSI_AUX  20
#define SPICS_AUX 26
#define CS_AUX_1() bcm2835_gpio_write(SPICS_AUX,HIGH)
#define CS_AUX_0() bcm2835_gpio_write(SPICS_AUX,LOW)
#define CS_AUX_IS_LOW()  (bcm2835_gpio_lev(SPICS_AUX) == 0)
#define CS_AUX_IS_HIGH() (bcm2835_gpio_lev(SPICS_AUX) == 1)
#define DRDY_AUX_IS_LOW()  (bcm2835_gpio_lev(MISO_AUX)==0)
#define DRDY_AUX_IS_HIGH() (bcm2835_gpio_lev(MISO_AUX)==1)

typedef enum {FALSEE = 0, TRUEE = !FALSEE} bool;

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

/* Sampling speed choice*/
/*
	11110000 = 30,000SPS (default)
	11100000 = 15,000SPS
	11010000 = 7,500SPS
	11000000 = 3,750SPS
	10110000 = 2,000SPS
	10100001 = 1,000SPS
	10010010 = 500SPS
	10000010 = 100SPS
	01110010 = 60SPS
	01100011 = 50SPS
	01010011 = 30SPS
	01000011 = 25SPS
	00110011 = 15SPS
	00100011 = 10SPS
	00010011 = 5SPS
	00000011 = 2.5SPS
*/
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

#define ADS1256_DRAE_COUNT = 15;

typedef struct{
	ADS1256_GAIN_E Gain;		/* GAIN  */
	ADS1256_DRATE_E DataRate;	/* DATA output  speed*/
	int32_t AdcNow[8];			/* ADC  Conversion value */
	uint8_t Channel;			/* The current channel*/
	uint8_t ScanMode;	/*Scanning mode,   0  Single-ended input  8 channel�� 1 Differential input  4 channel*/
}ADS1256_VAR_T;



/*Register definition�� Table 23. Register Map --- ADS1256 datasheet Page 30*/
enum{
	/*Register address, followed by reset the default values */
	REG_STATUS = 0,	// x1H
	REG_MUX    = 1, // 01H
	REG_ADCON  = 2, // 20H
	REG_DRATE  = 3, // F0H
	REG_IO     = 4, // E0H
	REG_OFC0   = 5, // xxH
	REG_OFC1   = 6, // xxH
	REG_OFC2   = 7, // xxH
	REG_FSC0   = 8, // xxH
	REG_FSC1   = 9, // xxH
	REG_FSC2   = 10, // xxH
};

/* Command definition�� TTable 24. Command Definitions --- ADS1256 datasheet Page 34 */
enum{
	CMD_WAKEUP  = 0x00,	// Completes SYNC and Exits Standby Mode 0000  0000 (00h)
	CMD_RDATA   = 0x01, // Read Data 0000  0001 (01h)
	CMD_RDATAC  = 0x03, // Read Data Continuously 0000   0011 (03h)
	CMD_SDATAC  = 0x0F, // Stop Read Data Continuously 0000   1111 (0Fh)
	CMD_RREG    = 0x10, // Read from REG rrr 0001 rrrr (1xh)
	CMD_WREG    = 0x50, // Write to REG rrr 0101 rrrr (5xh)
	CMD_SELFCAL = 0xF0, // Offset and Gain Self-Calibration 1111    0000 (F0h)
	CMD_SELFOCAL= 0xF1, // Offset Self-Calibration 1111    0001 (F1h)
	CMD_SELFGCAL= 0xF2, // Gain Self-Calibration 1111    0010 (F2h)
	CMD_SYSOCAL = 0xF3, // System Offset Calibration 1111   0011 (F3h)
	CMD_SYSGCAL = 0xF4, // System Gain Calibration 1111    0100 (F4h)
	CMD_SYNC    = 0xFC, // Synchronize the A/D Conversion 1111   1100 (FCh)
	CMD_STANDBY = 0xFD, // Begin Standby Mode 1111   1101 (FDh)
	CMD_RESET   = 0xFE, // Reset to Power-Up Values 1111   1110 (FEh)
};


ADS1256_VAR_T g_tADS1256;
static const uint8_t s_tabDataRate[ADS1256_DRATE_MAX] = {
	0xF0,		/*reset the default values  */
	0xE0,
	0xD0,
	0xC0,
	0xB0,
	0xA1,
	0x92,
	0x82,
	0x72,
	0x63,
	0x53,
	0x43,
	0x33,
	0x20,
	0x13,
	0x03
};


void bsp_DelayUS(uint64_t micros){bcm2835_delayMicroseconds(micros);}

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
static unsigned int LMP90100_DRDY (modbus_t *ctx)
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
static void LMP90100_Setup(void){
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

		//if (LMP90100_DRDY())
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
			bsp_DelayUS(3000);
		}
}

/*
*********************************************************************************************************
*	name: ADS1256_StartScan
*	function: Configuration DRDY PIN for external interrupt is triggered
*	parameter: _ucDiffMode : 0  Single-ended input  8 channel�� 1 Differential input  4 channe
*	The return value: NULL
*********************************************************************************************************
*/
void ADS1256_StartScan(uint8_t _ucScanMode){
	g_tADS1256.ScanMode = _ucScanMode;

		uint8_t ch_num;

		g_tADS1256.Channel = 0;

		for (ch_num = 0; ch_num < 8; ch_num++)
		{
			g_tADS1256.AdcNow[ch_num] = 0;
		}
}

/*
*********************************************************************************************************
*	name: ADS1256_Send8Bit
*	function: SPI bus to send 8 bit data
*	parameter: _data:  data
*	The return value: NULL
*********************************************************************************************************
*/
static void ADS1256_Send8Bit(uint8_t _data){
	bsp_DelayUS(2);
	bcm2835_spi_transfer(_data);
}

/*
*********************************************************************************************************
*	name: ADS1256_WaitDRDY
*	function: delay time  wait for automatic calibration
*	parameter:  NULL
*	The return value:  NULL
*********************************************************************************************************
*/
static void ADS1256_WaitDRDY(void){
	uint32_t ch_num;

	for (ch_num = 0; ch_num < 400000; ch_num++)
	{
		if (DRDY_IS_LOW())
		{
			break;
		}
	}
	if (ch_num >= 400000)
	{
		printf("ADS1256_WaitDRDY() Time Out ...\r\n");
	}
}

/*
*********************************************************************************************************
*	name: ADS1256_CfgADC
*	function: The configuration parameters of ADC, gain and data rate
*	parameter: _gain:gain 1-64
*                      _drate:  data  rate
*	The return value: NULL
*********************************************************************************************************
*/
void ADS1256_CfgADC(ADS1256_GAIN_E _gain, ADS1256_DRATE_E _drate){
	g_tADS1256.Gain = _gain;
	g_tADS1256.DataRate = _drate;

	ADS1256_WaitDRDY();

	{
		uint8_t buf[4];		/* Storage ads1256 register configuration parameters */

		/*Status register define
			Bits 7-4 ID3, ID2, ID1, ID0  Factory Programmed Identification Bits (Read Only)

			Bit 3 ORDER: Data Output Bit Order
				0 = Most Significant Bit First (default)
				1 = Least Significant Bit First
			Input data  is always shifted in most significant byte and bit first. Output data is always shifted out most significant
			byte first. The ORDER bit only controls the bit order of the output data within the byte.

			Bit 2 ACAL : Auto-Calibration
				0 = Auto-Calibration Disabled (default)
				1 = Auto-Calibration Enabled
			When Auto-Calibration is enabled, self-calibration begins at the completion of the WREG command that changes
			the PGA (bits 0-2 of ADCON register), DR (bits 7-0 in the DRATE register) or BUFEN (bit 1 in the STATUS register)
			values.

			Bit 1 BUFEN: Analog Input Buffer Enable
				0 = Buffer Disabled (default)
				1 = Buffer Enabled

			Bit 0 DRDY :  Data Ready (Read Only)
				This bit duplicates the state of the DRDY pin.

			ACAL=1  enable  calibration
		*/
		//buf[0] = (0 << 3) | (1 << 2) | (1 << 1);//enable the internal buffer
        buf[0] = (0 << 3) | (1 << 2) | (0 << 1);  // The internal buffer is prohibited

        //ADS1256_WriteReg(REG_STATUS, (0 << 3) | (1 << 2) | (1 << 1));

		buf[1] = 0x08;

		/*	ADCON: A/D Control Register (Address 02h)
			Bit 7 Reserved, always 0 (Read Only)
			Bits 6-5 CLK1, CLK0 : D0/CLKOUT Clock Out Rate Setting
				00 = Clock Out OFF
				01 = Clock Out Frequency = fCLKIN (default)
				10 = Clock Out Frequency = fCLKIN/2
				11 = Clock Out Frequency = fCLKIN/4
				When not using CLKOUT, it is recommended that it be turned off. These bits can only be reset using the RESET pin.

			Bits 4-3 SDCS1, SCDS0: Sensor Detect Current Sources
				00 = Sensor Detect OFF (default)
				01 = Sensor Detect Current = 0.5 �� A
				10 = Sensor Detect Current = 2 �� A
				11 = Sensor Detect Current = 10�� A
				The Sensor Detect Current Sources can be activated to verify  the integrity of an external sensor supplying a signal to the
				ADS1255/6. A shorted sensor produces a very small signal while an open-circuit sensor produces a very large signal.

			Bits 2-0 PGA2, PGA1, PGA0: Programmable Gain Amplifier Setting
				000 = 1 (default)
				001 = 2
				010 = 4
				011 = 8
				100 = 16
				101 = 32
				110 = 64
				111 = 64
		*/
		buf[2] = (0 << 5) | (0 << 3) | (_gain << 0);
		//ADS1256_WriteReg(REG_ADCON, (0 << 5) | (0 << 2) | (GAIN_1 << 1));	/*choose 1: gain 1 ;input 5V/
		buf[3] = s_tabDataRate[_drate];	// DRATE_10SPS;

		CS_0();	/* SPIƬѡ = 0 */
		ADS1256_Send8Bit(CMD_WREG | 0);	/* Write command register, send the register address */
		ADS1256_Send8Bit(0x03);			/* Register number 4,Initialize the number  -1*/

		ADS1256_Send8Bit(buf[0]);	/* Set the status register */
		ADS1256_Send8Bit(buf[1]);	/* Set the input channel parameters */
		ADS1256_Send8Bit(buf[2]);	/* Set the ADCON control register,gain */
		ADS1256_Send8Bit(buf[3]);	/* Set the output rate */

		CS_1();	/* SPI  cs = 1 */
	}

	bsp_DelayUS(50);
}

/*
*********************************************************************************************************
*	name: ADS1256_Recive8Bit
*	function: SPI bus receive function
*	parameter: NULL
*	The return value: NULL
*********************************************************************************************************
*/
static uint8_t ADS1256_Recive8Bit(void){
	uint8_t read = 0;
	read = bcm2835_spi_transfer(0xff);
	return read;
}

/*
*********************************************************************************************************
*	name: ADS1256_WriteReg
*	function: Write the corresponding register
*	parameter: _RegID: register  ID
*			 _RegValue: register Value
*	The return value: NULL
*********************************************************************************************************
*/
static void ADS1256_WriteReg(uint8_t _RegID, uint8_t _RegValue){
	CS_0();	/* SPI  cs  = 0 */
	ADS1256_Send8Bit(CMD_WREG | _RegID);	/*Write command register */
	ADS1256_Send8Bit(0x00);		/*Write the register number */

	ADS1256_Send8Bit(_RegValue);	/*send register value */
	CS_1();	/* SPI   cs = 1 */
}

/*
*********************************************************************************************************
*	name: ADS1256_WriteCmd
*	function: Sending a single byte order
*	parameter: _cmd : command
*	The return value: NULL
*********************************************************************************************************
*/
static void ADS1256_WriteCmd(uint8_t _cmd){
	CS_0();	/* SPI   cs = 0 */
	ADS1256_Send8Bit(_cmd);
	CS_1();	/* SPI  cs  = 1 */
}

/*
*********************************************************************************************************
*	name: ADS1256_SetChannel
*	function: Configuration channel number
*	parameter:  _ch:  channel number  0--7
*	The return value: NULL
*********************************************************************************************************
*/
static void ADS1256_SetChannel(uint8_t _ch){
	/*
	Bits 7-4 PSEL3, PSEL2, PSEL1, PSEL0: Positive Input Channel (AINP) Select
		0000 = AIN0 (default)
		0001 = AIN1
		0010 = AIN2 (ADS1256 only)
		0011 = AIN3 (ADS1256 only)
		0100 = AIN4 (ADS1256 only)
		0101 = AIN5 (ADS1256 only)
		0110 = AIN6 (ADS1256 only)
		0111 = AIN7 (ADS1256 only)
		1xxx = AINCOM (when PSEL3 = 1, PSEL2, PSEL1, PSEL0 are ��don��t care��)

		NOTE: When using an ADS1255 make sure to only select the available inputs.

	Bits 3-0 NSEL3, NSEL2, NSEL1, NSEL0: Negative Input Channel (AINN)Select
		0000 = AIN0
		0001 = AIN1 (default)
		0010 = AIN2 (ADS1256 only)
		0011 = AIN3 (ADS1256 only)
		0100 = AIN4 (ADS1256 only)
		0101 = AIN5 (ADS1256 only)
		0110 = AIN6 (ADS1256 only)
		0111 = AIN7 (ADS1256 only)
		1xxx = AINCOM (when NSEL3 = 1, NSEL2, NSEL1, NSEL0 are ��don��t care��)
	*/
	if (_ch > 7)
	{
		return;
	}
	ADS1256_WriteReg(REG_MUX, (_ch << 4) | (1 << 3));	/* Bit3 = 1, AINN connection AINCOM */
}

/*
*********************************************************************************************************
*	name: ADS1256_SetDiffChannel
*	function: The configuration difference channel
*	parameter:  _ch:  channel number  0--3
*	The return value:  four high status register
*********************************************************************************************************
*/
static void ADS1256_SetDiffChannel(uint8_t _ch){
	/*
	Bits 7-4 PSEL3, PSEL2, PSEL1, PSEL0: Positive Input Channel (AINP) Select
		0000 = AIN0 (default)
		0001 = AIN1
		0010 = AIN2 (ADS1256 only)
		0011 = AIN3 (ADS1256 only)
		0100 = AIN4 (ADS1256 only)
		0101 = AIN5 (ADS1256 only)
		0110 = AIN6 (ADS1256 only)
		0111 = AIN7 (ADS1256 only)
		1xxx = AINCOM (when PSEL3 = 1, PSEL2, PSEL1, PSEL0 are ��don��t care��)

		NOTE: When using an ADS1255 make sure to only select the available inputs.

	Bits 3-0 NSEL3, NSEL2, NSEL1, NSEL0: Negative Input Channel (AINN)Select
		0000 = AIN0
		0001 = AIN1 (default)
		0010 = AIN2 (ADS1256 only)
		0011 = AIN3 (ADS1256 only)
		0100 = AIN4 (ADS1256 only)
		0101 = AIN5 (ADS1256 only)
		0110 = AIN6 (ADS1256 only)
		0111 = AIN7 (ADS1256 only)
		1xxx = AINCOM (when NSEL3 = 1, NSEL2, NSEL1, NSEL0 are ��don��t care��)
	*/
	if (_ch == 0)
	{
		ADS1256_WriteReg(REG_MUX, (0 << 4) | 1);	/* DiffChannal  AIN0�� AIN1 */
	}
	else if (_ch == 1)
	{
		ADS1256_WriteReg(REG_MUX, (2 << 4) | 3);	/*DiffChannal   AIN2�� AIN3 */
	}
	else if (_ch == 2)
	{
		ADS1256_WriteReg(REG_MUX, (4 << 4) | 5);	/*DiffChannal    AIN4�� AIN5 */
	}
	else if (_ch == 3)
	{
		ADS1256_WriteReg(REG_MUX, (6 << 4) | 7);	/*DiffChannal   AIN6�� AIN7 */
	}
}

/*
*********************************************************************************************************
*	name: ADS1256_ReadData
*	function: read ADC value
*	parameter: NULL
*	The return value:  NULL
*********************************************************************************************************
*/
static int32_t ADS1256_ReadData(void){
	uint32_t read = 0;
    static uint8_t buf[3];

	CS_0();	/* SPI   cs = 0 */

	ADS1256_Send8Bit(CMD_RDATA);	/* read ADC command  */

	bsp_DelayUS(10);	/*delay time  */

	/*Read the sample results 24bit*/
    buf[0] = ADS1256_Recive8Bit();
    buf[1] = ADS1256_Recive8Bit();
    buf[2] = ADS1256_Recive8Bit();

    read = ((uint32_t)buf[0] << 16) & 0x00FF0000;
    read |= ((uint32_t)buf[1] << 8);  /* Pay attention to It is wrong   read |= (buf[1] << 8) */
    read |= buf[2];

	CS_1();	/* SPIƬѡ = 1 */

	/* Extend a signed number*/
    if (read & 0x800000)
    {
	    read |= 0xFF000000;
    }

	return (int32_t)read;
}


/*
*********************************************************************************************************
*	name: ADS1256_GetAdc
*	function: read ADC value
*	parameter:  channel number 0--7
*	The return value:  ADC vaule (signed number)
*********************************************************************************************************
*/
int32_t ADS1256_GetAdc(uint8_t _ch){
	int32_t iTemp;

	if (_ch > 7)
	{
		return 0;
	}

	iTemp = g_tADS1256.AdcNow[_ch];

	return iTemp;
}

/*
*********************************************************************************************************
*	name: ADS1256_ISR
*	function: Collection procedures
*	parameter: NULL
*	The return value:  NULL
*********************************************************************************************************
*/
void ADS1256_ISR(void){
	ADS1256_SetDiffChannel(g_tADS1256.Channel);	/* change DiffChannel */
	bsp_DelayUS(5);

	ADS1256_WriteCmd(CMD_SYNC);
	bsp_DelayUS(5);

	ADS1256_WriteCmd(CMD_WAKEUP);
	bsp_DelayUS(25);

	if (g_tADS1256.Channel == 0)
	{
		g_tADS1256.AdcNow[3] = ADS1256_ReadData();
	}
	else
	{
		g_tADS1256.AdcNow[g_tADS1256.Channel-1] = ADS1256_ReadData();
	}

	if (++g_tADS1256.Channel >= 4)
	{
		g_tADS1256.Channel = 0;
	}
}

/*
*********************************************************************************************************
*	name: ADS1256_Scan
*	function:
*	parameter:NULL
*	The return value:  1
*********************************************************************************************************
*/
uint8_t ADS1256_Scan(void){
	if (DRDY_IS_LOW())
	{
		ADS1256_ISR();
		return 1;
	}

	return 0;
}

/*
*********************************************************************************************************
*	name: Voltage_Convert
*	function:  Voltage value conversion function
*	parameter: Vref : The reference voltage 3.3V or 5V
*			   voltage : output DAC value
*	The return value:  NULL
*********************************************************************************************************
*/
uint16_t Voltage_Convert(float Vref, float voltage){
	uint16_t _D_;
	_D_ = (uint16_t)(65536 * voltage / Vref);

	return _D_;
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
	int32_t numChannels = 4;
	int32_t adc[numChannels];
	int32_t volt[numChannels];
	uint16_t tab_reg[100];
	int rc;

	uint8_t buf[3];

	for(int i = 0; i <= numChannels; i++){
		while((ADS1256_Scan() == 0));
	}

	for(int i = 0; i < 2; i++){
		adc[i] = ADS1256_GetAdc(i+2);
		volt[i] = (adc[i] * 100) / 167;

		Vin = volt[i] / 8 * ((1500 + 100000) / 1500); /* uV */
		Vin = Vin * 1.0425; //multiply by error factor

		if (Vin < 0){
			Vin = -Vin;
			printf("-%ld.%03ld %03ld V \n", Vin / 1000000, (Vin%1000000)/1000, Vin%1000);
			Vin = Vin/100000;
			tab_reg[0] = (uint16_t) Vin;
			rc = modbus_write_registers(ctx, 1001+i, 1, tab_reg);
			if(rc == -1){
				fprintf(stderr,"%s\n", modbus_strerror(errno));
			}
		}
		else{
			printf("%ld.%03ld %03ld V \n", Vin / 1000000, (Vin%1000000)/1000, Vin%1000);
			Vin = Vin/100000;
			tab_reg[0] = (uint16_t) Vin;
			rc = modbus_write_registers(ctx, 1001+i, 1, tab_reg);
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
		LMP90100_DispTemp(ctx);
		bsp_DelayUS(1000000);

		ADS1256_DispVoltage(ctx);
		bsp_DelayUS(1000000);
		printf("\33[%dA", 3);
	}

	bcm2835_spi_end();
	bcm2835_aux_spi_end();
	bcm2835_close();
	modbus_close(ctx);
	modbus_free(ctx);

	return 0;
}