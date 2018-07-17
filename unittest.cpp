/**
    Beckhoff BIOS API driver to access hwmon sensors for Beckhoff IPCs
    Copyright (C) 2014-2015  Beckhoff Automation GmbH
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

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#ifndef __FreeBSD__
#include <linux/types.h>
#include <linux/watchdog.h>
#endif /* #ifndef __FreeBSD__ */

#include <chrono>
#include <iostream>
#include <thread>

#include "TcBaDevDef_gpl.h"

#include <fructose/fructose.h>

/**
 * Unittest configuration
 */
#include "test_config.h"

#ifndef TEST_DEVICE
#error "no test configuration is enabled in test_config.h"
#else

#define pr_info printf

using namespace fructose;

template<size_t N>
struct BiosString
{
	char data[N];
	BiosString(const char* text = NULL)
	{
		if(text) {
			strncpy(data, text, sizeof(data));
		} else {
			memset(data, 0, sizeof(data));
		}
	}

	bool operator==(const BiosString& ref) const {
		return 0 == memcmp(this, &ref, sizeof(*this));
	}

	int snprintf(char* buffer, size_t len) const {
		return ::snprintf(buffer, len, "%.*s", (int)sizeof(data), data);
	};
};

template<typename T>
struct BiosTriple
{
	T data[3];
	BiosTriple(T first = 0, T second = 0, T third = 0)
		: data{first, second, third}
	{
	}

	bool operator==(const BiosTriple& ref) const {
		return 0 == memcmp(this, &ref, sizeof(*this));
	}

	int snprintf(char* buffer, size_t len) const {
		return ::snprintf(buffer, len, "%d.%d-%d", data[0], data[1], data[2]);
	};
};
typedef BiosTriple<uint8_t> BiosVersion;

struct BiosPair
{
	uint8_t first;
	uint8_t second;
	BiosPair(uint8_t x = 0, uint8_t y = 0)
		: first(x), second(y)
	{
	}

	bool operator==(const BiosPair& ref) const {
		return 0 == memcmp(this, &ref, sizeof(*this));
	}

	int snprintf(char* buffer, size_t len) const {
		return ::snprintf(buffer, len, "%d-%d", first, second);
	};
};

int ioctl_read(int file, uint32_t group, uint32_t offset, void* out, uint32_t size)
{
	struct bbapi_struct data {group, offset, NULL, 0, out, size};
	if (-1 == ioctl(file, BBAPI_CMD, &data)) {
		pr_info("%s(): failed for group: 0x%x offset: 0x%x with errno: %s\n", __FUNCTION__, group, offset, strerror(errno));
		return -1;
	}
	return 0;
}

int ioctl_write(int file, uint32_t group, uint32_t offset, const void* in, uint32_t size)
{
	struct bbapi_struct data {group, offset, in, size, NULL, 0};
	if (-1 == ioctl(file, BBAPI_CMD, &data)) {
		pr_info("%s(): failed for group: 0x%x offset: 0x%x with errno: %s\n", __FUNCTION__, group, offset, strerror(errno));
		return -1;
	}
	return 0;
}

struct BiosApi
{
	BiosApi(unsigned long group = 0)
		: m_File(open(FILE_PATH, O_RDWR)),
		m_Group(group)
	{
		if (-1 == m_File) {
			pr_info("Open device file '%s' failed!\n", FILE_PATH);
			throw new std::runtime_error("Open device file failed!");
		}
	}

	~BiosApi()
	{
		close(m_File);
	};

	void setGroupOffset(unsigned long group)
	{
		m_Group = group;
	}

	int ioctl_read(uint32_t offset, void* out, unsigned long size) const
	{
		return ::ioctl_read(m_File, m_Group, offset, out, size);
	}

	int ioctl_write(uint32_t offset, const void* in, unsigned long size) const
	{
		return ::ioctl_write(m_File, m_Group, offset, in, size);
	}

protected:
	const int m_File;
	unsigned long m_Group;
};

