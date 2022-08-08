#include <SPI.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>

#include "wifi_cred.h"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
BLEService pirService("19B10000-E8F2-537E-4F6C-D104768A1214"); // create service
BLEByteCharacteristic pirCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
const int led_pin = 11;
const int PIn = 2;     // the number of the input pin
int pirState = 0;
const char broker[] = "10.13.122.148"; // Address of the MQTT server
const int  port     = 1883;
//const int serverPort = 4080;
const char topic[] = "ToArduino/pir";
const char topic_pir[] = "FromArduino/pir";
String msg = "";
int count =0; 
int ble_status=0;
const long interval = 10000;
const long mqtt_check = 10000*12;
unsigned long previousMillis = 0;
unsigned long previousMillis1 = 10000*12;
int timer;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  //while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  //}
  wifi_ble();
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
  pinMode(PIn, INPUT);
}

void loop() {
  pirState = digitalRead(PIn);
  if(pirState == HIGH){
    digitalWrite(led_pin, HIGH);
    timer =1;
  }
  if(timer){
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    digitalWrite(led_pin, LOW);
  }
  }
  //Serial.println(mqttClient.connected());
  if(mqttClient.connected()){
    mqtt_func();
  }
  else{
    if(!ble_status){
      WiFi.disconnect();
      wifi_ble();
    }
    BLE.poll();
    BLEDevice central = BLE.central();
    if(central.connected()){
      pirCharacteristic.writeValue(pirState);
    }
   // unsigned long currentMillis = millis();
    //if (currentMillis - previousMillis1 >= mqtt_check){
     // previousMillis1 = currentMillis;
      //wifi_ble();
    //}
  }
}

void mqtt_func(){
  mqttClient.poll();
  if (pirState == HIGH) {
    bool retained = false;
    int qos = 1;
    bool dup = false;
    mqttClient.beginMessage(topic_pir, pirState, retained, qos, dup);
    mqttClient.print(pirState);
    mqttClient.endMessage();
  }
}
void wifi_ble(){
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(5000);
  }
  // you're connected now, so print out the data:
  Serial.print("You're connected to the network ");
  printWifiData();
  mqttClient.setUsernamePassword(Mqtt_User, Mqtt_Pass);
  if (!mqttClient.connect(broker, port)) {
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
    Serial.print("BLE started");
    ble_status = 1;
    delay(5000);
  }
  else{
    Serial.println("You're connected to the MQTT broker!");
    Serial.println();
    Serial.print("Subscribing to topics: ");
    Serial.println();
    mqttClient.onMessage(onMqttMessage);
  // subscribe to a topic
    // subscribe to a topic
    mqttClient.subscribe(topic,1);
    ble_status=0;
  }
}
void onMqttMessage(int messageSize){
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
    if(msg == "ON") {
      digitalWrite(led_pin, HIGH);
      timer =0;
    } else {
      digitalWrite(led_pin, LOW);
    }
    msg="";
}
void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
}
