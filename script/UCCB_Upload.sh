#!/bin/bash


/mnt/d/Programy/STMicroelectronics/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe -c port=usb1 -e all -d ./0303.hex 0x08000000 -v -s
