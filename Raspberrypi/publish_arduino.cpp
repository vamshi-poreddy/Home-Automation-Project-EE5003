#include <stdio.h>
#include "stdlib.h"
#include "string.h"
#include <iostream>
#include "MQTTClient.h"
#include <wiringPi.h>
#include <thread>
#include <chrono>
using namespace std;
#define ADDRESS "tcp://localhost:1883"
#define CLIENTID "rpi"
#define AUTHMETHOD "rphost"
#define AUTHTOKEN "greninja"
#define TOPIC1 "ToArduino/pir"
#define TOPIC2 "ToArduino/temp"
#define QOS 1
#define TIMEOUT 10000L
int count= 1;
volatile MQTTClient_deliveryToken deliveredtoken;

void to_arduino(int state, const char* TOPIC){
	MQTTClient client;
	MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token; 
	MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = AUTHMETHOD;
	opts.password = AUTHTOKEN;
	char state_str[5];
	if(state==1)
		sprintf(state_str,"ON");
	else
		sprintf(state_str,"OFF");
	pubmsg.payload = state_str;
	pubmsg.payloadlen = strlen(state_str);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	int rc;
	if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
	cout << "Failed to connect, return code " << rc << endl;
	}
	MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
	MQTTClient_disconnect(client, 1000);
	MQTTClient_destroy(&client);
}
	

int main(){
	if(wiringPiSetup()==-1)
		cout<<"Failed to read"<<endl;
	pinMode(5,OUTPUT);
	pinMode(6,OUTPUT);
	int led_state = digitalRead(5);
	int heater_state = digitalRead(6);
	//cout<<led_state<<endl;
	//cout<<heater_state<<endl;
	while(1){
		if(led_state!=digitalRead(5)){
			led_state=digitalRead(5);
			//cout<<led_state<<endl;
			to_arduino(led_state, TOPIC1);
			//thread th1(to_arduino, led_state);
			//this_thread::sleep_for(chrono::milliseconds(200));
			//th1.join();
		}
		if(heater_state!=digitalRead(6)){
			heater_state=digitalRead(6);
			//cout<<heater_state<<endl;
			to_arduino(heater_state, TOPIC2);
			//thread th2(to_arduino, heater_state);
			//th2.join();
		}
	
	}
	return 1;
}
