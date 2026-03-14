# FD_0604_LCD_Driver

Driver Software to interface FD_0604 LED Display with Arduino-compatible MCU boards

This project is provided purely for educational purposes only. Use at your own risk.

## Usage

Upload program to arduino, and wire according to wiring diagram.

Connect Serial interface to PC to send commands:

Any number for the given ranges can be displayed on the screen:
- 0000~3999 for normal orientation
- 000~999 for inverted orientation

**Alternative Available Commands**

| Argument  | Description                                                               |
| --------- | ------------------------------------------------------------------------- |
| TEMP      | Turns the display into a thermometer using thermosistor attached.         |
| CYCLE     | Cycles continuously 0-3999 / 0-999 with 100ms delay between numbers.      |
| INIT      | Flashes all possible digits and letters once.                             |
| NULL      | Shows --:-- on the display.                                               |
| OFF       | Turns off the display.                                                    |

**Configuration Commands**

| Argument  | Description                                                 |
| --------- | ----------------------------------------------------------- |
| HELP      | Shows the availabe commands page.                           |
| INVERT    | Flips the screen orientation.                               |
| MEM       | Prints to Serial the available free memory on MCU.          |

