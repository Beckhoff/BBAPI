/**
    Beckhoff BIOS API driver to access hwmon sensors for Beckhoff IPCs
    Copyright (C) 2015  Beckhoff Automation GmbH
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>

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

#ifndef TEST_DEVICE
#define TEST_DEVICE DEVICE_CX5130

/** general configuration */
#define CONFIG_GENERAL_BOARDINFO {"CX51x0\0", 1, 0, 49}
#define CONFIG_GENERAL_BOARDNAME "CBxx63\0\0\0\0\0\0\0\0\0"
#define CONFIG_GENERAL_VERSION {2, 13, 1}
#define CONFIG_USE_GPIO_EX 1

/** PWRCTRL configuration */
#define CONFIG_PWRCTRL_LAST_SHUTDOWN_ENABLED 0
#define CONFIG_PWRCTRL_BL_REVISION {1, 0, 26}
#define CONFIG_PWRCTRL_FW_REVISION {1, 0, 87}
#define CONFIG_PWRCTRL_DEVICE_ID 0x6
#define CONFIG_PWRCTRL_SERIAL "12003415020116"
#define CONFIG_PWRCTRL_PRODUCTION_DATE {7, 15}
#define CONFIG_PWRCTRL_TEST_COUNT 1
#define CONFIG_PWRCTRL_TEST_NUMBER "151020"

/** S-UPS configuration */
#define CONFIG_SUPS_PWRFAIL_TIMES {33060, 9437, 33060}
#define CONFIG_SUPS_GPIO_PIN_EX {0, 0x4, 0x5, 0x588, 0x00400000}

/** CX Power Supply configuration */
#define CONFIG_CXPWRSUPP_DISABLED 1

/** LED configuration */
#define CONFIG_LED_TC_ENABLED 0
#define CONFIG_LED_USER_ENABLED 0

/** Watchdog configuration */
#define CONFIG_WATCHDOG_DISABLED 1

#else
#define TEST_DEVICE /* redefine to get a nice warning from gcc */
#error "TEST_DEVICE was already defined"
#endif /* #ifndef TEST_DEVICE */
