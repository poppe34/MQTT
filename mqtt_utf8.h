/*
 * mqtt_utf8.h
 *
 *  Created on: Oct 20, 2013
 *      Author: poppe
 */

#ifndef MQTT_UTF8_H_
#define MQTT_UTF8_H_

#include "stdint.h"

typedef struct mqtt_utf {
	uint16_t len; //be very careful this is Big Endian
	uint8_t *string;
}mqtt_utf_t;

uint32_t mqtt_stringToUtf(uint8_t *S, uint8_t *B);

#endif /* MQTT_UTF8_H_ */
