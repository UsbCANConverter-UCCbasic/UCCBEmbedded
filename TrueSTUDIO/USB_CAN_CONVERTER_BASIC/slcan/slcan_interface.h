/*
 * slcan_interface.h
 *
 *  Created on: Apr 2, 2016
 *      Author: Vostro1440
 */

#ifndef SLCAN_INTERFACE_H_
#define SLCAN_INTERFACE_H_

#define LINE_MAXLEN 100

#define CAN_BR_10K 0
#define CAN_BR_20K 1
#define CAN_BR_50K 2
#define CAN_BR_100K 3
#define CAN_BR_125K 4
#define CAN_BR_250K 5
#define CAN_BR_500K 6
#define CAN_BR_800K 7
#define CAN_BR_1M 8

#define RX_STEP_TYPE 0
#define RX_STEP_ID_EXT 1
#define RX_STEP_ID_STD 6
#define RX_STEP_DLC 9
#define RX_STEP_DATA 10
#define RX_STEP_TIMESTAMP 26
#define RX_STEP_CR 30
#define RX_STEP_FINISHED 0xff

#define SLCAN_BELL 7
#define SLCAN_CR 13
#define SLCAN_LR 10

#include "stdint.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"

typedef struct
{
    uint32_t id; 				// identifier (11 or 29 bit)
    struct {
    	uint8_t rtr : 1;		// remote transmit request
    	uint8_t extended : 1;	// extended identifier
    } flags;

    uint8_t dlc;                  // data length code
    uint8_t data[8];			  // payload data
    uint32_t timestamp;           // timestamp
} slcan_canmsg_t;

void slcanProccessInput(uint8_t* line);
char slcanReciveCanFrame(CanRxMsgTypeDef *pRxMsg);

#endif /* SLCAN_INTERFACE_H_ */
