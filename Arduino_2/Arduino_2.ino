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

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
const int led_pin = 11;
const char broker[] = "10.13.122.148"; // Address of the MQTT server
const int  port     = 1883;
const char topic_status[] = "ToArduino/temp";
const char topic_temp[] = "ToArduino/temp_set";
const char topic1[] = "ToHost/temp";
String msg = "";
int count =0;
int led_state = 0;
float set_temp = 21.0;
const long interval = 2000;
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

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  printWifiData();

  /*BLE.begin();
  Serial.println("Bluetooth® Low Energy Central - LED control");
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");*/

  mqttClient.setUsernamePassword(Mqtt_User, Mqtt_Pass);
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
  }
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  Serial.print("Subscribing to topics: ");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe(topic_status);
  mqttClient.subscribe(topic_temp);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  dht.begin();
}

void loop() {
  mqttClient.poll();
 /* BLEDevice peripheral = BLE.available();
  if (peripheral.localName() != "PIR_status") {
      return;
    }
  if (peripheral.connect()) {
    Serial.println("Connected");
  }
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  }
  BLE.stopScan();
  BLECharacteristic pirCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");
  if (pirCharacteristic.value()){
    digitalWrite(LED_BUILTIN, HIGH);
  }*/
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  led_state = digitalRead(LED_BUILTIN);
  if (isnan(humidity) || isnan(temp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  if (temp >= set_temp && led_state==HIGH) {
    digitalWrite(LED_BUILTIN, LOW);
    //Serial.print("Heater Switched OFF");
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) { 
    previousMillis = currentMillis;
    send_status(temp, humidity);
  }
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
    if (mqttClient.messageTopic() == topic_temp){
      set_temp = msg.toFloat();
    }
    else{
      if(msg == "1") {
        digitalWrite(LED_BUILTIN, HIGH);
      }else {
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    msg="";
  }
 // if (!peripheral.connected()) {
  //  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  //}
  //delay(10000);
  //printWifiData();
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
void send_status(){
   tempObject["ctemp"]=temp;
   tempObject["humidity"]=humidity;
   tempObject["stemp"]=set_temp;
   tempObject["status"]=digitalRead(LED_BUILTIN);
   String jsonString = JSON.stringify(tempObject);
   Serial.println(jsonString);
   mqttClient.beginMessage(topic1);
   mqttClient.print(jsonString);
   mqttClient.endMessage();
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
