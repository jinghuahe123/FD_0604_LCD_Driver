# FD_0604_LCD_Driver

Driver Software to interface FD_0604 LED Display with Arduino-compatible MCU boards.

NOTE: Whilst the code should work on all AVR boards, only the NANO (ATMEGA328P) has been explicitly tested. 

**This software is distributed ‘as-is’ and without any warranty, whether express or implied, including but not limited to the implied warranties of merchantability, fitness for a particular purpose, and non‑infringement. The developers shall not be held liable for any claim, damages, or other liability arising from the use of this software.**

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

| Argument  | Description                                                       |
| --------- | ----------------------------------------------------------------- |
| HELP      | Shows the availabe commands page.                                 |
| INFO      | Shows the hardware information and pin connections of the board.  | 
| INVERT    | Flips the screen orientation.                                     |
| MEM       | Prints to Serial the available free memory on MCU.                |
| ERASE     | Erases previously displayed number history.                       |
| HISTORY   | Prints to Serial the displayed number history.                    |

**TODO / IMPROVEMENTS**

- EEPROM class writes more than one data type
- add secondary serial interface for explicitly setting a number ONLY