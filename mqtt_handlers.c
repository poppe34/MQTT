/*
 * mqtt_handlers.c
 *
 *  Created on: Dec 27, 2013
 *      Author: poppe
 */
#include "mqtt.h"

static int connAckCode = 0;


/*
 * Prototypes
 */
//Handlers
mqtt_err_t mqtt_pubHandler(mqtt_client_t *client, mqtt_msg_t *msg, uint8_t *buf);
mqtt_err_t mqtt_connAckHandler(mqtt_client_t *client, uint8_t *buf);
mqtt_err_t mqtt_pingRespHandler(mqtt_client_t *client, uint8_t *buf);
mqtt_err_t mqtt_pubRecHandler(mqtt_client_t *client, mqtt_msg_t *msg, uint8_t *buf);
mqtt_err_t mqtt_pubackHandler(mqtt_client_t *client, uint8_t *buf);
mqtt_err_t mqtt_subackHandler(mqtt_client_t *client, uint8_t *buf);
mqtt_err_t mqtt_pubcompHandler(mqtt_client_t *client, uint8_t *buf);

/**************************************************************************************************
 *
 * Message Handlers
 *
 *************************************************************************************************/
void mqtt_handler(mqtt_client_t *client, uint8_t *buf, size_t size)
{
	static mqtt_msg_t msg;

	while(size > 0)
	{
		msg.fHdr.firstByte = *buf++;
		buf += mqtt_lenDecode(buf, &msg.fHdr.pkt_len);

		switch(msg.fHdr.bits.msg_type)
		{
			case CONNACK:
				if(msg.fHdr.pkt_len == 2)
					mqtt_connAckHandler(client, buf);
				else
					printf("Connack has wrong len\n");
			break;

			case PUBLISH:
				printf("PUBLISH Receive\n");
				//TODO: I need to reformat all the type handlers to have the msg as well
				mqtt_pubHandler(client, &msg, buf);
			break;
			case PUBACK:
				printf("PUBACK Received\n");
				mqtt_pubackHandler(client, buf);

			break;
			case PUBREC:
				mqtt_pubRecHandler(client, &msg, buf);
				printf("PUBREC Received\n");
			break;

			case PUBREL:
				printf("PUBREL Received\n");
			break;

			case PUBCOMP:
				mqtt_pubcompHandler(client, buf);
				printf("PUBCOMP Received\n");
			break;

			case SUBACK:
				printf("SUBACK Received\n");
			break;

			case UNSUBACK:
				printf("UNSUBACK Received\n");
			break;
			case PINGRESP:
				mqtt_pingRespHandler(client, buf);
			break;
			default:

				printf("Unknown Message Type\n");
				return;
		}
		buf += (msg.fHdr.pkt_len + 2);
		size -= (msg.fHdr.pkt_len + 2);
	}
}

mqtt_err_t mqtt_pingRespHandler(mqtt_client_t *client, uint8_t *buf)
{
	mqtt_msg_t *msg;

	printf("MQTT: GOT PING RESPONCE\n");
	sys_sem_signal(&client->pingResp);

	msg = mqtt_findMsg(client, PINGREQ);

	if(msg)
		msg->status = MSG_COMPLETE;

	client->timeLastSent = sys_now();
	return mqtt_errSuccess;
}

mqtt_err_t mqtt_subackHandler(mqtt_client_t *client, uint8_t *buf)
{
	RT_ASSERT(client);
	mqtt_msg_t *msg = NULL;
	uint16_t msgID;
	uint8_t grantedQOS;

	msgID = *buf++;
	msgID <<= 8;
	msgID = *buf++;

	grantedQOS = *buf++;

	msg = mqtt_findSubWithID(client, msgID);

	if(msg)
	{
		printf("Sub ack with msgID: %i is found and acked", msgID);
		msg->status = MSG_COMPLETE;

	}

	return 0;
}

mqtt_err_t mqtt_pubRespHandler(mqtt_client_t *client, uint8_t *buf)
{
	mqtt_msg_t *msg;

	printf("MQTT: GOT PUB RESPONCE\n");

	msg = mqtt_findMsg(client, PUBLISH);

	if(msg)
		msg->status = MSG_COMPLETE;

	client->timeLastSent = sys_now();
	return mqtt_errSuccess;
}

