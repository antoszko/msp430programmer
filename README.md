# `msp430_programmer.py`

## [Blog Post (hackaday.io)](https://hackaday.io/project/195106-msp430-programmer-using-stm32)

This is a script for programming a hex file into a msp430. It is designed for and tested on a MSP430G2211 and MSP430G2231 MCU.
It is intended to be used alongside the JTAG-Programmer CubeIDE project installed on a NUCLEO-L432KC.

## Useage

To use this scipt you need a `.hex` file. I reccommend writing a program in Ti's Code Composer Studio and compiling it there.
Before compiling, make sure that you enable compiling hex file in preferences and set output type to intel hex.

You also need a NUCLEO-L423KC with the project JTAG-Programmer installed on it. 

Begin the script by typing in a terminal:

`$ python3 msp430_programmer.py <tty port> 115200 <hex file>`

- tty port can be found by doing `ls /dev/ | grep tty` in a terminal and finding which one is the NUCLEO
- baud rate is constant 115200
- hex file must be an intel hex file

non-verbose
```
$ python3 msp430_programmer.py /dev/ttyACM0 115200 hello_world.hex
[Host] Opening serial port /dev/ttyACM0 at 115200 baud rate
[Host] Waking up programmer...
       vret is 3 and received 0 bytes : 
       Starting
       Ready
[Host] Programmer is ready
[Host] Opening file hello_world.hex
[Host] Writing file length 360 to programmer...
       Received 360
[Host] Writing file to programmer...
       Verifying .hex file...
       Connecting to target...
       Got JTAG ID: 0x0089
       Erasing target flash...
       Writing target flash...
       Successfully wrote target flash. Time elapsed 0:00.228
       Shutting down JTAG connection...
[Host] Exit

```

verbose
```
$ python3 msp430_programmer.py /dev/ttyACM0 115200 hello_world.hex -v
[Host] Opening serial port /dev/ttyACM0 at 115200 baud rate
[Host] Waking up programmer...
       ret is 3 and received 0 bytes : 
       ret is 3 and received 0 bytes : 
       ret is 3 and received 0 bytes : 
       ret is 3 and received 0 bytes : 
       Starting
       Ready
[Host] Programmer is ready
[Host] Opening file hello_world.hex
[Host] Writing file length 360 to programmer...
       Received 2 bytes h
       Received 360
[Host] Writing file to programmer...
[Host] Finished writing file to programmer
       Received file of length 360 bytes
       Verifying .hex file...
       Data: 32 bytes @ 0xf800 cs=0xda
       Data: 32 bytes @ 0xf820 cs=0x98
       Data: 20 bytes @ 0xf840 cs=0xbc
       Data: 2 bytes @ 0xffe4 cs=0x1f
       Data: 2 bytes @ 0xffe6 cs=0x21
       Data: 2 bytes @ 0xffe8 cs=0x23
       Data: 2 bytes @ 0xffea cs=0x25
       Data: 2 bytes @ 0xfff0 cs=0x2b
       Data: 2 bytes @ 0xfff2 cs=0x2d
       Data: 2 bytes @ 0xfff4 cs=0x2f
       Data: 2 bytes @ 0xfffc cs=0x37
       Data: 2 bytes @ 0xfffe cs=0x25
       Eof cs=0x01
       Successfully verified .hex file
       Connecting to target...
       Got JTAG ID: 0x0089
       Successfully connected to target 0xf201
       Erasing target flash...
       Plan to flash main segment 0 (0xfffe)
       Plan to flash main segment 3 (0xf840)
       Successfully erased flash segment 0 (begins 0xfe00)
       Successfully erased flash segment 3 (begins 0xf800)
       Successfully erased target flash
       Writing target flash...
       0xfffe: f82e
       0xfffc: f842
       0xfff4: f842
       0xfff2: f842
       0xfff0: f842
       0xffea: f842
       0xffe8: f842
       0xffe6: f842
       0xffe4: f842
       0xf840: f84ad03200103ffd430343033fff4303431c4130
       0xf820: 00009381000023fbe3d200213ff24031028012b0f850430c12b0f800431c12b0
       0xf800: 832140b25a80012040f20080005743d2002240b10fa000009381000024058391
       Successfully wrote target flash. Time elapsed 0:00.228
       Shutting down JTAG connection...
[Host] Exit

[Host] Exit
```

