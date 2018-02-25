/* * * * * * * * * * * * * * * * * * * * * * * * * *
 * DS18B20 Firmware　　　　　　　　　　　　　　　　　　　　　　　　　　　　*
 * Copyright (c) 2017  							   *
 * K.Watanabe,Crescent 							   *
 * Released under the MIT license 				   *
 * http://opensource.org/licenses/mit-license.php  *
 * 17/12/27 v1.0 Initial Release                   *
 * 												   *
 * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "ds18b20.h"
#include "stdbool.h"

int8_t ds18b20_addr[ADDRESS_SIZE];
int16_t searchNode;
bool searched;



uint32_t GetUs(void) {

	uint32_t usTicks = HAL_RCC_GetSysClockFreq() / 1000000;
	register uint32_t ms, cycle_cnt;
	do {
		ms = HAL_GetTick();
		cycle_cnt = SysTick->VAL;
	}
	while (ms != HAL_GetTick());

	return (ms * 1000) + (usTicks * 1000 - cycle_cnt) / usTicks;
}



void Delay_us(uint16_t micros) {
	uint32_t start = GetUs();
	while (GetUs()-start < (uint32_t) micros) {
	asm("NOP");
	}
}

void DS18B20_GPIO_IN()  {
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = ONEWIRE_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;//GPIO_NOPULL;
	HAL_GPIO_Init(ONEWIRE_PORT, &GPIO_InitStruct);
}
void DS18B20_GPIO_OUT() {
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = ONEWIRE_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Speed= GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pull = GPIO_PULLUP;//GPIO_NOPULL;
	HAL_GPIO_Init(ONEWIRE_PORT, &GPIO_InitStruct);
}


void DS18B20_GPIO_Init(void)
{
	DS18B20_GPIO_OUT();
	HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 1);
}

uint8_t DS18B20_InitSeq(void)
{
	uint8_t retry = 0;

	DS18B20_GPIO_OUT();
	HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 0);
	Delay_us(750);
	HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 1);
	Delay_us(15);
	DS18B20_GPIO_IN();
	while(HAL_GPIO_ReadPin(ONEWIRE_PORT, ONEWIRE_PIN)==1 && retry<200){
		retry++;
		Delay_us(1);

	}
	if(retry>=200)
		return false;
	retry = 0;
	while(HAL_GPIO_ReadPin(ONEWIRE_PORT, ONEWIRE_PIN)==0 && retry<240)	{
		retry++;
		Delay_us(1);
	}
	if(retry>=240)return false;

	return true;
}

uint8_t DS18B20_ReadBit(void)
{
	DS18B20_GPIO_OUT();
	HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 0);
	Delay_us(2);//2
	HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 1);
	DS18B20_GPIO_IN();
	Delay_us(11);//12
	if(HAL_GPIO_ReadPin(ONEWIRE_PORT, ONEWIRE_PIN)==1)
	{
		Delay_us(50);//50
		return true;
	}
	else{
		Delay_us(50);
		return false;
	}
}

uint8_t DS18B20_ReadByte(void)
{
	uint8_t i, t, data = 0;

	for(i=0;i<8;i++)
	{
		t = DS18B20_ReadBit();
		data |= t<<i;
	}

	return data;
}


void DS18B20_WriteBit(uint8_t data)
{

	DS18B20_GPIO_OUT();

	if(data&(0x01))
	{
		HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 0);
		Delay_us(2);//2
		HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 1);
		Delay_us(60);//60
	}
	else{
		HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 0);
		Delay_us(60);//60
		HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 1);
		Delay_us(2);//2
	}

}

void DS18B20_WriteByte(uint8_t data)
{
	uint8_t i;

	DS18B20_GPIO_OUT();
	for(i=0;i<8;i++)
	{
		if(data&(1<<i))
		{
			HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 0);
			Delay_us(2);//2
			HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 1);
			Delay_us(60);//60
		}
		else{
			HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 0);
			Delay_us(60);//60
			HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, 1);
			Delay_us(2);//2
		}
	}
}



uint8_t DS18B20_StartConvert(void)
{
	if(DS18B20_InitSeq()==false)return false;
	DS18B20_WriteByte(SKIP_ROM_CMD_BYTE);
	DS18B20_WriteByte(CONVERT_T_CMD);

	return true;
}

float DS18B20_GetTemperature(void)
{
	float temp;
	uint16_t data = 0;
	uint8_t rdata[9];


	if(DS18B20_InitSeq()==false)return -128.0;
	DS18B20_WriteByte(SKIP_ROM_CMD_BYTE);
	DS18B20_WriteByte(READ_SCRATCHPAD_CMD);

	for(int i = 0; i < 9; i++)rdata[i]=DS18B20_ReadByte();


	uint8_t crc= DS18B20_CalcCRC((uint8_t*)&rdata, 8);

	data |= rdata[0];
	data |= rdata[1]<<8;


	if(data&0xF800)	temp = -(float)((~data+1)*0.0625);
	else temp = (float)(data*0.0625);

	if(DS18B20_StartConvert()==false)	return -128.0;



//	printf("Data: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x \n",
//			rdata[0],rdata[1],rdata[2],rdata[3],rdata[4],
//			rdata[5],rdata[6],rdata[7],rdata[8]);
//
//	printf("CRC: 0x%x, 0x%x \n",crc,rdata[8]);

	if(crc!=rdata[8])return -128.0;

	return temp;
}

float DS18B20_GetSelectDeviceTemperature(int8_t* addr)
{
	float temp;
	uint16_t data = 0;
	uint8_t rdata[9];

	if(DS18B20_InitSeq()==false)return -128.0;

	DS18B20_WriteByte(MATCH_ROM_CMD_BYTE);
	for(int i = 0; i < ADDRESS_SIZE; i++) DS18B20_WriteByte(addr[i]);

	DS18B20_WriteByte(READ_SCRATCHPAD_CMD);

	for(int i = 0; i < 9; i++)rdata[i]=DS18B20_ReadByte();


	uint8_t crc= DS18B20_CalcCRC((uint8_t*)&rdata, 8);

	data |= rdata[0];
	data |= rdata[1]<<8;

	if(data&0xF800)	temp = -(float)((~data+1)*0.0625);
	else temp = (float)(data*0.0625);

	if(DS18B20_StartConvert()==false)	return -128.0;

	if(crc!=rdata[8])return -128.0;

	return temp;


}

void DS18B20_ResetDeviceSearchInfo()
{
	searchNode = -1;
	searched = false;
    for (int i = 0; i < ADDRESS_SIZE; i++)
    {
    	ds18b20_addr[i] = 0;
    }
}

int8_t DS18B20_DeviceSearch(int8_t* newAddr)
{
	int8_t i;
	int8_t done = true;
	int16_t lastNode = -1;
	

	if (searched) return 0;

	if (DS18B20_InitSeq()==false) return 0;

	DS18B20_WriteByte(SEARCH_ROM_CMD_BYTE);

	for(i = 0; i < 64; i++)
	{
		int8_t tmp1 = DS18B20_ReadBit( );
		int8_t tmp2 = DS18B20_ReadBit( );
		int8_t tmp_byte = i/8;
		int8_t tmp_bit = 1 << (i & 7);

		if (tmp1 && tmp2) return 0;

		if (!tmp1 && !tmp2)
		{
			if (i == searchNode)
			{
				tmp1 = 1;
				searchNode = lastNode;
			}
			else if (i < searchNode)
			{
				if (ds18b20_addr[tmp_byte] & tmp_bit) tmp1 = 1;
				else
				{
					tmp1 = 0;
					done = false;
					lastNode = i;
				}
			}
			else
			{
				tmp1 = 0;
				searchNode = i;
				done = false;
			}
			lastNode = i;
		}

		if (tmp1) ds18b20_addr[tmp_byte] |= tmp_bit;
		else ds18b20_addr[tmp_byte] &= ~tmp_bit;

		DS18B20_WriteBit(tmp1);
	}

	if (done==true) searched = true;

	for (i = 0; i < ADDRESS_SIZE; i++) newAddr[i] = ds18b20_addr[i];

	return 1;
}



uint8_t DS18B20_CalcCRC(uint8_t* addr, uint8_t len){

	uint8_t i, j;
	uint8_t crc = 0;

    for (i = 0; i < len; i++)
    {
    	uint8_t data = (uint8_t)addr[i];
        for (j = 0; j < 8; j++)
        {
        	uint8_t mix = (uint8_t)((crc ^ data) & 0x01);
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            data >>= 1;
        }
    }

    return crc;
}
