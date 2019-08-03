/*
 * slcan_interface.c
 *
 *  Created on: Apr 2, 2016
 *      Author: Vostro1440
 */

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"
#include "string.h"
#include "slcan.h"
#include "slcan_additional.h"
#include "usbd_cdc_if.h"

#define SLCAN_BELL 7
#define SLCAN_CR 13
#define SLCAN_LR 10

extern volatile int32_t serialNumber;
// internal slcan_interface state
static uint8_t state = STATE_CONFIG;
static uint8_t timestamping = 0;


static uint8_t terminator = SLCAN_CR;

extern CAN_HandleTypeDef hcan;

uint8_t sl_frame[32];
volatile uint8_t sl_frame_len=0;
/**
  * @brief  Adds data to send buffer
  * @param  c - data to add
  * @retval None
  */
static void slcanSetOutputChar(uint8_t c)
{
	if (sl_frame_len < sizeof(sl_frame))
	{
		sl_frame[sl_frame_len] = c;
		sl_frame_len ++;
	}
}

/**
  * @brief  Add given nible value as hexadecimal string to bufferr
  * @param  c - data to add
  * @retval None
  */
static void slCanSendNibble(uint8_t ch)
{
	ch = ch > 9 ? ch - 10 + 'A' : ch + '0';
	slcanSetOutputChar(ch);
}

/**
  * @brief  Add given byte value as hexadecimal string to buffer
  * @param  value - data to add
  * @retval None
  */
static void slcanSetOutputAsHex(uint8_t ch) {
	slCanSendNibble(ch >> 4);
	slCanSendNibble(ch & 0x0F);
}


extern UART_HandleTypeDef huart2;
/**
  * @brief  Flush data on active interface
  * @param  None
  * @retval None
  */
extern USBD_HandleTypeDef hUsbDeviceFS;


// frame buffer
//#define FRAME_BUFFER_SIZE 1024
//uint8_t frameBuffer[FRAME_BUFFER_SIZE];
//uint32_t dataToSend = 0;

void slcanClose()
{
	HAL_NVIC_DisableIRQ(CEC_CAN_IRQn);
//	dataToSend = 0;
//            	todo into slleep
	state = STATE_CONFIG;
}

