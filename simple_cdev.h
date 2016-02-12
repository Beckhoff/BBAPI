/**
    Character Driver for Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2013 - 2016  Beckhoff Automation GmbH

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
