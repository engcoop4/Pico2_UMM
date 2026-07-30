The Universal Multi-Meter (UMM) project is designed to use ADCs to monitor voltage levels and calculate/display values like AC Current, DC Current, AC Voltage, DC Voltage, Frequency, Wattage, 1-Phase Power, 3-Phase Power, and any additional values that can be determined via metering. This device is meant to offer and combine measurements for all these into one device. 
In terms of how the programming operates, the core components are: GUI Display, Touch Screen, I2C Extensions (LEDs and Buttons), and Command Processing. 

ScreenDisplaysInits.c, ScreenDisplaysInits.h, LCDProcessing.c, and LCDProcessing.h all deal with the GUI Display including the base level functions and the actual menu displays.

TouchScreeninit.c, and TouchScreeninit.h both deal with the touch screen initializations, calibrations, sampling, and all other handlings required for it to function in addition to the button inputs.

I2CExtension.c, and I2CExtension.h both deal with the buttons and LED implementation created using the I2C Extenders. This will need to be modified to include additional I2C Extenders when the blade board architecture is further developed.
