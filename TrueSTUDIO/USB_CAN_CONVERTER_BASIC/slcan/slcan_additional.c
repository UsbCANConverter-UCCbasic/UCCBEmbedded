/*
 * can_additional.c
 *
 *  Created on: Jun 30, 2016
 *      Author: PLLUJUR1
 */

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"
#include "slcan.h"

extern CAN_HandleTypeDef hcan;
extern void Error_Handler(void);

void CANInit(void)
{
	if (HAL_CAN_Init(&hcan) != HAL_OK)
	{
		Error_Handler();
	}
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
		default:
			break;
	}

	CANInit();
}

