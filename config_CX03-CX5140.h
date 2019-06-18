// SPDX-License-Identifier: MIT
/**
    Beckhoff BIOS API driver to access hwmon sensors for Beckhoff IPCs
    Copyright (C) 2015 - 2018 Beckhoff Automation GmbH &Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
*/

#ifndef TEST_DEVICE
#define TEST_DEVICE 0x5140

/** general configuration */
#define CONFIG_GENERAL_BOARDINFO {"CX51x0\0", 2, 0, 120}
#define CONFIG_GENERAL_BOARDNAME "CBxx63\0\0\0\0\0\0\0\0\0"
#define CONFIG_GENERAL_VERSION {2, 20, 8}
#define CONFIG_USE_GPIO_EX 1

/** PWRCTRL configuration */
#define CONFIG_PWRCTRL_LAST_SHUTDOWN_ENABLED 0
#define CONFIG_PWRCTRL_BL_REVISION {1, 0, 29}
#define CONFIG_PWRCTRL_FW_REVISION {1, 2, 0}
#define CONFIG_PWRCTRL_DEVICE_ID 0x6
#define CONFIG_PWRCTRL_SERIAL "12003416390648"
#define CONFIG_PWRCTRL_PRODUCTION_DATE {50, 16}
#define CONFIG_PWRCTRL_TEST_COUNT 1
#define CONFIG_PWRCTRL_TEST_NUMBER "151020"

/** S-UPS configuration */
#define CONFIG_SUPS_STATUS_OFF 0xAA
#define CONFIG_SUPS_PWRFAIL_TIMES {26883, 19117, 19118}
#define CONFIG_SUPS_GPIO_PIN_EX {0, 0x4, 0x5, 0x588, 0x00400000}

/** CX Power Supply configuration */
#define CONFIG_CXPWRSUPP_DISABLED 1

/** LED configuration */
#define CONFIG_LED_TC_ENABLED 0
#define CONFIG_LED_USER_ENABLED 0

/** Watchdog configuration */
#define CONFIG_WATCHDOG_DISABLED 0

#else
#define TEST_DEVICE /* redefine to get a nice warning from gcc */
#error "TEST_DEVICE was already defined"
#endif /* #ifndef TEST_DEVICE */
