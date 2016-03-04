#include <Time.h>
#include <ESP8266mDNS.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include "Base64.h"
#include "CRC16.h"
 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////        !!PLEASE CHANGE THESE!!
String ssid    = "WiFi SSID";
String password = "WiFi Password";

String espName    = "esp-P1";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////        NETWORK
ESP8266WebServer  server(80);
MDNSResponder   mdns;

const char* APssid = "ESPap";
const char* APpassword = "123456789";



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////         GLOBAL VARIABLES
long    lastInterval  = 0;
const int httpPort    = 80;
float   temperature   = 0.0;
float   humidity    = 0.0;

String deviceType   = "DHT22";
long sendInterval   = 10000; //in millis

String Username     = "sensors";
String Password     = "iamasensor";

char authVal[40];  
char authValEncoded[40];

String host   = "192.168.0.115";

String ClientIP;
// send data
WiFiClient client;


//uint8_t DHTPIN = 2;  //data pin, GPIO2
//uint8_t DHTTYPE = DHT22;

DHT dht(2, DHT22, 20);


// Vars to store meter readings
float mEVLT = 0; //Meter reading Electrics - consumption low tariff
float mEVHT = 0; //Meter reading Electrics - consumption high tariff
float mEOLT = 0; //Meter reading Electrics - return low tariff
float mEOHT = 0; //Meter reading Electrics - return high tariff
float mEAV = 0;  //Meter reading Electrics - Actual consumption
float mEAT = 0;  //Meter reading Electrics - Actual return
float mGAS = 0;    //Meter reading Gas
float prevGAS = 0;


#define MAXLINELENGTH 128 // longest normal line is 47 char (+3 for \r\n\0)
char telegram[MAXLINELENGTH];

// Uncomment this block to use SoftSerial
//#define SERIAL_RX     D5  // pin for SoftwareSerial RX
//SoftwareSerial mySerial(SERIAL_RX, -1, true, MAXLINELENGTH); // (RX, TX. inverted, buffer)

unsigned int currentCRC=0;
const bool outputOnSerial = true;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////         HTML SNIPPLETS
String header       =  "<html lang='en'><head><title>ESP8266 Pimatic Client</title><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='http://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script><script src='http://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body>";
String navbar       =  "<nav class='navbar navbar-default'><div class='container-fluid'><div class='navbar-header'><a class='navbar-brand' href='/'>ESP8266 Pimatic Client</a></div><div><ul class='nav navbar-nav'><li class='active'><a href='/'><span class='glyphicon glyphicon-dashboard'></span> Status</a></li><li class='dropdown'><a class='dropdown-toggle' data-toggle='dropdown' href='#'><span class='glyphicon glyphicon-cog'></span> Configure<span class='caret'></span></a><ul class='dropdown-menu'><li><a href='/cliconf'>Client</a></li><li><a href='/serconf'>Server</a></li></ul></li></ul></div></div></nav>  "; 
String navbarNonActive    = "<nav class='navbar navbar-default'><div class='container-fluid'><div class='navbar-header'><a class='navbar-brand' href='/'>ESP8266 Pimatic Client</a></div><div><ul class='nav navbar-nav'><li><a href='/'><span class='glyphicon glyphicon-dashboard'></span> Status</a></li><li class='dropdown'><a class='dropdown-toggle' data-toggle='dropdown' href='#'><span class='glyphicon glyphicon-cog'></span> Configure<span class='caret'></span></a><ul class='dropdown-menu'><li><a href='/cliconf'>Client</a></li><li><a href='/serconf'>Server</a></li></ul></li></ul></div></div></nav>  ";
String containerStart   =  "<div class='container'><div class='row'>";
String containerEnd     =  "<div class='clearfix visible-lg'></div></div></div>";
String siteEnd        =  "</body></html>";
  
String panelHeaderName    =  "<div class='col-md-4'><div class='page-header'><h1>";
String panelHeaderEnd   =  "</h1></div>";
String panelEnd       =  "</div>";
  
String panelBodySymbol    =  "<div class='panel panel-default'><div class='panel-body'><span class='glyphicon glyphicon-";
String panelBodyName    =  "'></span> ";
String panelBodyValue   =  "<span class='pull-right'>";
String panelBodyEnd     =  "</span></div></div>";

