// SPDX-License-Identifier: MIT
/**
    Character Driver for Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2013 - 2018 Beckhoff Automation GmbH & Co. KG
*/

#include <linux/fs.h>
#include <linux/module.h>
#include "simple_cdev.h"

int simple_cdev_init(struct simple_cdev *dev, const char *classname,
		     const char *devicename, struct file_operations *file_ops)
{
	if (alloc_chrdev_region(&dev->dev, 0, 1, KBUILD_MODNAME) < 0) {
		pr_warn("alloc_chrdev_region() failed!\n");
		return -1;
	}

	cdev_init(&dev->cdev, file_ops);
	dev->cdev.owner = THIS_MODULE;
	kobject_set_name(&dev->cdev.kobj, "%s", devicename);
	if (cdev_add(&dev->cdev, dev->dev, 1) == -1) {
		pr_warn("cdev_add() failed!\n");
		goto rollback_region;
	}

	if ((dev->class = class_create(THIS_MODULE, classname)) == NULL) {
		pr_warn("class_create() failed!\n");
		goto rollback_cdev;
	}

	if (device_create(dev->class, NULL, dev->dev, NULL, "%s", devicename) == NULL) {
		pr_warn("device_create() failed!\n");
		goto rollback_class;
	}
	return 0;

rollback_class:
	class_destroy(dev->class);
rollback_cdev:
	cdev_del(&dev->cdev);
rollback_region:
	unregister_chrdev_region(dev->dev, 1);
	return -1;
}

void simple_cdev_remove(struct simple_cdev *dev)
{
	device_destroy(dev->class, dev->dev);
	class_destroy(dev->class);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->dev, 1);
}
