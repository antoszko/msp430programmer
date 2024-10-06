# `msp430_programmer.py`

## [Blog Post (hackaday.io)](https://hackaday.io/project/195106-msp430-programmer-using-stm32)

This is a script for programming a hex file into a msp430. It is designed for and tested on a MSP430G2211 MCU.
It is intended to be used alongside the JTAG-Programmer CubeIDE project installed on a NUCLEO-L432KC.

## Useage

To use this scipt you need a `.hex` file. I reccommend writing a program in Ti's Code Composer Studio and compiling it there.
Before compiling, make sure that you enable compiling hex file in preferences and set output type to intel hex.

You also need a NUCLEO-L423KC with the project JTAG-Programmer installed on it. 

1. begin the script by typing in a terminal:

	`$ python3 msp430_programmer.py <tty port> 115200 <hex file>`

	- tty port can be found by doing `ls /dev/ | grep tty` in a terminal and finding which one is the NUCLEO
	- baud rate is constant 115200
	- hex file must be an intel hex file
	
2. press the reset button on the NUCLEO

non-verbose
```
$ python3 msp430_programmer.py /dev/ttyACM0 115200 msp430g2xx1_1.asm.hex
[Host] Opening serial port /dev/ttyACM0 at 115200 baud rate.
       starting...
[Host] Programmer is ready
[Host] Opening file msp430g2xx1_1.asm.hex
[Host] Writing file length 236 to programmer...
[Host] Writing file to programmer...
       Verifying .hex file...
       Connecting to target...
       Erasing target flash...
       Writing target flash...
       Successfully wrote target flash. Time elapsed 0:00.090
       Shutting down JTAG connection...
[Host] Exit
```

verbose
```
$ python3 msp430_programmer.py /dev/ttyACM0 115200 msp430g2xx1_1.asm.hex -v
[Host] Opening serial port /dev/ttyACM0 at 115200 baud rate.
       starting...
[Host] Programmer is ready
[Host] Opening file msp430g2xx1_1.asm.hex
[Host] Writing file length 236 to programmer...
       Successfully received file length 236 bytes
[Host] Writing file to programmer...
[Host] Finished writing file to programmer
       Received file of length 236 bytes
       Verifying .hex file...
       Data: 32 bytes @ 0xf800 cs=0x71
       Data: 4 bytes @ 0xf820 cs=0x9e
       Data: 2 bytes @ 0xffe4 cs=0xf9
       Data: 2 bytes @ 0xffe6 cs=0xfb
       Data: 2 bytes @ 0xfff0 cs=0x05
       Data: 2 bytes @ 0xfff2 cs=0x07
       Data: 2 bytes @ 0xfff4 cs=0x09
       Data: 2 bytes @ 0xfff6 cs=0x0b
       Data: 2 bytes @ 0xfffc cs=0x11
       Data: 2 bytes @ 0xfffe cs=0xf7
       Eof cs=0x01
       Successfully verified .hex file
       Connecting to target...
       Successfully connected to target 0xf201
       Erasing target flash...
       Plan to flash main segment 0 (0xfffe)
       Plan to flash main segment 3 (0xf820)
       Successfully erased flash segment 0 (begins 0xfe00)
       Successfully erased flash segment 3 (begins 0xf800)
       Successfully erased target flash
       Writing target flash...
       0xfffe: f800
       0xfffc: f81c
       0xfff6: f81c
       0xfff4: f81c
       0xfff2: f81c
       0xfff0: f81c
       0xffe6: f81c
       0xffe4: f81c
       0xf820: 3ffd4303
       0xf800: 4031028040b25a800120d3d20022e3d20021403f5000831f23fe3ff9d0320010
       Successfully wrote target flash. Time elapsed 0:00.090
       Shutting down JTAG connection...
[Host] Exit
```

