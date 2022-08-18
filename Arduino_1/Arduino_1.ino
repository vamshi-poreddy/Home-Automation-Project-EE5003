#include <SPI.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>

#include "wifi_cred.h"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
BLEService pirService("19B10000-E8F2-537E-4F6C-D104768A1214"); // creating an LED service
BLEByteCharacteristic pirCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify); // PIR charasteristic with read and notify attributes

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
const int led_pin = 11; // Device Led Pin
const int PIn = 2;     // the number of the input pin
int pirState = 0;
const char broker[] = "10.13.122.148"; // Address of the MQTT broker
const int  port     = 1883;   //port for MQTT connection
//const int serverPort = 4080;
const char topic[] = "ToArduino/pir";   // Topic to control led pin from raspberry pi
const char topic_pir[] = "FromArduino/pir";   //Topic for communication with second arduino
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
  /*while (!Serial) {
    ; wait for serial port to connect. Needed for native USB port only
  //}*/
  pinMode(LED_BUILTIN, OUTPUT);
  wifi_ble();//function for starting up Wifi or BLE
  pinMode(led_pin, OUTPUT);//Setting LED pin mode to output
  digitalWrite(led_pin, LOW);
  pinMode(PIn, INPUT);//Setting pin mode with PIR to input
}

void loop() {
  pirState = digitalRead(PIn); //Read the pir state
  if(pirState == HIGH){
    digitalWrite(led_pin, HIGH);//Writes to LED device based on pir state
    timer =1;
  }
  if(timer){
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    digitalWrite(led_pin, LOW);//Swithching of LED device after 10 sec interval if was switched on from PIR state
  }
  }
  //Serial.println(mqttClient.connected());
  if(mqttClient.connected()){
    mqtt_func();//if MQTT client is conneceted calling the function for publishing and subscribing
  }
  else{
    if(!ble_status){ //if mqtt client is not connected and BLE is not switched on calls the below fucntion to set up after a 5 second delay
      delay(5000);
      WiFi.disconnect();//disconnecting the wifi
      wifi_ble();
    }
    if(ble_status){
    //once BLE is switched on calling the poll for keeping the connection
    BLE.poll();
    BLEDevice central = BLE.central();
    if(central.connected()){
      pirCharacteristic.writeValue(pirState); //updates the characteristics value when the device is connected
    }
   // unsigned long currentMillis = millis();
    //if (currentMillis - previousMillis1 >= mqtt_check){
     // previousMillis1 = currentMillis;
      //wifi_ble();
    //}
    }
  }
}

void mqtt_func(){
  mqttClient.poll(); //To keep the MQTT connection alive
  if (pirState == HIGH) {
    bool retained = false;
    int qos = 1;
    bool dup = false;
    mqttClient.beginMessage(topic_pir, pirState, retained, qos, dup);//publishing to Arduino 2 when PIR state is High 
    mqttClient.print(pirState);
    mqttClient.endMessage(); //End of message
  }
}
void wifi_ble(){
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    //Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    // wait 5 seconds for connection:
    delay(5000);
  }
  // you're connected now, so print out the data:
  Serial.print("You're connected to the network ");
  printWifiData();
  mqttClient.setUsernamePassword(Mqtt_User, Mqtt_Pass); //tries to connect to mqtt broker
  if (!mqttClient.connect(broker, port)) {
    //if connection to broker was not success BLE is switched ON 
     if (!BLE.begin()) {
      Serial.println("starting Bluetooth® Low Energy module failed!");
      while (true);
    }
    BLE.setLocalName("PIR_status");   // set the local name peripheral advertises
    BLE.setAdvertisedService(pirService);   // set the UUID for the service this peripheral advertises:
    pirService.addCharacteristic(pirCharacteristic); //adds the characteristic to service
    BLE.addService(pirService); //add the service
    pirCharacteristic.writeValue(0);
    BLE.advertise(); //advertise the service which allows other BLE modules to connect
    Serial.print("BLE started");
    digitalWrite(LED_BUILTIN, HIGH); //Builtin LED on Arduino is set to high if BLE is swithced on
    ble_status = 1; //setting the status to 1
    delay(5000); //waiting 5 sec for BLE setup and device coonection
  }
  else{
    //connection to mqtt is successful
    Serial.println("You're connected to the MQTT broker!");
    Serial.println();
    Serial.print("Subscribing to topics: ");
    Serial.println();
    mqttClient.onMessage(onMqttMessage); //callback function which is invoked when a message is recieved
    // subscribe to a topic
    mqttClient.subscribe(topic,1); 
    ble_status=0;
    digitalWrite(LED_BUILTIN, LOW);
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
    //Based on the message LED device status is changed
    if(msg == "ON") {
      digitalWrite(led_pin, HIGH);
      timer =0;
    } else {
      digitalWrite(led_pin, LOW);
    }
    msg=""; //clearing the message
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
