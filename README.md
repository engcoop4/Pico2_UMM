The Universal Multi-Meter (UMM) project is designed to use ADCs to monitor voltage levels and calculate/display values like AC Current, DC Current, AC Voltage, DC Voltage, Frequency, Wattage, 1-Phase Power, 3-Phase Power, and any additional values that can be determined via metering. This device is meant to offer and combine measurements for all these into one device. 
In terms of how the programming operates, the core components are: GUI Display, Touch Screen, I2C Extensions (LEDs and Buttons), Command Processing, SPI, and PI/O. 

ScreenDisplaysInits.c, ScreenDisplaysInits.h, LCDProcessing.c, and LCDProcessing.h all deal with the GUI Display including the base level functions and the actual menu displays. These operate for both the NewHaven screen and the Adafruit screen.

TouchScreeninit.c, and TouchScreeninit.h both deal with the touch screen initializations, calibrations, sampling, and all other handlings required for it to function in addition to the button inputs. These operate for both the NewHaven screen and the Adafruit screen.

I2CExtension.c, and I2CExtension.h both deal with the buttons and LED implementation created using the I2C Extenders. This will need to be modified to include additional I2C Extenders when the blade board architecture is further developed, connections are planned under "Pin_Assignments_Comprehensive."

cmdProcessing.c, and cmdProcessing.h both deal with the serial communication for sending commands to the device. These commands can be used for device configuration, screen configuration, and other settings such as baud rate. 

Hardware.c, and Hardware.h contain programming for the hardware components and overall general functions for the core capabilities of the program. 

Global.h acts as a header file to main.c almost, it holds many of the variable definitions used across the different files.

UART.c, and UART.h are currently unused, but UART will be needed for RS-485 communication which is eventually planned. File names may be changed, but for now these operate as placeholders.

initSPI.c, and initSPI.h are currently unused as they were previously used for screen display but were replaced with PI/O. These files can be renamed and used for the ADC SPI communication.

screen_spi.pio contains the programming for SPI clock and MOSI emulation. In order to bypass SPI bottleneck with the ADC and limitations from the Ethernet compatibility, the Raspberry Pi's built in Programmable I/Os can be used to emulate the SCLK and MOSI signal that SPI would generate. These operate with
their own assembly language exclusive to Raspberry Pi, but there are a lot of online resources for how to handle this.
