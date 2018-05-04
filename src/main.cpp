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
 
//Initial Time is Mon, 1 Jan 2018 00:00:00
#define TIME_INIT_VALUE  1514764800UL

DigitalOut alivenessLED(LED_1, 0);
DigitalOut actuatedLED(LED_2, 0);
DigitalOut fetOutput(FET_OUTPUT, 0);
//DigitalOut 3v3PowerEnable( , 0);
DigitalOut powerEnable5V(ENABLE_5V_PIN, 0);
DigitalOut powerEnable3V3(ENABLE_3V3_PIN, 0);

DigitalIn  raspberryPiStatus(RASPBERRY_PI_STATUS);

#if defined(DEBUG)
    // Create UART object
    Serial pc(UART_TX, UART_RX);
#endif
// Create I2C object
I2C i2c(I2C_SDA, I2C_SCL);
// Create ISL1208 object
ISL1208 rtc(&i2c);    
// RaspberryPiControl object
RaspberryPiControl raspberryPiControl;
 
const static char     DEVICE_NAME[] = "PiraSmart";
static const uint16_t uuid16_list[] = {LEDService::LED_SERVICE_UUID, PiraService::PIRA_SERVICE_UUID};
uint8_t  piraStatus;
uint32_t setTimeValue;
uint32_t onPeriodValue;
uint32_t offPeriodValue;
char getTimeValue[26] = "Tue Apr 10 12:00:00 2018\n";
char *temp;
uint8_t sendTime; 

Ticker ticker;
 
//Service pointers declarations
LEDService  *ledServicePtr;
PiraService *piraServicePtr;

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    BLE::Instance().gap().startAdvertising();
}
 
void periodicCallback(void)
{
    // Do blinky on LED1 to indicate system aliveness.
    alivenessLED = !alivenessLED; 
    // Set flag to update status every second    
    sendTime = 1; 
}
 
/**
 * This callback allows the LEDService to receive updates to the ledState Characteristic.
 *
 * @param[in] params
 *     Information about the characterisitc being updated.
 */
void onDataWrittenCallback(const GattWriteCallbackParams *params) {
    if ((params->handle == ledServicePtr->getValueHandle()) && (params->len == 1)) {
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

}
 
/**
 * This function is called when the ble initialization process has failed
 */
void onBleInitError(BLE &ble, ble_error_t error)
{
    /* Initialization error handling should go here */
#if defined(DEBUG)
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
    ble.gattServer().onDataWritten(onDataWrittenCallback);
 
    bool initialValueForLEDCharacteristic = false;
    ledServicePtr = new LEDService(ble, initialValueForLEDCharacteristic);

    setTimeValue = TIME_INIT_VALUE; 
    piraStatus = 0;
    onPeriodValue = 0;
    offPeriodValue = 0;
    piraServicePtr = new PiraService(ble, setTimeValue, piraStatus, getTimeValue, onPeriodValue, offPeriodValue);
    
    /* setup advertising */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms. */
    ble.gap().startAdvertising();
}

#if defined(DEBUG)
void init_uart(void)
{
    pc.baud(115200);
    pc.printf("Start...\n");
}
#endif

void init_rtc(void)
{
    i2c.frequency(400000); //400kHz
    
    //Try to open the ISL1208
    if (rtc.open()) 
    {
#if defined(DEBUG)
        pc.printf("Device detected!\n");
#endif
 
        //Configure the oscillator for a 32.768kHz crystal
        rtc.oscillatorMode(ISL1208::OSCILLATOR_CRYSTAL);
 
        //Check if we need to reset the time
        if (rtc.powerFailed()) 
        {
#if defined(DEBUG)
            //The time has been lost due to a power complete power failure
            pc.printf("Device has lost power! Resetting time...\n");
#endif 

            //Set RTC time to Mon, 1 Jan 2018 00:00:00
            rtc.time(TIME_INIT_VALUE);
        }
    }
    else
    {
#if defined(DEBUG)
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

#if defined(DEBUG)
    // UART needs to be initialized first to use it for debugging
    init_uart();
#endif
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
 
    while (true) {
        ble.waitForEvent();

        if (sendTime)
        {
            sendTime = 0;

            //Get the current time and send it to UART
            time_t seconds = rtc.time();
#if defined(DEBUG)
            pc.printf("Time as a basic string = %s", ctime(&seconds));
#endif
            
            // Write current time to containter variable which can be read over BLE
            temp = ctime(&seconds);
            memcpy(getTimeValue, temp, strlen((const char *)temp));
            piraServicePtr->updateTime(getTimeValue);
           
            // Update status value
            piraStatus++;
            piraServicePtr->updateStatus(&piraStatus);
            //seconds = (time_t)setTimeValue;
            //pc.printf("New Time Set from BlueTooth number = %d\n", setTimeValue);
            //pc.printf("New Time Set from BlueTooth = %s\n", ctime(&seconds));
            //rtc.time(seconds); 
            
#if defined(DEBUG)
            pc.printf("onPeriodValue = %d\n", onPeriodValue);
            pc.printf("offPeriodValue = %d\n", offPeriodValue);
#endif
            raspberryPiControl.powerHandler(&raspberryPiStatus, 
                                            &powerEnable5V,
                                            onPeriodValue,
                                            offPeriodValue);

        }
    }
}
