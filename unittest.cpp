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

#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>

#include "bbapi.h"
#include "TcBaDevDef_gpl.h"

#pragma GCC diagnostic ignored "-Wsign-compare"
#include <fructose/fructose.h>
#pragma GCC diagnostic warning "-Wsign-compare"


/**
 * Unittest configuration
 */
#include "test_config.h"
static const BADEVICE_MBINFO EXPECTED_GENERAL_BOARDINFO = CONFIG_EXPECTED_GENERAL_BBAPI_BOARDINFO;
static const uint8_t EXPECTED_GENERAL_BOARDNAME[16] = CONFIG_EXPECTED_GENERAL_BBAPI_BOARDNAME;
static const uint8_t EXPECTED_GENERAL_PLATFORM = CONFIG_EXPECTED_GENERAL_BBAPI_PLATFORM;
static const BADEVICE_VERSION EXPECTED_GENERAL_VERSION = CONFIG_EXPECTED_GENERAL_BBAPI_VERSION;

static const uint8_t EXPECTED_PWRCTRL_BL_REVISION[3] = CONFIG_EXPECTED_PWRCTRL_BL_REVISION;
static const uint8_t EXPECTED_PWRCTRL_FW_REVISION[3] = CONFIG_EXPECTED_PWRCTRL_FW_REVISION;
static const uint8_t EXPECTED_PWRCTRL_DEVICE_ID = CONFIG_EXPECTED_PWRCTRL_DEVICE_ID;
static const char EXPECTED_PWRCTRL_SERIAL[16+1] = CONFIG_EXPECTED_PWRCTRL_SERIAL;
static const uint8_t EXPECTED_PWRCTRL_PRODUCTION_DATE[2] = CONFIG_EXPECTED_PWRCTRL_PRODUCTION_DATE;
static const uint8_t EXPECTED_PWRCTRL_POSITION = CONFIG_EXPECTED_PWRCTRL_POSITION;
static const uint8_t EXPECTED_PWRCTRL_LAST_SHUTDOWN[3] = CONFIG_EXPECTED_PWRCTRL_LAST_SHUTDOWN;
static const uint8_t EXPECTED_PWRCTRL_TEST_COUNT = CONFIG_EXPECTED_PWRCTRL_TEST_COUNT;
static const char EXPECTED_PWRCTRL_TEST_NUMBER[6+1] = CONFIG_EXPECTED_PWRCTRL_TEST_NUMBER;


#define FILE_PATH	"/dev/BBAPI" 	// Path to character Device

#define DEBUG 1
#if DEBUG
#define pr_info printf
#else
#define pr_info(...)
#endif

#define STRING_SIZE 17				// Size of String for Display
#define BBAPI_CMD 0x5000			// BIOS API Command number for IOCTL call

#define CXPWRSUPP_BUTTON_STATE_OFF			0x00
#define CXPWRSUPP_BUTTON_STATE_RIGHT		0x01
#define CXPWRSUPP_BUTTON_STATE_LEFT			0x02
#define CXPWRSUPP_BUTTON_STATE_DOWN			0x04
#define CXPWRSUPP_BUTTON_STATE_UP			0x08
#define CXPWRSUPP_BUTTON_STATE_SELECT		0x10


int ioctl_read(int file, uint32_t group, uint32_t offset, void* out, uint32_t size)
{
	struct bbapi_struct data {group, offset, NULL, 0, out, size};
	if (-1 == ioctl(file, BBAPI_CMD, &data)) {
		pr_info("%s(): failed for group: 0x%x offset: 0x%x\n", __FUNCTION__, group, offset);
		return -1;
	}
	return 0;
}

int ioctl_write(int file, uint32_t group, uint32_t offset, void* in, uint32_t size)
{
	struct bbapi_struct data {group, offset, in, size, NULL, 0};
	if (-1 == ioctl(file, BBAPI_CMD, &data)) {
		pr_info("%s(): failed for group: 0x%x offset: 0x%x\n", __FUNCTION__, group, offset);
		return -1;
	}
	return 0;
}

