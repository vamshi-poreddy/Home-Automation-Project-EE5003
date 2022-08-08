#include <SPI.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>
#include <Arduino_JSON.h>
#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#include "wifi_cred.h"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
JSONVar tempObject;
BLECharacteristic pirCharacteristic;

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
const int led_pin = 10;
const char broker[] = "10.13.122.148"; // Address of the MQTT server
const int  port     = 1883;
const char topic_status[] = "ToArduino/temp";
const char topic_temp[] = "ToArduino/temp_set";
const char topic_pir[] = "FromArduino/pir";
const char topic1[] = "ToHost/temp";
String msg = "";
int count =0;
int led_state = 0;
float set_temp = 25.0;
const long interval = 2000;
const long mqtt_check = 10000*12;
unsigned long previousMillis = 0;
unsigned long previousMillis1 = 10000*12;
int mqtt_status;
int ble_status;
const int subscribeQos = 1;

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
  wifi_ble();
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
  dht.begin();
}

void loop() {
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) { 
    previousMillis = currentMillis;
    Serial.println(temp);
    Serial.println(humidity);
    //send_status(temp, humidity);
  }
  led_state = digitalRead(led_pin);
  if (isnan(humidity) || isnan(temp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  if (temp >= set_temp && led_state==HIGH) {
    digitalWrite(led_pin, LOW);
    //Serial.print("Heater Switched OFF");
  }
  if(mqttClient.connected())
    mqtt_func(temp, humidity);
  else{
    if(ble_status=1){
      //Serial.println("Entered");
      peripheral_ble();
    }
    else{
      WiFi.disconnect();
      wifi_ble();
    }
    //unsigned long currentMillis = millis();
    //if (currentMillis - previousMillis1 >= mqtt_check){ 
     // previousMillis1 = currentMillis;
      //wifi_ble();
    //}
  }
}

void mqtt_func(float temp, float humidity){
  mqttClient.poll();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) { 
    previousMillis = currentMillis;
    send_status(temp, humidity);
  }
}
void peripheral_ble(){
  BLEDevice peripheral = BLE.available();
  if (peripheral) {
    // discovered a peripheral, print out address, local name, and advertised service
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();
  //
    Serial.println(peripheral.localName());
  if (peripheral.localName() != "PIR_status") {
      return;
  }
  BLE.stopScan();
  if (peripheral.connect()) {
    Serial.println("Connected");
  }
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  }
  //pirCharacteristic = peripheral.characteristic("19B10001-E8F2-537E-4F6C-D104768A1214");
  }
  ble_func(peripheral);
}
void ble_func(BLEDevice peripheral){
  //Serial.println("Entered Ble func");
  while(peripheral.connected()){
    BLE.poll();
    pirCharacteristic = peripheral.characteristic("19B10001-E8F2-537E-4F6C-D104768A1214");
  //byte valued;
  //pirCharacteristic.readValue(valued);
  //Serial.println(valued);
  if (pirCharacteristic.value() && led_pin == LOW){
    //Serial.println("entered");
    digitalWrite(led_pin, HIGH);
  }
  float temp = dht.readTemperature();
  if (temp >= set_temp && led_state==HIGH) {
    digitalWrite(led_pin, LOW);
    //Serial.print("Heater Switched OFF");
  }
  }
  if (!peripheral.connected()) {
    BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
    peripheral_ble();
  }
}
void send_status(float temp, float humidity){
   bool retained = false;
   int qos = 1;
   bool dup = false;
   tempObject["ctemp"]=temp;
   tempObject["humidity"]=humidity;
   tempObject["stemp"]=set_temp;
   tempObject["status"]=digitalRead(led_pin);
   String jsonString = JSON.stringify(tempObject);
   Serial.println(jsonString);
   mqttClient.beginMessage(topic1, jsonString.length(), retained, qos, dup);
   mqttClient.print(jsonString);
   mqttClient.endMessage();
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
  Serial.print("You're connected to the network");
  printWifiData();
  mqttClient.setUsernamePassword(Mqtt_User, Mqtt_Pass);
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    BLE.begin();
    Serial.println("Bluetooth® Low Energy Central - LED control");
    BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
    //peripheral_ble();
    ble_status = 1;
    mqtt_status =0;
    Serial.println("connected to uuid");
  }
  else{
    mqtt_status = 1;
    ble_status = 0;
    Serial.println("You're connected to the MQTT broker!");
    Serial.println();
    Serial.print("Subscribing to topics: ");
    Serial.println();
    mqttClient.onMessage(onMqttMessage);
  // subscribe to a topic
    mqttClient.subscribe(topic_status, subscribeQos);
    mqttClient.subscribe(topic_temp, subscribeQos);
    mqttClient.subscribe(topic_pir, subscribeQos);
  }
}
void onMqttMessage(int messageSize){
    String recieved = mqttClient.messageTopic();
    // we received a message, print out the topic and contents
    Serial.print("Received a message with topic '");
    Serial.print(recieved);
    Serial.print("', length ");
    Serial.print(messageSize);
    Serial.println(" bytes:");
    Serial.print(mqttClient.messageQoS());
    // use the Stream interface to print the contents
    while (mqttClient.available()) {
      msg= msg+(char)mqttClient.read();
    }
    if (recieved == topic_status){
      if(msg == "ON") {
        digitalWrite(led_pin, HIGH);
      }else {
        digitalWrite(led_pin, LOW);
      } 
    }
    else if (recieved == topic_temp){
      set_temp = msg.toFloat();
      Serial.println(msg);
    }
    else if(recieved == topic_pir){
      if(msg == "1")
        digitalWrite(led_pin, HIGH);
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
