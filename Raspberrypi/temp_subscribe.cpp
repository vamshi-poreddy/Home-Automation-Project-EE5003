#include <stdio.h>
#include "stdlib.h"
#include "string.h"
#include <iostream>
#include "MQTTClient.h"
#include <wiringPi.h>
#include <json-c/json.h>
#include <time.h>
using namespace std;
#define ADDRESS "tcp://localhost:1883"
#define CLIENTID "rpi"
#define AUTHMETHOD "rphost"
#define AUTHTOKEN "greninja"
#define TOPIC "ToHost/temp"
#define QOS 1
#define TIMEOUT 10000L
int count= 1;
volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt) {
   printf("Message with token value %d delivery confirmed\n", dt);
   deliveredtoken = dt;
}


int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
   int i;
   char* payloadptr;
   char ch[100];
   struct json_object * parsed_msg;
   struct json_object * ctemp;
   struct json_object * humidity;
   struct json_object * stemp;
   struct json_object * status;
   payloadptr = (char*) message->payload;
   for(i=0; i<message->payloadlen; i++) {
      ch[i] = *payloadptr++;
   }
   time_t current_time = time(NULL);
   parsed_msg = json_tokener_parse(ch); //parsing json data
   json_object_object_get_ex(parsed_msg, "ctemp", &ctemp);
   json_object_object_get_ex(parsed_msg, "humidity", &humidity);
   json_object_object_get_ex(parsed_msg, "stemp", &stemp);
   json_object_object_get_ex(parsed_msg, "status", &status);

   cout<<"<result1>"<<ctime(&current_time)<<"</result1>";
   cout<<"<result2>"<<json_object_get_double(ctemp)<<"</result2>";
   cout<<"<result3>"<<json_object_get_int(humidity)<<"</result3>";
   cout<<"<result4>"<<json_object_get_double(stemp)<<"</result4>";
   count--;

   MQTTClient_freeMessage(&message);
   MQTTClient_free(topicName);
   if(wiringPiSetup()==-1)
		cout<<"Failed to read"<<endl;

   pinMode(6,OUTPUT);
	if(json_object_get_int(status))
      digitalWrite(6,1);
	else
      digitalWrite(6,0);

   return 1;
}
void connlost(void *context, char *cause) {
   printf("<result1>Connection lost-cause: %s</result1>",cause);
}

int main(int argc, char* argv[]) {
   MQTTClient client;
   MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
   int rc;
   MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
   opts.keepAliveInterval = 20;
   opts.cleansession = 1;
   opts.username = AUTHMETHOD;
   opts.password = AUTHTOKEN;
   MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
   printf("<result1>Failed to connect, return code %d </result1>", rc);
   exit(-1);
}

   MQTTClient_subscribe(client, TOPIC, QOS);
   while(count){}
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}

