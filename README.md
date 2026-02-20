# FD_0604_LCD_Driver

Driver Software to interface FD_06_04 LED Display with Arduino-compatible MCU boards

This project is provided purely for educational purposes. Use at your own risk.

## Usage

Upload program to arduino, and wire according to wiring diagram.

Connect Serial interface to PC to send commands:

Any number between 0 and 3999 inclusive can be entered to show on display. 

**Special Arguments:**

| Argument  | Description                                                 |
| --------- | ----------------------------------------------------------- |
| MEM       | Displays amount of free memory on MCU.                      |
| CYCLE     | Cycles from 0 to 3999 with 100ms delay between each number. |
