/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "mbed.h"
#include "ble/BLE.h"
#include "LEDService.h"
#include "PiraService.h"
#include "app_config.h"
#include "ISL1208.h"
#include "RaspberryPiControl.h"
#include "BatteryVoltage.h"
#include "BufferedSerial.h"
#include "HWatchDogTimer.h"
 
 
// Not sure if these defines regarding BLE are needed at all 
#define CONN_SUP_TIMEOUT 6000 // six seconds
#define SLAVE_LATENCY 4 // four events can be ignored, the fifth must be met
#define MIN_CONN_INTERVAL 250 //250 milliseconds
#define MAX_CONN_INTERVAL 350 //350 milliseconds

// Initial Time is Mon, 1 Jan 2018 00:00:00
#define TIME_INIT_VALUE  1514764800UL

#define ON_PERIOD_INIT_VALUE_s 7200
#define OFF_PERIOD_INIT_VALUE_s 7200

#define RX_BUFFER_SIZE      7        //Size in B

#define BLE_WATCHDOG_TIMER_THRESHOLD      (600)        //10 minutes in seconds
#define HW_WATCHDOG_TIMER_THRESHOLD	10	// value in seconds

//#define DEBUG

DigitalOut alivenessLED(LED_1, 0);
DigitalOut actuatedLED(LED_2, 0);
DigitalOut fetOutput(FET_OUTPUT, 0);
//DigitalOut 3v3PowerEnable( , 0);
DigitalOut powerEnable5V(ENABLE_5V_PIN, 0);
DigitalOut powerEnable3V3(ENABLE_3V3_PIN, 0);

DigitalIn  raspberryPiStatus(RASPBERRY_PI_STATUS, PullNone);

// Create UART object
//Serial pc(UART_TX, UART_RX);
BufferedSerial pc(UART_TX, UART_RX);
// Create I2C object
I2C i2c(I2C_SDA, I2C_SCL);
// Create ISL1208 object
ISL1208 rtc(&i2c);    
// RaspberryPiControl object
RaspberryPiControl raspberryPiControl;
// Battery Voltage object
BatteryVoltage batteryVoltage;

//Service pointers declarations
LEDService  *ledServicePtr;
PiraService *piraServicePtr;

const static char     DEVICE_NAME[] = "PiraSmart";
static const uint16_t uuid16_list[] = {LEDService::LED_SERVICE_UUID, PiraService::PIRA_SERVICE_UUID};
uint32_t piraStatus;
uint32_t setTimeValue;
uint32_t onPeriodValue;
uint32_t offPeriodValue;
uint32_t rebootThresholdValue;
uint32_t wakeupThresholdValue;
char getTimeValue[26] = "Tue Apr 10 12:00:00 2018\n";
char *temp;
uint8_t sendTime; 
uint8_t batteryLevelContainer;
uint8_t rxBuffer[RX_BUFFER_SIZE];
uint8_t rxBufferBLE[PIRA_SERVICE_COMMANDS_RX_BUFFER_SIZE];
uint8_t rxIndex;
uint8_t rxIndexBLE;
bool turnOnRpiState;
bool bleConnected;
uint32_t bleWatchdogTimer;

Ticker ticker;
 
void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
#ifdef DEBUG_BLE
    pc.printf("BLE disconnected\n");
#endif
    
    bleConnected = false;
#ifndef DEBUG_BLE
    bleWatchdogTimer = 0;
#endif

    BLE::Instance().gap().startAdvertising();

#ifdef DEBUG_BLE
    pc.printf("bleConnected = %d\n", bleConnected);
    pc.printf("bleWatchdogTimer = %d\n", bleWatchdogTimer);
#endif
}

void connectionCallback(const Gap::ConnectionCallbackParams_t *params)
{
#ifdef DEBUG_BLE
    pc.printf("BLE connected\n");
#endif

    bleWatchdogTimer = 0;
    bleConnected = true;

#ifdef DEBUG_BLE
    pc.printf("bleConnected = %d\n", bleConnected);
    pc.printf("bleWatchdogTimer = %d\n", bleWatchdogTimer);
#endif
}

void periodicCallback(void)
{
    // HW WatchDog reset    
    HWatchDogTimer::kick();
    // Do blinky on LED1 to indicate system aliveness.
    alivenessLED = !alivenessLED; 
    // Set flag to update status every second    
    sendTime = 1; 
    // If BLE connected, count BLE WDT timer
    if (bleConnected)
    {
        bleWatchdogTimer++;
    }
}
 
