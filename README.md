# pira-smart-firmware
Firmware for PiRa Smart device

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
