/*
 * mqtt_conf.h
 *
 *  Created on: Oct 20, 2013
 *      Author: poppe
 */

#ifndef MQTT_CONF_H_
#define MQTT_CONF_H_

#include "lwip/debug.h"

#define DEFALUT_CLIENT_ID "POPPEtest1234"
#define DEFAULT_SERVER_IP "192.168.34.45"

#define MQTT_MAX_PKT_SIZE  128

//TODO: Make more eligant place and setup for wills
#define MQTT_DEFAULT_WILL_TOPIC		"matt/will"
#define MQTT_DEFAULT_WILL_MSG		"dead"
#define MQTT_DEFAULT_WILL_QOS		1

#define MQTT_DEBUG	LWIP_DBG_ON
#endif /* MQTT_CONF_H_ */
