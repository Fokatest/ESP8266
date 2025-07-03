# ESP8266
Measuring Temperature and Humidity with ESP8266

Electrical diagram:

RTC module:

GND - > GND
VCC - > +5V
SDA - > D2/GPIO4/SDA
SCL - > D1/GPIO5/SCL

DHL11 sensor:

+ - > 3.3V
OUT - > D5/GPIO14
- - > GND

LEDs:

Green (+) - D6/GPIO12
Red (+) - D7/GPIO13
Green/Red(-) connect to GND with a 220 Ohm resistor or 49 Ohm for brighter light.
