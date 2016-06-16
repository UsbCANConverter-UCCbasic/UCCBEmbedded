/*
 * slcan_interface.c
 *
 *  Created on: Apr 2, 2016
 *      Author: Vostro1440
 */

#include "slcan_interface.h"
#include "slcan_port.h"


#define STATE_CONFIG 0
#define STATE_LISTEN 1
#define STATE_OPEN 2
// internal slcan_interface state
static uint8_t state = STATE_CONFIG;
static uint8_t timestamping = 0;



/**
 * Parse hex value of given string
 *
 * @param line Input string
 * @param len Count of characters to interpret
 * @param value Pointer to variable for the resulting decoded value
 * @return 0 on error, 1 on success
 */
uint8_t parseHex(uint8_t* line, uint8_t len, uint32_t* value) {
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
 * Send given value as hexadecimal string
 *
 * @param value Value to send as hex over the UART
 * @param len Count of characters to produce
 */
void sendHex(uint32_t value, uint8_t len) {

    char s[20];
    char *p = s;
    s[len] = 0;

    if (len > sizeof(s))
    {
    	return;
    }

    while (len--) {

        unsigned char hex = value & 0x0f;
        if (hex > 9) hex = hex - 10 + 'A';
        else hex = hex + '0';
        s[len] = hex;

        value = value >> 4;
    }


    while (p)
    {
    	slcanSetOutputChar(*p);
    	p ++;
    }
    slcanSetOutputChar(0);
}

/**
 * Send given byte value as hexadecimal string
 *
 * @param value Byte value to send over UART
 */
void sendByteHex(uint8_t value) {

//    sendHex(value, 2);

	uint8_t ch = value >> 4;
    if (ch > 9) ch = ch - 10 + 'A';
    else ch = ch + '0';
    slcanSetOutputChar(ch);

    ch = value & 0xF;
    if (ch > 9) ch = ch - 10 + 'A';
    else ch = ch + '0';
    slcanSetOutputChar(ch);
}


/**
 * Interprets given line and transmit can message
 *
 * @param line Line string which contains the transmit command
 */
uint8_t transmitStd(uint8_t* line) {
	slcan_canmsg_t canmsg;
    uint32_t temp;
    uint8_t idlen;

    canmsg.flags.rtr = ((line[0] == 'r') || (line[0] == 'R'));

    // upper case -> extended identifier
    if (line[0] < 'Z') {
        canmsg.flags.extended = 1;
        idlen = 8;
    } else {
        canmsg.flags.extended = 0;
        idlen = 3;
    }

    if (!parseHex(&line[1], idlen, &temp)) return 0;
    canmsg.id = temp;

    if (!parseHex(&line[1 + idlen], 1, &temp)) return 0;
    canmsg.dlc = temp;

    if (!canmsg.flags.rtr) {
        unsigned char i;
        unsigned char length = canmsg.dlc;
        if (length > 8) length = 8;
        for (i = 0; i < length; i++) {
            if (!parseHex(&line[idlen + 2 + i*2], 2, &temp)) return 0;
            canmsg.data[i] = temp;
        }
    }

    return slcanSendCanFrame(&canmsg);
}

/**
 * Parse given command line
 *
 * @param line Line string to parse
 */
void slcanProccessInput(uint8_t* line)
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
                    sendByteHex(value);
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
                sendByteHex(VERSION_HARDWARE_MAJOR);
                sendByteHex(VERSION_HARDWARE_MINOR);
                result = SLCAN_CR;
            }
            break;
        case 'v': // Get firmware version
            {

                slcanSetOutputChar('v');
                sendByteHex(VERSION_FIRMWARE_MAJOR);
                sendByteHex(VERSION_FIRMWARE_MINOR);
                result = SLCAN_CR;
            }
            break;
        case 'N': // Get serial number
            {
                slcanSetOutputChar('N');
                sendHex(0xFFFF, 4);
                result = SLCAN_CR;
            }
            break;
        case 'O': // Open CAN channel
            if (state == STATE_CONFIG)
            {
//            	mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x00); // set normal operating mode
//                clock_reset();
                state = STATE_OPEN;
                result = SLCAN_CR;
            }
            break;
        case 'l': // Loop-back mode
            if (state == STATE_CONFIG)
            {
            	//		mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x40); // set loop-back
                state = STATE_OPEN;
                result = SLCAN_CR;
            }
            break;
        case 'L': // Open CAN channel in listen-only mode
            if (state == STATE_CONFIG)
            {
//            	mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x60); // set listen-only mode

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
            if (state == STATE_OPEN)
            {
                if (transmitStd(line)) {
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
                sendByteHex(status);
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
                if (parseHex(&line[1], 2, &am0) && parseHex(&line[3], 2, &am1) && parseHex(&line[5], 2, &am2) && parseHex(&line[7], 2, &am3)) {
                    result = SLCAN_CR;
                }
            }
            break;
         case 'M': // Set accpetance filter code
            if (state == STATE_CONFIG)
            {
                unsigned long ac0, ac1, ac2, ac3;
                if (parseHex(&line[1], 2, &ac0) && parseHex(&line[3], 2, &ac1) && parseHex(&line[5], 2, &ac2) && parseHex(&line[7], 2, &ac3)) {
//                    mcp2515_set_SJA1000_filter_code(ac0, ac1, ac2, ac3);
                    result = SLCAN_CR;
                }
            }
            break;

    }

   slcanSetOutputChar(result);
}

/**
 * Get next character of given can message in ascii format
 *
 * @param canmsg Pointer to can message
 * @param step Current step
 * @return Next character to print out
 */
char slCanCanmsg2asciiGetNextChar(slcan_canmsg_t * canmsg, uint8_t * step) {

    char ch = SLCAN_BELL;
    uint8_t newstep = *step;

    if (*step == RX_STEP_TYPE) {

            // type

            if (canmsg->flags.extended) {
                newstep = RX_STEP_ID_EXT;
                if (canmsg->flags.rtr) ch = 'R';
                else ch = 'T';
            } else {
                newstep = RX_STEP_ID_STD;
                if (canmsg->flags.rtr) ch = 'r';
                else ch = 't';
            }

    } else if (*step < RX_STEP_DLC) {

	// id

        unsigned char i = *step - 1;
        unsigned char * id_bp = (unsigned char*) &canmsg->id;
        ch = id_bp[3 - (i / 2)];
        if ((i % 2) == 0) ch = ch >> 4;

        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';

        newstep++;

    } else if (*step < RX_STEP_DATA) {

	// length

        ch = canmsg->dlc;

        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';

        if ((canmsg->dlc == 0) || canmsg->flags.rtr) newstep = RX_STEP_TIMESTAMP;
        else newstep++;

    } else if (*step < RX_STEP_TIMESTAMP) {

        // data

        unsigned char i = *step - RX_STEP_DATA;
        ch = canmsg->data[i / 2];
        if ((i % 2) == 0) ch = ch >> 4;

        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';

        newstep++;
        if (newstep - RX_STEP_DATA == canmsg->dlc*2) newstep = RX_STEP_TIMESTAMP;

    } else if (timestamping && (*step < RX_STEP_CR)) {

        // timestamp
        unsigned char i = *step - RX_STEP_TIMESTAMP;
        if (i < 2) ch = (canmsg->timestamp >> 8) & 0xff;
        else ch = canmsg->timestamp & 0xff;
        if ((i % 2) == 0) ch = ch >> 4;

        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';

        newstep++;

    } else {

        // linebreak
        ch = SLCAN_CR;
        newstep = RX_STEP_FINISHED;
    }

    *step = newstep;
    return ch;
}
