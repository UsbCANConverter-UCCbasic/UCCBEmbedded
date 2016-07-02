/*
 * slcan_additional.h
 *
 *  Created on: Jun 30, 2016
 *      Author: PLLUJUR1
 */

#ifndef SLCAN_ADDITIONAL_H_
#define SLCAN_ADDITIONAL_H_


typedef union {
	struct
	{
		uint32_t bFilterActivation : 1;
		uint32_t bMode : 1;
		uint32_t bScale : 1;
		uint32_t bFIFO : 1;

		uint32_t bRTR : 1;
		uint32_t bExtedned : 1;
		uint32_t bReserved : 2;
	};
	uint32_t byte;
} tFilterFlags;

typedef struct {
	union {
		struct {
			uint16_t EXID17_13 : 5;
			uint16_t STID2_0   : 3;
			uint16_t STID10_3  : 8;
		};
		uint16_t reg;
	}h;
	union {
		struct {
			uint16_t reserved : 1;
			uint16_t RTR : 1;
			uint16_t IDE : 1;
			uint16_t EXID4_0   : 5;
			uint16_t EXID12_5  : 8;
		};
		uint16_t reg;
	}l;
}tCANfilter32;

tCANfilter32 slcanFillIdRegister(tFilterFlags flags, uint32_t id);
void CANInit(void);
void slcanSetCANBaudRate(uint8_t br);

#endif /* SLCAN_ADDITIONAL_H_ */

