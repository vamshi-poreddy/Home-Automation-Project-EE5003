#include <SPI.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>
#include <Arduino_JSON.h>
#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE); //Pin connected to and type of DHT used

#include "wifi_cred.h"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
JSONVar tempObject;
BLECharacteristic pirCharacteristic;

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
const int led_pin = 10; //LED device on pin 10
const char broker[] = "10.13.122.148"; // Address of the MQTT broker
const int  port     = 1883;
const char topic_status[] = "ToArduino/temp";
const char topic_temp[] = "ToArduino/temp_set";
const char topic_pir[] = "FromArduino/pir";
const char topic1[] = "ToHost/temp";
String msg = "";
int count =0;
int led_state = 0;
float set_temp = 27.0;  //Threshold Temperature
const long interval = 2000;
const long mqtt_check = 10000*12;
unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 =0;
int mqtt_status;
int ble_status;
const int subscribeQos = 1; //QoS of the MQTT subscriber

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  //Both the arduino are tested without connecting to PC so below lines are commented
  /*
  while (!Serial) {
    wait for serial port to connect. Needed for native USB port only
  }*/
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  pinMode(led_pin, OUTPUT); //LED device pin set as output
  pinMode(LED_BUILTIN, OUTPUT); //Builtin led set as output
  digitalWrite(led_pin, LOW);
  wifi_ble(); //Function for Wifi or BLE
  dht.begin(); //start the DHT service
}

void loop() {
  float temp = dht.readTemperature(); //Reading temperature from DHT11
  float humidity = dht.readHumidity(); //Reading Humidity from DHT11
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis1 >= interval) { //printing the values on serial monnitor at regular intervals
    previousMillis1 = currentMillis;
    Serial.println(temp);
    Serial.println(humidity);
    //send_status(temp, humidity);
  }
  led_state = digitalRead(led_pin);
  if (isnan(humidity) || isnan(temp)) { //retuns if failed to read from the sensor
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  if (temp >= set_temp && led_state==HIGH) { //Based on the threshlod value the connected device will be switched OFF
    digitalWrite(led_pin, LOW);
    //Serial.print("Heater Switched OFF");
  }
  if(mqttClient.connected()) //If MQTT client is connected below function is called
    mqtt_func(temp, humidity);
  else{
    if(ble_status=1){ //if ble is switched on peripheral_ble() is called
      //Serial.println("Entered");
      peripheral_ble();
    }
    else{
      WiFi.disconnect();
      delay(5000);
      wifi_ble(); //function called to try to make the MQTT connection again if not BLE 
    }
    //unsigned long currentMillis = millis();
    //if (currentMillis - previousMillis1 >= mqtt_check){ 
     // previousMillis1 = currentMillis;
      //wifi_ble();
    //}
  }
}

void mqtt_func(float temp, float humidity){
  mqttClient.poll(); //TO keep alive the mqtt connection
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) { 
    previousMillis = currentMillis;
    send_status(temp, humidity); //Call the send_status function every 2 seconds
  }
}
void peripheral_ble(){
  BLEDevice peripheral = BLE.available();
  //if a peripheral device is available below details are printed
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
  if (peripheral.localName() != "PIR_status") { //compares the name of the pheripheral device
      return;
  }
  BLE.stopScan(); //stops scanning for devices
  if (peripheral.connect()) { //try to connect to the peripeheral arduino device
    Serial.println("Connected");
  }
  //checks for the attributes
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  }
  //pirCharacteristic = peripheral.characteristic("19B10001-E8F2-537E-4F6C-D104768A1214");
  ble_func(peripheral); //calls the function
  }
  BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214"); //if not connected to peripheral try to scan for BLE device
  delay(5000);
}
void ble_func(BLEDevice peripheral){
  Serial.println("Entered Ble func");
  //peripheral.connect();
  while(peripheral.connected()){ //while connected to the arduino device
  BLE.poll();//poll to keep the connection alive
  pirCharacteristic = peripheral.characteristic("19B10001-E8F2-537E-4F6C-D104768A1214"); //check for the pir characterisctic
  byte valued;
  pirCharacteristic.readValue(valued); //reads the characteristic value
  //Serial.println(valued);
  led_state = digitalRead(led_pin); //read the edn device status
  if (valued && (led_state == 0)){
    //Serial.println("entered");
    digitalWrite(led_pin, HIGH); // switches ON if PIR characteristic value is high and device is OFF
  }
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (temp >= set_temp && led_state==HIGH) {
    digitalWrite(led_pin, LOW);
    //Serial.print("Heater Switched OFF");
  }
  if (millis() - previousMillis2 >= 5*1000UL){
    previousMillis2 = millis();
    Serial.print("PIR satate of Arduino 1: ");
    Serial.print(valued);
    Serial.println("Temperature and Humidity Values: ");
    Serial.print(temp);
    Serial.print("C ");
    Serial.print(humidity);
  }
  /*if (millis() - previousMillis1 >= 3*60*1000UL) 
  {
   Serial.println(WiFi.status());
   previousMillis1 = millis();  //get ready for the next iteration
   BLE.disconnect();
   BLE.end();
   delay(5000);
   wifi_ble();
  }*/
  }
  //if the connection is lost BLE starts scanning and peripheral_ble() function is called
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
   //Assigning the values of current temperature, humidity, threshold temperature, and device status to objcet and converting that object to json string
   String jsonString = JSON.stringify(tempObject);
   Serial.println(jsonString);
   mqttClient.beginMessage(topic1, jsonString.length(), retained, qos, dup); //Starting publishing to the topic with QoS 1
   mqttClient.print(jsonString); //publishing the message
   mqttClient.endMessage(); //end of message
}
 
void wifi_ble(){
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    Serial.println(status);
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(5000);
  }
  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  printWifiData();
  mqttClient.setUsernamePassword(Mqtt_User, Mqtt_Pass); //setting the username and password for making connection to broker
  if (!mqttClient.connect(broker, port)) { //try to connect to MQTT broker
    //WiFi.disconnect();
    //if connection failed wifi is stopped and BLE is started
    delay(2000);
    WiFi.end();
    status = WL_IDLE_STATUS;
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    delay(2000);
    BLE.begin(); //BLE is started
    digitalWrite(LED_BUILTIN, HIGH); //Built in LED is set to high if BLE is started
    //BLE.setEventHandler(BLEDiscovered, ble_func);
    Serial.println("Bluetooth® Low Energy Service started");
    BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");//BLE scans for the defined UUID
    delay(5000); //delay for scanning and setting up
    //peripheral_ble();
    ble_status = 1;
    mqtt_status =0;
    //Serial.println("Connected to UUID");
  }
  else{
    //if mqtt connection was successful
    mqtt_status = 1;
    ble_status = 0;
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("You're connected to the MQTT broker!");
    Serial.println();
    Serial.print("Subscribing to topics: ");
    Serial.println();
    mqttClient.onMessage(onMqttMessage); //callback function when a message is recieved
    // subscribe to below topc with QoS 1
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
    //Device subscribes to three topics based on the topics decisions are made
    if (recieved == topic_status){//topic to control the end device
      if(msg == "ON") {
        digitalWrite(led_pin, HIGH);
      }else {
        digitalWrite(led_pin, LOW);
      } 
    }
    //topic to set the threshold temperature
    else if (recieved == topic_temp){
      set_temp = msg.toFloat();
      Serial.println(msg);
    }
    //topic for Arduino 1 communication to set the device high  
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
