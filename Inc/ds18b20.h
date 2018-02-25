/* * * * * * * * * * * * * * * * * * * * * * * * * *
 * DS18B20 Firmware　　　　　　　　　　　　　　　　　　　　　　　　　　　　*
 * Copyright (c) 2017  							   *
 * K.Watanabe,Crescent 							   *
 * Released under the MIT license 				   *
 * http://opensource.org/licenses/mit-license.php  *
 * 17/12/27 v1.0 Initial Release                   *
 * 												   *
 * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "stm32f3xx_hal.h"
#include "math.h"
#include "gpio.h"

#define DS18B20_ID        0x28


#define ONEWIRE_PORT			GPIOA
#define ONEWIRE_PIN				GPIO_PIN_10


#define OVERDRIVE_SKIP_CMD    	0x3C
#define MATCH_ROM_CMD_BYTE      0x55
#define SEARCH_ROM_CMD_BYTE     0xF0
#define READ_ROM_CMD_BYTE		0x33
#define SKIP_ROM_CMD_BYTE		0xCC

#define CONVERT_T_CMD			0x44
#define READ_SCRATCHPAD_CMD		0xBE
#define WRITESCRATCH_CMD     	0x4E
#define COPYSCRATCH_CMD      	0x48
#define RECALLE2_CMD         	0xB8
#define READPOWERSUPPLY_CMD  	0xB4

#define ADDRESS_SIZE			8

void DS18B20_GPIO_Init(void);
uint8_t DS18B20_InitSeq(void);
float DS18B20_GetTemperature(void);
float DS18B20_GetSelectDeviceTemperature(int8_t* addr);
void DS18B20_ResetDeviceSearchInfo();
int8_t DS18B20_GetDeviceInfo();
int8_t DS18B20_DeviceSearch();
uint8_t DS18B20_CalcCRC(uint8_t* address, uint8_t len);
int8_t DS18B20_DeviceSearch(int8_t* newAddr);