template<typename T>
T bbapi_read(int file, unsigned long group, unsigned long offset)
{
	T value = 0;
	ioctl_read(file, group, offset, &value, sizeof(value));
	return value;
}

using namespace fructose;

struct BiosApi
{
	BiosApi(unsigned long group = 0)
		: m_File(open(FILE_PATH, O_RDWR)),
		m_Group(group)
	{
		if (-1 == m_File)
			throw new std::runtime_error("Open device file failed!");
	}

	~BiosApi()
	{
		close(m_File);
	};

	void setGroupOffset(unsigned long group)
	{
		m_Group = group;
	}

	template<typename T>
	T read(unsigned long offset) const
	{
		return bbapi_read<T>(m_File, m_Group, offset);
	}

	int ioctl_read(unsigned long offset, void* out, unsigned long size) const
	{
		return ::ioctl_read(m_File, m_Group, offset, out, size);
	}

protected:
	const int m_File;
	unsigned long m_Group;
};

struct TestSensors : public BiosApi, fructose::test_base<TestSensors>
{
	TestSensors() : BiosApi(BIOSIGRP_SYSTEM) {};

	void test_sensors(const std::string& test_name)
	{
		uint32_t num_sensors = read<uint32_t>(BIOSIOFFS_SYSTEM_COUNT_SENSORS);
		while (num_sensors > 0) {
			show_sensor(num_sensors);
			--num_sensors;
		}
	}

private:
	void show_sensor(uint32_t sensor)
	{
		SENSORINFO info;
		memset(&info, 0, sizeof(info));
		fructose_assert_ne(-1, ioctl_read(sensor, &info, sizeof(info)));
		fructose_loop_assert(sensor, INFOVALUE_STATUS_UNUSED != info.readVal.status);

		pr_info("%02u: %12s %s %s val:%u min:%u max:%u nom:%u)\n",
			sensor, info.desc,
			LOCATIONCAPS[info.eType].name, PROBECAPS[info.eType].name,
			info.readVal.value, info.minVal.value, info.maxVal.value, info.nomVal.value);
	}
};

#define TESTING_ENABLED 0
void print_mem(const unsigned char *p, size_t lines)
{
#if TESTING_ENABLED
	pr_info("mem at: %p\n", p);
	pr_info(" 0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F\n");
	while (lines > 0) {
		pr_info
		    ("%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
		     p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9],
		     p[10], p[11], p[12], p[13], p[14], p[15]);
		p += 16;
		--lines;
	}
#endif /* #if TESTING_ENABLED */
}

