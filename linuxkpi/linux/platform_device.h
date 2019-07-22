// SPDX-License-Identifier: MIT
/**
    FreeBSD wrapper to reuse Beckhoff BIOS API Linux driver
    Copyright (C) 2019 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
*/

#define platform_device_register(x) 0
#define platform_device_unregister(x) 0
struct platform_device {
	const char *const name;
	const int id;
	const struct {
		void(*release)(struct device*);
	} dev;
};
