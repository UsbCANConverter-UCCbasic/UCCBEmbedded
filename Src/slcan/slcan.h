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



#define VERSION_FIRMWARE_MAJOR 2
#define VERSION_FIRMWARE_MINOR 3

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

//ex TZ12020506

void slcanClose();
//int slcanFlushUSBBuffer();
uint8_t slcanReciveCanFrame(CanRxMsgTypeDef *pRxMsg);
int slCanProccesInput(uint8_t ch);
void slCanCheckCommand();


#endif /* SLCAN_PORT_H_ */
