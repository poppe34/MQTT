/*
 * mqtt_thread.c
 *
 *  Created on: Dec 27, 2013
 *      Author: poppe
 */

#include "mqtt.h"

static char thread_mqttThread_stack[1024];
struct rt_thread thread_mqttThread;
static uint8_t receiveBuf[2048];


/**************************************************************************************************
 *
 * Main Thread for the client
 *
 *************************************************************************************************/
void mqtt_clientThread(void *arg)
{
	mqtt_client_t *client = (mqtt_client_t *)arg;
	mqtt_msg_t *msg;
	mqtt_setKeepAlive(client, client->keepalive);
	size_t bytesRead;
	uint32_t timeSpent;

	int err;
	/* set socket option */
	int sock_opt = 500; /* 500 ms */
	lwip_setsockopt(client->socket, SOL_SOCKET, SO_RCVTIMEO, &sock_opt, sizeof(sock_opt));

	while(1)
	{
		bytesRead = recv(client->socket, receiveBuf, 256, 0);
		if(bytesRead < 1500 && bytesRead != 0)
		{
			LWIP_DEBUGF(MQTT_DEBUG, ("MQTT received 0x%x bytes of data\n", bytesRead));

			mqtt_handler(client, receiveBuf, bytesRead);
#if 0
			int x = 0;
			while(bytesRead)
			{
				printf("0x%X ", receiveBuf[x]);
				x++;
				bytesRead--;
			}
			printf("\n");
#endif
		}

		dllist_foreachNode(client->txList, mqtt_writeData, client);

		if(client->connected)
			mqtt_check_keepalive(client);

		if(MQTT_UPDATE_DELAY)
			sys_msleep(MQTT_UPDATE_DELAY);
	}

}


mqtt_err_t mqtt_netInit(void)
{

    /* init and start mqtt recv data thread */
    rt_thread_init(&thread_mqttThread,
                   "mqttRecv",
                   thread_mqttThread_stack,
                   RT_NULL,
                   &thread_mqttThread_stack[0],
                   sizeof(thread_mqttThread_stack),11,5);
    rt_thread_startup(&thread_mqttThread);

    return mqtt_errSuccess;

}
