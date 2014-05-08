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
#include "watchdog.h"

static int __init bbapi_watchdog_init_module(void)
{
	int result = -ENODEV;
	pr_info("%s, %s\n", DRV_DESCRIPTION, DRV_VERSION);
	result = bbapi_call_kern(NULL, NULL);
	return result;
}

static void __exit bbapi_watchdog_exit(void)
{
	pr_info("Watchdog unregistered\n");
}

module_init(bbapi_watchdog_init_module);
module_exit(bbapi_watchdog_exit);

MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR("Patrick Bruenn <p.bruenn@beckhoff.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
