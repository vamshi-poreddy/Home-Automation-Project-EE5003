#include <stdio.h>
#include "stdlib.h"
#include "string.h"
#include <iostream>
#include "MQTTClient.h"
#include <wiringPi.h>
#include <time.h>
using namespace std;
#define ADDRESS "tcp://localhost:1883"
#define CLIENTID "arduino"
#define AUTHMETHOD "rphost"
#define AUTHTOKEN "greninja"
#define TOPIC "ToArduino/temp_set"
#define QOS 1
#define TIMEOUT 5000L
int count= 1;
volatile MQTTClient_deliveryToken deliveredtoken;

int main(){
	MQTTClient client;
	MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token; 
	MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = AUTHMETHOD;
	opts.password = AUTHTOKEN;
	
	float temp=0;
	while(temp<10 || temp >30){
	cout<<"Enter the preferred room temperature between 10 to 30 degrees:"<<endl;
	cin>>temp;
	if(temp<10 || temp >30)
		cout<<"Invalid input please enter temparature value within range 10 and 30"<<endl;
	}
	
	int rc;
	if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
	cout << "Failed to connect, return code " << rc << endl;
	return -1;
	}
	char temp_str[5];
	sprintf(temp_str,"%0.2f",temp);
	pubmsg.payload = temp_str;
	pubmsg.payloadlen = strlen(temp_str);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	
	MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
	//delay(2000);
	//MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
	//cout << "Waiting for up to " << (int)(TIMEOUT/1000) <<" seconds for publication of " << temp_str <<" \non topic " << TOPIC << " for ClientID: " << CLIENTID << endl;
	rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
	//cout << "Message with token " << (int)token << " delivered." << endl;
	cout<<"Threshold temperature Published, Please check Sensor status in User Widgets"<<endl;
	MQTTClient_disconnect(client, 5000);
	MQTTClient_destroy(&client);
	return rc;
}
