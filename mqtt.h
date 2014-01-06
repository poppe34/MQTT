/*
 * mqtt.h
 *
 *  Created on: Oct 15, 2013
 *      Author: poppe
 */

#ifndef MQTT_H_
#define MQTT_H_

#include "mqtt_utf8.h"
#include "mqtt_conf.h"
#include "stdint.h"
#include "rtthread.h"
#include <arch/sys_arch.h>
#include <lwip/sockets.h>
#include "lwip/ip_addr.h"
#include "dlList.h"

#define PROTOCOL_NAME "MQTTClient"
#define PROTOCOL_VERSION_MAJOR 3
#define PROTOCOL_VERSION_MINOR 1
#define CODE_VERSION_MAJOR 	0
#define CODE_VERSION_MINOR	2
#define MAX_TX_FIFO_SIZE 8
#define MQTT_RETRIES	4
#define MQTT_DEFAULT_TIMEOUT_MS	5000

/* Message types */
#define CONNECT 1
#define CONNACK 2
#define PUBLISH 3
#define PUBACK 4
#define PUBREC 5
#define PUBREL 6
#define PUBCOMP 7
#define SUBSCRIBE 8
#define SUBACK 9
#define UNSUBSCRIBE 10
#define UNSUBACK 11
#define PINGREQ 12
#define PINGRESP 13
#define DISCONNECT 14

#define CONNACK_ACCEPTED 							0
#define CONNACK_REFUSED_PROTOCOL_VERSION 			1
#define CONNACK_REFUSED_IDENTIFIER_REJECTED 		2
#define CONNACK_REFUSED_SERVER_UNAVAILABLE 			3
#define CONNACK_REFUSED_BAD_USERNAME_PASSWORD 		4
#define CONNACK_REFUSED_NOT_AUTHORIZED 				5

#define MQTT_MAX_PAYLOAD 268435455

#define MQTT_UPDATE_DELAY	500 //MS delay....

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

#define MQTT_MALLOC(size)	rt_malloc(size);
#define MQTT_FREE(data)		rt_free(data);

extern uint8_t user_flag;
extern uint8_t pass_flag;
extern uint8_t username_string[10];
extern uint8_t password_string[10];

extern uint8_t client_id[];


typedef enum mqtt_err {
	mqtt_errSuccess,
	outOfMem,
	badSocket,
	mqtt_timeout,

}mqtt_err_t;

enum sendStatus{
	MSG_UNSENT,
	MSG_SENT,
	MSG_WAITING_FOR_ACK,
	MSG_WAITING_FOR_COMP,
	MSG_WAITING_FOR_PUBREL,
	MSG_WAITING_FOR_PUBACK,
	MSG_HOLD_IN_MEMORY,
	MSG_SEND_ERROR,
	MSG_COMPLETE,
};

enum connectStatus {
	CONNECTION_ACCEPTED,
	CONNECTION_BAD_VERSION,
	CONNECTION_IDENTITY_REJECTED,
	CONNECTION_SERVER_UNAVAIL,
	CONNECTION_NOT_AUTHORIZED
};

typedef struct mqtt_fixedHdr {
	union{
		uint8_t firstByte;

		struct {
			uint8_t retain : 1;
			uint8_t qos : 2 ;
			uint8_t dup_flag : 1;
			uint8_t msg_type : 4;
		}bits;
	};
	uint32_t pkt_len;
}mqtt_fixedHdr_t;

typedef union mqtt_varHdr {
	uint8_t firstbyte;

	struct {
		uint8_t reserved : 1;
		uint8_t clean_session : 1;
		uint8_t will_flag : 1;
		uint8_t will_qos : 2;
		uint8_t will_retain : 1;
		uint8_t pass_flag : 1;
		uint8_t user_flag : 1;
	}bits;
}mqtt_varHdr_t;
#if 0
struct {
	uint8_t user_flag : 1;
	uint8_t pass_flag : 1;
	uint8_t will_retain : 1;
	uint8_t will_qos : 2;
	uint8_t will_flag : 1;
	uint8_t clean_session : 1;
	uint8_t reserved : 1;
}bits;
#endif

typedef struct mqtt_msg {
	mqtt_fixedHdr_t fHdr;
	mqtt_varHdr_t 	vHdr;
	size_t 			len;
	uint32_t		timesent;
	uint16_t 		msgID;
	enum sendStatus	status;
	uint8_t			retries;
	uint8_t 		pkt[];

}mqtt_msg_t;

typedef struct mqtt_will {
	char					* topic;//Not a big fan of having the will here may look into moving
	uint8_t					* message;
	uint8_t					qos;
}mqtt_will_t;

typedef struct mqtt_pubSubMsg {
	uint8_t *topic;
	uint8_t *payload;
	uint32_t payLen;
	uint16_t msgID;//I don't know if I want this value here
	uint8_t  qos;
}mqtt_pubSubMsg_t;

typedef void (*pubMsgCallback)(struct mqtt_client *, mqtt_pubSubMsg_t *);

typedef struct mqtt_client {
	int						socket;
	char					name[16];
	char					* server;
	uint16_t				port;
	uint32_t 				keepalive;
	uint32_t				timeLastSent;
	uint32_t				timeout;
	mqtt_will_t				* will;
	List_t 					* txList;
	List_t					* subList;
	pubMsgCallback 			msgCB;
	void					(*subMsgHook)(struct mqtt_client *, mqtt_msg_t *);
	struct sockaddr_in 		serverAddr;
	sys_sem_t				pingResp;
	sys_sem_t				pubAck;
	sys_sem_t				connAck;
	sys_mutex_t				txOK;
	uint32_t				connected;
}mqtt_client_t;

typedef char * mqtt_topic_t;
typedef char * mqtt_payload_t;



/*** PROTOTYPES ***/
/* mqtt.c */
mqtt_err_t mqtt_check_keepalive(mqtt_client_t *client);
mqtt_err_t mqtt_createClient(mqtt_client_t *client, char *server, uint16_t port, uint32_t keepAlive, const char *name);
mqtt_err_t mqtt_sendConnect(mqtt_client_t *client);
mqtt_err_t mqtt_publish(mqtt_client_t *client, mqtt_topic_t topic,
		mqtt_payload_t payload, uint8_t qos, uint8_t retain);
mqtt_err_t mqtt_subscribe(mqtt_client_t *client, mqtt_topic_t topic, uint8_t qos);
void mqtt_registerMsgCB(mqtt_client_t *client, pubMsgCallback cb);

/* mqtt_thread.c */
void mqtt_clientThread(void *arg);
void mqtt_writeData(Node_t *node, mqtt_client_t *client);

void mqtt_sendPing(mqtt_client_t *client);
void mqtt_dnsCallback(const char *name, ip_addr_t *ipaddr, void *callback_arg);

size_t mqtt_lenEncode(uint8_t *buf, uint32_t pktLen);
mqtt_err_t mqtt_waitForConnAck(mqtt_client_t *client, uint32_t timeout);

/* mqtt_send.c */
mqtt_msg_t *mqtt_findPubWithID(mqtt_client_t *client, uint16_t msgID);
#endif /* MQTT_H_ */