struct TestBBAPI : fructose::test_base<TestBBAPI>
{
#define CHECK_VALUE(MSG, INDEX_OFFSET, EXPECTATION, TYPE) \
	test_range<TYPE>(#INDEX_OFFSET, INDEX_OFFSET, EXPECTATION, EXPECTATION, MSG)
#define CHECK_CLASS(MSG, INDEX_OFFSET, EXPECTATION, TYPE) \
	test_object<TYPE>(#INDEX_OFFSET, INDEX_OFFSET, EXPECTATION, MSG)
#define CHECK_RANGE(MSG, INDEX_OFFSET, RANGE, TYPE) \
	test_range<TYPE>(#INDEX_OFFSET, INDEX_OFFSET, RANGE, MSG)
#define READ_OBJECT(MSG, INDEX_OFFSET, EXPECTATION, TYPE) \
	test_object<TYPE>(#INDEX_OFFSET, INDEX_OFFSET, EXPECTATION, MSG, false)

	void test_CXPowerSupply(const std::string& test_name)
	{
#if CONFIG_CXPWRSUPP_DISABLED
		pr_info("\nCX power supply test case disabled\n");
		return;
#else
		bbapi.setGroupOffset(BIOSIGRP_CXPWRSUPP);
		pr_info("\nCX power supply test results:\n=============================\n");
		CHECK_VALUE("Type:                  %04d\n", BIOSIOFFS_CXPWRSUPP_GETTYPE, CONFIG_CXPWRSUPP_TYPE, uint32_t);
		CHECK_VALUE("Serial:                %04d\n", BIOSIOFFS_CXPWRSUPP_GETSERIALNO, CONFIG_CXPWRSUPP_SERIALNO, uint32_t);
		CHECK_CLASS("Fw ver.:                %s\n", BIOSIOFFS_CXPWRSUPP_GETFWVERSION, CONFIG_CXPWRSUPP_FWVERSION, BiosPair);
		CHECK_RANGE("Boot #:                %04d\n",     BIOSIOFFS_CXPWRSUPP_GETBOOTCOUNTER,   CONFIG_CXPWRSUPP_BOOTCOUNTER_RANGE, uint32_t);
		CHECK_RANGE("Optime:                %04d min.\n",BIOSIOFFS_CXPWRSUPP_GETOPERATIONTIME, CONFIG_CXPWRSUPP_OPERATIONTIME_RANGE, uint32_t);
		CHECK_RANGE("act. 5V:              %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GET5VOLT,         CONFIG_CXPWRSUPP_5VOLT_RANGE,   uint16_t);
		CHECK_RANGE("max. 5V:              %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GETMAX5VOLT,      CONFIG_CXPWRSUPP_5VOLT_RANGE,   uint16_t);
		CHECK_RANGE("act. 12V:             %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GET12VOLT,        CONFIG_CXPWRSUPP_12VOLT_RANGE,  uint16_t);
		CHECK_RANGE("max. 12V:             %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GETMAX12VOLT,     CONFIG_CXPWRSUPP_12VOLT_RANGE,  uint16_t);
		CHECK_RANGE("act. 24V:             %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GET24VOLT,        CONFIG_CXPWRSUPP_24VOLT_RANGE,  uint16_t);
		CHECK_RANGE("max. 24V:             %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GETMAX24VOLT,     CONFIG_CXPWRSUPP_24VOLT_RANGE,  uint16_t);
		CHECK_RANGE("act. temp.:           %5d C°\n",   BIOSIOFFS_CXPWRSUPP_GETTEMP,          CONFIG_CXPWRSUPP_TEMP_RANGE,    int8_t);
		CHECK_RANGE("min. temp.:           %5d C°\n",   BIOSIOFFS_CXPWRSUPP_GETMINTEMP,       CONFIG_CXPWRSUPP_TEMP_RANGE,    int8_t);
		CHECK_RANGE("max. temp.:           %5d C°\n",   BIOSIOFFS_CXPWRSUPP_GETMAXTEMP,       CONFIG_CXPWRSUPP_TEMP_RANGE,    int8_t);
		CHECK_RANGE("act. current:         %5d mA\n",   BIOSIOFFS_CXPWRSUPP_GETCURRENT,       CONFIG_CXPWRSUPP_CURRENT_RANGE, uint16_t);
		CHECK_RANGE("max. current:         %5d mA\n",   BIOSIOFFS_CXPWRSUPP_GETMAXCURRENT,    CONFIG_CXPWRSUPP_CURRENT_RANGE, uint16_t);
		CHECK_RANGE("act. power:           %5d mW\n",   BIOSIOFFS_CXPWRSUPP_GETPOWER,         CONFIG_CXPWRSUPP_POWER_RANGE,   uint32_t);
		CHECK_RANGE("max. power:           %5d mW\n",   BIOSIOFFS_CXPWRSUPP_GETMAXPOWER,      CONFIG_CXPWRSUPP_POWER_RANGE,   uint32_t);
		CHECK_VALUE("button state:          0x%02x\n",   BIOSIOFFS_CXPWRSUPP_GETBUTTONSTATE,   CONFIG_CXPWRSUPP_BUTTON_STATE, uint8_t);
#endif
	}

	void test_CXPowerSupply_display(const std::string& test_name)
	{
#if CONFIG_CXPWRSUPP_DISABLED
			pr_info("\nCX power supply write test case disabled\n");
			return;
#else
		static const char empty[16+1] = "                ";
		static const char line1[16+1] = "1234567890123456";
		static const char line2[16+1] = "6543210987654321";
		bbapi.setGroupOffset(BIOSIGRP_CXPWRSUPP);
		pr_info("\nCX power supply display test:\n=============================\n");
		uint8_t backlight = 0;
		fructose_assert(!bbapi.ioctl_write(BIOSIOFFS_CXPWRSUPP_ENABLEBACKLIGHT, &backlight, sizeof(backlight)));
		fructose_assert(!bbapi.ioctl_write(BIOSIOFFS_CXPWRSUPP_DISPLAYLINE1, &empty, sizeof(empty)));
		fructose_assert(!bbapi.ioctl_write(BIOSIOFFS_CXPWRSUPP_DISPLAYLINE2, &empty, sizeof(empty)));
		pr_info("Backlight should be OFF\n");
		std::this_thread::sleep_for(std::chrono::seconds(1));
		backlight = 0xff;
		fructose_assert(!bbapi.ioctl_write(BIOSIOFFS_CXPWRSUPP_ENABLEBACKLIGHT, &backlight, sizeof(backlight)));
		pr_info("Backlight should be ON\n");
		pr_info("Display should be empty\n");
		std::this_thread::sleep_for(std::chrono::seconds(1));
		fructose_assert(!bbapi.ioctl_write(BIOSIOFFS_CXPWRSUPP_DISPLAYLINE1, &line1, sizeof(line1)));
		fructose_assert(!bbapi.ioctl_write(BIOSIOFFS_CXPWRSUPP_DISPLAYLINE2, &line2, sizeof(line2)));
		pr_info("Display should show:\n%s\n%s\n\n", line1, line2);
#endif
	}

	void test_CXUPS(const std::string& test_name)
	{
		if(!CONFIG_CXUPS_ENABLED) {
			pr_info("\nCX UPS test case disabled\n");
			return;
		}
		bbapi.setGroupOffset(BIOSIGRP_CXUPS);
		pr_info("\nCX UPS test results:\n====================\n");
		CHECK_VALUE("UPS enabled:           0x%02x\n", BIOSIOFFS_CXUPS_GETENABLED, CONFIG_CXUPS_ENABLED, uint8_t);
		CHECK_CLASS("Fw ver.:                %s\n",    BIOSIOFFS_CXUPS_GETFIRMWAREVER, CONFIG_CXUPS_FIRMWAREVER, BiosPair);
		CHECK_VALUE("Power status:          0x%02x\n", BIOSIOFFS_CXUPS_GETPOWERSTATUS, CONFIG_CXUPS_POWERSTATUS, uint8_t);
		CHECK_VALUE("Battery status:        0x%02x\n", BIOSIOFFS_CXUPS_GETBATTERYSTATUS, CONFIG_CXUPS_BATTERYSTATUS, uint8_t);
		CHECK_VALUE("Battery capacity: %9d %\n",       BIOSIOFFS_CXUPS_GETBATTERYCAPACITY, CONFIG_CXUPS_BATTERYCAPACITY, uint8_t);
		CHECK_RANGE("Battery runtime:  %9d sec.\n",    BIOSIOFFS_CXUPS_GETBATTERYRUNTIME, CONFIG_CXUPS_BATTERYRUNTIME_RANGE, uint32_t);
		CHECK_RANGE("Boot:             %9d #\n",       BIOSIOFFS_CXUPS_GETBOOTCOUNTER, CONFIG_CXUPS_BOOTCOUNTER_RANGE, uint32_t);
		CHECK_RANGE("Optime:           %9d min.\n",    BIOSIOFFS_CXUPS_GETOPERATIONTIME, CONFIG_CXUPS_OPERATIONTIME_RANGE, uint32_t);
		CHECK_VALUE("Power fail:       %9d #\n",       BIOSIOFFS_CXUPS_GETPOWERFAILCOUNT, CONFIG_CXUPS_POWERFAILCOUNT, uint32_t);
		CHECK_VALUE("Battery critical:      0x%02x\n", BIOSIOFFS_CXUPS_GETBATTERYCRITICAL, CONFIG_CXUPS_BATTERYCRITICAL, uint8_t);
		CHECK_VALUE("Battery present:       0x%02x\n", BIOSIOFFS_CXUPS_GETBATTERYPRESENT, CONFIG_CXUPS_BATTERYPRESENT, uint8_t);
		CHECK_RANGE("act. output:      %9d mV\n",      BIOSIOFFS_CXUPS_GETOUTPUTVOLT, CONFIG_CXUPS_OUTPUTVOLT_RANGE, uint16_t);
		CHECK_RANGE("max. output:      %9d mV\n",      BIOSIOFFS_CXUPS_GETMAXOUTPUTVOLT, CONFIG_CXUPS_OUTPUTVOLT_RANGE, uint16_t);
		CHECK_RANGE("act. input:       %9d mV\n",      BIOSIOFFS_CXUPS_GETINPUTVOLT, CONFIG_CXUPS_INPUTVOLT_RANGE, uint16_t);
		CHECK_RANGE("max. input:       %9d mV\n",      BIOSIOFFS_CXUPS_GETMAXINPUTVOLT, CONFIG_CXUPS_INPUTVOLT_RANGE, uint16_t);
		CHECK_RANGE("act. temp.:       %9d C°\n",      BIOSIOFFS_CXUPS_GETTEMP, CONFIG_CXUPS_TEMP_RANGE,    int8_t);
		CHECK_RANGE("min. temp.:       %9d C°\n",      BIOSIOFFS_CXUPS_GETMINTEMP, CONFIG_CXUPS_TEMP_RANGE,    int8_t);
		CHECK_RANGE("max. temp.:       %9d C°\n",      BIOSIOFFS_CXUPS_GETMAXTEMP, CONFIG_CXUPS_TEMP_RANGE,    int8_t);
		CHECK_VALUE("act. charging:    %9d mA\n",      BIOSIOFFS_CXUPS_GETCHARGINGCURRENT, CONFIG_CXUPS_CURRENT, uint16_t);
		CHECK_RANGE("max. charging:    %9d mA\n",      BIOSIOFFS_CXUPS_GETMAXCHARGINGCURRENT, CONFIG_CXUPS_CURRENT_RANGE, uint16_t);
		CHECK_VALUE("act. charging:    %9d mW\n",      BIOSIOFFS_CXUPS_GETCHARGINGPOWER, CONFIG_CXUPS_POWER, uint32_t);
		CHECK_RANGE("max. charging:    %9d mW\n",      BIOSIOFFS_CXUPS_GETMAXCHARGINGPOWER, CONFIG_CXUPS_POWER_RANGE, uint32_t);
		CHECK_VALUE("act. discharging: %9d mA\n",      BIOSIOFFS_CXUPS_GETDISCHARGINGCURRENT, CONFIG_CXUPS_CURRENT, uint16_t);
		CHECK_RANGE("max. discharging: %9d mA\n",      BIOSIOFFS_CXUPS_GETMAXDISCHARGINGCURRENT, CONFIG_CXUPS_CURRENT_RANGE, uint16_t);
		CHECK_VALUE("act. discharging: %9d mW\n",      BIOSIOFFS_CXUPS_GETDISCHARGINGPOWER, CONFIG_CXUPS_POWER, uint32_t);
		CHECK_RANGE("max. discharging: %9d mW\n",      BIOSIOFFS_CXUPS_GETMAXDISCHARGINGPOWER, CONFIG_CXUPS_POWER_RANGE, uint32_t);
#if 0
//#define CONFIG_CXUPS_RESTARTMODE					0x0000000C	// OBSOLETE: Don't use it! Get restart mode, W:0, R:1 (BYTE) (0 := Disabled, 1 := Enabled)
//	#define BIOSIOFFS_CXUPS_SETRESTARTMODE					0x0000000D	// OBSOLETE: Don't use it! Set restart mode, W:1 (BYTE) (0 := Disable, 1 := Enable), R:0
	#define BIOSIOFFS_CXUPS_SETSHUTDOWNMODE				0x0000000E	// Set shutdown mode, W:1 (BYTE) (0 := Disable, 1:= Enable), R:0
#define CONFIG_CXUPS_BATTSERIALNUMBER				0x00000011	// Get serial number W:0, R:4
#define CONFIG_CXUPS_BATTHARDWAREVERSION			0x00000015	// Get hardware version W:0, R:2
#define CONFIG_CXUPS_BATTPRODUCTIONDATE				0x00000019	// Get production date W:0, R:4
#define CONFIG_CXUPS_LASTBATTCHANGEDATE		0x00000097	// Gets last battery change date , W:0, R:4
	#define BIOSIOFFS_CXUPS_SETLASTBATTCHANGEDATE		0x00000098	// Sets last battery change date W:4, R:0
#define CONFIG_CXUPS_BATTRATEDCAPACITY			0x00000099	// Get rated capacity W:0, R:4 [mAh]
#define CONFIG_CXUPS_SMBUSADDRESS				0x000000F0	// Get SMBus address W:0, R:2 (hHost, address)
#endif
	}

	void test_General(const std::string& test_name)
	{
		bbapi.setGroupOffset(BIOSIGRP_GENERAL);
		pr_info("\nGeneral test results:\n=====================\n");
		CHECK_CLASS("Mainboard: %s\n", BIOSIOFFS_GENERAL_GETBOARDINFO, CONFIG_GENERAL_BOARDINFO, BADEVICE_MBINFO);
		CHECK_CLASS("Board: %s\n", BIOSIOFFS_GENERAL_GETBOARDNAME, CONFIG_GENERAL_BOARDNAME, BiosString<16>);
		CHECK_VALUE("platform:     0x%02x (0x00->32 bit, 0x01-> 64bit)\n", BIOSIOFFS_GENERAL_GETPLATFORMINFO, CONFIG_GENERAL_PLATFORM, uint8_t);
		CHECK_CLASS("BIOS API %s\n", BIOSIOFFS_GENERAL_VERSION, CONFIG_GENERAL_VERSION, BADEVICE_VERSION);
	}

	void test_LED(const std::string& test_name, const std::string& led_name, uint32_t offset)
	{
		const size_t num_colors = 4;
		const char colors [num_colors][6] = {
			"off",
			"red",
			"blue",
			"green",
		};
		for (uint8_t color = num_colors - 1; color < num_colors; --color) {
			fructose_assert(!bbapi.ioctl_write(offset, &color, sizeof(color)));
			pr_info("%s should be %s, hit ENTER to continue", led_name.c_str(), colors[color]);
			std::cin.get();
		}
	}

	void test_manual_LEDs(const std::string& test_name)
	{
		bbapi.setGroupOffset(BIOSIGRP_LED);
		pr_info("\nLED test:\n=========\n");
		if (CONFIG_LED_TC_ENABLED) {
			test_LED(test_name, "TwinCAT LED", BIOSIOFFS_LED_SET_TC);
		} else {
			pr_info("TwinCAT LED test case disabled\n");
		}

		if (CONFIG_LED_USER_ENABLED) {
			test_LED(test_name, "User LED", BIOSIOFFS_LED_SET_USER);
		} else {
			pr_info("User LED test case disabled\n");
		}
	}

	void test_PwrCtrl(const std::string& test_name)
	{
		bbapi.setGroupOffset(BIOSIGRP_PWRCTRL);
		pr_info("\nPower control test results:\n===========================\n");
		CHECK_CLASS("Bl ver.:      %s\n", BIOSIOFFS_PWRCTRL_BOOTLDR_REV, CONFIG_PWRCTRL_BL_REVISION, BiosVersion);
		CHECK_CLASS("Fw ver.:      %s\n", BIOSIOFFS_PWRCTRL_FIRMWARE_REV, CONFIG_PWRCTRL_FW_REVISION, BiosVersion);
		CHECK_VALUE("Device id:    0x%02x\n", BIOSIOFFS_PWRCTRL_DEVICE_ID, CONFIG_PWRCTRL_DEVICE_ID, uint8_t);
		CHECK_RANGE("Optime:       %04d min.\n",BIOSIOFFS_PWRCTRL_OPERATING_TIME, CONFIG_PWRCTRL_OPERATION_TIME_RANGE, uint32_t);
		READ_OBJECT("Temp. [min-max]: %s °C\n", BIOSIOFFS_PWRCTRL_BOARD_TEMP, 0, BiosPair);
		READ_OBJECT("Volt. [min-max]: %s V\n", BIOSIOFFS_PWRCTRL_INPUT_VOLTAGE, 0, BiosPair);
		CHECK_CLASS("Serial:       %s\n", BIOSIOFFS_PWRCTRL_SERIAL_NUMBER, CONFIG_PWRCTRL_SERIAL, BiosString<17>);
		CHECK_RANGE("Boot #:       %04d\n", BIOSIOFFS_PWRCTRL_BOOT_COUNTER, CONFIG_PWRCTRL_BOOT_COUNTER_RANGE, uint16_t);
		CHECK_CLASS("Production date: %s\n", BIOSIOFFS_PWRCTRL_PRODUCTION_DATE, CONFIG_PWRCTRL_PRODUCTION_DATE, BiosPair);
		CHECK_VALUE("µC Position:  0x%02x\n", BIOSIOFFS_PWRCTRL_BOARD_POSITION, CONFIG_PWRCTRL_POSITION, uint8_t);
		if (CONFIG_PWRCTRL_LAST_SHUTDOWN_ENABLED) {
			CHECK_CLASS("Last shutdown reason: %s\n", BIOSIOFFS_PWRCTRL_SHUTDOWN_REASON, CONFIG_PWRCTRL_LAST_SHUTDOWN, BiosVersion);
		}
		CHECK_VALUE("Test count:   %03d\n", BIOSIOFFS_PWRCTRL_TEST_COUNTER, CONFIG_PWRCTRL_TEST_COUNT, uint8_t);
		CHECK_CLASS("Test number:  %s\n", BIOSIOFFS_PWRCTRL_TEST_NUMBER, CONFIG_PWRCTRL_TEST_NUMBER, BiosString<7>);
	}

	void test_SUPS(const std::string& test_name)
	{
#if CONFIG_SUPS_DISABLED
		pr_info("S-UPS test case disabled\n");
		return;
#else
		bbapi.setGroupOffset(BIOSIGRP_SUPS);
		pr_info("\nSUPS test results:\n==================\n");
		uint8_t enable = 0;
		fructose_assert(!bbapi.ioctl_write(BIOSIOFFS_SUPS_ENABLE, &enable, sizeof(enable)));
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		CHECK_VALUE("Status:    0x%02x\n", BIOSIOFFS_SUPS_STATUS, CONFIG_SUPS_STATUS_OFF, uint8_t);
		enable = 1;
		fructose_assert(!bbapi.ioctl_write(BIOSIOFFS_SUPS_ENABLE, &enable, sizeof(enable)));
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		CHECK_VALUE("Status:    0x%02x\n", BIOSIOFFS_SUPS_STATUS, CONFIG_SUPS_STATUS_100, uint8_t);

		CHECK_CLASS("Revision:               %s\n", BIOSIOFFS_SUPS_REVISION, CONFIG_SUPS_REVISION, BiosPair);
		CHECK_RANGE("Power fail:       %9d #\n",       BIOSIOFFS_SUPS_PWRFAIL_COUNTER, CONFIG_SUPS_POWERFAILCOUNT_RANGE, uint16_t);
		CHECK_CLASS("Power failed:           %s\n", BIOSIOFFS_SUPS_PWRFAIL_TIMES, CONFIG_SUPS_PWRFAIL_TIMES, BiosTriple<uint32_t>);

		uint8_t shutdownType[] {0x01, 0xA1, 0xFF};
		for(size_t i = 0; i < sizeof(shutdownType); ++i) {
			fructose_assert(!bbapi.ioctl_write(BIOSIOFFS_SUPS_SET_SHUTDOWN_TYPE, &shutdownType[i], sizeof(shutdownType[0])));
			CHECK_VALUE("Shutdown type:  0x%02x\n",       BIOSIOFFS_SUPS_GET_SHUTDOWN_TYPE, shutdownType[i], uint8_t);
		}

		CHECK_VALUE("S-UPS active:     %9d #\n",       BIOSIOFFS_SUPS_ACTIVE_COUNT, CONFIG_SUPS_ACTIVE_COUNT, uint8_t);
		CHECK_VALUE("S-UPS Power fail: %9d #\n",       BIOSIOFFS_SUPS_INTERNAL_PWRF_STATUS, CONFIG_SUPS_INTERNAL_PWRF_STATUS, uint8_t);
		CHECK_VALUE("Capacitor test:   %9d #\n",       BIOSIOFFS_SUPS_TEST_RESULT, CONFIG_SUPS_TEST_RESULT, uint8_t);
#ifdef CONFIG_SUPS_GPIO_PIN_EX
		CHECK_CLASS("GPIO:    %s\n", BIOSIOFFS_SUPS_GPIO_PIN_EX, CONFIG_SUPS_GPIO_PIN_EX, Bapi_GpioInfoEx);
#else
		CHECK_CLASS("GPIO:    %s\n", BIOSIOFFS_SUPS_GPIO_PIN, CONFIG_SUPS_GPIO_PIN, TSUps_GpioInfo);
#endif
#endif
	}

	void test_System(const std::string& test_name)
	{
		bbapi.setGroupOffset(BIOSIGRP_SYSTEM);
		uint32_t num_sensors;
		bbapi.ioctl_read(BIOSIOFFS_SYSTEM_COUNT_SENSORS, &num_sensors, sizeof(num_sensors));
		pr_info("\nSystem test results:\n====================\n");
		while (num_sensors > 0) {
			SENSORINFO info;
			pr_info("%02d:", num_sensors);
			CHECK_CLASS("%s\n", num_sensors, info, SENSORINFO);
			--num_sensors;
		}
	}

private:
	BiosApi bbapi;

	template<typename T>
	void test_object(const std::string& nIndexOffset, const unsigned long offset, const T expectedValue, const std::string& msg, const bool doCompare = true)
	{
		char text[256];
		T value {0};
		fructose_loop_assert(nIndexOffset, -1 != bbapi.ioctl_read(offset, &value, sizeof(value)));
		if (doCompare) {
			fructose_loop_assert(nIndexOffset, value == expectedValue);
		}
		value.snprintf(text, sizeof(text));
		pr_info(msg.c_str(), text);
	}

	template<typename T>
	void test_range(const std::string& nIndexOffset, const unsigned long offset, const T lower, const T upper, const std::string& msg)
	{
		T value {0};
		fructose_loop_assert(nIndexOffset, -1 != bbapi.ioctl_read(offset, &value, sizeof(value)));
		fructose_loop2_assert(nIndexOffset, lower, value, lower <= value);
		fructose_loop2_assert(nIndexOffset, upper, value, upper >= value);
		pr_info(msg.c_str(), value);
	}
};

void RunAtPos(size_t pos)
{
	const int fd = open("/dev/cx_display", O_WRONLY);
	if (-1 == fd) {
		perror(NULL);
		return;
	}

	/** move cursor to our position */
	for (size_t i = 0; i <= pos; ++i) {
		static const char tab = '\t';
		write(fd, &tab, 1);
	}

	/** print some characters */
	for (unsigned char c = '~'; c != ('/' + pos % 10); c--) {
		const unsigned char replace_char[2] = {'\b', c};
		write(fd, replace_char, sizeof(replace_char));
		/** sleep a little to let the other threads work, too */
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	close(fd);
}

struct TestDisplay : fructose::test_base<TestDisplay>
{
	void test_Simple(const std::string& test_name)
	{
#if CONFIG_CXPWRSUPP_DISABLED
		pr_info("CX2100 text display test disabled\n");
		return;
#else
		(std::cout << "Simple display test running...").flush();
		const int fd_1 = open("/dev/cx_display", O_WRONLY);
		const int fd_2 = open("/dev/cx_display", O_WRONLY);
		static const std::string line("\x11\f\bx\n\bxx\t\t\t\t\t\t\t\t\t\t\t\t\t\tx");
		static const std::string line_2("\x11\r\b\r\b\r#\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t#");
		static const std::string line_3("0123456789ABCDEFFEDCBA9876543210XXXXXX");
		static const std::string light_off("\x13");
		static const char *should   = "|x              x|\n|x              x|\n";
		static const char *should_2 = "|#              x|\n|x              #|\n";
		static const char *should_3 = "|0123456789ABCDEF|\n|FEDCBA9876543210|\n";
		static const char *bar_h =  "===================\n";
		fructose_assert_eq((ssize_t)line.size(), write(fd_1, line.c_str(), line.size()));
		pr_info("Display should look like this:\n%s%s%s\nhit ENTER to continue", bar_h, should, bar_h);
		std::cin.get();

		fructose_assert_eq((ssize_t)light_off.size(), write(fd_1, light_off.c_str(), light_off.size()));
		pr_info("Display backlight should be switched off\nhit ENTER to continue");
		std::cin.get();

		fructose_assert_eq((ssize_t)line_2.size(), write(fd_1, line_2.c_str(), line_2.size()));
		pr_info("Display should look like this:\n%s%s%s\nhit ENTER to continue", bar_h, should_2, bar_h);
		std::cin.get();

		fructose_assert_eq((ssize_t)line_3.size(), write(fd_2, line_3.c_str(), line_3.size()));
		pr_info("Display should look like this:\n%s%s%s\nhit ENTER to continue", bar_h, should_3, bar_h);
		std::cin.get();

		fructose_assert_ne(-1, fd_1);
		fructose_assert_ne(-1, fd_2);
		fructose_assert_eq(0, close(fd_1));
		fructose_assert_eq(0, close(fd_2));
		if (!error()) {
			std::cout << " done." << std::endl;
		}
#endif
	}

	void test_SMP(const std::string& test_name)
	{
#if CONFIG_CXPWRSUPP_DISABLED
		pr_info("CX2100 text display test disabled\n");
		return;
#else
		(std::cout << "Multithreaded display test running...").flush();
		std::vector<std::thread> threads(32);
		for (size_t i = 0; i < threads.size(); ++i) {
			threads[i] = std::thread(&RunAtPos, i);
		}

		for (auto& t: threads) {
			t.join();
		}
		std::cout << " done." << std::endl;
#endif
	}
};

struct TestWatchdog : fructose::test_base<TestWatchdog>
{
	static const int SHORT_TIMEOUT = 2;
	void test_Simple(const std::string& test_name)
	{
#if CONFIG_WATCHDOG_DISABLED
		pr_info("Simple watchdog test disabled\n");
		return;
#else
		(std::cout << "Simple watchdog test running...").flush();
		const int timeout_sec = SHORT_TIMEOUT;
		const int fd = open("/dev/watchdog", O_WRONLY);
		fructose_assert_ne(-1, fd);
		fructose_assert_eq(0, ioctl(fd, WDIOC_SETTIMEOUT, &timeout_sec));
		for (int i = 0; i < 2 * timeout_sec; ++i) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			fructose_assert_eq(1, write(fd, "V", 1));
		}
		fructose_assert_eq(0, close(fd));
		std::this_thread::sleep_for(std::chrono::seconds(timeout_sec));
		if (!error()) {
			std::cout << " done." << std::endl;
		}
#endif
	}

	void test_IOCTL(const std::string& test_name)
	{
		pr_info("\nWatchdog test:\n==============\n");
#if CONFIG_WATCHDOG_DISABLED
		pr_info("IOCTL watchdog test disabled\n");
		return;
#else
		(std::cout << "IOCTL watchdog test running...").flush();

		const int fd = open("/dev/watchdog", O_WRONLY);
		fructose_assert_ne(-1, fd);

		// durations over 255 sec. have to be mapped to minutes
		const int timeout_min = 256;
		int read = 0;
		fructose_assert_eq(0, ioctl(fd, WDIOC_SETTIMEOUT, &timeout_min));
		fructose_assert_eq(0, ioctl(fd, WDIOC_GETTIMEOUT, &read));
		fructose_assert_eq(4 * 60, read);

		// setting a too large timeout should fail, old value should remain
		const int timeout_overflow = BBAPI_WATCHDOG_MAX_TIMEOUT_SEC + 1;
		fructose_assert_eq(-1, ioctl(fd, WDIOC_SETTIMEOUT, &timeout_overflow));
		fructose_assert_eq(0, ioctl(fd, WDIOC_GETTIMEOUT, &read));
		fructose_assert_eq(4 * 60, read);

		// test maximum seconds
		const int timeout_sec = 255;
		fructose_assert_eq(0, ioctl(fd, WDIOC_SETTIMEOUT, &timeout_sec));
		fructose_assert_eq(0, ioctl(fd, WDIOC_GETTIMEOUT, &read));
		fructose_assert_eq(timeout_sec, read);

		// test maximum minutes
		const int timeout_max_minutes = BBAPI_WATCHDOG_MAX_TIMEOUT_SEC;
		fructose_assert_eq(0, ioctl(fd, WDIOC_SETTIMEOUT, &timeout_max_minutes));
		fructose_assert_eq(0, ioctl(fd, WDIOC_GETTIMEOUT, &read));
		fructose_assert_eq(timeout_max_minutes, read);

		const char identity[] = "bbapi_wdt\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
		const uint32_t options = CONFIG_WATCHDOG_OPTIONS;
		struct watchdog_info ident;
		fructose_assert_eq(0, ioctl(fd, WDIOC_GETSUPPORT, &ident));
		fructose_assert_eq(0, memcmp(identity, ident.identity, sizeof(ident.identity)));
		fructose_assert_eq(0U, ident.firmware_version);
		fructose_assert_eq(options, ident.options);

		// pretimeout is not supported by BBAPI watchdog
		const int pretimeout = 10;
		int pre;
		fructose_assert_eq(-1, ioctl(fd, WDIOC_SETPRETIMEOUT, &pretimeout));
		fructose_assert_eq(0, ioctl(fd, WDIOC_GETPRETIMEOUT, &pre));
		fructose_assert_eq(0, pre);

		// timeleft is not supported by BBAPI watchdog
		int timeleft;
		fructose_assert_eq(-1, ioctl(fd, WDIOC_GETTIMELEFT, &timeleft));
		fructose_assert_eq(1, write(fd, "V", 1));
		fructose_assert_eq(0, close(fd));
		std::cout << "\nBBAPI watchdog feature 'to early trigger' not supported\n";
#endif
	}

	void test_KeepAlive(const std::string& test_name)
	{
#if CONFIG_WATCHDOG_DISABLED
		pr_info("Watchdog keepalive test disabled\n");
		return;
#else
		(std::cout << "Watchdog keep alive ping test running...").flush();
		const int timeout = SHORT_TIMEOUT;
		const int fd = open("/dev/watchdog", O_WRONLY);
		fructose_assert_ne(-1, fd);
		fructose_assert_eq(0, ioctl(fd, WDIOC_SETTIMEOUT, &timeout));
		for (int i = 0; i < 2 * timeout; ++i) {
			int status = 0;
			std::this_thread::sleep_for(std::chrono::seconds(1));
			fructose_assert_eq(0, ioctl(fd, WDIOC_KEEPALIVE));
			fructose_assert_eq(0, ioctl(fd, WDIOC_GETSTATUS, &status));
			fructose_assert(status & WDIOF_KEEPALIVEPING);
			fructose_assert_eq(0, ioctl(fd, WDIOC_GETSTATUS, &status));
			fructose_assert_eq(0, status & WDIOF_KEEPALIVEPING);
		}
		fructose_assert_eq(1, write(fd, "V", 1));
		fructose_assert_eq(0, close(fd));
		if (!error()) {
			std::cout << " done." << std::endl;
		}
#endif
	}
};

struct TestOneTime : fructose::test_base<TestOneTime>
{

	void test_SUPS(const std::string& test_name)
	{
		fructose_assert(!bbapi.ioctl_write(BIOSIOFFS_SUPS_CAPACITY_TEST, NULL, 0));
	}

	void test_MagicClose(const std::string& test_name)
	{
#if CONFIG_WATCHDOG_DISABLED
		pr_info("Watchdog magic close test disabled\n");
		return;
#else
		(std::cout << "MagicClose watchdog test running...").flush();
		const int timeout = TestWatchdog::SHORT_TIMEOUT;
		const int fd = open("/dev/watchdog", O_WRONLY);
		fructose_assert_ne(-1, fd);
		fructose_assert_eq(0, ioctl(fd, WDIOC_SETTIMEOUT, &timeout));
		for (int i = 0; i < 2 * timeout; ++i) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			fructose_assert_eq(1, write(fd, "V", 1));
		}
		fructose_assert_eq(1, write(fd, "\0", 1));
		fructose_assert_eq(0, close(fd));
		(std::cout << "WARNING watchdog will fire soon...").flush();
		std::this_thread::sleep_for(std::chrono::seconds(timeout + 2));
		fructose_fail("You shouldn't see, if the watchdog would work as expected!");
#endif
	}
private:
	BiosApi bbapi;
};

int main(int argc, char *argv[])
{
	TestDisplay displayTest;
	displayTest.add_test("test_Simple", &TestDisplay::test_Simple);
	displayTest.add_test("test_SMP", &TestDisplay::test_SMP);
	displayTest.run(argc, argv);

	TestBBAPI bbapiTest;
	bbapiTest.add_test("test_General", &TestBBAPI::test_General);
	bbapiTest.add_test("test_PwrCtrl", &TestBBAPI::test_PwrCtrl);
	bbapiTest.add_test("test_SUPS", &TestBBAPI::test_SUPS);
	bbapiTest.add_test("test_System", &TestBBAPI::test_System);
	bbapiTest.add_test("test_CXPowerSupply", &TestBBAPI::test_CXPowerSupply);
	bbapiTest.add_test("test_CXUPS", &TestBBAPI::test_CXUPS);
	bbapiTest.add_test("test_CXPowerSupply_display", &TestBBAPI::test_CXPowerSupply_display);
	bbapiTest.add_test("test_manual_LEDs", &TestBBAPI::test_manual_LEDs);
	bbapiTest.run(argc, argv);

	TestWatchdog wdTest;
	wdTest.add_test("test_IOCTL", &TestWatchdog::test_IOCTL);
	wdTest.add_test("test_Simple", &TestWatchdog::test_Simple);
	wdTest.add_test("test_KeepAlive", &TestWatchdog::test_KeepAlive);
	wdTest.run(argc, argv);

	TestOneTime oneTimeTest;
	oneTimeTest.add_test("test_SUPS", &TestOneTime::test_SUPS);
	oneTimeTest.add_test("test_MagicClose", &TestOneTime::test_MagicClose);
//	oneTimeTest.run(argc, argv);
	return 0;
}
#endif /* #ifndef TEST_DEVICE */
