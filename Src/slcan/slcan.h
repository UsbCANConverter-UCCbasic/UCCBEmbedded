/*
 * slcan_port.h
 *
 *  Created on: Apr 4, 2016
 *      Author: PLLUJUR1
 */

#ifndef SLCAN_PORT_H_
#define SLCAN_PORT_H_

#include "stdint.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"


#define VERSION_FIRMWARE_MAJOR 4
#define VERSION_FIRMWARE_MINOR 1

#define VERSION_HARDWARE_MAJOR 1
#define VERSION_HARDWARE_MINOR 1

#define CAN_BR_10K 0
#define CAN_BR_20K 1
#define CAN_BR_50K 2
#define CAN_BR_100K 3
#define CAN_BR_125K 4
#define CAN_BR_250K 5
#define CAN_BR_500K 6
#define CAN_BR_800K 7
#define CAN_BR_1M 8
#define CAN_BR_83K 36

#define LINE_MAXLEN 100

#define STATE_CONFIG 0
#define STATE_LISTEN 1
#define STATE_OPEN 2

void slcanOutputFlush(void);
void slcanClose();
uint8_t slcanReciveCanFrame(CanRxMsgTypeDef *pRxMsg);
int slCanProccesInput(uint8_t ch);
uint8_t slCanCheckCommand(uint8_t *line);
uint8_t slcan_getState(void);

extern uint8_t command[LINE_MAXLEN];
extern uint8_t lastInputBuffer;

#endif /* SLCAN_PORT_H_ */
