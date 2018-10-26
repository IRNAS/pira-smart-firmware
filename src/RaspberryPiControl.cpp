#include "RaspberryPiControl.h"

RaspberryPiControl::RaspberryPiControl(void) {
    //Constructor
    state = WAIT_STATUS_ON_STATE;
    timeoutOn = 0;
    timeoutOff = 0;
    timeoutReboot = 0;
    rpi_on = false;
}

void RaspberryPiControl::powerHandler(DigitalIn *raspberryPiStatus, 
                                      DigitalOut *powerEnable5V,
                                      uint32_t onThreshold,
                                      uint32_t offThreshold,
                                      uint32_t wakeupThreshold,
                                      uint32_t rebootThreshold,
                                      bool forceOffPeriodEnd)
{
    switch(state) {
        case IDLE_STATE:
            #ifdef DEBUG
                pc.printf("IDLE_STATE\n");
            #endif
            //Check if we need to wakeup RaspberryPi
            timeoutOff++;
            // Typical usecase would be that wakeupThreshold < offThreshold
            if ((timeoutOff >= offThreshold)    || 
                (timeoutOff >= wakeupThreshold) || 
                forceOffPeriodEnd) 
            {
                timeoutOff = 0;
                // Turn OFF RaspberryPi and set on threshold value
                //Turn ON 5V power supply
                powerEnable5V->write(1);
                state = WAIT_STATUS_ON_STATE;
            }
            break;

        case WAIT_STATUS_ON_STATE:
            #ifdef DEBUG
                pc.printf("WAIT_STATUS_ON_STATE\n");
            #endif
            //Wait when RaspberryPi pulls up STATUS pin
            //NOTE: Temporarely check reversed logic
            timeoutOn++;

            if (raspberryPiStatus->read()) {
                //timeoutOn = 0; // we should reset timer?
                //Add some timeout also
                state = WAKEUP_STATE;
            }
            else if (timeoutOn >= onThreshold) {
                //Turn Off 5V power supply
                powerEnable5V->write(0);
                //Reset timeout counter
                timeoutOn = 0;
                forceOffPeriodEnd = false;
                state = IDLE_STATE;
            }
            break;

        case WAKEUP_STATE:
            rpi_on = true;
            #ifdef DEBUG
                pc.printf("WAKEUP_STATE\n");
            #endif
            //Send sensor data and when wakeup period set, shutdown
            //In order for timeout to work, this function should be executed every 1s

            //Check status pin and then turn off power supply.
            //Or after timeout, turn off power supply anyway without waiting for status.
            if (!raspberryPiStatus->read()) {
                timeoutReboot = 0; 
                timeoutOn = 0;
                rpi_on = false;
                state = REBOOT_DETECTION;
                break;
            }
            timeoutOn++;

            if (timeoutOn >= onThreshold) {
                //Turn Off 5V power supply
                powerEnable5V->write(0);
                //Reset timeout counter
                timeoutOn = 0;
                forceOffPeriodEnd = false;
                rpi_on = false;
                state = IDLE_STATE;
            }
            break;

        case REBOOT_DETECTION:
            #ifdef DEBUG
                pc.printf("REBOOT_DETECTION\n");
            #endif
            //Wait for reboot timeout
            timeoutReboot++;
            if (timeoutReboot >= rebootThreshold) {
                timeoutReboot = 0;
                if(raspberryPiStatus->read()) {
                    state = WAKEUP_STATE;
                }
                else {
                    //Definitely turn off RPi power supply
                    powerEnable5V->write(0);
                    forceOffPeriodEnd = false;
                    state = IDLE_STATE;
                }
            }   
            break;

        default:
            break;
    }
}

