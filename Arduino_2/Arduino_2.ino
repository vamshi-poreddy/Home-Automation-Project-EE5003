#include <SPI.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#include "wifi_cred.h"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
const int led_pin = 11;
const char broker[] = "10.13.122.25"; // Address of the MQTT server
const int  port     = 1883;
const int serverPort = 4080;
const char topic_status[] = "ToArduino/temp_status";
const char topic_temp[] = "ToArduino/temp_set";
const char topic1[] = "ToHost/temp";
String msg = "";
int count =0;
float set_temp = 26.0;
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

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  printWifiData();

  mqttClient.setUsernamePassword(Mqtt_User, Mqtt_Pass);
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    //while (1);
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
  //pinMode(PIn, INPUT);
  
}

void loop() {
  mqttClient.poll();
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (isnan(humidity) || isnan(temp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  if (temp > set_temp) {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Heater Switched OFF");
  }
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    Serial.println(temp);
    Serial.println(humidity);
    previousMillis = currentMillis;
    mqttClient.beginMessage(topic1);
    mqttClient.print(digitalRead(LED_BUILTIN));
    mqttClient.endMessage();
    //count++;
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
      if(msg == "ON") {
        digitalWrite(LED_BUILTIN, HIGH);
      }else {
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    msg="";
  }
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

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
}
