// SPDX-License-Identifier: MIT
/**
    Watchdog driver using the Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2014 - 2018 Beckhoff Automation GmbH & Co. KG
*/

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/watchdog.h>

#include "../api.h"
#include "../TcBaDevDef.h"

#define DRV_VERSION      "0.6"
#define DRV_DESCRIPTION  "Beckhoff BIOS API watchdog driver"

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define WDOG_KEEPALIVE 15	/* == BIT_NUMBER(WDIOF_KEEPALIVEPING) */
#define BBAPI_WATCHDOG_TIMEOUT_SEC 60	/* default timeout for watchdog */

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout,
		 "Watchdog cannot be stopped once started (default="
		 __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static int bbapi_wd_write(uint32_t offset, void *in, uint32_t size)
{
	return -bbapi_write(BIOSIGRP_WATCHDOG, offset, in, size);
}

static int wd_start(struct watchdog_device *const wd)
{
	static const uint32_t offset_enable =
	    BIOSIOFFS_WATCHDOG_ACTIVATE_PWRCTRL;
	static const uint32_t offset_timeout =
	    BIOSIOFFS_WATCHDOG_ENABLE_TRIGGER;
	static const uint32_t offset_timebase = BIOSIOFFS_WATCHDOG_CONFIG;
	uint8_t enable = 1;
	uint8_t timebase = wd->timeout > 255;
	uint8_t timeout = timebase ? wd->timeout / 60 : wd->timeout;

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

static int wd_ping(struct watchdog_device *wd)
{
	int result;

	set_bit(WDOG_KEEPALIVE, &wd->status);

	result = bbapi_wd_write(BIOSIOFFS_WATCHDOG_IORETRIGGER, NULL, 0);
	if (BIOSAPI_SRVNOTSUPP == result) {
		pr_info_once("this platform doesn't support io retrigger\n");
		return wd_start(wd);
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
	uint8_t disable = 0;

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
	.ping = wd_ping,
	.status = wd_status,
	.set_timeout = wd_set_timeout,
};

static const struct watchdog_info wd_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE | WDIOF_KEEPALIVEPING,
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
	pr_info("%s, %s (nowayout=%d)\n", DRV_DESCRIPTION, DRV_VERSION,
		nowayout);
	watchdog_set_nowayout(&g_wd, nowayout);
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
MODULE_LICENSE("GPL and additional rights");
MODULE_VERSION(DRV_VERSION);
