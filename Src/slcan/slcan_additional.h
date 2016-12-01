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
	};
	uint32_t reg;
} tCANFilterFlagsConf;

typedef union{
	struct {
		uint32_t bRTR1 : 1;
		uint32_t bExtedned1 : 1;
		uint32_t bRTR2 : 1;
		uint32_t bExtedned2 : 1;
	};
	uint32_t reg;
} tCANFilterFlagsId;

typedef struct {
		uint16_t  EXID17_15 : 3;
		uint16_t  RTR : 1;
		uint16_t  IDE : 1;
		uint16_t  STID2_0 : 3;

		uint16_t  STID10_3 : 8;
}tCANreg16;


typedef struct {
	union {
		struct {
			uint16_t EXID17_13 : 5;
			uint16_t STID2_0   : 3;
			uint16_t STID10_3  : 8;
		} f32;
		uint16_t reg;
		tCANreg16 f16;
	}h;
	union {
		struct {
			uint16_t reserved : 1;
			uint16_t RTR : 1;
			uint16_t IDE : 1;
			uint16_t EXID4_0   : 5;
			uint16_t EXID12_5  : 8;
		} f32;
		uint16_t reg;
		tCANreg16 f16;
	}l;
}tCANfilter;

tCANfilter slcanFillIdRegister32(tCANFilterFlagsId fl, uint32_t id);
tCANfilter slcanFillIdRegister16(tCANFilterFlagsId fl, uint32_t id);
HAL_StatusTypeDef slcanClearAllFilters(void);

HAL_StatusTypeDef CANInit(void);
void slcanSetCANBaudRate(uint8_t br);

#endif /* SLCAN_ADDITIONAL_H_ */

