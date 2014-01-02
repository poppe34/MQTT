/*
 * mqtt_net.c
 *
 *  Created on: Oct 15, 2013
 *      Author: poppe
 */

#include "stm32f4xx.h"
#include <board.h>
#include <rtthread.h>

#include "mqtt.h"
#include "lwip/ip_addr.h"

/* LWIP mqtt Client */
static volatile int dnsDone = 0;




//ALIGN(RT_ALIGN_SIZE)

//static struct rt_data_queue TxQueue;
static void rt_thread_entry_mqttThread(void* parameter)
{
	//rt_data_queue_init(TxQueue, MAX_TXQUEUE_SIZE, )
}

mqtt_err_t mqtt_netTx(mqtt_msg_t *msg, uint32_t timeout)
{

	return 0;
}

mqtt_err_t mqtt_waitForDNS(uint32_t timeout)
{
	rt_tick_t ticks = timeout * RT_TICK_PER_SECOND/1000;
	ticks += rt_tick_get();

	while(!dnsDone){
		if(rt_tick_get() > ticks)
		{
			dnsDone = 0;
			return mqtt_timeout;
		}
	}
		dnsDone = 0;
		return mqtt_errSuccess;
}

void mqtt_dnsCallback(const char *name, ip_addr_t *ipaddr, void *callback_arg)
{
	mqtt_client_t *client = (mqtt_client_t *)callback_arg;

	client->serverAddr.sin_addr.s_addr = ipaddr;

	printf("IP ADDRESS FOUND: %s\n", ipaddr_ntoa(ipaddr));

	dnsDone = 1;
}