struct TestBBAPI : fructose::test_base<TestBBAPI>
{
#define CHECK(INDEX_OFFSET, DATATYPE) \
	test_generic<sizeof(DATATYPE)>(bbapi, #INDEX_OFFSET, INDEX_OFFSET, NULL)
#define CHECK_VALUE(INDEX_OFFSET, EXPECTATION) \
	test_generic<sizeof(EXPECTATION)>(bbapi, #INDEX_OFFSET, INDEX_OFFSET, &(EXPECTATION))
#define CHECK_RANGE(INDEX_OFFSET, RANGE, TYPE) \
	test_range<TYPE>(bbapi, #INDEX_OFFSET, INDEX_OFFSET, RANGE)

	void test_General(const std::string& test_name)
	{
		bbapi.setGroupOffset(BIOSIGRP_GENERAL);
		BADEVICE_MBINFO info;
		bbapi.ioctl_read(BIOSIOFFS_GENERAL_GETBOARDINFO, &info, sizeof(info));
		fructose_assert_same_data(&EXPECTED_GENERAL_BOARDINFO, &info, sizeof(info));
		pr_info("%s hw: %d v%d.%d\n", info.MBName, info.MBRevision, info.biosMajVersion, info.biosMinVersion);
		char boardname[16] = {0};
		bbapi.ioctl_read(BIOSIOFFS_GENERAL_GETBOARDNAME, &boardname, sizeof(boardname));
		fructose_assert_same_data(&EXPECTED_GENERAL_BOARDNAME, &boardname, sizeof(boardname));
		pr_info("Board: %s\n", boardname);
		uint8_t platform = bbapi.read<uint8_t>(BIOSIOFFS_GENERAL_GETPLATFORMINFO);
		fructose_assert_eq(EXPECTED_GENERAL_PLATFORM, platform);
		pr_info("platform is %s bit\n", platform ? "64" : "32");
		BADEVICE_VERSION version;
		bbapi.ioctl_read(BIOSIOFFS_GENERAL_VERSION, &version, sizeof(version));
		fructose_assert_same_data(&EXPECTED_GENERAL_VERSION, &version, sizeof(version));
		pr_info("BIOS API ver.: %d rev.: %d build: %d\n", version.version, version.revision, version.build);
	}

	void test_PwrCtrl(const std::string& test_name)
	{
		bbapi.setGroupOffset(BIOSIGRP_PWRCTRL);
		CHECK_VALUE(BIOSIOFFS_PWRCTRL_BOOTLDR_REV, EXPECTED_PWRCTRL_BL_REVISION);
		CHECK_VALUE(BIOSIOFFS_PWRCTRL_FIRMWARE_REV, EXPECTED_PWRCTRL_FW_REVISION);
		CHECK_VALUE(BIOSIOFFS_PWRCTRL_DEVICE_ID, EXPECTED_PWRCTRL_DEVICE_ID);
		CHECK_RANGE(BIOSIOFFS_PWRCTRL_OPERATING_TIME, CONFIG_EXPECTED_PWRCTRL_OPERATION_TIME_RANGE, uint32_t);
		CHECK(BIOSIOFFS_PWRCTRL_BOARD_TEMP, uint8_t[2]);
		CHECK(BIOSIOFFS_PWRCTRL_INPUT_VOLTAGE, uint8_t[2]);
		CHECK_VALUE(BIOSIOFFS_PWRCTRL_SERIAL_NUMBER, EXPECTED_PWRCTRL_SERIAL);
		CHECK_RANGE(BIOSIOFFS_PWRCTRL_BOOT_COUNTER, CONFIG_EXPECTED_PWRCTRL_BOOT_COUNTER_RANGE, uint16_t);
		CHECK_VALUE(BIOSIOFFS_PWRCTRL_PRODUCTION_DATE, EXPECTED_PWRCTRL_PRODUCTION_DATE);
		CHECK_VALUE(BIOSIOFFS_PWRCTRL_BOARD_POSITION, EXPECTED_PWRCTRL_POSITION);
		CHECK_VALUE(BIOSIOFFS_PWRCTRL_SHUTDOWN_REASON, EXPECTED_PWRCTRL_LAST_SHUTDOWN);
		CHECK_VALUE(BIOSIOFFS_PWRCTRL_TEST_COUNTER, EXPECTED_PWRCTRL_TEST_COUNT);
		CHECK_VALUE(BIOSIOFFS_PWRCTRL_TEST_NUMBER, EXPECTED_PWRCTRL_TEST_NUMBER);
	}

private:
	BiosApi bbapi;
	template<size_t N>
	void test_generic(const BiosApi& bbapi, const std::string& nIndexOffset, const unsigned long offset, const void *const expectedValue)
	{
		uint8_t value[N];
		memset(value, 0, sizeof(value));
		fructose_loop_assert(nIndexOffset, -1 != bbapi.ioctl_read(offset, &value, sizeof(value)));
		if(expectedValue) {
			fructose_loop_assert(nIndexOffset, 0 == memcmp(expectedValue, &value, sizeof(value)));
			print_mem(value, 1);
		}
	}

	template<typename T>
	void test_range(const BiosApi& bbapi, const std::string& nIndexOffset, const unsigned long offset, const T lower, const T upper)
	{
		T value = 0;
		fructose_loop_assert(nIndexOffset, -1 != bbapi.ioctl_read(offset, &value, sizeof(value)));
		fructose_loop2_assert(nIndexOffset, lower, value, lower <= value);
		fructose_loop2_assert(nIndexOffset, upper, value, upper >= value);
	}
};

void cx_pwrctrl_show(int file)
{
	char serial[16];
	uint8_t bl_rev[3];
	uint8_t fw_rev[3];
	uint8_t shutdown[3];
	uint8_t temperature[2];
	uint8_t voltage[2];
	uint8_t manufactured[2];
	char test[6];
	memset(test, 0, sizeof(test));
	memset(serial, 0, sizeof(serial));
	uint8_t location = bbapi_read<uint8_t>(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_BOARD_POSITION);
	ioctl_read(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_BOOTLDR_REV, &bl_rev, sizeof(bl_rev));
	ioctl_read(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_FIRMWARE_REV, &fw_rev, sizeof(fw_rev));
	ioctl_read(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_BOARD_TEMP, &temperature, sizeof(temperature));
	ioctl_read(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_INPUT_VOLTAGE, &voltage, sizeof(voltage));
	ioctl_read(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_SERIAL_NUMBER, &serial, sizeof(serial));
	ioctl_read(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_PRODUCTION_DATE, &manufactured, sizeof(manufactured));
	ioctl_read(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_SHUTDOWN_REASON, &shutdown, sizeof(shutdown));
	ioctl_read(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_TEST_NUMBER, &test, sizeof(test));
	pr_info("Bootloader:      %u.%u-%u\n", bl_rev[0], bl_rev[1], bl_rev[2]);
	pr_info("Firmware:        %u.%u-%u\n", fw_rev[0], fw_rev[1], fw_rev[2]);
	pr_info("Device ID:       0x%02x\n", bbapi_read<uint8_t>(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_DEVICE_ID));
	pr_info("Operating time:  %u minutes since production\n", bbapi_read<uint32_t>(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_OPERATING_TIME));
	pr_info("Temperature [min, max]: [%d°C, %d°C]\n", temperature[0], temperature[1]);
	pr_info("Voltage [min, max]: [%dmV, %dmV]\n", voltage[0]*100, voltage[1]*100);
	pr_info("Serial: %s\n", serial);
	pr_info("Boot counter:     %u\n", bbapi_read<uint16_t>(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_BOOT_COUNTER));
	pr_info("Manufactured in week %u of 20%02u\n", manufactured[0], manufactured[1]);
	pr_info("PwCtrl is running in %s\n", (location == 0xff) ? "bootloader" : (location == 0) ? "firmware" : "unkown");
	pr_info("Last shutdown: 0x%02x%02x%02x\n", shutdown[0], shutdown[1], shutdown[2]);
	pr_info("Test count:    %u\n", bbapi_read<uint8_t>(file, BIOSIGRP_PWRCTRL, BIOSIOFFS_PWRCTRL_TEST_COUNTER));
	pr_info("Test number:   %s\n", test);
}

void cx_sups_show(int file)
{
	uint16_t revision = bbapi_read<uint16_t>(file, BIOSIGRP_SUPS, BIOSIOFFS_SUPS_REVISION);
	pr_info("S-UPS status:    0x%02x\n", bbapi_read<uint8_t>(file, BIOSIGRP_SUPS, BIOSIOFFS_SUPS_STATUS));
	pr_info("S-UPS revision:  %d.%d\n", revision >> 8, 0xff & revision);
	pr_info("# Power fails:  %d\n", bbapi_read<uint16_t>(file, BIOSIGRP_SUPS, BIOSIOFFS_SUPS_PWRFAIL_COUNTER));

#if 0
	#define BIOSIOFFS_SUPS_ENABLE								0x00000000	// Enable/disable SUPS, W:1, R:0
	#define BIOSIOFFS_SUPS_PWRFAIL_TIMES					0x00000004	// Get latest power fail time stamps, W:0, R:12
	#define BIOSIOFFS_SUPS_SET_SHUTDOWN_TYPE				0x00000005	// Set the Shutdown behavior, W:1, R:0
	#define BIOSIOFFS_SUPS_GET_SHUTDOWN_TYPE				0x00000006	// Get the Shutdown behavior and reset, W:0, R:1
	#define BIOSIOFFS_SUPS_ACTIVE_COUNT						0x00000007	// Get the SUSV Active Count and reset, W:0, R:1
	#define BIOSIOFFS_SUPS_INTERNAL_PWRF_STATUS			0x00000008	// Get the number of Pwr-Fail in the SUSV, W:0, R:1
	#define BIOSIOFFS_SUPS_CAPACITY_TEST					0x00000009	// Capacitator test, W:0, R:0
	#define BIOSIOFFS_SUPS_TEST_RESULT						0x0000000a	// Get SUPS test result, W:0, R:1
	#define BIOSIOFFS_SUPS_GPIO_PIN							0x000000a0	// Get the Address and the GPIO-Pin from PWR-Fail PIN, W:0, R:4
#endif
}

void cx_power_supply_show(int file)
{
	uint16_t fw_version;
	pr_info("Type:         %04d\n", bbapi_read<uint32_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETTYPE));
	pr_info("Serial:       %04d\n", bbapi_read<uint32_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETSERIALNO));
	fw_version = bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETFWVERSION);
	pr_info("Fw ver.:      %02d.%02d\n", fw_version >> 8, fw_version & 0xff);
	pr_info("Boot #:       %04d\n", bbapi_read<uint32_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETBOOTCOUNTER));
	pr_info("Optime:       %04d minutes since production\n", bbapi_read<uint32_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETOPERATIONTIME));
	pr_info("act. 5V:      %5d mV\n", bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GET5VOLT));
	pr_info("max. 5V:      %5d mV\n", bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETMAX5VOLT));
	pr_info("act. 12V:     %5d mV\n", bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GET12VOLT));
	pr_info("max. 12V:     %5d mV\n", bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETMAX12VOLT));
	pr_info("act 24V:      %5d mV\n", bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GET24VOLT));
	pr_info("max. 24V:     %5d mV\n", bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETMAX24VOLT));
	pr_info("act. temp.:   %5d C°\n", bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETTEMP));
	pr_info("min. temp.:   %5d C°\n", bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETMINTEMP));
	pr_info("max. temp.:   %5d C°\n", bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETMAXTEMP));
	pr_info("act. current: %5d mA\n", bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETCURRENT));
	pr_info("max. current: %5d mA\n", bbapi_read<uint16_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETMAXCURRENT));
	pr_info("act power:  %7d mW\n", bbapi_read<uint32_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETPOWER));
	pr_info("max. power: %7d mW\n", bbapi_read<uint32_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETMAXPOWER));
	pr_info("button state:     0x%02x\n", bbapi_read<uint8_t>(file, BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETBUTTONSTATE));