/**
 * This callback allows the LEDService to receive updates to the ledState Characteristic.
 *
 * @param[in] params
 *     Information about the characterisitc being updated.
 */
void onDataWrittenCallback(const GattWriteCallbackParams *params) {
    if ((params->handle == ledServicePtr->getValueHandle()) && (params->len == 1)) 
    {
        actuatedLED = *(params->data);
    }
    else if ((params->handle == piraServicePtr->getSetTimeValueHandle()) && (params->len == 4))
    {
        //When time data received (in seconds format) update RTC value
        memset(&setTimeValue, 0x00, sizeof(setTimeValue)); 
        memcpy(&setTimeValue, params->data, params->len);  
        rtc.time((time_t)setTimeValue); 
    }
    else if ((params->handle == piraServicePtr->getOnPeriodSecondsValueHandle()) && (params->len == 4))
    {
        memset(&onPeriodValue, 0x00, sizeof(onPeriodValue)); 
        memcpy(&onPeriodValue, params->data, params->len);  
        //onPeriodValue = *(params->data);
    }
    else if ((params->handle == piraServicePtr->getOffPeriodSecondsValueHandle()) && (params->len == 4))
    {
        memset(&offPeriodValue, 0x00, sizeof(offPeriodValue)); 
        memcpy(&offPeriodValue, params->data, params->len);  
        //offPeriodValue = *(params->data);
    }
    else if ((params->handle == piraServicePtr->getTurnOnRpiStateValueHandle()) && (params->len == 1))
    {
        turnOnRpiState = *(params->data);
    }   
    else if (params->handle == piraServicePtr->getCommandsInterfaceValueHandle())
    {
        memset(&rxBufferBLE, 0x00, sizeof(rxBufferBLE)); 
        memcpy(&rxBufferBLE, params->data, params->len);  
    }
    else if ((params->handle == piraServicePtr->getRebootPeriodSecondsValueHandle()) && (params->len == 4))
    {
        memset(&rebootThresholdValue, 0x00, sizeof(rebootThresholdValue)); 
        memcpy(&rebootThresholdValue, params->data, params->len);  
        //offPeriodValue = *(params->data);
    }
    else if ((params->handle == piraServicePtr->getWakeupPeriodSecondsValueHandle()) && (params->len == 4))
    {
        memset(&wakeupThresholdValue, 0x00, sizeof(wakeupThresholdValue)); 
        memcpy(&wakeupThresholdValue, params->data, params->len);  
        //offPeriodValue = *(params->data);
    }
}
 
/**
 * This function is called when the ble initialization process has failed
 */
void onBleInitError(BLE &ble, ble_error_t error)
{
    /* Initialization error handling should go here */
#ifdef DEBUG
    pc.printf("Error occured\n");
#endif
}
 
/**
 * Callback triggered when the ble initialization process has finished
 */
void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;
 
    if (error != BLE_ERROR_NONE) {
        /* In case of error, forward the error handling to onBleInitError */
        onBleInitError(ble, error);
        return;
    }
 
    /* Ensure that it is the default instance of BLE */
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }
 
    ble.gap().onDisconnection(disconnectionCallback);
    ble.gap().onConnection(connectionCallback);
    ble.gattServer().onDataWritten(onDataWrittenCallback);
 
    bool initialValueForLEDCharacteristic = false;
    ledServicePtr = new LEDService(ble, initialValueForLEDCharacteristic);

    setTimeValue = TIME_INIT_VALUE; 
    piraStatus = 0;
    //Set ON and OFF period values to 30min by default
    onPeriodValue = ON_PERIOD_INIT_VALUE_s;
    offPeriodValue = OFF_PERIOD_INIT_VALUE_s;
    wakeupThresholdValue = OFF_PERIOD_INIT_VALUE_s;
    rebootThresholdValue = raspberryPiControl.REBOOT_TIMEOUT_s;
    batteryLevelContainer = 0;
    turnOnRpiState = 0;
    piraServicePtr = new PiraService(ble, 
                                     setTimeValue, 
                                     piraStatus, 
                                     getTimeValue, 
                                     onPeriodValue, 
                                     offPeriodValue, 
                                     batteryLevelContainer,
                                     turnOnRpiState,
                                     NULL,
                                     rebootThresholdValue,
                                     wakeupThresholdValue);
    
    /* setup advertising */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms. */
    ble.gap().startAdvertising();
}

