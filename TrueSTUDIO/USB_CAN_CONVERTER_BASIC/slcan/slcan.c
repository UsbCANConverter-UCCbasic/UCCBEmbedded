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
#include "usbd_cdc_if.h"

#define LINE_MAXLEN 100
#define SLCAN_BELL 7
#define SLCAN_CR 13
#define SLCAN_LR 10

#define STATE_CONFIG 0
#define STATE_LISTEN 1
#define STATE_OPEN 2

uint32_t serialNumber = 0x01040816;
// internal slcan_interface state
static uint8_t state = STATE_CONFIG;
static uint8_t timestamping = 0;

static void slcanProccessInput(uint8_t* line);
extern CAN_HandleTypeDef hcan;

void slcanSetCANBaudRate(uint8_t br)
{ //todo it is for 75% sampling point
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

	if (HAL_CAN_Init(&hcan) != HAL_OK)
	{
		// todo Error handler
	}
}

uint8_t sl_frame[32];
uint8_t sl_frame_len=0;
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
static void slcanOutputFlush(void)
{
	CDC_Transmit_FS(sl_frame, sl_frame_len);
	HAL_UART_Transmit(&huart2,sl_frame,sl_frame_len,100); //ll todo figure out time
	sl_frame_len = 0;
}

/**
  * @brief  Add to input buffer data from interfaces
  * @param  ch - data to add
  * @retval None
  */
void slCanProccesInput(uint8_t ch)
{
	static uint8_t line[LINE_MAXLEN];
	static uint8_t linepos = 0;

    if (ch == SLCAN_CR) {
        line[linepos] = 0;
        slcanProccessInput(line);
        linepos = 0;
    } else if (ch != SLCAN_LR) {
        line[linepos] = ch;
        if (linepos < LINE_MAXLEN - 1) linepos++;
    }
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


    hcan.pTxMsg->RTR = ((line[0] == 'r') || (line[0] == 'R'));

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

    if (!hcan.pTxMsg->RTR) {
    	uint8_t i;
        uint8_t length = hcan.pTxMsg->DLC;
        if (length > 8) length = 8;
        for (i = 0; i < length; i++) {
            if (!parseHex(&line[idlen + 2 + i*2], 2, &temp)) return 0;
            hcan.pTxMsg->Data[i] = temp;
        }
    }

    return HAL_CAN_Transmit(&hcan, 1000);
}


/**
 * @brief  Parse given command line
 * @param  line Line string to parse
 * @retval None
 */
