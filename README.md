# ESP8266-P1-transmitter
This sketch written in the Arduino language makes an ESP8266 send incoming P1 smartmeter data to thingspeak and/or MQTT.

I have used this MQTT library for pubsubclient:
https://github.com/Imroy/pubsubclient

In the future I wish to refactor this to use the more commonly used MQTT library for arduino, I also might integrate it with a nice interface alike with https://github.com/renearts/esp8266-pimatic-Arduino or esp8266.nu. It would be a lot easier to integrate with various domotics systems as it then can feature both HTTP REST and MQTT based protocols. 
