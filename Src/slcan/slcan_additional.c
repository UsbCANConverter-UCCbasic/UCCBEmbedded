/*
 * can_additional.c
 *
 *  Created on: Jun 30, 2016
 *      Author: PLLUJUR1
 */

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"
#include "slcan.h"
#include "slcan_additional.h"

extern CAN_HandleTypeDef hcan;
extern IWDG_HandleTypeDef hiwdg;
HAL_StatusTypeDef CANInit(void)
{
    while (HAL_CAN_Init(&hcan) == HAL_TIMEOUT)
    {
    	HAL_IWDG_Refresh(&hiwdg);
    }
    return HAL_OK;
}

HAL_StatusTypeDef slcanClearAllFilters(void)
{
	CAN_FilterConfTypeDef sFilterConfig;
	sFilterConfig.FilterNumber = 0;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = 0x0000;
	sFilterConfig.FilterIdLow = 0;
	sFilterConfig.FilterMaskIdHigh = 0x0000;
	sFilterConfig.FilterMaskIdLow = 0;
	sFilterConfig.FilterFIFOAssignment = 0;
	sFilterConfig.FilterActivation = ENABLE;
	sFilterConfig.BankNumber = 0;

	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
		return HAL_ERROR;
	else
		return HAL_OK;
}

tCANfilter slcanFillIdRegister32(tCANFilterFlagsId fl, uint32_t id)
{
	tCANfilter f;
	f.h.reg = 0;
	f.l.reg = 0;

	f.l.f32.RTR = fl.bRTR1;
	f.l.f32.IDE = fl.bExtedned1;
	if (fl.bExtedned1)
	{
		f.l.f32.EXID4_0 = id;
		f.l.f32.EXID12_5 = id >> 5;
		f.h.f32.EXID17_13 = id >> 13;
	} else {
		f.h.f32.STID2_0 = id;
		f.h.f32.STID10_3 = id >> 3;
	}
	return f;
}

tCANfilter slcanFillIdRegister16(tCANFilterFlagsId fl, uint32_t id)
{
	tCANfilter f;
	f.h.reg = 0;
	f.l.reg = 0;

	f.l.f16.RTR = fl.bRTR1;
	f.l.f16.IDE = fl.bExtedned1;
	if (fl.bExtedned1)
	{
		f.l.f16.STID2_0 = id;
		f.l.f16.STID10_3 = id >> 3;
		f.l.f16.EXID17_15 = id >> 8;
	}

	f.l.f16.RTR = fl.bRTR2;
	f.l.f16.IDE = fl.bExtedned2;
	if (fl.bExtedned2)
	{
		f.h.f16.STID2_0 = id >> 16;
		f.h.f16.STID10_3 = id >> (3 + 16);
		f.h.f16.EXID17_15 = id >> (8 + 16);
	}

	return f;
}

void slcanSetCANBaudRate(uint8_t br)
{ //todo it is for 75% sampling point

	hcan.Init.SJW = CAN_SJW_2TQ;
	hcan.Init.BS1 = CAN_BS1_11TQ;
	hcan.Init.BS2 = CAN_BS2_4TQ;

	switch (br)
	{
		case CAN_BR_1M:
			hcan.Init.Prescaler = 3;
		break;
		case CAN_BR_500K:
			hcan.Init.Prescaler = 6;
		break;
		case CAN_BR_250K:
			hcan.Init.Prescaler = 12;
		break;
		case CAN_BR_125K:
			hcan.Init.Prescaler = 24;
		break;
		case CAN_BR_100K:
			hcan.Init.Prescaler = 30;
		break;
		case CAN_BR_50K:
			hcan.Init.Prescaler = 60;
		break;
		case CAN_BR_20K:
			hcan.Init.Prescaler = 150;
		break;
		case CAN_BR_10K:
			hcan.Init.Prescaler = 300;
		break;
		case CAN_BR_83K:
			hcan.Init.Prescaler = 36;
		break;

		default:
			break;
	}

	CANInit();
}

