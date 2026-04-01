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
| RAW       | Shows RAW Input from an analog input.                                     |
| CYCLE     | Cycles continuously from 0-3999 / 0-999.                                  |
| INIT      | Flashes all possible digits and letters once.                             |
| NULL      | Shows --:-- on the display.                                               |
| OFF       | Turns off the display.                                                    |

**Configuration Commands**

| Argument  | Description                                                       |
| --------- | ----------------------------------------------------------------- |
| HELP      | Shows the availabe commands page.                                 |
| INFO      | Shows the hardware information and pin connections of the board.  | 
| SETTINGS  | Shows settings page and changes hardware configurations.          |
| MEM       | Prints to Serial the available free memory on MCU.                |
| ERASE     | Erases previously displayed number history.                       |
| RESET     | Resets all parameters (EEPROM) to factory defaults.               |
| HISTORY   | Prints to Serial the displayed number history.                    |

**Settings Menu**

| Option    | Description                                                       |
| --------- | ----------------------------------------------------------------- |
| [1]       | Exits back to main menu.                                          |
| [2]       | Set CYCLE Interval time between increasing numbers.               |
| [3]       | Set Temperature Refresh Interval Time.                            |
| [4]       | Set RAW Input Refresh Interval Time.                              |
| [5]       | Flip Display Orientation.                                         |
| [6]       | Enable / Disable Temperature Serial Output.                       |

**TODO / IMPROVEMENTS**

- EEPROM class writes more than one data type
- remove requirement for Arduino String class and change for const char*