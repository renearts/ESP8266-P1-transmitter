# ESP8266-P1-transmitter
This sketch written in the Arduino language makes an ESP8266 send incoming P1 smartmeter data to pimatic via the HTTP API.

It currently sports an interface which is not of much use, but hey, it looks fancy ;)


### Connection of the P1 meter to the ESP8266
You need to connect the smart meter with a RJ11 connector. This is the pinout to use
![RJ11 P1 connetor](http://gejanssen.com/howto/Slimme-meter-uitlezen/RJ11-pinout.png)

Connect GND->GND on ESP, RTS->3.3V on ESP and RxD->any digital pin on ESP. In this sketch I use the RX input with a BS170 to invert te signal (see P1port.png for the connection diagram). It is also possible to use SoftSerial: uncomment lines 68/69 and 517, comment/delete line 516. 
