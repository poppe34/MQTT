/*
 * mqtt_send.c
 *
 *  Created on: Oct 15, 2013
 *      Author: poppe
 */
#include "mqtt.h"
#include "rtthread.h"
#include <stdlib.h>
#include "stdio.h"

#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/sockets.h>
#include <lwip/ip_addr.h>
#include <lwip/debug.h>
#include <lwip/dns.h>
#include <lwip/sys.h>
#include <arch/sys_arch.h>
#include <netif/ethernetif.h>
#include "stm32f4xx_eth.h"



mqtt_msg_t *mqtt_findPubWithID(mqtt_client_t *client, uint16_t msgID);

uint16_t mqtt_makeMsgID(void);
void mqtt_sendPing(mqtt_client_t *client);
mqtt_err_t mqtt_waitForDNS(uint32_t timeout);
mqtt_err_t mqtt_sendConnect(mqtt_client_t *client);
static mqtt_err_t mqtt_waitForPingResp(mqtt_client_t *client, uint32_t timeout);
mqtt_err_t mqtt_freeMsg(mqtt_msg_t *msg);
int mqtt_dllFindMsgType(void *data, void *type);
int mqtt_dllFindPubWithID(void *data, void *id);
mqtt_msg_t *mqtt_findMsg(mqtt_client_t *client, uint8_t type);
void mqtt_writeData(Node_t *node, mqtt_client_t *client);
uint8_t mqtt_lenDecode(uint8_t *buf, uint32_t *sizeHolder);
mqtt_err_t mqtt_pubrel(mqtt_client_t *client, uint8_t dup, uint16_t id);
mqtt_err_t mqtt_send(mqtt_client_t *client, mqtt_msg_t *msg);



static uint16_t runningMsgID = 100;


void mqtt_writeData(Node_t *node, mqtt_client_t *client)
{
	RT_ASSERT(client);
	RT_ASSERT(node);
	int msgLen;
	uint8_t *msgPtr;
	mqtt_msg_t *msg = (mqtt_msg_t *)node->data;

	switch(msg->status)
	{
	case MSG_UNSENT:
		msg->status = MSG_SENT;//TODO: Find a home for changing this status
#if 0
		msgLen = msg->len;
		msgPtr = msg->pkt;

		while(msgLen)
		{
			printf("0x%x ", *msgPtr++);
			msgLen--;
		}
		printf("\n");
#endif
		send(client->socket, msg->pkt, msg->len, 0);
		msg->timesent = client->timeLastSent = sys_now();//TODO: This is reset a ton... Find a home for this too.
		//I put the time last set here to prevent sending two pings after timeout
		//TODO: I want some kind of way to prevent multible connects or pings I don't think this will be it.

		switch(msg->fHdr.bits.msg_type)
		{
		case CONNECT:
			break;
		case  PUBLISH:
			switch(msg->fHdr.bits.qos)
			{
			case 0:
				msg->status = MSG_COMPLETE;
				break;
			case 1:
			case 2:
				msg->status = MSG_WAITING_FOR_ACK;
				break;
			}
			break;
		case PUBREC:
			break;
		case PUBREL:
			msg->status = MSG_COMPLETE;
			break;
		case SUBSCRIBE:
			switch(msg->fHdr.bits.qos)
				{
				case 0:
					msg->status = MSG_COMPLETE;
					break;
				case 1:
				case 2:
					msg->status = MSG_WAITING_FOR_ACK;
					break;
				}
			break;
		case PINGREQ:
			break;
		}

		printf("MQTT: sending packet for: %s size:%i at %i\n", client->name, msg->len, client->timeLastSent);
	break;

	case MSG_SENT:

		if((sys_now() - msg->timesent) > client->timeout)
		{
			/*FIXME no retries yet */
			msg->status = MSG_SEND_ERROR;
			printf("temp:MSG ERROR\n");
			client->connected = 0;
			//msg->status = MSG_COMPLETE;
		}
	break;

	case MSG_COMPLETE:
		printf("MSG: Complete\n");
		dllist_removeNode(client->txList, node, 1);//This seems dangerous I need to come up with a better method
	}

}


mqtt_err_t mqtt_setKeepAlive(mqtt_client_t *client, size_t keepAlive)
{
	return mqtt_errSuccess;
}

mqtt_msg_t * mqtt_createMsg(size_t size)
{
	return(mqtt_msg_t *)MQTT_MALLOC(sizeof(mqtt_msg_t)+size);
}

mqtt_err_t mqtt_freeMsg(mqtt_msg_t *msg)
{
	MQTT_FREE(msg);
	return mqtt_errSuccess;
}
/**************************************************************************************************
 *
 * MQTT Send Messages commands
 *
 *************************************************************************************************/
