/**
    Beckhoff BIOS API driver to access hwmon sensors for Beckhoff IPCs
    Copyright (C) 2014  Beckhoff Automation GmbH
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
#define TEST_DEVICE DEVICE_CX5000

/** general configuration */
#define CONFIG_GENERAL_BOARDINFO {"CX553\0", 1, 0, 128}
#define CONFIG_GENERAL_BOARDNAME "CBxx53\0\0\0\0\0\0\0\0\0"
#define CONFIG_GENERAL_VERSION {1, 21, 1}

/** PWRCTRL configuration */
#define CONFIG_PWRCTRL_LAST_SHUTDOWN_ENABLED 0
#define CONFIG_PWRCTRL_BL_REVISION {0, 14, 0}
#define CONFIG_PWRCTRL_FW_REVISION {0, 17, 50}
#define CONFIG_PWRCTRL_DEVICE_ID 0xD
#define CONFIG_PWRCTRL_SERIAL "0000000000000000"
#define CONFIG_PWRCTRL_PRODUCTION_DATE {0, 0}
#define CONFIG_PWRCTRL_TEST_COUNT 0
#define CONFIG_PWRCTRL_TEST_NUMBER "000000"

/** S-UPS configuration */
#define CONFIG_SUPS_PWRFAIL_TIMES {64513, 49902, 49968}

/** CX Power Supply configuration */
#define CONFIG_CXPWRSUPP_DISABLED 1

/** LED configuration */
#define CONFIG_LED_TC_ENABLED 1
#define CONFIG_LED_USER_ENABLED 0

#else
#define TEST_DEVICE /* redefine to get a nice warning from gcc */
#error "TEST_DEVICE was already defined"
#endif /* #ifndef TEST_DEVICE */
