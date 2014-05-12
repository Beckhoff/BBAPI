/**
    Watchdog driver using the Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2014  Beckhoff Automation GmbH

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
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/watchdog.h>

#include "../api.h"
#include "../bbapi.h"
#include "../TcBaDevDef_gpl.h"
#include "watchdog.h"

static int bbapi_wd_write(uint32_t offset, const void *in, uint32_t size)
{
	return bbapi_write(BIOSIGRP_WATCHDOG, offset, in, size);
}

static int wd_ping(struct watchdog_device *wd)
{
	set_bit(WDOG_KEEPALIVE, &wd->status);
	return bbapi_wd_write(BIOSIOFFS_WATCHDOG_IORETRIGGER, NULL, 0);
}

/**
 * Linux watchdog API uses seconds in a range of unsigned int, Beckhoff
 * BIOS API uses minutes or seconds as a timebase together with an
 * uint8_t timeout value. This function converts Linux watchdog seconds
 * into BBAPI timebase + timeout
 * As long as sec fits into a uint8_t we use seconds as a time base
 */
static int wd_set_timeout(struct watchdog_device *wd, unsigned int sec)
{
	if (sec > 255) {
		wd->timeout = sec - (sec % 60);
	} else {
		wd->timeout = sec;
	}
	return 0;
}

static int wd_start(struct watchdog_device *const wd)
{
	const uint8_t enable = 1;
	const uint8_t timebase = wd->timeout > 255;
	const uint8_t timeout = timebase ? wd->timeout / 60 : wd->timeout;
	if (bbapi_wd_write(BIOSIOFFS_WATCHDOG_ACTIVATE_PWRCTRL, &enable, sizeof(enable))) {
		pr_warn("%s(): select PwrCtrl IO watchdog failed\n", __FUNCTION__);
		return -1;
	}
	if (bbapi_wd_write(BIOSIOFFS_WATCHDOG_CONFIG, &timebase, sizeof(timebase))) {
		pr_warn("%s(): BIOSIOFFS_WATCHDOG_CONFIG failed\n", __FUNCTION__);
		return -1;
	}
	if (bbapi_wd_write(BIOSIOFFS_WATCHDOG_ENABLE_TRIGGER, &timeout, sizeof(timeout))) {
		pr_warn("%s(): enable watchdog failed\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

static unsigned int wd_status(struct watchdog_device *const wd)
{
	const unsigned int value = wd->status;
	clear_bit(WDOG_KEEPALIVE, &wd->status);
	return value;
}

static int wd_stop(struct watchdog_device *wd)
{
	const uint8_t disable = 0;
	if (bbapi_wd_write(BIOSIOFFS_WATCHDOG_ENABLE_TRIGGER, &disable, sizeof(disable))) {
		pr_warn("%s(): disable watchdog failed\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

static const struct watchdog_ops wd_ops = {
	.owner = THIS_MODULE,
	.start = wd_start,
	.stop = wd_stop,
	.ping = wd_ping,
	.status = wd_status,
	.set_timeout = wd_set_timeout,
#if 0
	long (*ioctl)(struct watchdog_device *, unsigned int, unsigned long);
#endif
};
static const struct watchdog_info wd_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE | WDIOF_KEEPALIVEPING,
	.firmware_version = 0,
	.identity = "bbapi_watchdog",
};

static struct watchdog_device g_wd = {
	.info = &wd_info,
	.ops = &wd_ops,
	.timeout = BBAPI_WATCHDOG_TIMEOUT_SEC,
	.max_timeout = BBAPI_WATCHDOG_MAX_TIMEOUT_SEC,
};

static int __init bbapi_watchdog_init_module(void)
{
	BUILD_BUG_ON(1 << WDOG_KEEPALIVE != WDIOF_KEEPALIVEPING);
	pr_info("%s, %s\n", DRV_DESCRIPTION, DRV_VERSION);
	return watchdog_register_device(&g_wd);
}

static void __exit bbapi_watchdog_exit(void)
{
	watchdog_unregister_device(&g_wd);
	pr_info("Watchdog unregistered\n");
}

module_init(bbapi_watchdog_init_module);
module_exit(bbapi_watchdog_exit);

MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR("Patrick Bruenn <p.bruenn@beckhoff.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
