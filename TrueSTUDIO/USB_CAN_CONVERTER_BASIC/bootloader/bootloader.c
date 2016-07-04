/*
 * bootloader.c
 * taken from topping.will from st forum(https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/Flat.aspx?RootFolder=%2Fpublic%2FSTe2ecommunities%2Fmcu%2FLists%2Fcortex%5Fmx%5Fstm32%2FJump%20to%20USB%20DFU%20Bootloader%20in%20startup%20code%20on%20STM32F042&FolderCTID=0x01200200770978C69A1141439FE559EB459D7580009C4E14902C3CDE46A77F0FFD06506F5B&currentviews=1355)
 * thanks Will.
 */

#include "stm32f0xx_hal.h"
#include "usb_device.h"



#define BOOTLOADER_MAGIC_ADDR ((uint32_t*) ((uint32_t) 0x20001000)) //4k into SRAM (out of 6k)
#define BOOTLOADER_MAGIC_TOKEN 0xDEADBEEF  // :D

//Value taken from CD00167594.pdf page 35, system memory start.
#define BOOTLOADER_START_ADDR 0x1fffc400 //for ST32F042

void RebootToBootloader(){
//call this at any time to initiate a reboot into bootloader

  *BOOTLOADER_MAGIC_ADDR = BOOTLOADER_MAGIC_TOKEN;
    NVIC_SystemReset();
}

//This is the first thing the micro runs after startup_stm32f0xx.s
//Only the SRAM is initialized at this point
void bootloaderSwitcher(){
  uint32_t jumpaddr,tmp;
  GPIO_InitTypeDef GPIO_InitStruct;

  tmp=*BOOTLOADER_MAGIC_ADDR;
  //tmp=BOOTLOADER_MAGIC_TOKEN; //DEBUG
  if (tmp == BOOTLOADER_MAGIC_TOKEN){
    *BOOTLOADER_MAGIC_ADDR=0; //Zero it so we don't loop by accident

    //For the LQFP48 of STM32F042, the BOOT0 pin is at PF11
    __HAL_RCC_GPIOF_CLK_ENABLE()
    	;

//	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_11, GPIO_PIN_RESET);

	/*Configure GPIO pin : CAN_MOD_Pin */
	GPIO_InitStruct.Pin = GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    void (*bootloader)(void) = 0; //Zero the function pointer.
    jumpaddr = *(__IO uint32_t*)(BOOTLOADER_START_ADDR + 4);
    bootloader = (void (*)(void)) jumpaddr; //Set the function pointer to bootaddr +4
    __set_MSP(*(__IO uint32_t*) BOOTLOADER_START_ADDR); //load the stackpointer - bye bye program
    bootloader(); //GO TO DFU MODE MOFO :D

    //this should never be hit, trap for debugging
    while(1){}
  }
}

