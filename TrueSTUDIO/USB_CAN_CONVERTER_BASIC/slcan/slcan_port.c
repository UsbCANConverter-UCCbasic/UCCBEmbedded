/*
 * slcan_interface.c
 *
 *  Created on: Apr 2, 2016
 *      Author: Vostro1440
 */

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"
#include "string.h"

#include "slcan_port.h"
#include "slcan_interface.h"
#include "usbd_cdc_if.h"


void slcanSetCANBaudRate(uint8_t br)
{

}

/* send  UART input - used by slcan_interface*/
void slcanSetOutputChar(uint8_t c)
{
	CDC_Transmit_FS(&c, 1);
}

/* reciving UART input - used in main application*/
void slCanProccesInput(uint8_t ch)
{
	static uint8_t line[LINE_MAXLEN];
	uint8_t linepos = 0;

    if (ch == SLCAN_CR) {
        line[linepos] = 0;
        slcanProccessInput(line);
        linepos = 0;
    } else if (ch != SLCAN_LR) {
        line[linepos] = ch;
        if (linepos < LINE_MAXLEN - 1) linepos++;
    }
}


extern CAN_HandleTypeDef hcan;
/* sending CAN frame - used by slcan_interface*/
uint8_t slcanSendCanFrame(slcan_canmsg_t* canmsg)
{
	hcan.pTxMsg->DLC = canmsg->dlc;
	if (canmsg->flags.extended)
	{
		hcan.pTxMsg->IDE = CAN_ID_EXT;
		hcan.pTxMsg->ExtId = canmsg->id;
	} else
	{
		hcan.pTxMsg->IDE = CAN_ID_STD;
		hcan.pTxMsg->StdId = canmsg->id;
	}
	hcan.pTxMsg->RTR = canmsg->flags.rtr;
	memcpy(hcan.pTxMsg->Data,canmsg->data,canmsg->dlc);

	return HAL_CAN_Transmit(&hcan, 1000);
}

/* reciving CAN frame - called in main aplication*/
void  slcanReciveCanFrame()
{
	slcan_canmsg_t canmsg;
	uint8_t step = RX_STEP_TYPE;

	canmsg.dlc = hcan.pRxMsg->DLC;
	if (hcan.pRxMsg->IDE == CAN_ID_EXT)
	{
		canmsg.id = hcan.pRxMsg->ExtId;
		canmsg.flags.extended = 1;
	} else
	{
		canmsg.id = hcan.pRxMsg->StdId;
		canmsg.flags.extended = 0;
	}
	canmsg.flags.rtr = (hcan.pRxMsg->RTR == CAN_RTR_REMOTE) ? 1 : 0;

	// canmsg.timestamp
	memcpy(canmsg.data, hcan.pRxMsg->Data, canmsg.dlc);

	slcanSetOutputChar(slCanCanmsg2asciiGetNextChar(&canmsg, &step));

	if (step == RX_STEP_FINISHED)
	{
		step = RX_STEP_TYPE;
	}
}
