#!/bin/bash
# This example script shows how to use the 'sups_pwrfail' gpio, provided
# bbapi_power on platforms with S-UPS.
#
# Copyright (C) 2015 - 2016  Beckhoff Automation GmbH
# Author: Patrick Bruenn <p.bruenn@beckhoff.com>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

if [ $# -ne 1 ]; then
	echo -e "Usage:\n $0 <gpio_base>\n\nexample:\n $0 511\n"
	echo -e "If you don't know your <gpio_base>, take a look into your kernel log:"
	echo -e " dmesg | grep \"registered bbapi_power as\"\n\n"
    exit -1
fi

gpio_id=$1
gpio=/sys/class/gpio/sups_pwrfail/value

echo ${gpio_id} > /sys/class/gpio/export
if ! [ -r ${gpio} ]; then
	echo "Unable to read'${gpio}'"
	exit 1
fi

while [ $(<${gpio}) -eq 0 ]; do
	sleep 0.1
	printf "."
done
echo -e "\nPower loss detected -> send SIGPWR to 'init' for immediate shutdown"
#kill -s SIGPWR 1
