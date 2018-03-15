#include "mbed.h"
#include "app_config.h"
#include "gnss.h"

#define CHECK_TALKER(s) ((buffer[3] == s[0]) && (buffer[4] == s[1]) && (buffer[5] == s[2]))

DigitalOut ledGreen(LED_1);
DigitalOut ledBlue(LED_2);
DigitalOut ledRed(LED_3);
InterruptIn button1(BUTTON_1);

//Serial USBTX, USBRX);
Serial pc(UART_TX, UART_RX);

void button1_detect(void)
{
    pc.printf("Button pressed...\n");
    //led2 = !led2;
}

int main() 
{
    //Initialize variables
    pc.baud(9600);
    pc.printf("Start...\n");

    GnssI2C gnss(I2C_SDA, I2C_SCL);
    int gnssReturnCode;
    int length;
    char buffer[256];
    
    pc.printf ("Starting up...\n");
    if (gnss.init()) {
        pc.printf ("Waiting for GNSS to receive something...\n");
        while (1) {
            gnssReturnCode = gnss.getMessage(buffer, sizeof(buffer));
            if (gnssReturnCode > 0) {
                ledGreen = 0;
                ledBlue = 1;
                ledRed = 1;
                length = LENGTH(gnssReturnCode);

                pc.printf("NMEA: %.*s\n", length - 2, buffer);

                if ((PROTOCOL(gnssReturnCode) == GnssParser::NMEA) && (length > 6)) {
                    // Talker is $GA=Galileo $GB=Beidou $GL=Glonass $GN=Combined $GP=GNSS
                    if ((buffer[0] == '$') || buffer[1] == 'G') {
                      if (CHECK_TALKER("GLL")) {
                            double latitude = 0, longitude = 0;
                            char ch;

                            if (gnss.getNmeaAngle(1, buffer, length, latitude) && 
                                gnss.getNmeaAngle(3, buffer, length, longitude) && 
                                gnss.getNmeaItem(6, buffer, length, ch) && (ch == 'A')) {
                                ledBlue = 0;
                                ledRed = 0;
                                ledGreen = 0;

                                pc.printf("\nGNSS: location is %.5f %.5f.\n\n", latitude, longitude); 
                                pc.printf("I am here: https://maps.google.com/?q=%.5f,%.5f\n\n",
                                       latitude, longitude); 
                            }
                        } else if (CHECK_TALKER("GGA") || CHECK_TALKER("GNS")) {
                            double altitude = 0; 
                            const char *timeString = NULL;

                            // Altitude
                            if (gnss.getNmeaItem(9, buffer, length, altitude)) {
                                pc.printf("\nGNSS: altitude is %.1f m.\n", altitude); 
                            }

                            // Time
                            timeString = gnss.findNmeaItemPos(1, buffer, buffer + length);
                            if (timeString != NULL) {
                                ledBlue = 0;
                                ledRed = 1;
                                ledGreen = 1;

                                pc.printf("\nGNSS: time is %.6s.\n\n", timeString);
                            }
                        } else if (CHECK_TALKER("VTG")) {
                            double speed = 0; 

                            // Speed
                            if (gnss.getNmeaItem(7, buffer, length, speed)) {
                                pc.printf("\nGNSS: speed is %.1f km/h.\n\n", speed);
                            }
                        }
                    }
                }
            }
        }
    } else {
        pc.printf("Unable to initialise GNSS.\n");
    }

    ledRed = 0;
    ledGreen = 1;
    ledBlue = 1;
    pc.printf("Should never get here.\n");
    MBED_ASSERT(false);
//    button1.fall(button1_detect);
//    led1 = LED_OFF;
//    led2 = LED_OFF;
//    led3 = LED_OFF;
//
//    while(1) 
//    {
//        wait(0.5);
//        led1 = LED_OFF;
//        wait(0.5);
//        led1 = LED_ON;
//    }
}
