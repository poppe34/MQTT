/*
 * mqtt_utf8.c

 *
 *  Created on: Oct 20, 2013
 *      Author: poppe
 */
#include "mqtt.h"
#include "string.h"

uint32_t mqtt_stringToUtf(uint8_t *S, uint8_t *B)
{
	uint32_t size = strlen(S);

	*B++ = ((size >> 8) & 0xff);
	*B++ = (size & 0xff);

	memcpy(B, S, size);

	return (size + 2);
}


