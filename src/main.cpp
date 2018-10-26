#include "mbed.h"
#include "app_config.h"
#include "ISL1208.h"
#include "RaspberryPiControl.h"
#include "BatteryVoltage.h"
#include "BufferedSerial.h"
#include "HWatchDogTimer.h"

// Initial Time is Mon, 1 Jan 2018 00:00:00
#define TIME_INIT_VALUE  1514764800
// Default Safety on (power) and Safety off (sleep) values in seconds
#define ON_PERIOD_INIT_VALUE_s 7200
#define OFF_PERIOD_INIT_VALUE_s 7200
// UART receiving buffer
#define RX_BUFFER_SIZE      7        //Size in B
// System watchdogs
#define HW_WATCHDOG_TIMER_THRESHOLD	10	// value in seconds

DigitalOut alivenessLED(LED_1, 0);
DigitalOut actuatedLED(LED_2, 0);
DigitalOut fetOutput(FET_OUTPUT, 0);
DigitalOut powerEnable5V(ENABLE_5V_PIN, 0);
DigitalOut powerEnable3V3(ENABLE_3V3_PIN, 0);

DigitalIn  raspberryPiStatus(RASPBERRY_PI_STATUS, PullDown);

// Create UART object
BufferedSerial pc(UART_TX, UART_RX);
// Create I2C object
I2C i2c(I2C_SDA, I2C_SCL);
// Create ISL1208 object
ISL1208 rtc(&i2c);
// RaspberryPiControl object
RaspberryPiControl raspberryPiControl;
// Battery Voltage object
BatteryVoltage batteryVoltage;

// Uart variables
uint8_t rxBuffer[RX_BUFFER_SIZE];
uint8_t rxIndex;
// Pira functionality variables
uint32_t piraStatus;
uint32_t setTimeValue;
uint32_t onPeriodValue;
uint32_t offPeriodValue;
uint32_t rebootThresholdValue;
uint32_t wakeupThresholdValue;
char getTimeValue[26] = "Tue Apr 10 12:00:00 2018\n";
char *temp;
bool turnOnRpiState;	// If set to true, force Pi wakeup
// Pira operations variables
uint8_t send_time; 
uint8_t batteryLevelContainer;
uint32_t bleWatchdogTimer;
bool uart_enabled;
//bool i2c_enabled;

Ticker tick;

int init_rtc() {
    i2c.frequency(400000); //400kHz

    //Try to open the ISL1208
    if (rtc.open()) {
		#ifdef DEBUG
        pc.printf("Device detected!\n");
		#endif
        //Configure the oscillator for a 32.768kHz crystal
        rtc.oscillatorMode(ISL1208::OSCILLATOR_CRYSTAL);
 
        //Check if we need to reset the time
        if (rtc.powerFailed()) {
			//The time has been lost due to a complete power failure
			#ifdef DEBUG
            pc.printf("Device has lost power! Resetting time...\n");
			#endif 
            //Set RTC time to Mon, 1 Jan 2018 00:00:00
            rtc.time(TIME_INIT_VALUE);
        }
		return true;
    }
    else {
		#ifdef DEBUG
        pc.printf("Device NOT detected!\n");
		#endif
		//actuatedLED = 1;
		return false;
    }
}

void init_uart(void) {
    // Create UART object
	for (int i = 0; i < RX_BUFFER_SIZE; i++)
    {
        rxBuffer[i] = 0;
    }
	rxIndex = 0;
	pc.baud(115200);
	uart_enabled = true;
}

void flip() {
	// Blink led to show system on
	alivenessLED = !alivenessLED;
	// HW WatchDog reset    
    HWatchDogTimer::kick();
	// Set flag to update status every second
	send_time = 1;
}
/*
void change_i2c_state() {
	if (i2c_enabled) {	// disable it
		NRF_TWI0->ENABLE = TWI_ENABLE_ENABLE_Disabled << TWI_ENABLE_ENABLE_Pos;
		NRF_TWI0->POWER = 0;
		i2c_enabled = false;
	}
	else {	// enable it
		NRF_TWI0->POWER = 1;
		NRF_TWI0->ENABLE = TWI_ENABLE_ENABLE_Enabled << TWI_ENABLE_ENABLE_Pos;
		wait(0.5);
		i2c_enabled = init_rtc();
	}
}
*/
void enable_uart() {
	if (!uart_enabled) {
		NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
		//NRF_UART0->ENABLE = 1;
		NRF_UART0->TASKS_STARTTX = 1;
		NRF_UART0->TASKS_STARTRX = 1;
		wait(0.5);
		init_uart();
	}
}