void uartCommandParse(uint8_t *rxBuffer, uint8_t len)
{
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

    if (secondChar == ':')
    {
        switch(firstChar)
        {
            case 't':
                //pc.printf("t: received\n");
                rtc.time((time_t)data);
                break;
            case 'p':
                //pc.printf("p: received\n");
                onPeriodValue = data;
                break;
            case 's':
                //pc.printf("s: received\n");
                offPeriodValue = data;
                break;
            case 'c':
                //pc.printf("c: received\n");
                pc.printf("To be defined how to react on c: command\n");
                break;
            case 'r':
                //pc.printf("r: received\n");
                rebootThresholdValue = data;
                break;
            case 'w':
                //pc.printf("w: received\n");
                wakeupThresholdValue = data;
                break;
            default:
                break;
        }
    }
}

void uartCommandSend(char command, uint32_t data)
{
#ifndef DEBUG_BLE
    pc.putc((int)command);
    pc.putc(':');
    pc.putc((int)((data & 0xFF000000)>>24));
    pc.putc((int)((data & 0x00FF0000)>>16));
    pc.putc((int)((data & 0x0000FF00)>>8));
    pc.putc((int)( data & 0x000000FF));
    pc.putc('\n');
#endif
}

#ifdef SEND_TIME_AS_STRING
void uartCommandSendArray(char command, char *array, uint8_t len)
{
    pc.putc((int)command);
    pc.putc(':');
    for (int i = 0; i < len; i++)
    {
        pc.putc((int)array[i]);
    }
}
#endif

void uartCommandReceive(void)
{
    while (pc.readable())
    {
        // Receive characters 
        rxBuffer[rxIndex] = pc.getc();

        if (rxIndex == 0)
        {
            if (rxBuffer[rxIndex] != 't' &&
                rxBuffer[rxIndex] != 'p' &&
                rxBuffer[rxIndex] != 's' &&
                rxBuffer[rxIndex] != 'c' &&
                rxBuffer[rxIndex] != 'r' &&
                rxBuffer[rxIndex] != 'w')
            {
                //rxIndex reset is added for clarity
                rxIndex = 0;
                //break should not happen
                //break;
            }
            else
            {
                rxIndex++;
            }
                
        }
        else if (rxIndex == 1)
        {
            if (rxBuffer[rxIndex] != ':')
            {
                rxIndex = 0;
                //break;
            }
            else
            {
                rxIndex++;
            }
        }
        else
        {
            if (rxBuffer[rxIndex] == '\n')
            {
                //All data withing the packet has been received, parse the packet and execute commands
                if (rxIndex == 6)
                {
                    //pc.printf("All characters has been received\n");
                    uartCommandParse(rxBuffer, RX_BUFFER_SIZE);
                    rxIndex = 0;
                    //break;
                }
                else
                {
                    for (int i = 0; i < RX_BUFFER_SIZE; i++)
                    {
                        rxBuffer[i] = 0;
                    }
                    rxIndex = 0;
                } 
            }
            else if (rxIndex == 6)
            {
                for (int i = 0; i < RX_BUFFER_SIZE; i++)
                {
                    rxBuffer[i] = 0;
                }
                rxIndex = 0;
            }
            else
            {
                rxIndex++;
                if (rxIndex > 6)
                {
                    rxIndex = 0;
                }
            }
        }
    }
}

void init_uart(void)
{
    pc.baud(115200);
    //pc.printf("Start...\n");
}

void init_rtc(void)
{
    i2c.frequency(400000); //400kHz
    
    //Try to open the ISL1208
    if (rtc.open()) 
    {
#ifdef DEBUG
        pc.printf("Device detected!\n");
#endif
 
        //Configure the oscillator for a 32.768kHz crystal
        rtc.oscillatorMode(ISL1208::OSCILLATOR_CRYSTAL);
 
        //Check if we need to reset the time
        if (rtc.powerFailed()) 
        {
#ifdef DEBUG
            //The time has been lost due to a power complete power failure
            pc.printf("Device has lost power! Resetting time...\n");
#endif 

            //Set RTC time to Mon, 1 Jan 2018 00:00:00
            rtc.time(TIME_INIT_VALUE);
        }
    }
    else
    {
#ifdef DEBUG
        pc.printf("Device NOT detected!\n");
#endif
    }
}

