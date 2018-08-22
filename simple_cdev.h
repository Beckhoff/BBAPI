// SPDX-License-Identifier: MIT
/**
    Character Driver for Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2013 - 2018 Beckhoff Automation GmbH & Co. KG
*/

#ifndef _SIMPLE_CDEV_H_
#define _SIMPLE_CDEV_H_

#include <linux/cdev.h>
#include <linux/device.h>

struct simple_cdev {
	dev_t dev;		// First device number
	struct cdev cdev;	// Character device structure
	struct class *class;	// Device class
};

extern int simple_cdev_init(struct simple_cdev *dev, const char *classname,
			    const char *devicename,
			    struct file_operations *file_ops);
extern void simple_cdev_remove(struct simple_cdev *dev);
#endif /* #ifndef _SIMPLE_CDEV_H_ */
