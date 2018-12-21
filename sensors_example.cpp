// SPDX-License-Identifier: MIT
/**
    Example code to read sensor values from the BBAPI

    Copyright (C) 2016 - 2018 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
*/

#include "TcBaDevDef.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
	int bbapi_dev = open("/dev/bbapi", O_RDWR);
	int status;

	if (-1 == bbapi_dev) {
		printf("Open '/dev/bbapi' failed\n");
		return -1;
	}

	uint32_t num_sensors;
	struct bbapi_struct cmd_num_sensors = {
		BIOSIGRP_SYSTEM,
		BIOSIOFFS_SYSTEM_COUNT_SENSORS,
		NULL,
		0,
		&num_sensors,
		sizeof(num_sensors)
	};

	status = ioctl(bbapi_dev, BBAPI_CMD, &cmd_num_sensors);
	if (status) {
		printf("Read number of sensors failed\n");
		return status;
	}

	while (num_sensors > 0) {
		char text[256];
		SENSORINFO info;
		struct bbapi_struct cmd_read_sensor = {
			BIOSIGRP_SYSTEM,
			num_sensors,
			NULL,
			0,
			&info,
			sizeof(info)
		};

		status = ioctl(bbapi_dev, BBAPI_CMD, &cmd_read_sensor);
		if (status) {
			printf("Read sensor #%u failed with 0x%x\n",
			       num_sensors, status);
			return status;
		}
		SENSORINFO_snprintf(&info, text, sizeof(text));
		printf("%02d: %s\n", num_sensors, text);
		--num_sensors;
	}
}
