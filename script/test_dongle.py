#!/usr/bin/python

from pyusbtin.usbtin import USBtin
from pyusbtin.canmessage import CANMessage
from time import sleep

import sys

can1_rx = 0
can2_rx = 0

def log_data1(msg):
    global can1_rx
    print("RX")
    print(msg)
    can1_rx = can1_rx + 1
    
def log_data2(msg):
    global can2_rx
    print("RX")
    print(msg)
    can2_rx = can2_rx + 1

while(True):
   input("Press Enter to start Test...")

   usbtin1 = USBtin()
   usbtin1.connect(sys.argv[1])
   usbtin1.add_message_listener(log_data1)
   usbtin1.open_can_channel(100000, USBtin.ACTIVE)
   # usbtin1.serial_port.timeout = 1000

   usbtin2 = USBtin()
   usbtin2.connect(sys.argv[2])
   usbtin2.add_message_listener(log_data2)
   usbtin2.open_can_channel(100000, USBtin.ACTIVE)
   # usbtin2.serial_port.timeout = 1000

   can1_rx = 0
   can2_rx = 0
   test_pending = 1  
   test_cycle = 0
   while(test_pending):
      print("TX")
      usbtin1.send(CANMessage(0x100, [1,2,3,4]))
      sleep(0.1)
      usbtin2.send(CANMessage(0x200, [7,8]))
      sleep(0.1)
      if ((can1_rx > 1) and (can2_rx > 1)):
         test_pending = 0
         print("-------- DONGLE OK ------------")
      test_cycle = test_cycle + 1
      if (test_cycle > 10):
         test_pending = 0
         print("-------- DONGLE NO RX/TX ------------") 
   usbtin1.close_can_channel()
   usbtin2.close_can_channel()



