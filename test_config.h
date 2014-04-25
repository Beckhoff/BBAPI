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
#ifndef _TEST_CONFIG_H_
#define _TEST_CONFIG_H_

/** general configuration */
/* unittest configuration for my (Patrick) CX2030 */
#define CONFIG_GENERAL_BBAPI_BOARDINFO {"CX20x0\0", 1, 1, 52}
#define CONFIG_GENERAL_BBAPI_BOARDNAME "CX20x0\0\0\0\0\0\0\0\0\0"
#if defined(__i386__)
#define CONFIG_GENERAL_BBAPI_PLATFORM 0x00
#elif defined(__x86_64__)
#define CONFIG_GENERAL_BBAPI_PLATFORM 0x01
#endif
#define CONFIG_GENERAL_BBAPI_VERSION {2, 9, 1}

/** PWRCTRL configuration */
#define CONFIG_PWRCTRL_BL_REVISION {0, 14, 0}
#define CONFIG_PWRCTRL_FW_REVISION {0, 17, 36}
#define CONFIG_PWRCTRL_DEVICE_ID 0xC
#define CONFIG_PWRCTRL_OPERATION_TIME_RANGE 60000, 80000
#define CONFIG_PWRCTRL_MIN_TEMP_RANGE 10, 20
#define CONFIG_PWRCTRL_MAX_TEMP_RANGE 70, 112
#define CONFIG_PWRCTRL_MIN_VOLT_RANGE 49, 50
#define CONFIG_PWRCTRL_MAX_VOLT_RANGE 49, 50
#define CONFIG_PWRCTRL_SERIAL "0000000000000000"
#define CONFIG_PWRCTRL_BOOT_COUNTER_RANGE 300, 400
#define CONFIG_PWRCTRL_PRODUCTION_DATE {0, 0}
#define CONFIG_PWRCTRL_POSITION 0x00
#define CONFIG_PWRCTRL_LAST_SHUTDOWN {0, 0, 0}
#define CONFIG_PWRCTRL_TEST_COUNT 0
#define CONFIG_PWRCTRL_TEST_NUMBER "000000"

/** S-UPS configuration */
#define CONFIG_SUPS_DISABLED 1

/** CX Power Supply configuration */
#define CONFIG_CXPWRSUPP_DISABLED 1

#endif /* #ifndef _TEST_CONFIG_H_ */
