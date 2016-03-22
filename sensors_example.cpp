/**
    Example code to read sensor values from the BBAPI

    Copyright (C) 2016  Beckhoff Automation GmbH
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files
    (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "TcBaDevDef_gpl.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
	int bbapi_dev = open("/dev/bbapi", O_RDWR);

	if (-1 == bbapi_dev) {
		printf("Open '/dev/bbapi' failed\n");
		return -1;
	}

	uint32_t num_sensors;
	struct bbapi_struct cmd_num_sensors {
		BIOSIGRP_SYSTEM, BIOSIOFFS_SYSTEM_COUNT_SENSORS,
		nullptr, 0, &num_sensors, sizeof(num_sensors)
	};

	if (-1 == ioctl(bbapi_dev, BBAPI_CMD, &cmd_num_sensors)) {
		printf("Read number of sensors failed\n");
		return -1;
	}

	while (num_sensors > 0) {
		char text[256];
		SENSORINFO info;
		struct bbapi_struct cmd_read_sensor {
			BIOSIGRP_SYSTEM, num_sensors,
			nullptr, 0, &info, sizeof(info)
		};

		if (-1 == ioctl(bbapi_dev, BBAPI_CMD, &cmd_read_sensor)) {
			printf("Read sensor #%u failed\n", num_sensors);
			return -1;
		}
		info.snprintf(text, sizeof(text));
		printf("%02d: %s\n", num_sensors, text);
		--num_sensors;
	}
}
