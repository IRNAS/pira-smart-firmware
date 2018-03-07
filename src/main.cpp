#include "mbed.h"
#include "app_config.h"

DigitalOut led1(LED_1);
InterruptIn button1(BUTTON_1);

//Serial pc(USBTX, USBRX);
Serial pc(UART_TX, UART_RX);

void button1_detect(void)
{
    pc.printf("Button pressed...\n");
}

int main() 
{
    //Initialize variables
    pc.baud(115200);
    pc.printf("Start...\n");

    button1.fall(button1_detect);
    led1 = 0;

    while(1) 
    {
        wait(0.5);
        led1 = 1;
        wait(0.5);
        led1 = 0;
    }
}
