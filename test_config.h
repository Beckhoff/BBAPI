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

/** device definitions */
#define DEVICE_CX2030_CX2100_0004 0x21000004
#define DEVICE_CX2030_CX2100_0904 0x21000904
#define DEVICE_CX5000 0x5000

/** reused ranges */
#define BOOTCOUNTER_RANGE 1, 1500
#define OPERATION_TIME_RANGE  1, 200000
#define _24VOLT_RANGE 23600, 28000
#define CONFIG_CXPWRSUPP_TEMP_RANGE -35, 85

#if defined(__i386__)
#define CONFIG_GENERAL_PLATFORM 0x00
#elif defined(__x86_64__)
#define CONFIG_GENERAL_PLATFORM 0x01
#endif

#define FILE_PATH	"/dev/bbapi" 	// Path to character Device

/** select test device */
//#include "config_cx5000.h"
//#include "config_cx2030_cx2100-0004.h"
#include "config_cx2030_cx2100-0904.h"

#endif /* #ifndef _TEST_CONFIG_H_ */