void slcanOutputFlush(void)
{
	if (sl_frame_len > 0)
	{
//		if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) // use auxiliary uart only if usb not connected
//			HAL_UART_Transmit(&huart2,sl_frame,sl_frame_len,100); //ll todo figure out time
//		else
		{
			while (((USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData)->TxState){;} //should change by hardware
			while (CDC_Transmit_FS(sl_frame, sl_frame_len) != USBD_OK);
		}
		sl_frame_len = 0;
	}

}

/**
  * @brief  Add to input buffer data from interfaces
  * @param  ch - data to add
  * @retval None
  */
uint8_t command[LINE_MAXLEN] = {0};

int slCanProccesInput(uint8_t ch)
{
	static uint8_t line[LINE_MAXLEN];
	static uint8_t linepos = 0;

    if (ch == SLCAN_CR) {
        line[linepos] = 0;
        memcpy(command,line,linepos);

        linepos = 0;
        return 1;
    } else if (ch != SLCAN_LR) {
        line[linepos] = ch;
        if (linepos < LINE_MAXLEN - 1) linepos++;
    }
    return 0;
}


/**
  * @brief  Parse hex value of given string
  * @param  canmsg - line Input string
  * 		len    - of characters to interpret
  * 		value  - Pointer to variable for the resulting decoded value
  * @retval 0 on error, 1 on success
  */
static uint8_t parseHex(uint8_t* line, uint8_t len, uint32_t* value) {
    *value = 0;
    while (len--) {
        if (*line == 0) return 0;
        *value <<= 4;
        if ((*line >= '0') && (*line <= '9')) {
           *value += *line - '0';
        } else if ((*line >= 'A') && (*line <= 'F')) {
           *value += *line - 'A' + 10;
        } else if ((*line >= 'a') && (*line <= 'f')) {
           *value += *line - 'a' + 10;
        } else return 0;
        line++;
    }
    return 1;
}

/**
 * @brief  Interprets given line and transmit can message
 * @param  line Line string which contains the transmit command
 * @retval HAL status
 */
static uint8_t transmitStd(uint8_t* line) {
    uint32_t temp;
    uint8_t idlen;
    HAL_StatusTypeDef tr;

    if ((line[0] == 'r') || (line[0] == 'R'))
    {
    	hcan.pTxMsg->RTR = CAN_RTR_REMOTE;
    } else
    {
    	hcan.pTxMsg->RTR = CAN_RTR_DATA;
    }

    // upper case -> extended identifier
    if (line[0] < 'Z') {
    	idlen = 8;
        if (!parseHex(&line[1], idlen, &temp)) return 0;
        hcan.pTxMsg->IDE = CAN_ID_EXT;
        hcan.pTxMsg->ExtId = temp;

    } else {
    	idlen = 3;
    	if (!parseHex(&line[1], idlen, &temp)) return 0;
		hcan.pTxMsg->IDE = CAN_ID_STD;
		hcan.pTxMsg->StdId = temp;
    }


    if (!parseHex(&line[1 + idlen], 1, &temp)) return 0;
    hcan.pTxMsg->DLC = temp;

    if (hcan.pTxMsg->RTR == CAN_RTR_DATA) {
    	uint8_t i;
        uint8_t length = hcan.pTxMsg->DLC;
        if (length > 8) length = 8;
        for (i = 0; i < length; i++) {
            if (!parseHex(&line[idlen + 2 + i*2], 2, &temp)) return 0;
            hcan.pTxMsg->Data[i] = temp;
        }
    }

    HAL_NVIC_DisableIRQ(CEC_CAN_IRQn);
    tr = HAL_CAN_Transmit(&hcan, 0);
    HAL_NVIC_EnableIRQ(CEC_CAN_IRQn);
    return tr;
}

void RebootToBootloader();
/**
 * @brief  Parse given command line
 * @param  line Line string to parse
 * @retval None
 */
uint8_t slCanCheckCommand(uint8_t *line)
{
	uint8_t result = SLCAN_BELL;
	if (line[0] == 0)
	{
		return 0;
	}
    switch (line[0]) {
    	case 'a':
    	{
    		if (terminator == SLCAN_CR)
    			terminator = SLCAN_LR;
    		else
    			terminator = SLCAN_CR;
    		result = terminator;
    		break;
    	}
        case 'S': // Setup with standard CAN bitrates
            if (state == STATE_CONFIG)
            {
                switch (line[1]) {
                    case '0': slcanSetCANBaudRate(CAN_BR_10K);  result = terminator; break;
                    case '1': slcanSetCANBaudRate(CAN_BR_20K);  result = terminator; break;
                    case '2': slcanSetCANBaudRate(CAN_BR_50K);  result = terminator; break;
                    case '3': slcanSetCANBaudRate(CAN_BR_100K); result = terminator; break;
                    case '4': slcanSetCANBaudRate(CAN_BR_125K); result = terminator; break;
                    case '5': slcanSetCANBaudRate(CAN_BR_250K); result = terminator; break;
                    case '6': slcanSetCANBaudRate(CAN_BR_500K); result = terminator; break;
                    case '7': slcanSetCANBaudRate(CAN_BR_800K); result = terminator; break;
                    case '8': slcanSetCANBaudRate(CAN_BR_1M);   result = terminator; break;
                    case '9': slcanSetCANBaudRate(CAN_BR_83K);   result = terminator; break;

                }
            }
            break;

        case 'G': // Read given MCP2515 register
        case 'W':
			result = terminator;
			break;

        case 's': // Setup with user defined timing settings for CNF1/CNF2/CNF3
            if (state == STATE_CONFIG)
            {
                uint32_t sjw, bs1, bs2, pre;
                if (parseHex(&line[1], 2, &sjw) && parseHex(&line[3], 2, &bs1) &&
                		parseHex(&line[5], 2, &bs2) && parseHex(&line[7], 4, &pre) ) {

                	hcan.Init.SJW = sjw;
                	hcan.Init.BS1 = bs1;
                	hcan.Init.BS2 = bs2;
                	hcan.Init.Prescaler = pre;
                	CANInit();
                    result = terminator;
                }
            }
            break;
        case 'V': // Get hardware version
            {

                slcanSetOutputChar('V');
                slcanSetOutputAsHex(VERSION_HARDWARE_MAJOR);
                slcanSetOutputAsHex(VERSION_HARDWARE_MINOR);
                result = terminator;
            }
            break;
        case 'v': // Get firmware version
            {

                slcanSetOutputChar('v');
                slcanSetOutputAsHex(VERSION_FIRMWARE_MAJOR);
                slcanSetOutputAsHex(VERSION_FIRMWARE_MINOR);
                result = terminator;
            }
            break;
        case 'N': // Get serial number
            {
                slcanSetOutputChar('N');
                slcanSetOutputAsHex((uint8_t)(serialNumber));
                slcanSetOutputAsHex((uint8_t)(serialNumber>>8));
                slcanSetOutputAsHex((uint8_t)(serialNumber>>16));
                slcanSetOutputAsHex((uint8_t)(serialNumber>>24));
                result = terminator;
            }
            break;
        case 'o':
        case 'O': // Open CAN channel
//            if (state == STATE_CONFIG)
            {
            	hcan.Init.Mode = CAN_MODE_NORMAL;

            	if(CANInit() == HAL_OK)
            	{
					HAL_NVIC_EnableIRQ(CEC_CAN_IRQn);
	//                clock_reset();
					state = STATE_OPEN;
					result = terminator;
            	}
            }
            break;
        case 'l': // Loop-back mode
//            if (state == STATE_CONFIG)
            {
            	hcan.Init.Mode = CAN_MODE_LOOPBACK;
            	HAL_NVIC_EnableIRQ(CEC_CAN_IRQn);
            	CANInit();
                state = STATE_OPEN;
                result = terminator;
            }
            break;
        case 'L': // Open CAN channel in listen-only mode
            if (state == STATE_CONFIG)
            {
            	hcan.Init.Mode = CAN_MODE_SILENT;
            	HAL_NVIC_EnableIRQ(CEC_CAN_IRQn);
            	CANInit();
                state = STATE_LISTEN;
                result = terminator;
            }
            break;
        case 'C': // Close CAN channel
//            if (state != STATE_CONFIG)
            {
            	HAL_NVIC_DisableIRQ(CEC_CAN_IRQn);
//            	dataToSend = 0;
//            	todo into slleep
                state = STATE_CONFIG;
                result = terminator;
            }
            break;
        case 'r': // Transmit standard RTR (11 bit) frame
        case 'R': // Transmit extended RTR (29 bit) frame
        case 't': // Transmit standard (11 bit) frame
        case 'T': // Transmit extended (29 bit) frame
            if (state == STATE_OPEN)
            {
                if (transmitStd(line) == HAL_OK) {
                    if (line[0] < 'Z') slcanSetOutputChar('Z');
                    else slcanSetOutputChar('z');
                    result = terminator;
                }
            }
            break;
        case 'F': // Read status flags
            {
                unsigned char status = HAL_CAN_GetError(&hcan);
//                unsigned char flags = HAL_CAN_GetError(&hcan);
//                if (flags & 0x01) status |= 0x04; // error warning
//                if (flags & 0xC0) status |= 0x08; // data overrun
//                if (flags & 0x18) status |= 0x20; // passive error
//                if (flags & 0x20) status |= 0x80; // bus error
                slcanSetOutputChar('F');
                slcanSetOutputAsHex(status);
                result = terminator;
            }
            break;
         case 'Z': // Set time stamping
            {
                unsigned long stamping;
                if (parseHex(&line[1], 1, &stamping)) {
                    timestamping = (stamping != 0);
                    result = terminator;
                }
            }
            break;
         case 'm': // Set accpetance filter mask
         case 'M': // Set accpetance filter code
            if (line[1] == 'd') // disable all filtering
            {
            	slcanClearAllFilters();
            	result = terminator;
            	slcanSetOutputChar('M');
            } else
            {
            	CAN_FilterConfTypeDef sFilterConfig;
            	tCANfilter freg,mreg;
            	uint32_t n,b;
            	tCANFilterFlagsConf fflags;
            	tCANFilterFlagsId idflags,maskflags;
            	uint32_t id,mask;

            	if (!parseHex(&line[1], 2, &n)) break;
            	if (!parseHex(&line[3], 2, &b)) break;
            	if (!parseHex(&line[5], 1, &fflags.reg)) break;
            	if (!parseHex(&line[6], 8, &id)) break;
            	if (!parseHex(&line[14], 1, &idflags.reg)) break;
            	if (!parseHex(&line[15], 8, &mask)) break;
            	if (!parseHex(&line[23], 1, &maskflags.reg)) break;

            	if (fflags.bScale == CAN_FILTERSCALE_32BIT)
            	{
					freg = slcanFillIdRegister32(idflags, id);
					mreg = slcanFillIdRegister32(maskflags, mask);
            	} else
            	{
            		freg = slcanFillIdRegister16(idflags, id);
            		mreg = slcanFillIdRegister16(maskflags, mask);
            	}

            	sFilterConfig.FilterNumber = n;
            	sFilterConfig.BankNumber = b;
            	sFilterConfig.FilterActivation = fflags.bFilterActivation; // ENABLE == 1
				sFilterConfig.FilterMode = fflags.bMode; // CAN_FILTERMODE_IDLIST == 1
				sFilterConfig.FilterScale = fflags.bScale;  // CAN_FILTERSCALE_32BIT == 1
				sFilterConfig.FilterFIFOAssignment = fflags.bFIFO;
				sFilterConfig.FilterIdHigh = freg.h.reg;
				sFilterConfig.FilterIdLow = freg.l.reg;
				sFilterConfig.FilterMaskIdHigh = mreg.h.reg;
				sFilterConfig.FilterMaskIdLow = mreg.l.reg;

				slcanSetOutputChar('M');
				if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) == HAL_OK)
					result = terminator;
            }
            break;
         case 'b':
        	 if ((line[1] == 'o') && (line[2] == 'o') && (line[3] == 't')){
        		 line[0] = 0;
        		 line[1] = 0;
        		 line[2] = 0;
        		 line[3] = 0;
        		 RebootToBootloader();
        	 }
        	 break;

    }

   line[0] = 0;
   slcanSetOutputChar(result);
   return 1;
}

