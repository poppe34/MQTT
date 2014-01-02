/*
 * mqtt_finsh.c
 *
 *  Created on: Oct 15, 2013
 *      Author: poppe
 */

#include <rtthread.h>
#include "mqtt.h"
#include <finsh.h>

static mqtt_client_t myClient;
static char willTopic[] = MQTT_DEFAULT_WILL_TOPIC;
static uint8_t willMsg[] = MQTT_DEFAULT_WILL_MSG;


static mqtt_will_t myWill = {
		.topic = willTopic,
		.message = willMsg,
		.qos = MQTT_DEFAULT_WILL_QOS,
};

void mqttConnect(void)
{
	myClient.will = &myWill;
	//mqtt_client_t *secondClient = (mqtt_client_t *)malloc(sizeof(mqtt_client_t));
	//mqtt_createClient(&myClient, "85.119.83.194", 1883, 10000, "MyClient");
	mqtt_createClient(&myClient, "192.168.34.127", 1883, 100000, "MyClient");
	//mqtt_createClient(secondClient, "192.168.34.127", 1883, 10000, "MyClient");
}
FINSH_FUNCTION_EXPORT(mqttConnect, connect to mosquitto server);

void mqttSub(uint8_t qos)
{
	mqtt_subscribe(&myClient, "matt/pizza", qos);
}
FINSH_FUNCTION_EXPORT(mqttSub, subscribe to message);
void mqttPub(void)
{
	mqtt_publish(&myClient, "matt/pizza", "pepperoni", 2, 0);
}
FINSH_FUNCTION_EXPORT(mqttPub, publish a message);
//FINSH_FUNCTION_EXPORT_ALIAS()