String inputBodyStart   =  "<form action='' method='POST'><div class='panel panel-default'><div class='panel-body'>";
String inputBodyName    =  "<div class='form-group'><div class='input-group'><span class='input-group-addon' id='basic-addon1'>";
String inputBodyPOST    =  "</span><input type='text' name='";
String inputBodyClose   =  "' class='form-control' aria-describedby='basic-addon1'></div></div>";
String inputBodyEnd     =  "</div><div class='panel-footer clearfix'><div class='pull-right'><button type='submit' class='btn btn-default'>Send</button></div></div></div></form>";


//String landingNav     = "<nav class='navbar navbar-default'> <div class='container-fluid'> <div class='navbar-header'> <a class='navbar-brand' href='/'>ESP8266 Pimatic Client</a> </div></div></nav><br><br><br>";
//String landingStartPartA  = "<div class='container'> <div class='row' > <div class='col-md-offset-3 col-md-6'> <div class='panel panel-default'> <div class='panel-heading'> Please login to your WiFi Network </div><div class='panel-body'> <form method='post' action=''> <div class='form-group'><div class='input-group'><span class='input-group-addon' id='basic-addon1'>Wifi Name </span><input type='text' class='form-control' placeholder='Enter SSID' aria-describedby='basic-addon1' name='wifiname'></div></div>";
//String landingStartPartB    =   "<div class='form-group'><div class='input-group'><span class='input-group-addon' id='basic-addon1'>WiFi Pass </span><input type='password' class='form-control' placeholder='Password' aria-describedby='basic-addon1' name='wifipass'></div></div><div class='form-group'><div class='input-group'><span class='input-group-addon' id='basic-addon1'>Device Name ";
//String landingStartPartC      =    "</span><input type='text' class='form-control' placeholder='Name your device e.g. kitchen' aria-describedby='basic-addon1' name='devicename'></div></div><a class='btn btn-primary' data-toggle='collapse' href='#collapseExample' aria-expanded='false' aria-controls='collapseExample'> Available Networks </a> <div class='pull-right'> <button type='submit' class='btn btn-default'>Send</button> </div></form> <div class='collapse' id='collapseExample'> <div class='well'>";
//String landingEnd     = "</div></div></div></div></div></div></body></html>";


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////         ROOT 
void handle_root() {

  Serial.println(server.args());
  
  // get IP
  IPAddress ip = WiFi.localIP();
  ClientIP = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  delay(500);
  
  String title1     = panelHeaderName + String("Sensor Data") + panelHeaderEnd;
  String Humidity   = panelBodySymbol + String("tint") + panelBodyName + String("Humidity") + panelBodyValue + humidity + String("%") + panelBodyEnd;
  String Temperature    = panelBodySymbol + String("fire") + panelBodyName + String("Temperature") + panelBodyValue + temperature + String("°C") + panelBodyEnd + panelEnd;
  
  String title2     = panelHeaderName + String("Client Settings") + panelHeaderEnd;
  String IPAddClient    = panelBodySymbol + String("globe") + panelBodyName + String("IP Address") + panelBodyValue + ClientIP + panelBodyEnd;
  String DeviceType   = panelBodySymbol + String("scale") + panelBodyName + String("Device Type") + panelBodyValue + deviceType + panelBodyEnd;
  String ClientName   = panelBodySymbol + String("tag") + panelBodyName + String("Client Name") + panelBodyValue + espName + panelBodyEnd;
  String Interval   = panelBodySymbol + String("hourglass") + panelBodyName + String("Interval") + panelBodyValue + sendInterval + String(" millis") + panelBodyEnd;
  String Uptime     = panelBodySymbol + String("time") + panelBodyName + String("Uptime") + panelBodyValue + hour() + String(" h ") + minute() + String(" min ") + second() + String(" sec") + panelBodyEnd + panelEnd;
  
  String title3     = panelHeaderName + String("Server Settings") + panelHeaderEnd;
  String IPAddServ    = panelBodySymbol + String("globe") + panelBodyName + String("IP Address") + panelBodyValue + host + panelBodyEnd;
  String User     = panelBodySymbol + String("user") + panelBodyName + String("Username") + panelBodyValue + Username + panelBodyEnd + panelEnd;
  
  
  //String data = title1 + Humidity + Temperature + title2 + IPAddClient + DeviceType + ClientName + Interval + Uptime + title3 + IPAddServ + User;
  //server.send ( 200, "text/html", header + navbar + containerStart + data + containerEnd + siteEnd );
   server.send ( 200, "text/html", header + navbar + containerStart + title1 + Humidity + Temperature + title2 + IPAddClient + DeviceType + ClientName + Interval + Uptime + title3 + IPAddServ + User + containerEnd + siteEnd);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////         CONFIG - Client
void handle_cliconf() {

  String payload=server.arg("name");
  if (payload.length() > 0 ) {
    espName = payload;
  }
  Serial.println(payload);

  payload=server.arg("type");
  if (payload.length() > 0 ) {
    deviceType = payload;
  }
  Serial.println(payload);

  payload=server.arg("interval");
  if (payload.length() > 0 ) {
    sendInterval = payload.toInt();
  }
  Serial.println(payload);
  
  String title1 = panelHeaderName + String("Client Configuration") + panelHeaderEnd; 
  
  String data = title1 + inputBodyStart + inputBodyName + String("Name") + inputBodyPOST + String("name") + inputBodyClose + inputBodyName + String("Device Type") + inputBodyPOST + String("type") + inputBodyClose + inputBodyName + String("Interval in seconds") + inputBodyPOST + String("interval") + inputBodyClose + inputBodyEnd;
  server.send ( 200, "text/html", header + navbarNonActive + containerStart + data + containerEnd + siteEnd );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////         CONFIG - Server
void handle_serconf() {
  
  String payload=server.arg("server");
  if (payload.length() > 0 ) {
    host = payload;
  }
  Serial.println(payload);
  
  payload=server.arg("user");
  if (payload.length() > 0 ) {
    Username = payload;
  }
  Serial.println(payload);

  payload=server.arg("password");
  if (payload.length() > 0 ) {
    Password = payload;
  }
  Serial.println(payload);
  
  String title1 = panelHeaderName + String("Server Configuration") + panelHeaderEnd;
  
  String data = title1 + inputBodyStart + inputBodyName + String("Pimatic Server") + inputBodyPOST + String("server")  + inputBodyClose + inputBodyName + String("Username") + inputBodyPOST + String("user") + inputBodyClose + inputBodyName + String("Password") + inputBodyPOST + String("password") + inputBodyClose + inputBodyEnd;
  
  server.send ( 200, "text/html", header + navbarNonActive + containerStart + data + containerEnd + siteEnd);
  
  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////         LANDING
/*
void landing() {
  
  String payload=server.arg("wifiname");
  if (payload.length() > 0 ) {
    ssid = payload;
  }
  Serial.println(payload);
  payload=server.arg("wifipass");
  if (payload.length() > 0 ) {
    password = payload;
  }
  Serial.println(payload);
  
  payload=server.arg("devicename");
  if (payload.length() > 0 ) {
    espName = payload;
  }
  Serial.println(payload);
  
  String landing = header + landingNav + landingStartPartA + landingStartPartB + landingStartPartC + landingEnd;
  server.send ( 200, "text/html", landing);
}
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////         SEND DATA
bool send_data(float data, String sensorname) {
  
  String yourdata;
    
  char host_char_array[host.length()+1];
  host.toCharArray(host_char_array,host.length()+1);
  
  if (!client.connect(host_char_array, httpPort)) {
    Serial.println("connection failed");
    return 0;
  }
  
  // calculate Base64Login
  memset(authVal,0,40);
  (Username + String(":") + Password).toCharArray(authVal, 40);
  memset(authValEncoded,0,40);
  base64_encode(authValEncoded, authVal, (Username + String(":") + Password).length());
  
//  char base64login[40];
  
//  (Username + String(":") + Password).toCharArray(base64login, 40);
  
  //Send Humidity
  yourdata = "{\"type\": \"value\", \"valueOrExpression\": \"" + String(data, 3) + "\"}";
    
  client.print("PATCH /api/variables/");
  client.print(sensorname);
  client.print(" HTTP/1.1\r\n");
  client.print("Authorization: Basic ");
  client.print(authValEncoded);
  client.print("\r\n");
  client.print("Host: " + host +"\r\n");
  client.print("Content-Type:application/json\r\n");
  client.print("Content-Length: ");
  client.print(yourdata.length());
  client.print("\r\n\r\n");
  client.print(yourdata);
    
  delay(100);  
  while (client.available()) {
    String line = client.readStringUntil('\r');
  }
  return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////         AP
void setupAP(void) {
  delay(100);
  WiFi.softAP(APssid,APpassword,6);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

void UpdateGas()
{
  //sends over the gas setting to domoticz
  if(prevGAS!=mGAS)
  {
    char sFloatString[16];
//    dtostrf(mGAS,9,3,sFloatString);
    if(send_data(mGAS,"mGAS"))
      prevGAS=mGAS;
  }
}

void UpdateElectricity()
{
  char sFloatString[16];
//  sprintf(sValue, "%d;%d;%d;%d;%d;%d", mEVLT, mEVHT, mEOLT, mEOHT, mEAV, mEAT);
//  SendToDomo(domoticzEneryIdx, 0, sValue);
//  dtostrf(mEVLT,9,3,sFloatString);
  send_data(mEVLT,"mEVLT");
//  dtostrf(mEVHT,9,3,sFloatString);
  send_data(mEVHT,"mEVHT");
//  dtostrf(mEOLT,9,3,sFloatString);
  send_data(mEOLT,"mEOLT");
//  dtostrf(mEOHT,9,3,sFloatString);
  send_data(mEOHT,"mEOHT");
//  dtostrf(mEAV,9,3,sFloatString);
  send_data(mEAV,"mEAV");
//  dtostrf(mEAT,9,3,sFloatString);
  send_data(mEAT,"mEAT");
}

bool isNumber(char* res, int len) {
  for (int i = 0; i < len; i++) {
    if (((res[i] < '0') || (res[i] > '9'))  && (res[i] != '.' && res[i] != 0)) {
      return false;
    }
  }
  return true;
}

int FindCharInArrayRev(char array[], char c, int len) {
  for (int i = len - 1; i >= 0; i--) {
    if (array[i] == c) {
      return i;
    }
  }
  return -1;
}

long getValidVal(long valNew, long valOld, long maxDiffer)
{
  //check if the incoming value is valid
      if(valOld > 0 && ((valNew - valOld > maxDiffer) && (valOld - valNew > maxDiffer)))
        return valOld;
      return valNew;
}

float getValue(char* buffer, int maxlen) {
  int s = FindCharInArrayRev(buffer, '(', maxlen - 2);
  if (s < 8) return 0;
  if (s > 32) s = 32;
  int l = FindCharInArrayRev(buffer, '*', maxlen - 2) - s - 1;
  if (l < 4) return 0;
  if (l > 12) return 0;
  char res[16];
  memset(res, 0, sizeof(res));

  if (strncpy(res, buffer + s + 1, l)) {
    if (isNumber(res, l)) {
      return (atof(res));
    }
  }
  return 0;
}

bool decodeTelegram(int len) {
  //need to check for start
  int startChar = FindCharInArrayRev(telegram, '/', len);
  int endChar = FindCharInArrayRev(telegram, '!', len);
  bool validCRCFound = false;
  if(startChar>=0)
  {
    //start found. Reset CRC calculation
    currentCRC=CRC16(0x0000,(unsigned char *) telegram+startChar, len-startChar);
    if(outputOnSerial)
    {
      for(int cnt=startChar; cnt<len-startChar;cnt++)
        Serial.print(telegram[cnt]);
    }    
    //Serial.println("Start found!");
    
  }
  else if(endChar>=0)
  {
    //add to crc calc 
    currentCRC=CRC16(currentCRC,(unsigned char*)telegram+endChar, 1);
    char messageCRC[4];
    strncpy(messageCRC, telegram + endChar + 1, 4);
    if(outputOnSerial)
    {
      for(int cnt=0; cnt<len;cnt++)
        Serial.print(telegram[cnt]);
    }    
    validCRCFound = (strtol(messageCRC, NULL, 16) == currentCRC);
    if(validCRCFound)
      Serial.println("\nVALID CRC FOUND!"); 
    else
      Serial.println("\n===INVALID CRC FOUND!===");
    currentCRC = 0;
  }
  else
  {
    currentCRC=CRC16(currentCRC, (unsigned char*)telegram, len);
    if(outputOnSerial)
    {
      for(int cnt=0; cnt<len;cnt++)
        Serial.print(telegram[cnt]);
    }
  }

  long val =0;
  long val2=0;
  // 1-0:1.8.1(000992.992*kWh)
  // 1-0:1.8.1 = Elektra verbruik laag tarief (DSMR v4.0)
  if (strncmp(telegram, "1-0:1.8.1", strlen("1-0:1.8.1")) == 0) 
    mEVLT =  getValue(telegram, len);
  

  // 1-0:1.8.2(000560.157*kWh)
  // 1-0:1.8.2 = Elektra verbruik hoog tarief (DSMR v4.0)
  if (strncmp(telegram, "1-0:1.8.2", strlen("1-0:1.8.2")) == 0) 
    mEVHT = getValue(telegram, len);
    

  // 1-0:2.8.1(000348.890*kWh)
  // 1-0:2.8.1 = Elektra opbrengst laag tarief (DSMR v4.0)
  if (strncmp(telegram, "1-0:2.8.1", strlen("1-0:2.8.1")) == 0) 
    mEOLT = getValue(telegram, len);
   

  // 1-0:2.8.2(000859.885*kWh)
  // 1-0:2.8.2 = Elektra opbrengst hoog tarief (DSMR v4.0)
  if (strncmp(telegram, "1-0:2.8.2", strlen("1-0:2.8.2")) == 0) 
    mEOHT = getValue(telegram, len);
    

  // 1-0:1.7.0(00.424*kW) Actueel verbruik
  // 1-0:2.7.0(00.000*kW) Actuele teruglevering
  // 1-0:1.7.x = Electricity consumption actual usage (DSMR v4.0)
  if (strncmp(telegram, "1-0:1.7.0", strlen("1-0:1.7.0")) == 0) 
    mEAV = getValue(telegram, len);
    
  if (strncmp(telegram, "1-0:2.7.0", strlen("1-0:2.7.0")) == 0)
    mEAT = getValue(telegram, len);
   

  // 0-1:24.2.1(150531200000S)(00811.923*m3)
  // 0-1:24.2.1 = Gas (DSMR v4.0) on Kaifa MA105 meter
  if (strncmp(telegram, "0-1:24.2.1", strlen("0-1:24.2.1")) == 0) 
    mGAS = getValue(telegram, len);

  return validCRCFound;
}

// uncomment this block to use SoftSerial
/* 
void readTelegramSoftSerial() {
  if (mySerial.available()) {
    memset(telegram, 0, sizeof(telegram));
    while (mySerial.available()) {
      int len = mySerial.readBytesUntil('\n', telegram, MAXLINELENGTH);
      telegram[len] = '\n';
      telegram[len+1] = 0;
      yield();
      if(decodeTelegram(len+1))
      {
         UpdateElectricity();
         UpdateGas();
      }
    } 
  }
}
*/

// Comment or delete this block to disable use of hardware serial
void readTelegram() {
  if (Serial.available()) {
    memset(telegram, 0, sizeof(telegram));
    while (Serial.available()) {
      int len = Serial.readBytesUntil('\n', telegram, MAXLINELENGTH);
      telegram[len] = '\n';
      telegram[len+1] = 0;
      yield();
      if(decodeTelegram(len+1))
      {
         UpdateElectricity();
         UpdateGas();
      }
    } 
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////         SETUP 
void setup(void)
{
  Serial.begin(115200);
  //mySerial.begin(115200);  // uncomment this line to use SoftSerial
   
      WiFi.begin(ssid.c_str(), password.c_str());
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      } 
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      
      server.on("/", handle_root);
  
      server.on("/cliconf", handle_cliconf);
  
      server.on("/serconf", handle_serconf);

      //server.on("/landing", landing);
  
      if (!mdns.begin(espName.c_str(), WiFi.localIP())) {
        Serial.println("Error setting up MDNS responder!");
        while(1) { 
          delay(1000);
        }
      }
      server.begin();
      Serial.println("HTTP server started");
      dht.begin();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////           MAIN 
void loop(void)
{
  
//    if (millis() - lastInterval > sendInterval) {
//      send_data();
//      lastInterval = millis();
//    }
  readTelegram();
  //readTelegramSoftSerial();
  server.handleClient();
  
} 