mqtt_err_t mqtt_pubHandler(mqtt_client_t *client, mqtt_msg_t *msg, uint8_t *buf)
{
	RT_ASSERT(client);
	RT_ASSERT(msg);

	uint32_t bufferLeft = msg->fHdr.pkt_len;
	mqtt_pubSubMsg_t PSMsg;
	uint32_t strLen;

//	PSMsg.msgID = (uint32_t)((*buf++) << 8);
//	PSMsg.msgID = (uint32_t)(*buf++);

	strLen = (uint32_t)((*buf++) << 8);
	strLen = (uint32_t)(*buf++);

	//Determine the length left for the payload 2 for the utf str len and also actual str len
	bufferLeft -= 2;
	bufferLeft -= strLen;
	PSMsg.topic = (uint8_t *)(rt_malloc(strLen+1));

	memcpy(PSMsg.topic, buf, strLen);
	PSMsg.topic[strLen] = '\0';
	buf += strLen;

	PSMsg.payload = (uint8_t *)(rt_malloc(bufferLeft+1));

	memcpy(PSMsg.payload, buf, bufferLeft);
	PSMsg.payload[bufferLeft] = '\0';
	buf += strLen;

	printf("Topic len:%i Payload len: %i\n", strLen, bufferLeft);


	printf("------ Received Pub MSG: -------\n");
	//printf("MSG ID")
	printf(" Topic: %s\n", PSMsg.topic);
	//printf(" Payload: %s\n", PSMsg.payload);

	/**
	 * I am going to add a hook here to bridge coap to mqtt
	 * currently this is very weak and needs to be improved
	 */
#if defined(MQTT_SEND_TO_HOOK)
		coap_outsideDataInput(PSMsg.payload, bufferLeft);
#endif

	if(client->msgCB)
	{
		client->msgCB(client, &PSMsg);
	}

	//FOR NOW I AM FREEING THE BUFFERS MAYNEED TO CHANGE
	MQTT_FREE(PSMsg.payload);
	MQTT_FREE(PSMsg.topic);
	/*
	 * TODO: I have alot to do in here I hate it all I don't like the malloc calls
	 * and I would like to get rid of that also I am not sure what method I am going to
	 * using in calling the application to give it indication that we have a published
	 * MSG.  I still don't know how or when I want to free both the MSG and the PSMsg
	 */

	return 0;

}

mqtt_err_t mqtt_pubRecHandler(mqtt_client_t *client, mqtt_msg_t *msg, uint8_t *buf)
{
	RT_ASSERT(client);
	RT_ASSERT(msg);

	uint16_t msgID;
	uint8_t dup = 0;

	msgID = *buf++;
	msgID <<= 8;

	msgID = *buf++;

	printf("pubRec: with msg ID: %i\n", msgID);
	//TODO: I need to update the msg status to keep a tally on where the msg is.

	msg = mqtt_findPubWithID(client, msgID);

	if(msg)
		msg->status = MSG_WAITING_FOR_COMP;
	else
		dup = 1;

	mqtt_pubrel(client, dup, msgID);
	return 0;

}

mqtt_err_t mqtt_pubackHandler(mqtt_client_t *client, uint8_t *buf)
{
	RT_ASSERT(client);
	mqtt_msg_t *msg = NULL;
	uint16_t msgID;

	msgID = *buf++;
	msgID <<= 8;

	msgID = *buf++;

	printf("pubRec: with msg ID: %i\n", msgID);
		//TODO: I need to update the msg status to keep a tally on where the msg is.

	msg = mqtt_findPubWithID(client, msgID);

	if(msg)
	{
		msg->status = MSG_COMPLETE;
	}

	return 0;
}

mqtt_err_t mqtt_pubcompHandler(mqtt_client_t *client, uint8_t *buf)
{
	RT_ASSERT(client);
	mqtt_msg_t *msg;

	uint16_t msgID;

	msgID = *buf++;
	msgID <<= 8;

	msgID = *buf++;

	msg = mqtt_findPubWithID(client, msgID);

	if(msg)
	{
		msg->status = MSG_COMPLETE;
	}


	return 0;
}

mqtt_err_t mqtt_connAckHandler(mqtt_client_t *client, uint8_t *buf)
{
	mqtt_msg_t *msg = 0;
	if(buf[0] == 0)
		connAckCode =  buf[1];

	msg = mqtt_findMsg(client, CONNECT);

	if(msg)
		msg->status = MSG_COMPLETE;

	client->timeLastSent = sys_now();
	client->connected = -1;

	printf("MQTT: GOT CONNACK for %s\n", client->name);
	sys_sem_signal(&client->connAck);

	return 0;
}


