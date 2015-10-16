/**
    Watchdog driver using the Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2014-2015  Beckhoff Automation GmbH

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
#include "../TcBaDevDef_gpl.h"
#include "watchdog.h"

static int bbapi_wd_write(uint32_t offset, const void *in, uint32_t size)
{
	return bbapi_write(BIOSIGRP_WATCHDOG, offset, in, size);
}

#ifdef ENABLE_KEEPALIVEPING
static int wd_ping(struct watchdog_device *wd)
{
	set_bit(WDOG_KEEPALIVE, &wd->status);
	return bbapi_wd_write(BIOSIOFFS_WATCHDOG_IORETRIGGER, NULL, 0);
}
#endif

static int wd_start(struct watchdog_device *const wd)
{
	static const uint32_t offset_enable = BIOSIOFFS_WATCHDOG_ACTIVATE_PWRCTRL;
	static const uint32_t offset_timeout = BIOSIOFFS_WATCHDOG_ENABLE_TRIGGER;
	static const uint32_t offset_timebase = BIOSIOFFS_WATCHDOG_CONFIG;
	static const uint8_t enable = 1;
	const uint8_t timebase = wd->timeout > 255;
	const uint8_t timeout = timebase ? wd->timeout / 60 : wd->timeout;

	int result = bbapi_wd_write(offset_enable, &enable, sizeof(enable));

	switch (result) {
		case BIOSAPI_SRVNOTSUPP:
			pr_info("change watchdog mode not supported\n");
		case BIOSAPIERR_NOERR:
			break;
		default:
			pr_warn("select PwrCtrl IO failed with: 0x%x\n", result);
			return result;
	}

	result = bbapi_wd_write(offset_timebase, &timebase, sizeof(timebase));
	if (result) {
		pr_warn("set timebase failed with: 0x%x\n", result);
		return result;
	}

	result = bbapi_wd_write(offset_timeout, &timeout, sizeof(timeout));
	if (result) {
		pr_warn("enable watchdog failed with: 0x%x\n", result);
	}
	return result;
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
	return wd_start(wd);
}

static unsigned int wd_status(struct watchdog_device *const wd)
{
	const unsigned int value = wd->status;
	clear_bit(WDOG_KEEPALIVE, &wd->status);
	return value;
}

static int wd_stop(struct watchdog_device *wd)
{
	const uint32_t offset = BIOSIOFFS_WATCHDOG_ENABLE_TRIGGER;
	const uint8_t disable = 0;
	if (bbapi_wd_write(offset, &disable, sizeof(disable))) {
		pr_warn("%s(): disable watchdog failed\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

static const struct watchdog_ops wd_ops = {
	.owner = THIS_MODULE,
	.start = wd_start,
	.stop = wd_stop,
#ifdef ENABLE_KEEPALIVEPING
	.ping = wd_ping,
#endif
	.status = wd_status,
	.set_timeout = wd_set_timeout,
};

static const struct watchdog_info wd_info = {
#ifdef ENABLE_KEEPALIVEPING
	.options = WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE | WDIOF_KEEPALIVEPING,
#else
	.options = WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE,
#endif
	.firmware_version = 0,
	.identity = KBUILD_MODNAME,
};

static struct watchdog_device g_wd = {
	.info = &wd_info,
	.ops = &wd_ops,
	.timeout = BBAPI_WATCHDOG_TIMEOUT_SEC,
	.max_timeout = BBAPI_WATCHDOG_MAX_TIMEOUT_SEC,
};

static int __init bbapi_watchdog_init_module(void)
{
	BUILD_BUG_ON(BIT_MASK(WDOG_KEEPALIVE) != WDIOF_KEEPALIVEPING);
#ifdef ENABLE_KEEPALIVEPING
	BUG_ON(bbapi_board_is("CBxx53\0\0\0\0\0\0\0\0\0"));
#endif
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