void disable_uart() {
	if (uart_enabled) { 
		NRF_UART0->TASKS_STOPTX = 1;
		NRF_UART0->TASKS_STOPRX = 1;
		//NRF_UART0->ENABLE = 0;
		NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Disabled << UART_ENABLE_ENABLE_Pos;
		uart_enabled = false;
	}
}

void uartCommandParse(uint8_t *rxBuffer, uint8_t len) {
    uint8_t firstChar = rxBuffer[0];
    uint8_t secondChar = rxBuffer[1];
    // uint8_t dataLen = (uint8_t)(len - 2);
    // Since data type of data is uint32_t, it is by default dataLen = 4
    // but it is implemented like this in order to later have possibility to update it to different data type 
    // or to use less than 4B
    uint32_t data = 0;
    /*
    for (int i = 0, j = 2; ((i >= (int)dataLen) || (j >= 6)); i++, j++)
    {
        data |= (uint32_t)(rxBuffer[j] << ((3-i)*8));
    }
    */
    data = (rxBuffer[2] << 24) | (rxBuffer[3] << 16) | (rxBuffer[4] << 8) | (rxBuffer[5]);

    if (secondChar == ':') {
        switch(firstChar) {
            case 't':
                rtc.time((time_t)data);
                break;
            case 'p':
                onPeriodValue = data;
                break;
            case 's':
                offPeriodValue = data;
                break;
            case 'c':
                //pc.printf("To be defined how to react on c: command\n");
                break;
            case 'r':
                rebootThresholdValue = data;
                break;
            case 'w':
                wakeupThresholdValue = data;
                break;
            default:
                break;
        }
    }
}

void uartCommandSend(char command, uint32_t data) {
	#ifndef DEBUG
    pc.putc((int)command);
    pc.putc(':');
    pc.putc((int)((data & 0xFF000000)>>24));
    pc.putc((int)((data & 0x00FF0000)>>16));
    pc.putc((int)((data & 0x0000FF00)>>8));
    pc.putc((int)( data & 0x000000FF));
    pc.putc('\n');
	#endif
}

void uartCommandReceive(void) {
    while (pc.readable()) {
        // Receive characters 
        rxBuffer[rxIndex] = pc.getc();

        if (rxIndex == 0) {
            if (rxBuffer[rxIndex] != 't' &&
                rxBuffer[rxIndex] != 'p' &&
                rxBuffer[rxIndex] != 's' &&
                rxBuffer[rxIndex] != 'c' &&
                rxBuffer[rxIndex] != 'r' &&
                rxBuffer[rxIndex] != 'w') {
                //rxIndex reset is added for clarity
                rxIndex = 0;
                //break should not happen
                //break;
            }
            else {
                rxIndex++;
            }     
        }
        else if (rxIndex == 1) {
            if (rxBuffer[rxIndex] != ':') {
                rxIndex = 0;
                //break;
            }
            else {
                rxIndex++;
            }
        }
        else {
            if (rxBuffer[rxIndex] == '\n') {
                //All data withing the packet has been received, parse the packet and execute commands
                if (rxIndex == 6) {
                    //pc.printf("All characters has been received\n");
                    uartCommandParse(rxBuffer, RX_BUFFER_SIZE);
                    rxIndex = 0;
                    //break;
                }
                else {
                    for (int i = 0; i < RX_BUFFER_SIZE; i++) {
                        rxBuffer[i] = 0;
                    }
                    rxIndex = 0;
                } 
            }
            else if (rxIndex == 6) {
                for (int i = 0; i < RX_BUFFER_SIZE; i++) {
                    rxBuffer[i] = 0;
                }
                rxIndex = 0;
            }
            else {
                rxIndex++;
                if (rxIndex > 6) {
                    rxIndex = 0;
                }
            }
        }
    }
}

void systemInitComplete() {
	setTimeValue = TIME_INIT_VALUE;
	piraStatus = 0;
	onPeriodValue = ON_PERIOD_INIT_VALUE_s;
	offPeriodValue = OFF_PERIOD_INIT_VALUE_s;
	wakeupThresholdValue = OFF_PERIOD_INIT_VALUE_s;
	rebootThresholdValue = raspberryPiControl.REBOOT_TIMEOUT_s;
	batteryLevelContainer = 0;
	turnOnRpiState = 0;
}