/**
 * @brief  get slcan state
 * @param  none
 * @retval slcan state
 */
uint8_t slcan_getState()
{
	return state;
}

/**
 * @brief  reciving CAN frame
 * @param  canmsg Pointer to can message
 * 			step Current step
 * @retval Next character to print out
 */
uint8_t slcanReciveCanFrame(CanRxMsgTypeDef *pRxMsg)
{
	uint8_t i;

	// type
	if (pRxMsg->IDE == CAN_ID_EXT) {
		if (pRxMsg->RTR == CAN_RTR_REMOTE)
		{
			slcanSetOutputChar('R');
		}
		else
		{
			slcanSetOutputChar('T');
		}
		// id
		for (i = 4; i != 0; i--)
		{
			slcanSetOutputAsHex(((uint8_t*)&pRxMsg->ExtId)[i - 1]);
		}
	} else
	{
		if (pRxMsg->RTR == CAN_RTR_REMOTE)
		{
			slcanSetOutputChar('r');
		}
		else
		{
			slcanSetOutputChar('t');
		}
		//id
		slCanSendNibble(((uint8_t*)&pRxMsg->StdId)[1] & 0x0F);
		slcanSetOutputAsHex(((uint8_t*)&pRxMsg->StdId)[0]);
	}
	// length
	slCanSendNibble(pRxMsg->DLC);

	//data
	if ((pRxMsg->DLC > 0) && (pRxMsg->RTR != CAN_RTR_REMOTE))
	{
		for (i = 0;  i != pRxMsg->DLC; i ++)
		{
			slcanSetOutputAsHex(pRxMsg->Data[i]);
		}
	}
	slcanSetOutputChar(terminator);
	slcanOutputFlush();
	return 0;
}
