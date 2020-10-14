#!/bin/sh -l

set -e

insmod ./bbapi.ko
insmod ./button/bbapi_button.ko
insmod ./display/bbapi_display.ko
insmod ./power/bbapi_power.ko
insmod ./sups/bbapi_sups.ko
insmod ./wdt/bbapi_wdt.ko

dmesg | grep bbapi
