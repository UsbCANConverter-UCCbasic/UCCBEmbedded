/*
 * slcan_port.h
 *
 *  Created on: Apr 4, 2016
 *      Author: PLLUJUR1
 */

#ifndef SLCAN_PORT_H_
#define SLCAN_PORT_H_


#include "slcan_interface.h"

#define VERSION_FIRMWARE_MAJOR 2
#define VERSION_FIRMWARE_MINOR 1

#define VERSION_HARDWARE_MAJOR 1
#define VERSION_HARDWARE_MINOR 2




void slcanSetCANBaudRate(uint8_t br);
void slcanSetOutputChar(uint8_t c);
void slCanProccesInput(uint8_t ch);
uint8_t slcanSendCanFrame(slcan_canmsg_t* canmsg);
void  slcanReciveCanFrame();


#endif /* SLCAN_PORT_H_ */