int main(void)
{
    // Enable 3V3 power for RTC and LoRa
    powerEnable3V3 = 1; 
    // Initially enable RaspberryPi power
    powerEnable5V = 1;
    // Initialize variables
    sendTime = 0;
    for (int i = 0; i < RX_BUFFER_SIZE; i++)
    {
        rxBuffer[i] = 0;
    }
    for (int i = 0; i < PIRA_SERVICE_COMMANDS_RX_BUFFER_SIZE; i++)
    {
        rxBufferBLE[i] = 0;
    }
    rxIndex = 0;
    rxIndexBLE = 0;
    bleConnected = false;
    bleWatchdogTimer = 0;

    // init HW Watchdog timer - attached to blink periodicCallback
    HWatchDogTimer::init(HW_WATCHDOG_TIMER_THRESHOLD);

    // UART needs to be initialized first to use it for communication with RPi
    init_uart();
    // I2C and RTC init
    init_rtc();
    // periodicCallback must be attached after I2C is initialized
    ticker.attach(periodicCallback, 1); /* Blink LED every second */

    // Initialize BLE
    BLE &ble = BLE::Instance();
    ble.init(bleInitComplete);
 
    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (ble.hasInitialized()  == false) { /* spin loop */ }
 
    while (true) 
    {
        ble.waitForEvent();

        if (bleConnected)
        {
            if (bleWatchdogTimer > BLE_WATCHDOG_TIMER_THRESHOLD)
            {
                BLE::Instance().gap().disconnect(Gap::LOCAL_HOST_TERMINATED_CONNECTION);
                bleConnected = false;
                bleWatchdogTimer = 0;
            }
        }

        uartCommandReceive();
        
        if (sendTime)
        {
            sendTime = 0;

            // Get the current time from RTC 
            time_t seconds = rtc.time();
#ifdef DEBUG
            pc.printf("Time as a basic string = %s", ctime(&seconds));
#endif
            
            // Write current time to containter variable which can be read over BLE
            temp = ctime(&seconds);
            memcpy(getTimeValue, temp, strlen((const char *)temp));
            piraServicePtr->updateTime(getTimeValue);
           
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
            piraServicePtr->updateStatus(&piraStatus);
            // Send status to RPi -> time until next sleep and then battery level
            // pc.printf("p:%d\n", piraStatus);
            uartCommandSend('o', piraStatus);

            batteryLevelContainer = batteryVoltage.batteryLevelGet();
            piraServicePtr->updateBatteryLevel(&batteryLevelContainer);
            // pc.printf("b:%d\n", batteryLevelContainer);
            uartCommandSend('b', (uint32_t)batteryLevelContainer);

            // Send rest of the values in order to verify changes
            uartCommandSend('p', onPeriodValue);
            uartCommandSend('s', offPeriodValue);
            uartCommandSend('r', rebootThresholdValue);
            uartCommandSend('w', wakeupThresholdValue);
            // Send RPi status pin value
            uartCommandSend('a', (uint32_t)raspberryPiStatus.read());

            // Update BLE containers
            piraServicePtr->updateOnPeriodSeconds(&onPeriodValue);
            piraServicePtr->updateOffPeriodSeconds(&offPeriodValue);
            piraServicePtr->updateRebootPeriodSeconds(&rebootThresholdValue);
            piraServicePtr->updateWakeupPeriodSeconds(&wakeupThresholdValue);
#ifdef DEBUG
            pc.printf("Battery level in V = %d\n", (int)(batteryVoltage.batteryVoltageGet(batteryLevelContainer)*100));
            pc.printf("onPeriodValue = %d\n", onPeriodValue);
            pc.printf("offPeriodValue = %d\n", offPeriodValue);
            pc.printf("rebootThresholdValue = %d\n", rebootThresholdValue);
            pc.printf("wakeupThresholdValue = %d\n", wakeupThresholdValue);
            pc.printf("turnOnRpiState = %d\n", turnOnRpiState); 
            pc.printf("Status Pin = %d\n", raspberryPiStatus.read());
#endif
            raspberryPiControl.powerHandler(&raspberryPiStatus, 
                                            &powerEnable5V,
                                            onPeriodValue,
                                            offPeriodValue,
                                            wakeupThresholdValue,
                                            rebootThresholdValue,
                                            turnOnRpiState);
        }
    }
}