#if 0
	#define BIOSIOFFS_CXPWRSUPP_ENABLEBACKLIGHT			0x00000060	// Set display backlight, W:1 (0x00 := OFF, 0xFF := ON), R:0
	#define BIOSIOFFS_CXPWRSUPP_DISPLAYLINE1				0x00000061	// Set display line 1, W:17(BYTE[17]), R:0
	#define BIOSIOFFS_CXPWRSUPP_DISPLAYLINE2				0x00000062	// Set display line 2, W:17(BYTE[17]), R:0
#endif
}

void cx_ups_show(int file)
{
	const uint8_t enabled = bbapi_read<uint32_t>(file, BIOSIGRP_CXUPS, BIOSIOFFS_CXUPS_GETENABLED);
	pr_info("UPS:         %s\n", enabled ? "enabled" : "disabled" );

#if 0
	#define BIOSIOFFS_CXUPS_GETENABLED						0x00000000	// Get UPS enabled state, W:0, R:1 (BYTE) (0 := Disabled, 1 := Enabled)
	#define BIOSIOFFS_CXUPS_SETENABLED						0x00000001	// Set UPS enabled state, W:1 (BYTE) (0 := Disable, 1 := Enable), R:0
	#define BIOSIOFFS_CXUPS_GETFIRMWAREVER					0x00000002	// Get firmware version, W:0, R:2 (BYTE[0..1]) (BYTE[0] := Version, BYTE[1] := Revision)
	#define BIOSIOFFS_CXUPS_GETPOWERSTATUS					0x00000003	// Get power status, W:0, R:1 (BYTE) (0 := Unknown, 1 := Online, 2 := OnBatteries)
	#define BIOSIOFFS_CXUPS_GETBATTERYSTATUS				0x00000004	// Get battery status, W:0, R:1 (BYTE) (0 := Unknown, 1 := Ok, 2 := Change batteries)
	#define BIOSIOFFS_CXUPS_GETBATTERYCAPACITY			0x00000005	// Get battery capacity, W:0, R:1 (BYTE) [%]
	#define BIOSIOFFS_CXUPS_GETBATTERYRUNTIME				0x00000006	// Get battery runtime, W:0, R:4 (DWORD) [s]
	#define BIOSIOFFS_CXUPS_GETBOOTCOUNTER					0x00000007	// Get boot counter, W:0, R:4 (DWORD) [n]
	#define BIOSIOFFS_CXUPS_GETOPERATIONTIME				0x00000008	// Get operation time (minutes since production time), W:0, R:4 (DWORD) [min]
	#define BIOSIOFFS_CXUPS_GETPOWERFAILCOUNT				0x00000009	// Get powerfail count, W:0, R:4 (DWORD) [n]
	#define BIOSIOFFS_CXUPS_GETBATTERYCRITICAL			0x0000000A	// Get battery critical, W:0, R:1 (BYTE) (0 := Cap. ok, 1 := Cap. critical)
	#define BIOSIOFFS_CXUPS_GETBATTERYPRESENT				0x0000000B	// Get battery present, W:0, R:1 (BYTE) (0 := Not present, 1 := Present)
//	#define BIOSIOFFS_CXUPS_GETRESTARTMODE					0x0000000C	// OBSOLETE: Don't use it! Get restart mode, W:0, R:1 (BYTE) (0 := Disabled, 1 := Enabled)
//	#define BIOSIOFFS_CXUPS_SETRESTARTMODE					0x0000000D	// OBSOLETE: Don't use it! Set restart mode, W:1 (BYTE) (0 := Disable, 1 := Enable), R:0
	#define BIOSIOFFS_CXUPS_SETSHUTDOWNMODE				0x0000000E	// Set shutdown mode, W:1 (BYTE) (0 := Disable, 1:= Enable), R:0
	#define BIOSIOFFS_CXUPS_GETBATTSERIALNUMBER				0x00000011	// Get serial number W:0, R:4
	#define BIOSIOFFS_CXUPS_GETBATTHARDWAREVERSION			0x00000015	// Get hardware version W:0, R:2
	#define BIOSIOFFS_CXUPS_GETBATTPRODUCTIONDATE				0x00000019	// Get production date W:0, R:4
	#define BIOSIOFFS_CXUPS_GETOUTPUTVOLT					0x00000032	// Get 9~12V output voltage, W:0, R:2 (WORD) [mV]
	#define BIOSIOFFS_CXUPS_GETMAXOUTPUTVOLT				0x00000033	// Get max. 9~12V output voltage, W:0, R:2 (WORD) [mV]
	#define BIOSIOFFS_CXUPS_GETINPUTVOLT					0x00000034	// Get 24V input voltage, W:0, R:2 (WORD) [mV]
	#define BIOSIOFFS_CXUPS_GETMAXINPUTVOLT				0x00000035	// Get max. 24V input voltage, W:0, R:2 (WORD) [mV]
	#define BIOSIOFFS_CXUPS_GETTEMP							0x00000036	// Get temperature, W:0, R:1 (Signed BYTE)[°C]
	#define BIOSIOFFS_CXUPS_GETMINTEMP						0x00000037	// Get min. temperature, W:0, R:1 (Signed BYTE)[°C]
	#define BIOSIOFFS_CXUPS_GETMAXTEMP						0x00000038	// Get max. temperature, W:0, R:1 (Signed BYTE)[°C]
	#define BIOSIOFFS_CXUPS_GETCHARGINGCURRENT			0x00000039	// Get charging current, W:0, R:2 (WORD) [mA]
	#define BIOSIOFFS_CXUPS_GETMAXCHARGINGCURRENT		0x0000003A	// Get max. charging current, W:0, R:2 (WORD) [mA]
	#define BIOSIOFFS_CXUPS_GETCHARGINGPOWER				0x0000003B	// Get charging power consumption, W:0, R:4 (DWORD) [mW]
	#define BIOSIOFFS_CXUPS_GETMAXCHARGINGPOWER			0x0000003C	// Get max. charging power consumption, W:0, R:4 (DWORD) [mW]
	#define BIOSIOFFS_CXUPS_GETDISCHARGINGCURRENT		0x0000003D	// Get discharging current, W:0, R:2 (WORD) [mA]
	#define BIOSIOFFS_CXUPS_GETMAXDISCHARGINGCURRENT	0x0000003E	// Get max. discharging current, W:0, R:2 (WORD) [mA]
	#define BIOSIOFFS_CXUPS_GETDISCHARGINGPOWER			0x0000003F	// Get discharging power consumption, W:0, R:4 (DWORD) [mW]
	#define BIOSIOFFS_CXUPS_GETMAXDISCHARGINGPOWER		0x00000040	// Get max. discharging power consumption, W:0, R:4 (DWORD) [mW]
	#define BIOSIOFFS_CXUPS_GETLASTBATTCHANGEDATE		0x00000097	// Gets last battery change date , W:0, R:4
	#define BIOSIOFFS_CXUPS_SETLASTBATTCHANGEDATE		0x00000098	// Sets last battery change date W:4, R:0
	#define BIOSIOFFS_CXUPS_GETBATTRATEDCAPACITY			0x00000099	// Get rated capacity W:0, R:4 [mAh]
	#define BIOSIOFFS_CXUPS_GETSMBUSADDRESS				0x000000F0	// Get SMBus address W:0, R:2 (hHost, address)
#endif
}

void set_led(int file, unsigned long offset, uint8_t color)
{
	ioctl_write(file, BIOSIGRP_LED, offset, &color, sizeof(color));
}

int main(int argc, char *argv[])
{
	int file = open(FILE_PATH, O_RDWR);
	if ( -1 == file) {
		pr_info("Open '%s' failed\n", FILE_PATH);
		return -1;
	}

	TestSensors sensorTest;
	sensorTest.add_test("test_sensors", &TestSensors::test_sensors);
	sensorTest.run(argc, argv);

	TestBBAPI bbapiTest;
	bbapiTest.add_test("test_General", &TestBBAPI::test_General);
	bbapiTest.add_test("test_PwrCtrl", &TestBBAPI::test_PwrCtrl);
	bbapiTest.run(argc, argv);

#if 0
	cx_pwrctrl_show(file);
	cx_sups_show(file);
	cx_power_supply_show(file);
	cx_ups_show(file);
	set_led(file, BIOSIOFFS_LED_SET_TC, 1);
	set_led(file, BIOSIOFFS_LED_SET_USER, 1);
	//bbapi_callbacks_install(file);
#endif

	if(file != -1) close(file);
	return 0;
}
