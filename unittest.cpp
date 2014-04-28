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
static const BADEVICE_MBINFO EXPECTED_GENERAL_BOARDINFO = CONFIG_GENERAL_BBAPI_BOARDINFO;
static const uint8_t EXPECTED_GENERAL_BOARDNAME[16] = CONFIG_GENERAL_BBAPI_BOARDNAME;
static const uint8_t EXPECTED_GENERAL_PLATFORM = CONFIG_GENERAL_BBAPI_PLATFORM;
static const BADEVICE_VERSION EXPECTED_GENERAL_VERSION CONFIG_GENERAL_BBAPI_VERSION;

static const uint8_t EXPECTED_PWRCTRL_BL_REVISION[3] = CONFIG_PWRCTRL_BL_REVISION;
static const uint8_t EXPECTED_PWRCTRL_FW_REVISION[3] = CONFIG_PWRCTRL_FW_REVISION;
static const uint8_t EXPECTED_PWRCTRL_DEVICE_ID = CONFIG_PWRCTRL_DEVICE_ID;
static const char EXPECTED_PWRCTRL_SERIAL[16+1] = CONFIG_PWRCTRL_SERIAL;
static const uint8_t EXPECTED_PWRCTRL_PRODUCTION_DATE[2] = CONFIG_PWRCTRL_PRODUCTION_DATE;
static const uint8_t EXPECTED_PWRCTRL_POSITION = CONFIG_PWRCTRL_POSITION;
static const uint8_t EXPECTED_PWRCTRL_LAST_SHUTDOWN[3] = CONFIG_PWRCTRL_LAST_SHUTDOWN;
static const uint8_t EXPECTED_PWRCTRL_TEST_COUNT = CONFIG_PWRCTRL_TEST_COUNT;
static const char EXPECTED_PWRCTRL_TEST_NUMBER[6+1] = CONFIG_PWRCTRL_TEST_NUMBER;

static const uint32_t EXPECTED_CXPWRSUPP_TYPE = CONFIG_CXPWRSUPP_TYPE;
static const uint32_t EXPECTED_CXPWRSUPP_SERIALNO = CONFIG_CXPWRSUPP_SERIALNO;
static const uint8_t EXPECTED_CXPWRSUPP_FWVERSION[2] = CONFIG_CXPWRSUPP_FWVERSION;
static const uint32_t EXPECTED_CXPWRSUPP_BUTTON_STATE = CONFIG_CXPWRSUPP_BUTTON_STATE;

#define FILE_PATH	"/dev/BBAPI" 	// Path to character Device

#define DEBUG 1
#if DEBUG
#define pr_info printf
#else
#define pr_info(...)
#endif

