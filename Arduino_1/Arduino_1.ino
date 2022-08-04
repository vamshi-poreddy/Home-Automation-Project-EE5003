#include <SPI.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>

#include "wifi_cred.h"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
BLEService pirService("19B10010-E8F2-537E-4F6C-D104768A1214"); // create service
BLEByteCharacteristic pirCharacteristic("19B10012-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
const int led_pin = 11;
const int PIn = 2;     // the number of the input pin
int pirState = 0;
const char broker[] = "10.13.122.148"; // Address of the MQTT server
const int  port     = 1883;
const int serverPort = 4080;
const char topic[] = "ToArduino/pir";
const char topic1[] = "ToHost/pir";
String msg = "";
int count =0; 
const long interval = 10000;
unsigned long previousMillis = 0;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  //while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  //}
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.print("You're connected to the network");
  printWifiData();
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (true);
  }
  
  BLE.setLocalName("PIR_status");   // set the local name peripheral advertises
  BLE.setAdvertisedService(pirService);   // set the UUID for the service this peripheral advertises:

  pirService.addCharacteristic(pirCharacteristic);
  BLE.addService(pirService);
  pirCharacteristic.writeValue(0);
  BLE.advertise();
  
  mqttClient.setUsernamePassword(Mqtt_User, Mqtt_Pass);
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
  }
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  Serial.print("Subscribing to topic: ");
  Serial.println(topic);
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe(topic);
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
  //pinMode(PIn, INPUT);
  
}

void loop() {
  BLE.poll();
  mqttClient.poll();
  pirState = digitalRead(PIn);
  pirCharacteristic.writeValue(pirState);
  if (pirState == HIGH) {
    // turn LED on:
    digitalWrite(led_pin, HIGH);
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
    mqttClient.beginMessage(topic1);
    mqttClient.print("HIGH");
    mqttClient.endMessage();
    //count++;
    }
  }
  else{
  int messageSize = mqttClient.parseMessage();
  if (messageSize) {
    int i =0;
    // we received a message, print out the topic and contents
    Serial.print("Received a message with topic '");
    Serial.print(mqttClient.messageTopic());
    Serial.print("', length ");
    Serial.print(messageSize);
    Serial.println(" bytes:");

    // use the Stream interface to print the contents
    while (mqttClient.available()) {
      msg= msg+(char)mqttClient.read();
    }
    Serial.println(msg);
    if(msg == "1") {
      digitalWrite(led_pin, HIGH);
    } else {
      digitalWrite(led_pin, LOW);
    }
    msg="";
  }
  //delay(10000);
  //Sensor_Status();
  //printWifiData();
}
}
/*void Sensor_Status(){
  pirState = digitalRead(PIn);
  if (pirState == HIGH) {
    // turn LED on:
    digitalWrite(led_pin, HIGH);
  } else {
    // turn LED off:
    digitalWrite(led_pin, LOW);
  }
}*/

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
}