void mqtt_sendPing(mqtt_client_t *client)
{
	mqtt_msg_t *msg = mqtt_createMsg(4);
	int err;

	RT_ASSERT(client);

	if(!msg)
		printf("MQTT: out of memory\n");
	
	printf("Sending %s Ping\n", client->name);

	msg->fHdr.bits.msg_type = PINGREQ;
	msg->fHdr.bits.dup_flag =0;
	msg->fHdr.bits.qos = 0;
	msg->fHdr.bits.retain = 0;
	msg->pkt[0] = msg->fHdr.firstByte;
	msg->pkt[1] = 0;
	
	msg->len = 2;
	mqtt_send(client, msg);
#if 0
	if(mqtt_waitForPingResp(client, 5000))
		{
			printf("mqtt: server timeout\n");
		}
#endif
}


mqtt_err_t mqtt_pubrel(mqtt_client_t *client, uint8_t dup, uint16_t id)
{
	mqtt_msg_t *msg = mqtt_createMsg(4);
	uint8_t *buf = msg->pkt;

	msg->fHdr.pkt_len = 2;
	msg->fHdr.bits.retain = 0;
	msg->fHdr.bits.msg_type = PUBREL;
	msg->fHdr.bits.qos = 1;
	msg->fHdr.bits.dup_flag = dup;

	*buf++ = msg->fHdr.firstByte;
	*buf++ = 2;
	*buf++ = (0xff & (id >> 8));
	*buf++ = 0xff & id;

	msg->len = 4;
	mqtt_send(client, msg);
	return 0;
}
/**************************************************************************************************
 *
 * Message wait/blocking..... put into thread
 *
 *************************************************************************************************/
mqtt_err_t mqtt_waitForConnAck(mqtt_client_t *client, uint32_t timeout)
{
	int err;
	err = sys_arch_sem_wait(&client->connAck, timeout);
	if(err == -1)
	{
		return mqtt_timeout;
	}
	return mqtt_errSuccess;
}

static mqtt_err_t mqtt_waitForPingResp(mqtt_client_t *client, uint32_t timeout)
{
	int err;
	printf("waiting for ping for %s\n", client->name);
	err = sys_arch_sem_wait(&client->pingResp, timeout);
	printf("waited %ims for ping\n", err);
	if(err == timeout)
	{
		return mqtt_timeout;
	}
	return mqtt_errSuccess;
}




/**************************************************************************************************
 *
 * MQTT Misc.
 *
 *************************************************************************************************/
uint16_t mqtt_makeMsgID(void)
{
	return runningMsgID++;
}

mqtt_msg_t *mqtt_findMsg(mqtt_client_t *client, uint8_t type)
{
	mqtt_msg_t *msg;
	RT_ASSERT(client);

	msg = dllist_searchWithinData(client->txList, mqtt_dllFindMsgType, (void *)type);

	return msg;
}

mqtt_msg_t *mqtt_findPubWithID(mqtt_client_t *client, uint16_t msgID)
{
	mqtt_msg_t *msg;
	RT_ASSERT(client);

	msg = dllist_searchWithinData(client->txList, mqtt_dllFindPubWithID, (void *)msgID);

	return msg;
}

//TODO: I pass a pointer and then cast them to an interger.  THis works fine for a 32bit system but it may not for an 8bit
int mqtt_dllFindMsgType(void *data, void *type)
{
	mqtt_msg_t *msg = (mqtt_msg_t *)data;
	uint8_t msgType = type;

	if(msg->fHdr.bits.msg_type == msgType)
		return -1;

	return 0;
}

int mqtt_dllFindPubWithID(void *data, void *id)
{
	mqtt_msg_t *msg = (mqtt_msg_t *)data;
	uint16_t msgID = (uint16_t)id;

	if(msg->msgID == msgID)
		return -1;

	return 0;
}


uint8_t mqtt_lenDecode(uint8_t *buf, uint32_t *sizeHolder)
{
	uint32_t multiplier = 1;
	size_t value = 0;
	int cnt = 0;
	uint8_t digit;

	do {
		digit = *buf++;
		value += (digit & 0x7f) * multiplier;
		multiplier *= 128;
		cnt++;
	}while ((digit & 0x80) != 0);

	*sizeHolder = value;
	return cnt;
}

size_t mqtt_lenEncode(uint8_t *buf, uint32_t pktLen)
{
	int digit, cnt = 0;
	do
	{
		digit = pktLen%128;
		pktLen /= 128;

		if ( pktLen > 0 )
		{
			digit |= 0x80;
		}

		*buf++ = digit;
		cnt++;
	}while ( pktLen );

	return cnt;
}


mqtt_err_t mqtt_send(mqtt_client_t *client, mqtt_msg_t *msg)
{
	LWIP_ASSERT("Client is not active", client);
	int err;
	//sys_mutex_lock(&client->txOK);

	msg->status = MSG_UNSENT;
	dllist_addData(client->txList, msg);
	//sys_mutex_unlock(&client->txOK);

	return mqtt_errSuccess;
}
