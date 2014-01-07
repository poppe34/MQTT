/*
 * mqtt.c
 *
 *  Created on: Oct 20, 2013
 *      Author: poppe
 */
#include "mqtt.h"
#include "lwip/sys.h"

uint8_t user_flag = 0;
uint8_t pass_flag = 0;

uint8_t username_string[10];
uint8_t password_string[10];

uint8_t client_id[23] = DEFALUT_CLIENT_ID;



mqtt_utf_t username = {
		0,
		username_string,
};
mqtt_utf_t password = {
		0,
		password_string
};

mqtt_err_t mqtt_sendConnect(mqtt_client_t *client)
{

	mqtt_msg_t *msg = mqtt_createMsg(80);//FIXME: Fix the length to be correct based on values
	uint8_t *buf;
	printf("sending Connect for:%s\n", client->name);

	buf = msg->pkt;

	msg->fHdr.bits.msg_type = CONNECT;
	msg->fHdr.bits.dup_flag = 0;
	msg->fHdr.bits.qos = 0;
	msg->fHdr.bits.retain = 0;
	*buf++ = msg->fHdr.firstByte;

	//printf("Fixed Header first byte: 0x%X for %s\n", msg->fHdr.firstByte, client->name);
	//TODO: fix to a dynamic len right now it has to have a constant client ID len
	msg->fHdr.pkt_len = (44); //client id + variable header + client id len
	*buf++ = msg->fHdr.pkt_len;
	if(user_flag)
	{
		msg->vHdr.bits.user_flag = 1;
		//TODO: add username and password code
		if(pass_flag)
		{
			msg->vHdr.bits.pass_flag = 1;
		}
	}

	/*******************************************
	 * Variable Header
	 ******************************************/

	/* Protocol */
	buf += mqtt_stringToUtf("MQIsdp", buf);

	/* Version */
	*buf++ = 3;

	//TODO: change clean session to be selectable
	msg->vHdr.bits.pass_flag = 0;
	msg->vHdr.bits.user_flag = 0;
	if(client->will)
	{
		msg->vHdr.bits.will_flag = 1;
		msg->vHdr.bits.will_qos = client->will->qos;
		msg->vHdr.bits.will_retain = 0;
	}
	else
	{
		msg->vHdr.bits.will_flag = 0;
		msg->vHdr.bits.will_qos = 0;
		msg->vHdr.bits.will_retain = 0;
	}

	msg->vHdr.bits.clean_session = 1;
	*buf++ = msg->vHdr.firstbyte;

	/* Keep Alive */
	uint32_t toSec = client->keepalive / 1000;
	*buf++ = (uint8_t)(0xff & (toSec >> 8));
	*buf++ = 0xff & toSec;

	/* order the variable header must be in
	 * client ID
	 * will topic
	 * will message
	 * username
	 * password */

	//TODO Add user authentication
	buf += mqtt_stringToUtf(client_id, buf);//FIXME and variable client ID

	if(client->will)
	{
		buf += mqtt_stringToUtf(client->will->topic, buf);
		buf += mqtt_stringToUtf(client->will->message, buf);
	}
	msg->len = buf - msg->pkt;

	mqtt_send(client, msg);

	/* wait for connack for 5s */
	if(mqtt_waitForConnAck(client, 10000))
	{
		printf("mqtt: server timeout\n");
	}

	//TODO: Add error code handler after we attempt to connect

	//TODO: I might start the thread here
	return mqtt_errSuccess;
}

mqtt_err_t mqtt_publish(mqtt_client_t *client, mqtt_topic_t topic,
		mqtt_payload_t payload, uint8_t qos, uint8_t retain)
{

	uint32_t neededLen = (strlen(topic)+2)+(strlen(payload));
	if(qos >= 1)
		neededLen += 2;
	/*
	 * Add 2 to the length to allow room for the fixed header:
	 * TODO: I need to fix this if the length of the body is longer that 127
	*/
	mqtt_msg_t *msg = mqtt_createMsg(neededLen+2);
	uint8_t *ptr = msg->pkt;

	msg->fHdr.pkt_len = neededLen;
	msg->fHdr.bits.retain = retain;
	msg->fHdr.bits.msg_type = PUBLISH;
	msg->fHdr.bits.qos = qos;

	/* Fixed Header */
	*ptr++ = msg->fHdr.firstByte;
	ptr += mqtt_lenEncode(ptr, msg->fHdr.pkt_len);

	/* Variable Header */
	/* Topic */
	ptr += mqtt_stringToUtf(topic, ptr);

	/* Message ID */
	if(qos >=1 )
	{
		msg->msgID = mqtt_makeMsgID();
		*ptr++ = (0xff & (msg->msgID >> 8));
		*ptr++ = 0xff & msg->msgID;
	}

	/* Payload */
	memcpy(ptr, payload, strlen(payload));

	ptr += strlen(payload);
	/* Msg Len */
	msg->len = ptr - msg->pkt;
	//TODO: I want to create a queue of outgoing messages
	mqtt_send(client, msg);

	return mqtt_errSuccess;
}

mqtt_err_t mqtt_subscribe(mqtt_client_t *client, mqtt_topic_t topic, uint8_t qos)
{
	mqtt_msg_t *msg = mqtt_createMsg(40);
	uint8_t *ptr = msg->pkt;
	//TODO: I created a subPub msg for subscription us it
	msg->fHdr.pkt_len = (1+strlen(topic)+2);//1 for QOS 2 for string len plus message ID len of 2 plus string len
	msg->fHdr.bits.retain = 0;
	msg->fHdr.bits.msg_type = SUBSCRIBE;
	msg->fHdr.bits.qos = qos;
	msg->fHdr.bits.dup_flag = 0;

	if(msg->fHdr.bits.qos >= 0)
	{
		msg->fHdr.pkt_len += 2;
		msg->msgID = mqtt_makeMsgID();
	}
	/* Fixed Header */
	*ptr++ = msg->fHdr.firstByte;
	ptr += mqtt_lenEncode(ptr, msg->fHdr.pkt_len);

	//MSG ID
	if(msg->fHdr.bits.qos >= 0)
	{

		*ptr++ = (0xff & (msg->msgID >> 8));
		*ptr++ = 0xff & msg->msgID;
	}
	/* Variable Header */
	/* Topic */
	ptr += mqtt_stringToUtf(topic, ptr);

	/* Requested QOS */
	*ptr++ = msg->fHdr.bits.qos;

	/* Msg Len */
	msg->len = ptr - msg->pkt;

	mqtt_send(client, msg);
	printf("MQTT: sending Sub: with len: %i buf size: %i\n", msg->fHdr.pkt_len, msg->len);
	printf("MQTT: subscribe to topic: %s\n", topic);

	//TODO: The spec allows for multible subscriptions to be made at once... this is not impletemented
	return 0;
}

mqtt_err_t mqtt_disconnect(mqtt_client_t *client)
{
	//TODO: add disconnect to MQTT code

	closesocket(client->socket);

	//session = NULL;

	return mqtt_errSuccess;

}

mqtt_err_t mqtt_check_keepalive(mqtt_client_t *client)
{
	LWIP_ASSERT("Client != NULL",client != NULL);

	uint32_t currentTime = sys_now(), timeOut;
	timeOut = client->timeLastSent + client->keepalive;

	if(timeOut <= currentTime)
	{
		mqtt_sendPing(client);
	}
	return mqtt_errSuccess;
}

void mqtt_registerMsgCB(mqtt_client_t *client, pubMsgCallback cb)
{
	RT_ASSERT(client);
	RT_ASSERT(cb);

	client->msgCB = cb;
}
