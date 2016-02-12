/**
    1-second UPS driver using the Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2015 - 2016  Beckhoff Automation GmbH

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

#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include "../api.h"
#include "../TcBaDevDef_gpl.h"

#define DRV_VERSION      "0.2"
#define DRV_DESCRIPTION  "Beckhoff BIOS API 1-second UPS driver"

struct bbapi_sups_info {
	struct gpio_chip gpio_chip;
	struct Bapi_GpioInfoEx gpio_info;
};

#define sups_read(offset, buffer) \
	bbapi_read(BIOSIGRP_SUPS, offset, &buffer, sizeof(buffer))

static int sups_gpio_get(struct gpio_chip *chip, unsigned nr)
{
	struct bbapi_sups_info *pbi =
	    container_of(chip, struct bbapi_sups_info, gpio_chip);

	return inl(pbi->gpio_info.address) & pbi->gpio_info.bitmask;
}

static const char *sups_gpio_names[] = {
	"sups_pwrfail",
};

static const struct gpio_chip sups_gpio_chip = {
	.label = KBUILD_MODNAME,
	.owner = THIS_MODULE,
	.get = sups_gpio_get,
	.base = -1,
	.ngpio = 1,
	.names = sups_gpio_names,
};

static int init_sups(struct bbapi_sups_info *pbi)
{
	int status;

	if (sups_read(BIOSIOFFS_SUPS_GPIO_PIN_EX, pbi->gpio_info)) {
		struct TSUps_GpioInfo legacy;

		if (sups_read(BIOSIOFFS_SUPS_GPIO_PIN, legacy)) {
			pr_err("BIOSIOFFS_SUPS_GPIO_PIN not supported\n");
			return -ENODEV;
		}

		pbi->gpio_info.address = legacy.ioAddr + legacy.offset;
		pbi->gpio_info.bitmask = legacy.params;
		pbi->gpio_info.length = 4;
	}

	if (!request_region
	    (pbi->gpio_info.address, pbi->gpio_info.length,
	     sups_gpio_names[0])) {
		pr_err("request_region() failed\n");
		return -ENODEV;
	}

	memcpy(&pbi->gpio_chip, &sups_gpio_chip, sizeof(pbi->gpio_chip));
	status = gpiochip_add(&pbi->gpio_chip);
	if (status) {
		release_region(pbi->gpio_info.address, pbi->gpio_info.length);
		return status;
	}

	pr_info("registered %s as gpiochip%d with #%d GPIOs.\n",
		pbi->gpio_chip.label, pbi->gpio_chip.base,
		pbi->gpio_chip.ngpio);
	return 0;
}

static int bbapi_sups_probe(struct platform_device *pdev)
{
	struct bbapi_sups_info *pbi;
	int status = 0;

	pbi = kzalloc(sizeof(*pbi), GFP_KERNEL);
	if (!pbi) {
		return -ENOMEM;
	}
	dev_set_drvdata(&pdev->dev, pbi);

	status = init_sups(pbi);
	if (status) {
		kfree(pbi);
	}
	return status;
}

static int bbapi_sups_remove(struct platform_device *pdev)
{
	struct bbapi_sups_info *pbi = platform_get_drvdata(pdev);

	if (pbi->gpio_info.address) {
		gpiochip_remove(&pbi->gpio_chip);
		release_region(pbi->gpio_info.address, pbi->gpio_info.length);
	}
	kfree(pbi);
	return 0;
}

static struct platform_driver bbapi_sups_driver = {
	.driver = {
		   .name = KBUILD_MODNAME,
		   },
	.probe = bbapi_sups_probe,
	.remove = bbapi_sups_remove,
};

module_platform_driver(bbapi_sups_driver);
MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR("Patrick Bruenn <p.bruenn@beckhoff.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