static void slcanProccessInput(uint8_t* line)
{
	uint8_t result = SLCAN_BELL;

    switch (line[0]) {
        case 'S': // Setup with standard CAN bitrates
            if (state == STATE_CONFIG)
            {
                switch (line[1]) {
                    case '0': slcanSetCANBaudRate(CAN_BR_10K);  result = SLCAN_CR; break;
                    case '1': slcanSetCANBaudRate(CAN_BR_20K);  result = SLCAN_CR; break;
                    case '2': slcanSetCANBaudRate(CAN_BR_50K);  result = SLCAN_CR; break;
                    case '3': slcanSetCANBaudRate(CAN_BR_100K); result = SLCAN_CR; break;
                    case '4': slcanSetCANBaudRate(CAN_BR_125K); result = SLCAN_CR; break;
                    case '5': slcanSetCANBaudRate(CAN_BR_250K); result = SLCAN_CR; break;
                    case '6': slcanSetCANBaudRate(CAN_BR_500K); result = SLCAN_CR; break;
                    case '7': slcanSetCANBaudRate(CAN_BR_800K); result = SLCAN_CR; break;
                    case '8': slcanSetCANBaudRate(CAN_BR_1M);   result = SLCAN_CR; break;
                }
            }
            break;
        case 's': // Setup with user defined timing settings for CNF1/CNF2/CNF3
            if (state == STATE_CONFIG)
            {
                unsigned long cnf1, cnf2, cnf3;
                if (parseHex(&line[1], 2, &cnf1) && parseHex(&line[3], 2, &cnf2) && parseHex(&line[5], 2, &cnf3)) {
//                    mcp2515_set_bittiming(cnf1, cnf2, cnf3);
                    result = SLCAN_CR;
                }
            }
            break;
        case 'G': // Read given MCP2515 register
            {
                unsigned long address;
                if (parseHex(&line[1], 2, &address)) {
                    unsigned char value = 0;//mcp2515_read_register(address);
                    slcanSetOutputAsHex(value);
                    result = SLCAN_CR;
                }
            }
            break;
        case 'W': // Write given MCP2515 register
            {
                unsigned long address, data;
                if (parseHex(&line[1], 2, &address) && parseHex(&line[3], 2, &data)) {
//                    mcp2515_write_register(address, data);
                    result = SLCAN_CR;
                }

            }
            break;
        case 'V': // Get hardware version
            {

                slcanSetOutputChar('V');
                slcanSetOutputAsHex(VERSION_HARDWARE_MAJOR);
                slcanSetOutputAsHex(VERSION_HARDWARE_MINOR);
                result = SLCAN_CR;
            }
            break;
        case 'v': // Get firmware version
            {

                slcanSetOutputChar('v');
                slcanSetOutputAsHex(VERSION_FIRMWARE_MAJOR);
                slcanSetOutputAsHex(VERSION_FIRMWARE_MINOR);
                result = SLCAN_CR;
            }
            break;
        case 'N': // Get serial number
            {
                slcanSetOutputChar('N');
                slcanSetOutputAsHex((uint8_t)serialNumber);
                slcanSetOutputAsHex((uint8_t)serialNumber>>8);
                slcanSetOutputAsHex((uint8_t)serialNumber>>16);
                slcanSetOutputAsHex((uint8_t)serialNumber>>24);
                result = SLCAN_CR;
            }
            break;
        case 'O': // Open CAN channel
            if (state == STATE_CONFIG)
            {
            	hcan.Init.Mode = CAN_MODE_NORMAL;
            	if (HAL_CAN_Init(&hcan) != HAL_OK)
            	{
            		// todo Error handler
            	}
//                clock_reset();
                state = STATE_OPEN;
                result = SLCAN_CR;
            }
            break;
        case 'l': // Loop-back mode
            if (state == STATE_CONFIG)
            {
            	hcan.Init.Mode = CAN_MODE_LOOPBACK;
				if (HAL_CAN_Init(&hcan) != HAL_OK)
				{
					// todo Error handler
				}
                state = STATE_OPEN;
                result = SLCAN_CR;
            }
            break;
        case 'L': // Open CAN channel in listen-only mode
            if (state == STATE_CONFIG)
            {
            	hcan.Init.Mode = CAN_MODE_SILENT;
				if (HAL_CAN_Init(&hcan) != HAL_OK)
				{
					// todo Error handler
				}
                state = STATE_LISTEN;
                result = SLCAN_CR;
            }
            break;
        case 'C': // Close CAN channel
            if (state != STATE_CONFIG)
            {
//            	mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x80); // set configuration mode
                state = STATE_CONFIG;
                result = SLCAN_CR;
            }
            break;
        case 'r': // Transmit standard RTR (11 bit) frame
        case 'R': // Transmit extended RTR (29 bit) frame
        case 't': // Transmit standard (11 bit) frame
        case 'T': // Transmit extended (29 bit) frame
//            if (state == STATE_OPEN) todo Przywrocic OPEN
            {
                if (transmitStd(line+1)) {
                    if (line[0] < 'Z') slcanSetOutputChar('Z');
                    else slcanSetOutputChar('z');
                    result = SLCAN_CR;
                }

            }
            break;
        case 'F': // Read status flags
            {
                unsigned char flags = 0;//mcp2515_read_register(MCP2515_REG_EFLG);
                unsigned char status = 0;

                if (flags & 0x01) status |= 0x04; // error warning
                if (flags & 0xC0) status |= 0x08; // data overrun
                if (flags & 0x18) status |= 0x20; // passive error
                if (flags & 0x20) status |= 0x80; // bus error

                slcanSetOutputChar('F');
                slcanSetOutputAsHex(status);
                result = SLCAN_CR;
            }
            break;
         case 'Z': // Set time stamping
            {
                unsigned long stamping;
                if (parseHex(&line[1], 1, &stamping)) {
                    timestamping = (stamping != 0);
                    result = SLCAN_CR;
                }
            }
            break;
         case 'm': // Set accpetance filter mask
            if (state == STATE_CONFIG)
            {
                unsigned long am0, am1, am2, am3;
//                    mcp2515_set_SJA1000_filter_mask(am0, am1, am2, am3);
//                if (parseHex(&line[1], 2, &am0) && parseHex(&line[3], 2, &am1) && parseHex(&line[5], 2, &am2) && parseHex(&line[7], 2, &am3)) {
                    result = SLCAN_CR;
//                }
            }
            break;
         case 'M': // Set accpetance filter code
            if (state == STATE_CONFIG)
            {
                unsigned long ac0, ac1, ac2, ac3;
//                if (parseHex(&line[1], 2, &ac0) && parseHex(&line[3], 2, &ac1) && parseHex(&line[5], 2, &ac2) && parseHex(&line[7], 2, &ac3)) {
//                    mcp2515_set_SJA1000_filter_code(ac0, ac1, ac2, ac3);
                    result = SLCAN_CR;
//                }
            }
            break;

    }

   slcanSetOutputChar(result);
   slcanOutputFlush();
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
	if (pRxMsg->ExtId == CAN_ID_EXT) {
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
	slcanSetOutputChar(SLCAN_CR);
	slcanOutputFlush();
	return 0;
}
