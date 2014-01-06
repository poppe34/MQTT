/*
 * mqtt_client.c
 *
 *  Created on: Oct 15, 2013
 *      Author: poppe
 */

#include <mqtt.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/sockets.h>
#include <lwip/ip_addr.h>
#include <lwip/debug.h>
#include <lwip/dns.h>
#include <lwip/sys.h>
#include <lwip/inet.h>
#include <arch/sys_arch.h>
#include <netif/ethernetif.h>


/**
 * Move to mqtt.c
 */
mqtt_err_t mqtt_createClient(mqtt_client_t *client, char *server, uint16_t port, uint32_t keepAlive, const char *name)
{
	RT_ASSERT(client);
	struct sockaddr_in serverAddr;
	struct in_addr addr;
	uint32_t serveraddr;
	int err;

//	memset(client, 0, sizeof(mqtt_client_t));

	memcpy(client->name, name, strlen(name));
	client->keepalive = keepAlive;
	client->timeout = MQTT_DEFAULT_TIMEOUT_MS;

	serveraddr = inet_addr(server);

	if(serveraddr != INADDR_NONE)
	{
		client->serverAddr.sin_addr.s_addr = ipaddr_addr(server);
	}
	else
	{
		LWIP_DEBUG(MQTT_DEBUG, ("Waiting to find IP\n"));
		err = dns_gethostbyname(server, &client->serverAddr.sin_addr.s_addr, mqtt_dnsCallback, client);
		if(err == ERR_INPROGRESS)
		{
			if(mqtt_waitForDNS(5000))
			{
				LWIP_DEBUG(MQTT_DEBUG, ("MQTT: no IP found\n"));
				return badSocket;
			}
			LWIP_DEBUG(MQTT_DEBUG, ("IP found:\n"));
		}
	}

	client->serverAddr.sin_port = htons(port);
	client->serverAddr.sin_family = AF_INET;

	sys_sem_new(&client->pingResp, 0);
	sys_sem_new(&client->connAck, 0);
	sys_sem_new(&client->pubAck, 0);
	sys_mutex_new(&client->txOK);

	client->txList = MQTT_MALLOC(sizeof(List_t));
	dllist_initList(client->txList, DLL_FIFO);

	client->socket = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );

	LWIP_DEBUG(MQTT_DEBUG, ("client %s socket number: %i\n", client->name, client->socket));
	if(client->socket < 0 || client->socket > 100)
	{
		rt_kprintf( "MQTT: SOCKET FAILED\n" );
		return badSocket;
	}
	/* set socket option */
	int sock_opt = 5000; /* 5 seconds */
	lwip_setsockopt(client->socket, SOL_SOCKET, SO_RCVTIMEO, &sock_opt, sizeof(sock_opt));

	err = connect(client->socket, (struct sockaddr *)&client->serverAddr, sizeof(struct sockaddr_in));
	if(err)
	{
		LWIP_DEBUG(MQTT_DEBUG, ("MQTT: did not connect to socket: %i\n", client->socket));
		return badSocket;
	}
	sys_thread_t t = sys_thread_new("MQTTRecv", mqtt_clientThread, client, 8192, 5);

	LWIP_DEBUG(MQTT_DEBUG, ("Created client: %s:\n", client->name));

	mqtt_sendConnect(client);//FIXME: I don't want to in the future auto connect

	return mqtt_errSuccess;

}





