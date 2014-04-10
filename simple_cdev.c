/**
    Character Driver for Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2013 - 2014  Beckhoff Automation GmbH

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

#include <linux/fs.h>
#include <linux/module.h>
#include "simple_cdev.h"

int simple_cdev_init(struct simple_cdev *dev, const char *classname,
		     const char *devicename, struct file_operations *file_ops)
{
	if (alloc_chrdev_region(&dev->dev, 0, 1, KBUILD_MODNAME) < 0) {
		return -1;
	}
	pr_debug("Character Device region allocated <Major, Minor> <%d, %d>\n",
		 MAJOR(dev->dev), MINOR(dev->dev));
	if ((dev->class = class_create(THIS_MODULE, classname)) == NULL) {
		pr_warn("class_create() failed!\n");
		unregister_chrdev_region(dev->dev, 1);
		return -1;
	}
	//Create device File
	if (device_create(dev->class, NULL, dev->dev, NULL, devicename) == NULL) {
		pr_warn("device_create() failed!\n");
		class_destroy(dev->class);
		unregister_chrdev_region(dev->dev, 1);
		return -1;
	}
	//Init Device File
	cdev_init(&dev->cdev, file_ops);
	if (cdev_add(&dev->cdev, dev->dev, 1) == -1) {
		pr_warn("cdev_add() failed!\n");
		device_destroy(dev->class, dev->dev);
		class_destroy(dev->class);
		unregister_chrdev_region(dev->dev, 1);
		return -1;
	}
	return 0;
}

void simple_cdev_remove(struct simple_cdev *dev)
{
	cdev_del(&dev->cdev);
	device_destroy(dev->class, dev->dev);
	class_destroy(dev->class);
	unregister_chrdev_region(dev->dev, 1);
}
