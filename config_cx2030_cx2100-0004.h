// SPDX-License-Identifier: MIT
/**
    Beckhoff BIOS API driver to access hwmon sensors for Beckhoff IPCs
    Copyright (C) 2014 - 2018 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
*/

#ifndef TEST_DEVICE
#define TEST_DEVICE DEVICE_CX2030_CX2100_0004

/** general configuration */
#define CONFIG_GENERAL_BOARDINFO {"CX20x0\0", 1, 1, 52}
#define CONFIG_GENERAL_BOARDNAME "CX20x0\0\0\0\0\0\0\0\0\0"
#define CONFIG_GENERAL_VERSION {2, 9, 1}

/** PWRCTRL configuration */
#define CONFIG_PWRCTRL_LAST_SHUTDOWN_ENABLED 1
#define CONFIG_PWRCTRL_BL_REVISION {0, 14, 0}
#define CONFIG_PWRCTRL_FW_REVISION {0, 17, 36}
#define CONFIG_PWRCTRL_DEVICE_ID 0xC
#define CONFIG_PWRCTRL_SERIAL "0000000000000000"
#define CONFIG_PWRCTRL_PRODUCTION_DATE {0, 0}
#define CONFIG_PWRCTRL_TEST_COUNT 0
#define CONFIG_PWRCTRL_TEST_NUMBER "000000"

/** S-UPS configuration */
#define CONFIG_SUPS_DISABLED 1

/** CX Power Supply configuration */
#define CONFIG_CXPWRSUPP_TYPE 4
#define CONFIG_CXPWRSUPP_SERIALNO 1467

/** LED configuration */
#define CONFIG_LED_TC_ENABLED 0
#define CONFIG_LED_USER_ENABLED 0

#else
#define TEST_DEVICE /* redefine to get a nice warning from gcc */
#error "TEST_DEVICE was already defined"
#endif /* #ifndef TEST_DEVICE */
