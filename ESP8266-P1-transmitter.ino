
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>

String apiKey = "yourapikey";
const char* ssid = "yourWifiSSID";
const char* password = "password";

#define BUFFER_SIZE 100

int messages_recvd = 0;

MDNSResponder mdns;

ESP8266WebServer server ( 80 );

const char* TSP_server = "api.thingspeak.com";

IPAddress mqttserver(127,0,0,1); // fill in your MQTT server

WiFiClient wclient;
PubSubClient client(wclient, mqttserver);

char lastmessage_recvd[100];
String lastmessage;

char count = 0;
int incomingByte = 0;
int T1_pos;
int T2_pos;
int T7_pos;
int T8_pos;
int P1_pos;
int P2_pos;
int P3_pos;
int G1_pos;
String inputString;
String T1;
String T2;
String T7;
String T8;
String P1;
String P2;
String P3;
String G1;
String outputString;
String lastString;

const int led = 13;

void callback(const MQTT::Publish& pub) {
  messages_recvd++;
  Serial.print(pub.topic());
  Serial.print(" => ");
  if (pub.has_stream()) {
    uint8_t buf[BUFFER_SIZE];
    int read;
    while (read = pub.payload_stream()->read(buf, BUFFER_SIZE)) {
      Serial.write(buf, read);
    }
    pub.payload_stream()->stop();
    Serial.println("");
  } else
    Serial.println(pub.payload_string());
    
    String message = "I received ";
    message += messages_recvd;
    message += " messages\n";
  
    lastmessage = pub.payload_string();    
    lastmessage.toCharArray(lastmessage_recvd,pub.payload_len()+1);
    
//    snprintf("I received %d messages",messages_recvd)
//    MQTT::Publish newpub("/test","I received %d messages",messages_recvd);
//    MQTT::Publish newpub("outTopic", pub.payload(), pub.payload_len());
    client.publish("/test",message);
        
}

void handleRoot() {
	digitalWrite ( led, 1 );
	char temp[600];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	snprintf ( temp, 600,

"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <p><h2>Last received MQTT message: </h2></p>\
    <p>%s<p>\
    <p><h2>Last received meter values:</h2></p>\
    <p>T1: %d</p>\
    <p>T2: %d</p>\
    <p>P1: %d</p>\
    <p>P2: %d</p>\
    <p>G1: %d</p>\
  </body>\
</html>",

		hr, min % 60, sec % 60, lastmessage_recvd, T1.toInt(), T2.toInt(), (P1.toInt())*1000, P2.toInt(), G1.toInt()
	);
	server.send ( 200, "text/html", temp );
	digitalWrite ( led, 0 );
}

void handleNotFound() {
	digitalWrite ( led, 1 );
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for ( uint8_t i = 0; i < server.args(); i++ ) {
		message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
	}

	server.send ( 404, "text/plain", message );
	digitalWrite ( led, 0 );
}

void setup ( void ) {
	pinMode ( led, OUTPUT );
	digitalWrite ( led, 0 );
	Serial.begin ( 115200 );
	WiFi.begin ( ssid, password );
	Serial.println ( "" );

	// Wait for connection
	while ( WiFi.status() != WL_CONNECTED ) {
		delay ( 500 );
		Serial.print ( "." );
	}

	Serial.println ( "" );
	Serial.print ( "Connected to " );
	Serial.println ( ssid );
	Serial.print ( "IP address: " );
	Serial.println ( WiFi.localIP() );

	if ( mdns.begin ( "esp8266", WiFi.localIP() ) ) {
		Serial.println ( "MDNS responder started" );
	}

  client.set_callback(callback);
  
	server.on ( "/", handleRoot );
	server.on ( "/inline", []() {
		server.send ( 200, "text/plain", "this works as well" );
	} );
	server.onNotFound ( handleNotFound );
	server.begin();
	Serial.println ( "HTTP server started" );
}

void loop ( void ) {
	mdns.update();
	server.handleClient();
  
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      if (client.connect("arduinoClient")) {
        // subscribe to test1 topic on MQTT. This 
        client.subscribe("/test1");
      }
    }

    if (client.connected()) {
      CheckSerial();      
      client.loop();    
    }
  }
}

void CheckSerial(){
  while (Serial.available() > 0) {
    
    incomingByte = Serial.read();    
    char inChar = (char)incomingByte;
    inputString += inChar; 
   }


   //If output from Smart meter is long enough, process it. Length needs to be checked individually just in case
   if (inputString.endsWith("!")) {
//      Serial.println(inputString);
      lastString = inputString;

      //Publish entire unparsed string to MQTT for external parsing
      client.publish("/test/P1_string",inputString);

      // Extract substings/values
      T1_pos = inputString.indexOf("1-0:1.8.1", 0);
      T1 = inputString.substring(T1_pos + 10, T1_pos + 20);
 
      T2_pos = inputString.indexOf("1-0:1.8.2", T1_pos + 1);
      T2 = inputString.substring(T2_pos + 10, T2_pos + 20);
      
      T7_pos = inputString.indexOf("1-0:2.8.1", T1_pos + 1);
      T7 = inputString.substring(T7_pos + 10, T7_pos + 20);
      
      T8_pos = inputString.indexOf("1-0:2.8.2", T7_pos + 1);
      T8 = inputString.substring(T8_pos + 10, T8_pos + 20);
      
      P1_pos = inputString.indexOf("1-0:1.7.0", T8_pos + 1);
      P1 = inputString.substring(P1_pos + 10, P1_pos + 16);
      
      P2_pos = inputString.indexOf("1-0:2.7.0", P1_pos + 1);
      P2 = inputString.substring(P2_pos + 10, P2_pos + 16);

      G1_pos = inputString.indexOf("0-1:24.2.1", P2_pos + 1);
      G1 = inputString.substring(G1_pos + 26, G1_pos + 35);

      // generate outputring
      outputString = "{T1:" + T1 + ",T2:" + T2 + ",T7:" + T7 + ",T8:" + T8 + ",P1:" + P1 + ",P2:" + P2  + ",G1:" + G1 + "}";
      client.publish("/test/P1",outputString);
      client.publish("/sensor/P1_meter/T1",T1);
      client.publish("/sensor/P1_meter/T2",T2);
      client.publish("/sensor/P1_meter/T7",T7);
      client.publish("/sensor/P1_meter/T8",T8);
      client.publish("/sensor/P1_meter/P1",P1);
      client.publish("/sensor/P1_meter/P2",P2);
      client.publish("/sensor/P1_meter/G1",G1);
      
      if (count == 2) {
        count = 0;
        SendToThingSpeak();
      } else {
        count++;
      }
            
      inputString = "";
   }
  
}

void SendToThingSpeak() {
  if (wclient.connect(TSP_server,80)) {  //   "184.106.153.149" or api.thingspeak.com
    String postStr = apiKey;
         postStr +="&field1=";
         postStr += T1;
         postStr +="&field2=";
         postStr += T2;
         postStr +="&field3=";
         postStr += P1;
         postStr +="&field4=";
         postStr += G1;
         postStr += "\r\n\r\n";
    
    wclient.print("POST /update HTTP/1.1\n");
    wclient.print("Host: api.thingspeak.com\n");
    wclient.print("Connection: close\n");
    wclient.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
    wclient.print("Content-Type: application/x-www-form-urlencoded\n");
    wclient.print("Content-Length: ");
    wclient.print(postStr.length());
    wclient.print("\n\n");
    wclient.print(postStr);
  }
  wclient.stop();
  
}

