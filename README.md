# pira-smart-firmware
Firmware for PiRa Smart device

## Using the device
All the values are updated every second.

### General operation
After power up of the device, 5V power supply is turned ON to power RaspberryPi. Then, status pin is checked until high. When status pin is pulled up, power supply ON period counting starts. \
If status pin has been pulled low during ON period, then possible reboot is checked. After 60s, if status pin is still low, 5V power supply is turned OFF. If status pin is pulled up after 60s period has expired, power supply ON period counting starts again. \
During ON period counting, if timeout expires (ON period value) power supply is turned OFF immediately. \
After OFF period has expired, 5V power supply is turned ON and the cycle repeats. 

### UART
Device is sending data on UART every second if format given below: \
`t:Sun Apr  3 02:01:00 2005<LF>`\
`p:1788<LF>`\
`b:127<LF>`

First string starts with a `t:` and it represents current time read from RTC. \
Second string starts with a `p:` which represents period until controlled device (RaspberryPi) will be awake. \
Third string starts with a `b:` which represents battery level expressed in ADC format. \
In order to calculate battery level in volts, following equation should be used: \
batteryLevelV = ADC * 1.2 * (100k + 249k) / (255 * 100k)

All the lines end with linefeed character.

Additionally, sleep period can be set through UART. Value consisted of 4 bytes is expected with LSB byte first. These bytes represent OFF period in seconds. \
Example: \
Desired OFF period is 2000s \
Following values should be sent through UART interface to PiRa device in given order (send 0xD0 first): \
`0xD0 0x07 0x00 0x00`

### RTC
Real Time Clock is attached to I2C bus. It is used to get correct time. 
For the first use, RTC time can be set through BLE interface. Then main and backup batteries will keep the correct time. 

### BLE
For connecting to PiRa, for now, use nRF Connect application for Android phones. \
PiRa service UUID: 0xB000 \
Characteristics UUIDs:\n
- Set Time: UUID 0xB001, Format: seconds since January 1st 1970, 00:00:00, UINT32 
- Get Time: UUID 0xB002, Format: String, Example: "Tue Apr 10 12:00:00 2018" 
- Get Status: UUID 0xB003, Format: Seconds until next power supply turn OFF, Example: \
`0xD0 0x07 0x00 0x00 == 2000s` 
- Set On Period: UUID 0xB004, Format: Power Supply ON time period value in seconds, UINT32 
- Set Off Period: UUID 0xB005, Format: Power Supply OFF time period value in seconds, UINT32 
- Get Battery Level: UUID 0xB005, Format: 8 bit ADC value. 

## Compiling the code
### Online compiler
Import code from URL (https://github.com/IRNAS/pira-smart-firmware.git)
Select option "Update all libraries to the latest revision".
Select platform for which code will be compiled (RedBearLab BLE NANO) and hit "Compile" button.
Online compiler will offer you to save hex file.

### mbed-cli
In order to compile the code localy, it is required to install mbed-cli and all dependency tools.\
See: https://github.com/ARMmbed/mbed-cli \
Then it is required to import code from GitHub:\
`$ mbed import https://github.com/IRNAS/pira-smart-firmware.git`\
After import is finished, execute mbed compile command with desired target and tools selected, for example:\
`$ mbed mbed compile -m RBLAB_BLENANO -t GCC_ARM`\
Resulting hex file will be located in build folder (e.g. ./BUILD/RBLAB_BLENANO/GCC_ARM/

## Download code to the board
In order to download code to mbed platform, just copy hex file to the MBED drive. 