#define STRING_SIZE 17				// Size of String for Display
#define BBAPI_CMD 0x5000			// BIOS API Command number for IOCTL call


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
T /*__attribute__ ((deprecated))*/ bbapi_read(int file, unsigned long group, unsigned long offset)
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

	// TODO REMOVE this function was deprecated because it doesn't check the return value of the ioctl call to the driver
	template<typename T>
	T /*__attribute__ ((deprecated))*/ read(unsigned long offset) const
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
	test_generic<sizeof(DATATYPE)>(bbapi, #INDEX_OFFSET, INDEX_OFFSET, NULL, "CHECK")
#define CHECK_ARRAY_OLD(MSG, INDEX_OFFSET, EXPECTATION) \
	test_generic<sizeof(EXPECTATION)>(bbapi, #INDEX_OFFSET, INDEX_OFFSET, &(EXPECTATION), MSG)
#define CHECK_VALUE(MSG, INDEX_OFFSET, EXPECTATION, TYPE) \
	test_value<TYPE>(bbapi, #INDEX_OFFSET, INDEX_OFFSET, EXPECTATION, MSG)
#define CHECK_OBJECT(MSG, INDEX_OFFSET, EXPECTATION, TYPE) \
	test_object<TYPE>(bbapi, #INDEX_OFFSET, INDEX_OFFSET, EXPECTATION, MSG)
#define CHECK_RANGE(MSG, INDEX_OFFSET, RANGE, TYPE) \
	test_range<TYPE>(bbapi, #INDEX_OFFSET, INDEX_OFFSET, RANGE, MSG)

	void test_CXPowerSupply(const std::string& test_name)
	{
		bbapi.setGroupOffset(BIOSIGRP_CXPWRSUPP);
		CHECK_VALUE("Type:         %04d\n", BIOSIOFFS_CXPWRSUPP_GETTYPE, EXPECTED_CXPWRSUPP_TYPE, uint32_t);
		CHECK_VALUE("Serial:       %04d\n", BIOSIOFFS_CXPWRSUPP_GETSERIALNO, EXPECTED_CXPWRSUPP_SERIALNO, uint32_t);
		CHECK_ARRAY_OLD("Fw ver.:     ", BIOSIOFFS_CXPWRSUPP_GETFWVERSION, EXPECTED_CXPWRSUPP_FWVERSION);
		CHECK_RANGE("Boot #:       %04d\n",     BIOSIOFFS_CXPWRSUPP_GETBOOTCOUNTER,   CONFIG_CXPWRSUPP_BOOTCOUNTER_RANGE, uint32_t);
		CHECK_RANGE("Optime:       %04d min\n", BIOSIOFFS_CXPWRSUPP_GETOPERATIONTIME, CONFIG_CXPWRSUPP_OPERATIONTIME_RANGE, uint32_t);
		CHECK_RANGE("act. 5V:      %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GET5VOLT,         CONFIG_CXPWRSUPP_5VOLT_RANGE,   uint16_t);
		CHECK_RANGE("max. 5V:      %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GETMAX5VOLT,      CONFIG_CXPWRSUPP_5VOLT_RANGE,   uint16_t);
		CHECK_RANGE("act. 12V:     %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GET12VOLT,        CONFIG_CXPWRSUPP_12VOLT_RANGE,  uint16_t);
		CHECK_RANGE("max. 12V:     %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GETMAX12VOLT,     CONFIG_CXPWRSUPP_12VOLT_RANGE,  uint16_t);
		CHECK_RANGE("act. 24V:     %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GET24VOLT,        CONFIG_CXPWRSUPP_24VOLT_RANGE,  uint16_t);
		CHECK_RANGE("max. 24V:     %5d mV\n",   BIOSIOFFS_CXPWRSUPP_GETMAX24VOLT,     CONFIG_CXPWRSUPP_24VOLT_RANGE,  uint16_t);
		CHECK_RANGE("act. temp.:   %5d C°\n",   BIOSIOFFS_CXPWRSUPP_GETTEMP,          CONFIG_CXPWRSUPP_TEMP_RANGE,    int8_t);
		CHECK_RANGE("min. temp.:   %5d C°\n",   BIOSIOFFS_CXPWRSUPP_GETMINTEMP,       CONFIG_CXPWRSUPP_TEMP_RANGE,    int8_t);
		CHECK_RANGE("max. temp.:   %5d C°\n",   BIOSIOFFS_CXPWRSUPP_GETMAXTEMP,       CONFIG_CXPWRSUPP_TEMP_RANGE,    int8_t);
		CHECK_RANGE("act. current: %5d mA\n",   BIOSIOFFS_CXPWRSUPP_GETCURRENT,       CONFIG_CXPWRSUPP_CURRENT_RANGE, uint16_t);
		CHECK_RANGE("max. current: %5d mA\n",   BIOSIOFFS_CXPWRSUPP_GETMAXCURRENT,    CONFIG_CXPWRSUPP_CURRENT_RANGE, uint16_t);
		CHECK_RANGE("act. power:   %5d mW\n",   BIOSIOFFS_CXPWRSUPP_GETPOWER,         CONFIG_CXPWRSUPP_POWER_RANGE,   uint32_t);
		CHECK_RANGE("max. power:   %5d mW\n",   BIOSIOFFS_CXPWRSUPP_GETMAXPOWER,      CONFIG_CXPWRSUPP_POWER_RANGE,   uint32_t);
		CHECK_VALUE("button state:     0x%02x\n", BIOSIOFFS_CXPWRSUPP_GETBUTTONSTATE, EXPECTED_CXPWRSUPP_BUTTON_STATE, uint8_t);

#if 0
		#define BIOSIOFFS_CXPWRSUPP_ENABLEBACKLIGHT			0x00000060	// Set display backlight, W:1 (0x00 := OFF, 0xFF := ON), R:0
		#define BIOSIOFFS_CXPWRSUPP_DISPLAYLINE1				0x00000061	// Set display line 1, W:17(BYTE[17]), R:0
		#define BIOSIOFFS_CXPWRSUPP_DISPLAYLINE2				0x00000062	// Set display line 2, W:17(BYTE[17]), R:0
#endif
	}

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
		CHECK_VALUE("platform:     0x%02x (0x00->32 bit, 0x01-> 64bit)\n", BIOSIOFFS_GENERAL_GETPLATFORMINFO, EXPECTED_GENERAL_PLATFORM, uint8_t);
		CHECK_OBJECT("BIOS API %s\n", BIOSIOFFS_GENERAL_VERSION, EXPECTED_GENERAL_VERSION, BADEVICE_VERSION);
	}

	void test_PwrCtrl(const std::string& test_name)
	{
		bbapi.setGroupOffset(BIOSIGRP_PWRCTRL);
		CHECK_ARRAY_OLD("Bl ver.:      ", BIOSIOFFS_PWRCTRL_BOOTLDR_REV, EXPECTED_PWRCTRL_BL_REVISION);
		CHECK_ARRAY_OLD("Fw ver.:      ", BIOSIOFFS_PWRCTRL_FIRMWARE_REV, EXPECTED_PWRCTRL_FW_REVISION);
		CHECK_VALUE("Device id:    0x%02x\n", BIOSIOFFS_PWRCTRL_DEVICE_ID, EXPECTED_PWRCTRL_DEVICE_ID, uint8_t);
		CHECK_RANGE("Optime:       %04d minutes since production\n", BIOSIOFFS_PWRCTRL_OPERATING_TIME, CONFIG_PWRCTRL_OPERATION_TIME_RANGE, uint32_t);
		CHECK      (BIOSIOFFS_PWRCTRL_BOARD_TEMP, uint8_t[2]);
		CHECK      (BIOSIOFFS_PWRCTRL_INPUT_VOLTAGE, uint8_t[2]);
		CHECK_ARRAY_OLD("Serial:       ", BIOSIOFFS_PWRCTRL_SERIAL_NUMBER, EXPECTED_PWRCTRL_SERIAL);
		CHECK_RANGE("Boot #:       %04d\n", BIOSIOFFS_PWRCTRL_BOOT_COUNTER, CONFIG_PWRCTRL_BOOT_COUNTER_RANGE, uint16_t);
		CHECK_ARRAY_OLD("Production date: ", BIOSIOFFS_PWRCTRL_PRODUCTION_DATE, EXPECTED_PWRCTRL_PRODUCTION_DATE);
		CHECK_VALUE("µC Position:  0x%02x\n", BIOSIOFFS_PWRCTRL_BOARD_POSITION, EXPECTED_PWRCTRL_POSITION, uint8_t);
		CHECK_ARRAY_OLD("Last shutdown reason: ", BIOSIOFFS_PWRCTRL_SHUTDOWN_REASON, EXPECTED_PWRCTRL_LAST_SHUTDOWN);
		CHECK_VALUE("Test count:   %03d\n", BIOSIOFFS_PWRCTRL_TEST_COUNTER, EXPECTED_PWRCTRL_TEST_COUNT, uint8_t);
		CHECK_ARRAY_OLD("Test number: ", BIOSIOFFS_PWRCTRL_TEST_NUMBER, EXPECTED_PWRCTRL_TEST_NUMBER);
	}

	void test_SUPS(const std::string& test_name)
	{
		if(CONFIG_SUPS_DISABLED) {
			pr_info("S-UPS test case disabled\n");
			return;
		}
		bbapi.setGroupOffset(BIOSIGRP_SUPS);
		uint16_t revision = bbapi.read<uint16_t>(BIOSIOFFS_SUPS_REVISION);
		pr_info("S-UPS status:    0x%02x\n", bbapi.read<uint8_t>(BIOSIOFFS_SUPS_STATUS));
		pr_info("S-UPS revision:  %d.%d\n", revision >> 8, 0xff & revision);
		pr_info("# Power fails:  %d\n", bbapi.read<uint16_t>(BIOSIOFFS_SUPS_PWRFAIL_COUNTER));
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

	void test_System(const std::string& test_name)
	{
		bbapi.setGroupOffset(BIOSIGRP_SYSTEM);
		uint32_t num_sensors = bbapi.read<uint32_t>(BIOSIOFFS_SYSTEM_COUNT_SENSORS);
		while (num_sensors > 0) {
			show_sensor(num_sensors);
			--num_sensors;
		}
	}

private:
	BiosApi bbapi;

	void show_sensor(uint32_t sensor)
	{
		SENSORINFO info;
		memset(&info, 0, sizeof(info));
		fructose_assert_ne(-1, bbapi.ioctl_read(sensor, &info, sizeof(info)));
		fructose_loop_assert(sensor, INFOVALUE_STATUS_UNUSED != info.readVal.status);

		pr_info("%02u: %12s %s %s val:%u min:%u max:%u nom:%u)\n",
			sensor, info.desc,
			LOCATIONCAPS[info.eType].name, PROBECAPS[info.eType].name,
			info.readVal.value, info.minVal.value, info.maxVal.value, info.nomVal.value);
	}

	template<size_t N>
	void test_generic(const BiosApi& bbapi, const std::string& nIndexOffset, const unsigned long offset, const void *const expectedValue, const std::string& msg)
	{
		uint8_t value[N];
		memset(value, 0, sizeof(value));
		fructose_loop_assert(nIndexOffset, -1 != bbapi.ioctl_read(offset, &value, sizeof(value)));
		if(expectedValue) {
			fructose_loop_assert(nIndexOffset, 0 == memcmp(expectedValue, &value, sizeof(value)));
			pr_info(msg.c_str());
			for(int i = N - 1; i >= 0; --i) {
				pr_info(" %02x", value[i]);
			}
			pr_info("\n");
			print_mem(value, 1);
		}
	}

	template<typename T>
	void test_value(const BiosApi& bbapi, const std::string& nIndexOffset, const unsigned long offset, const T expectedValue, const std::string& msg)
	{
		T value {0};
		fructose_loop_assert(nIndexOffset, -1 != bbapi.ioctl_read(offset, &value, sizeof(value)));
		fructose_loop_assert(nIndexOffset, expectedValue == value);
		pr_info(msg.c_str(), value);
	}

	template<typename T>
	void test_object(const BiosApi& bbapi, const std::string& nIndexOffset, const unsigned long offset, const T expectedValue, const std::string& msg)
	{
		T value {0};
		fructose_loop_assert(nIndexOffset, -1 != bbapi.ioctl_read(offset, &value, sizeof(value)));
		fructose_loop_assert(nIndexOffset, expectedValue == value);
		pr_info(msg.c_str(), value.ToString());
	}

	template<typename T>
	void test_range(const BiosApi& bbapi, const std::string& nIndexOffset, const unsigned long offset, const T lower, const T upper, const std::string& msg)
	{
		T value = 0;
		fructose_loop_assert(nIndexOffset, -1 != bbapi.ioctl_read(offset, &value, sizeof(value)));
		fructose_loop2_assert(nIndexOffset, lower, value, lower <= value);
		fructose_loop2_assert(nIndexOffset, upper, value, upper >= value);
		pr_info(msg.c_str(), value);
	}
};

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

	TestBBAPI bbapiTest;
	bbapiTest.add_test("test_General", &TestBBAPI::test_General);
	bbapiTest.add_test("test_PwrCtrl", &TestBBAPI::test_PwrCtrl);
	bbapiTest.add_test("test_SUPS", &TestBBAPI::test_SUPS);
	bbapiTest.add_test("test_System", &TestBBAPI::test_System);
	bbapiTest.add_test("test_CXPowerSupply", &TestBBAPI::test_CXPowerSupply);
	bbapiTest.run(argc, argv);

#if 0
	cx_ups_show(file);
	set_led(file, BIOSIOFFS_LED_SET_TC, 1);
	set_led(file, BIOSIOFFS_LED_SET_USER, 1);
	//bbapi_callbacks_install(file);
#endif

	if(file != -1) close(file);
	return 0;
}
