// SPDX-License-Identifier: MIT
/**
    Beckhoff BIOS API driver to access hwmon sensors for Beckhoff IPCs
    Copyright (C) 2015 - 2019 Beckhoff Automation GmbH &Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
*/

#ifdef TEST_DEVICE
#define TEST_DEVICE 0xC6015

/** general configuration */
#define CONFIG_GENERAL_BOARDINFO {"CB3163\0", 5, 0, 118}
#define CONFIG_GENERAL_BOARDNAME "CBxx63\0\0\0\0\0\0\0\0\0"
#define CONFIG_GENERAL_VERSION {2, 20, 8}
#define CONFIG_USE_GPIO_EX 1

/** PWRCTRL configuration */
#define CONFIG_PWRCTRL_LAST_SHUTDOWN_ENABLED 0
#define CONFIG_PWRCTRL_BL_REVISION {1, 0, 31}
#define CONFIG_PWRCTRL_FW_REVISION {1, 2, 0}
#define CONFIG_PWRCTRL_DEVICE_ID 0x6
#define CONFIG_PWRCTRL_SERIAL "16272917280074"
#define CONFIG_PWRCTRL_PRODUCTION_DATE {30, 17}
#define CONFIG_PWRCTRL_TEST_COUNT 1
#define CONFIG_PWRCTRL_TEST_NUMBER "131630"

/** S-UPS configuration */
#define CONFIG_SUPS_DISABLED 1

/** CX Power Supply configuration */
#define CONFIG_CXPWRSUPP_DISABLED 1

/** LED configuration */
#define CONFIG_LED_TC_ENABLED 1
#define CONFIG_LED_USER_ENABLED 0

/** Watchdog configuration */
#define CONFIG_WATCHDOG_DISABLED 1

#else
#define TEST_DEVICE /* redefine to get a nice warning from gcc */
#error "TEST_DEVICE was already defined"
#endif /* #ifndef TEST_DEVICE */