int main() {
    // Enable 3V3 power for RTC and LoRa
    powerEnable3V3 = 1;
	// Initially enable RaspberryPi power
    powerEnable5V = 1; 

	// Init system variables
	send_time = 0;

	// Init UART
	init_uart();

	// Init I2C and RTC
	//i2c_enabled = init_rtc();
	init_rtc();
	wait(1.0);

	// Init HW watchdog to reset program if it fails
	HWatchDogTimer::init(HW_WATCHDOG_TIMER_THRESHOLD);

	// Blink LED every second
	//alivenessLED = 1;
	tick.attach(&flip, 1.0);

	// Init functionality variables to default values
	systemInitComplete();

	while(true) {
		if (send_time) {
			send_time = 0;
			// If Rpi status pin is on
			if (raspberryPiControl.getRpiState()) {
				enable_uart();
				uartCommandReceive();

				// Get the current time from RTC 
				time_t seconds = rtc.time();
				#ifdef DEBUG
					pc.printf("Time as a basic string = %s", ctime(&seconds));
				#endif
				
				// Write current time to containter variable which can be read over BLE
				temp = ctime(&seconds);
				memcpy(getTimeValue, temp, strlen((const char *)temp));
				//piraServicePtr->updateTime(getTimeValue);
				#ifdef SEND_TIME_AS_STRING
					// Send time to RaspberryPi in string format
					pc.printf("t:%s", getTimeValue);
				#else
					// Send time in seconds since Jan 1 1970 00:00:00
					// pc.printf("t:%d\n", seconds);
					uartCommandSend('t', seconds);
				#endif
				// Update status values
				// Seconds left before next power supply turn off
				piraStatus = onPeriodValue - raspberryPiControl.timeoutOnGet();
				//piraServicePtr->updateStatus(&piraStatus);
				// Send status to RPi -> time until next sleep and then battery level
				// pc.printf("p:%d\n", piraStatus);
				uartCommandSend('o', piraStatus);
				batteryLevelContainer = batteryVoltage.batteryLevelGet();
				//piraServicePtr->updateBatteryLevel(&batteryLevelContainer);
				// pc.printf("b:%d\n", batteryLevelContainer);
				uartCommandSend('b', (uint32_t)batteryLevelContainer);
				
				// Send rest of the values in order to verify changes
				uartCommandSend('p', onPeriodValue);
				uartCommandSend('s', offPeriodValue);
				uartCommandSend('r', rebootThresholdValue);
				uartCommandSend('w', wakeupThresholdValue);
				// Send RPi status pin value
				uartCommandSend('a', (uint32_t)raspberryPiStatus.read());
				/*
				// Update BLE containers
				piraServicePtr->updateOnPeriodSeconds(&onPeriodValue);
				piraServicePtr->updateOffPeriodSeconds(&offPeriodValue);
				piraServicePtr->updateRebootPeriodSeconds(&rebootThresholdValue);
				piraServicePtr->updateWakeupPeriodSeconds(&wakeupThresholdValue);
				*/
				#ifdef DEBUG
					pc.printf("Overview - Pira status = %d\n", piraStatus);
					pc.printf("Battery level in V = %d\n", (int)(batteryVoltage.batteryVoltageGet(batteryLevelContainer)*100));
					pc.printf("onPeriodValue = %d\n", onPeriodValue);
					pc.printf("offPeriodValue = %d\n", offPeriodValue);
					pc.printf("rebootThresholdValue = %d\n", rebootThresholdValue);
					pc.printf("wakeupThresholdValue = %d\n", wakeupThresholdValue);
					pc.printf("turnOnRpiState = %d\n", turnOnRpiState); 
					pc.printf("Status Pin = %d\n", raspberryPiStatus.read());
				#endif
			}
			else {
				// if Rpi status pin is off
				disable_uart();
			}
			// Update Rpi handler status
			raspberryPiControl.powerHandler(&raspberryPiStatus, 
												&powerEnable5V,
												onPeriodValue,
												offPeriodValue,
												wakeupThresholdValue,
												rebootThresholdValue,
												turnOnRpiState);
		}
		sleep();
	}
}
