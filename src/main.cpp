#include "mbed.h"
#include "app_config.h"

DigitalOut led1(LED_1);
DigitalOut led2(LED_2);
DigitalOut led3(LED_3);
InterruptIn button1(BUTTON_1);

//Serial pc(USBTX, USBRX);
Serial pc(UART_TX, UART_RX);

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

    button1.fall(button1_detect);
    led1 = LED_OFF;
    led2 = LED_OFF;
    led3 = LED_OFF;

    while(1) 
    {
        wait(0.5);
        led1 = LED_OFF;
        wait(0.5);
        led1 = LED_ON;
    }
}
