#include "mbed.h"
#include "app_config.h"
#include "ISL1208.h"

DigitalOut led1(LED_1);
DigitalOut led2(LED_2);
DigitalOut led3(LED_3);
InterruptIn button1(BUTTON_1);

//Serial pc(USBTX, USBRX);
Serial pc(UART_TX, UART_RX);
// Create I2C object
I2C i2c(I2C_SDA, I2C_SCL);
// Create ISL1208 object
ISL1208 rtc(&i2c);    

void button1_detect(void)
{
    pc.printf("Button pressed...\n");
    led2 = !led2;
}

int main() 
{
    //Initialize variables
    pc.baud(115200);
    pc.printf("Start...\n");

    i2c.frequency(400000); //400kHz

    button1.fall(button1_detect);
    led1 = LED_OFF;
    led2 = LED_OFF;
    led3 = LED_OFF;

    //Try to open the ISL1208
    if (rtc.open()) 
    {
        led1 = LED_ON;
        pc.printf("Device detected!\n");
 
        //Configure the oscillator for a 32.768kHz crystal
        rtc.oscillatorMode(ISL1208::OSCILLATOR_CRYSTAL);
 
        // Reset the time 
        //rtc.time(1522407240);

        //Check if we need to reset the time
        if (rtc.powerFailed()) 
        {
            led3 = LED_ON;
            //The time has been lost due to a power complete power failure
            pc.printf("Device has lost power! Resetting time...\n");

            //Set RTC time to Wed, 28 Oct 2009 11:35:37
            rtc.time(1522758120);
        }

        while(1) 
        {
            led2 = !led2;
            //Get the current time
            time_t seconds = rtc.time();
 
            //Print the time in various formats
            pc.printf("\nTime as seconds since January 1, 1970 = %d\n", seconds);
            pc.printf("Time as a basic string = %s", ctime(&seconds));
            char buffer[32];
            strftime(buffer, 32, "%I:%M %p\n", localtime(&seconds));
            pc.printf("Time as a custom formatted string = %s", buffer);
 
            //Delay for 1.0 seconds
            wait(1.0);
        }
    } 
    else 
    {
        pc.printf("Device not detected!\n");
        led3 = LED_ON;
    } 

    return 0;
}
