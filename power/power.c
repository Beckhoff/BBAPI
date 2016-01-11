/**
    CX power supply driver using the Beckhoff BIOS API
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
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/workqueue.h>

#include "../api.h"
#include "../TcBaDevDef_gpl.h"

#define DRV_VERSION      "0.1"
#define DRV_DESCRIPTION  "Beckhoff BIOS API power supply driver"

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

struct bbapi_cx2100_info {
	const char *manufacturer;
	char serial[16];

	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor;
	struct power_supply *psy;
	const struct power_supply_desc *psy_desc;

	uint16_t charging_current_mA;
	uint8_t capacity_percent;
	uint8_t power_status;
	uint8_t battery_present;
	uint32_t battery_runtime_s;
	char temp_C;

	struct gpio_chip gpio_chip;
	struct Bapi_GpioInfoEx gpio_info;
};

static int ups_get_status(const struct bbapi_cx2100_info *pbi)
{
#define CXUPS_POWERSTATUS_ONBATTERIES 0x02
#define CXUPS_POWERSTATUS_ONLINE 0x01

	if (CXUPS_POWERSTATUS_ONBATTERIES == pbi->power_status) {
		return POWER_SUPPLY_STATUS_DISCHARGING;
	}

	if (CXUPS_POWERSTATUS_ONLINE != pbi->power_status) {
		return POWER_SUPPLY_STATUS_UNKNOWN;
	}

	if (pbi->capacity_percent >= 100) {
		return POWER_SUPPLY_STATUS_FULL;
	}

	if (pbi->charging_current_mA) {
		return POWER_SUPPLY_STATUS_CHARGING;
	}
	return POWER_SUPPLY_STATUS_NOT_CHARGING;
}

static void bbapi_cx2100_read_status(struct bbapi_cx2100_info *pbi)
{
	bbapi_read(BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETTEMP,
		   &pbi->temp_C, sizeof(pbi->temp_C));
	bbapi_read(BIOSIGRP_CXUPS, BIOSIOFFS_CXUPS_GETPOWERSTATUS,
		   &pbi->power_status, sizeof(pbi->power_status));
	bbapi_read(BIOSIGRP_CXUPS, BIOSIOFFS_CXUPS_GETBATTERYPRESENT,
		   &pbi->battery_present, sizeof(pbi->battery_present));
	bbapi_read(BIOSIGRP_CXUPS, BIOSIOFFS_CXUPS_GETBATTERYCAPACITY,
		   &pbi->capacity_percent, sizeof(pbi->capacity_percent));
	bbapi_read(BIOSIGRP_CXUPS, BIOSIOFFS_CXUPS_GETBATTERYRUNTIME,
		   &pbi->battery_runtime_s, sizeof(pbi->battery_runtime_s));
	bbapi_read(BIOSIGRP_CXUPS, BIOSIOFFS_CXUPS_GETCHARGINGCURRENT,
		   &pbi->charging_current_mA, sizeof(pbi->charging_current_mA));
}

static void bbapi_power_monitor(struct work_struct *work)
{
	struct bbapi_cx2100_info *pbi = container_of(work,
						     struct bbapi_cx2100_info,
						     monitor.work);

	bbapi_cx2100_read_status(pbi);
	if (pbi->psy) {
		power_supply_changed(pbi->psy);
	}
	queue_delayed_work(pbi->monitor_wqueue, &pbi->monitor, HZ * 5);
}

static enum power_supply_property cx2100_0904_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_SERIAL_NUMBER,
};

static int bbapi_power_get_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	const struct bbapi_cx2100_info *pbi = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = ups_get_status(pbi);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = pbi->battery_present;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = !(pbi->power_status == CXUPS_POWERSTATUS_ONLINE);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = pbi->capacity_percent;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = pbi->temp_C;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		val->intval = pbi->battery_runtime_s;
		break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = pbi->manufacturer;
		break;
	case POWER_SUPPLY_PROP_SERIAL_NUMBER:
		val->strval = pbi->serial;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static const struct power_supply_desc cx2100_0904_desc = {
	.name = "cx2100-0904",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = cx2100_0904_props,
	.num_properties = ARRAY_SIZE(cx2100_0904_props),
	.get_property = bbapi_power_get_property,
};

#define cx2100_read(offset, buffer) \
	bbapi_read(BIOSIGRP_CXPWRSUPP, offset, &buffer, sizeof(buffer))

#define sups_read(offset, buffer) \
	bbapi_read(BIOSIGRP_SUPS, offset, &buffer, sizeof(buffer))

static int init_workqueue(struct bbapi_cx2100_info *pbi, struct device *parent)
{
	INIT_DELAYED_WORK(&pbi->monitor, bbapi_power_monitor);
	pbi->monitor_wqueue = create_singlethread_workqueue(dev_name(parent));
	if (!pbi->monitor_wqueue) {
		dev_err(parent, "wqueue init failed\n");
		return -ESRCH;
	}

	if (!queue_delayed_work(pbi->monitor_wqueue, &pbi->monitor, HZ * 1)) {
		destroy_workqueue(pbi->monitor_wqueue);
		return -ESRCH;
	}
	return 0;
}

static int init_cx2100_0904(struct bbapi_cx2100_info *pbi,
			    struct device *parent)
{
	static struct power_supply_config psy_cfg = { };
	uint32_t serial = 0;

	cx2100_read(BIOSIOFFS_CXPWRSUPP_GETSERIALNO, serial);
	snprintf(pbi->serial, sizeof(pbi->serial), "%u", serial);

	pbi->manufacturer = "Beckhoff Automation";

	psy_cfg.drv_data = pbi;
	pbi->psy = power_supply_register(parent, &cx2100_0904_desc, &psy_cfg);
	if (IS_ERR(pbi->psy)) {
		dev_err(parent, "failed to register power supply\n");
		return PTR_ERR(pbi->psy);
	}
	return init_workqueue(pbi, parent);
}

static int power_gpio_get(struct gpio_chip *chip, unsigned nr)
{
	struct bbapi_cx2100_info *pbi =
	    container_of(chip, struct bbapi_cx2100_info, gpio_chip);

	return inl(pbi->gpio_info.address) & pbi->gpio_info.bitmask;
}

static const char *power_gpio_names[] = {
	"sups_pwrfail",
};

static const struct gpio_chip power_gpio_chip = {
	.label = KBUILD_MODNAME,
	.owner = THIS_MODULE,
	.get = power_gpio_get,
	.base = -1,
	.ngpio = 1,
	.names = power_gpio_names,
};

static int init_sups(struct bbapi_cx2100_info *pbi)
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
	     power_gpio_names[0])) {
		pr_err("request_region() failed\n");
		return -ENODEV;
	}

	memcpy(&pbi->gpio_chip, &power_gpio_chip, sizeof(pbi->gpio_chip));
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

static int bbapi_power_init(struct bbapi_cx2100_info *pbi,
			    struct device *parent)
{
#define CXPWRSUPP_TYPE_CX2100_0904 904u
#define CXPWRSUPP_TYPE_CX2100_0004 4u
	uint32_t type;

	if (!cx2100_read(BIOSIOFFS_CXPWRSUPP_GETTYPE, type)) {
		switch (type) {
		case CXPWRSUPP_TYPE_CX2100_0904:
			return init_cx2100_0904(pbi, parent);
		case CXPWRSUPP_TYPE_CX2100_0004:
			return -ENODEV;
		default:
			pr_warn("unknown cx2100 type: %x|%u\n", type, type);
			return -ENODEV;
		}
	}
	return init_sups(pbi);
}

static int bbapi_power_probe(struct platform_device *pdev)
{
	struct bbapi_cx2100_info *pbi;
	int status = 0;

	pbi = kzalloc(sizeof(*pbi), GFP_KERNEL);
	if (!pbi) {
		return -ENOMEM;
	}
	dev_set_drvdata(&pdev->dev, pbi);

	status = bbapi_power_init(pbi, &pdev->dev);
	if (status) {
		kfree(pbi);
	}
	return status;
}

static int bbapi_power_remove(struct platform_device *pdev)
{
	struct bbapi_cx2100_info *pbi = platform_get_drvdata(pdev);

	if (pbi->gpio_info.address) {
		gpiochip_remove(&pbi->gpio_chip);
		release_region(pbi->gpio_info.address, pbi->gpio_info.length);
	}
	if (pbi->psy) {
		cancel_delayed_work_sync(&pbi->monitor);
		destroy_workqueue(pbi->monitor_wqueue);
		power_supply_unregister(pbi->psy);
	}
	kfree(pbi);
	return 0;
}

static struct platform_driver bbapi_power_driver = {
	.driver = {
		   .name = KBUILD_MODNAME,
		   },
	.probe = bbapi_power_probe,
	.remove = bbapi_power_remove,
};

module_platform_driver(bbapi_power_driver);
MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR("Patrick Bruenn <p.bruenn@beckhoff.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
