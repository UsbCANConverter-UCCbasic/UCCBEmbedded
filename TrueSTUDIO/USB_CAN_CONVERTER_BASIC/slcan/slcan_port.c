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
void slcanSendAsHex(uint8_t ch)
{
	uint8_t chl = ch & 0x0F;
	chl = chl > 9 ? chl - 10 + 'A' : chl + '0';

	uint8_t chh = ch >> 4;
	chh = chh > 9 ? chh - 10 + 'A' : chh + '0';

	slcanSetOutputChar(chh);
	slcanSetOutputChar(chl);

}
/* send  UART input - used by slcan_interface*/
extern UART_HandleTypeDef huart2;
void slcanSetOutputChar(uint8_t c)
{
	CDC_Transmit_FS(&c, 1);
	HAL_UART_Transmit(&huart2,&c,1,100);
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
