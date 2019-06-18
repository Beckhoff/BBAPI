// SPDX-License-Identifier: MIT
/**
    Beckhoff BIOS API driver to access hwmon sensors for Beckhoff IPCs
    Copyright (C) 2018 - 2019 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
*/

#ifndef TEST_DEVICE
#define TEST_DEVICE 0x5000

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
#define CONFIG_SUPS_STATUS_OFF 0xAF

/** CX Power Supply configuration */
#define CONFIG_CXPWRSUPP_DISABLED 1

/** LED configuration */
#define CONFIG_LED_TC_ENABLED 1
#define CONFIG_LED_USER_ENABLED 0

#else
#define TEST_DEVICE /* redefine to get a nice warning from gcc */
#error "TEST_DEVICE was already defined"
#endif /* #ifndef TEST_DEVICE */
