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
static const BADEVICE_VERSION EXPECTED_GENERAL_VERSION CONFIG_GENERAL_BBAPI_VERSION;

static const uint8_t EXPECTED_PWRCTRL_BL_REVISION[3] = CONFIG_PWRCTRL_BL_REVISION;
static const uint8_t EXPECTED_PWRCTRL_FW_REVISION[3] = CONFIG_PWRCTRL_FW_REVISION;
static const char EXPECTED_PWRCTRL_SERIAL[16+1] = CONFIG_PWRCTRL_SERIAL;
static const uint8_t EXPECTED_PWRCTRL_PRODUCTION_DATE[2] = CONFIG_PWRCTRL_PRODUCTION_DATE;
static const uint8_t EXPECTED_PWRCTRL_LAST_SHUTDOWN[3] = CONFIG_PWRCTRL_LAST_SHUTDOWN;
static const char EXPECTED_PWRCTRL_TEST_NUMBER[6+1] = CONFIG_PWRCTRL_TEST_NUMBER;

static const uint8_t EXPECTED_CXPWRSUPP_FWVERSION[2] = CONFIG_CXPWRSUPP_FWVERSION;

static const uint8_t EXPECTED_CXUPS_FIRMWAREVER[2] = CONFIG_CXUPS_FIRMWAREVER;

static const uint8_t EXPECTED_SUPS_REVISION[2] = CONFIG_SUPS_REVISION;

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

using namespace fructose;

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
	test_generic<sizeof(DATATYPE)>(#INDEX_OFFSET, INDEX_OFFSET, NULL, "CHECK")
#define CHECK_ARRAY_OLD(MSG, INDEX_OFFSET, EXPECTATION) \
	test_generic<sizeof(EXPECTATION)>(#INDEX_OFFSET, INDEX_OFFSET, &(EXPECTATION), MSG)
#define CHECK_VALUE(MSG, INDEX_OFFSET, EXPECTATION, TYPE) \
	test_range<TYPE>(#INDEX_OFFSET, INDEX_OFFSET, EXPECTATION, EXPECTATION, MSG)
#define CHECK_OBJECT(MSG, INDEX_OFFSET, EXPECTATION, TYPE) \
	test_object<TYPE>(#INDEX_OFFSET, INDEX_OFFSET, EXPECTATION, MSG)
#define CHECK_RANGE(MSG, INDEX_OFFSET, RANGE, TYPE) \
	test_range<TYPE>(#INDEX_OFFSET, INDEX_OFFSET, RANGE, MSG)

	void test_CXPowerSupply(const std::string& test_name)
	{
		bbapi.setGroupOffset(BIOSIGRP_CXPWRSUPP);
		pr_info("\nCX power supply test results:\n=============================\n");
		CHECK_VALUE("Type:         %04d\n", BIOSIOFFS_CXPWRSUPP_GETTYPE, CONFIG_CXPWRSUPP_TYPE, uint32_t);
		CHECK_VALUE("Serial:       %04d\n", BIOSIOFFS_CXPWRSUPP_GETSERIALNO, CONFIG_CXPWRSUPP_SERIALNO, uint32_t);
		CHECK_ARRAY_OLD("Fw ver.:     ", BIOSIOFFS_CXPWRSUPP_GETFWVERSION, EXPECTED_CXPWRSUPP_FWVERSION);
		CHECK_RANGE("Boot #:       %04d\n",     BIOSIOFFS_CXPWRSUPP_GETBOOTCOUNTER,   CONFIG_CXPWRSUPP_BOOTCOUNTER_RANGE, uint32_t);
		CHECK_RANGE("Optime:       %04d min.\n",BIOSIOFFS_CXPWRSUPP_GETOPERATIONTIME, CONFIG_CXPWRSUPP_OPERATIONTIME_RANGE, uint32_t);
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
		CHECK_VALUE("button state: 0x%02x\n",   BIOSIOFFS_CXPWRSUPP_GETBUTTONSTATE,   CONFIG_CXPWRSUPP_BUTTON_STATE, uint8_t);
#if 0
		#define BIOSIOFFS_CXPWRSUPP_ENABLEBACKLIGHT			0x00000060	// Set display backlight, W:1 (0x00 := OFF, 0xFF := ON), R:0
		#define BIOSIOFFS_CXPWRSUPP_DISPLAYLINE1				0x00000061	// Set display line 1, W:17(BYTE[17]), R:0
		#define BIOSIOFFS_CXPWRSUPP_DISPLAYLINE2				0x00000062	// Set display line 2, W:17(BYTE[17]), R:0
#endif
	}
	void test_CXUPS(const std::string& test_name)
	{
		if(CONFIG_CXUPS_DISABLED) {
			pr_info("\nCX UPS test case disabled\n");
			return;
		}
		bbapi.setGroupOffset(BIOSIGRP_CXUPS);
		pr_info("\nCX UPS test results:\n====================\n");
		CHECK_VALUE("UPS enabled:           0x%02x\n", BIOSIOFFS_CXUPS_GETENABLED, CONFIG_CXUPS_ENABLED, uint8_t);
		CHECK_ARRAY_OLD("Fw ver.:        ",            BIOSIOFFS_CXUPS_GETFIRMWAREVER, EXPECTED_CXUPS_FIRMWAREVER);
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
		BADEVICE_MBINFO info;
		bbapi.ioctl_read(BIOSIOFFS_GENERAL_GETBOARDINFO, &info, sizeof(info));
		fructose_assert_same_data(&EXPECTED_GENERAL_BOARDINFO, &info, sizeof(info));
		pr_info("%s hw: %d v%d.%d\n", info.MBName, info.MBRevision, info.biosMajVersion, info.biosMinVersion);
		char boardname[16] = {0};
		bbapi.ioctl_read(BIOSIOFFS_GENERAL_GETBOARDNAME, &boardname, sizeof(boardname));
		fructose_assert_same_data(&EXPECTED_GENERAL_BOARDNAME, &boardname, sizeof(boardname));
		pr_info("Board: %s\n", boardname);
		CHECK_VALUE("platform:     0x%02x (0x00->32 bit, 0x01-> 64bit)\n", BIOSIOFFS_GENERAL_GETPLATFORMINFO, CONFIG_GENERAL_PLATFORM, uint8_t);
		CHECK_OBJECT("BIOS API %s\n", BIOSIOFFS_GENERAL_VERSION, EXPECTED_GENERAL_VERSION, BADEVICE_VERSION);
	}

	void test_PwrCtrl(const std::string& test_name)
	{
		bbapi.setGroupOffset(BIOSIGRP_PWRCTRL);
		pr_info("\nPower control test results:\n===========================\n");
		CHECK_ARRAY_OLD("Bl ver.:      ", BIOSIOFFS_PWRCTRL_BOOTLDR_REV, EXPECTED_PWRCTRL_BL_REVISION);
		CHECK_ARRAY_OLD("Fw ver.:      ", BIOSIOFFS_PWRCTRL_FIRMWARE_REV, EXPECTED_PWRCTRL_FW_REVISION);
		CHECK_VALUE("Device id:    0x%02x\n", BIOSIOFFS_PWRCTRL_DEVICE_ID, CONFIG_PWRCTRL_DEVICE_ID, uint8_t);
		CHECK_RANGE("Optime:       %04d min.\n",BIOSIOFFS_PWRCTRL_OPERATING_TIME, CONFIG_PWRCTRL_OPERATION_TIME_RANGE, uint32_t);
		CHECK      (BIOSIOFFS_PWRCTRL_BOARD_TEMP, uint8_t[2]);
		CHECK      (BIOSIOFFS_PWRCTRL_INPUT_VOLTAGE, uint8_t[2]);
		CHECK_ARRAY_OLD("Serial:       ", BIOSIOFFS_PWRCTRL_SERIAL_NUMBER, CONFIG_PWRCTRL_SERIAL);
		CHECK_RANGE("Boot #:       %04d\n", BIOSIOFFS_PWRCTRL_BOOT_COUNTER, CONFIG_PWRCTRL_BOOT_COUNTER_RANGE, uint16_t);
		CHECK_ARRAY_OLD("Production date: ", BIOSIOFFS_PWRCTRL_PRODUCTION_DATE, EXPECTED_PWRCTRL_PRODUCTION_DATE);
		CHECK_VALUE("µC Position:  0x%02x\n", BIOSIOFFS_PWRCTRL_BOARD_POSITION, CONFIG_PWRCTRL_POSITION, uint8_t);
		CHECK_ARRAY_OLD("Last shutdown reason: ", BIOSIOFFS_PWRCTRL_SHUTDOWN_REASON, EXPECTED_PWRCTRL_LAST_SHUTDOWN);
		CHECK_VALUE("Test count:   %03d\n", BIOSIOFFS_PWRCTRL_TEST_COUNTER, CONFIG_PWRCTRL_TEST_COUNT, uint8_t);
		CHECK_ARRAY_OLD("Test number: ", BIOSIOFFS_PWRCTRL_TEST_NUMBER, EXPECTED_PWRCTRL_TEST_NUMBER);
	}

	void test_SUPS(const std::string& test_name)
	{
		if(CONFIG_SUPS_DISABLED) {
			pr_info("S-UPS test case disabled\n");
			return;
		}
#if 0
		bbapi.setGroupOffset(BIOSIGRP_SUPS);
		pr_info("\nSUPS test results:\n====================\n");
		uint16_t revision = bbapi.read<uint16_t>(BIOSIOFFS_SUPS_REVISION);
		pr_info("S-UPS status:    0x%02x\n", bbapi.read<uint8_t>(BIOSIOFFS_SUPS_STATUS));
		pr_info("S-UPS revision:  %d.%d\n", revision >> 8, 0xff & revision);
		CHECK_ARRAY_OLD("S-UPS revision:  ", BIOSIOFFS_SUPS_REVISION, EXPECTED_SUPS_REVISION);
		pr_info("# Power fails:  %d\n", bbapi.read<uint16_t>(BIOSIOFFS_SUPS_PWRFAIL_COUNTER));
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
		SENSORINFO info;
		bbapi.setGroupOffset(BIOSIGRP_SYSTEM);
		uint32_t num_sensors;
		bbapi.ioctl_read(BIOSIOFFS_SYSTEM_COUNT_SENSORS, &num_sensors, sizeof(num_sensors));
		pr_info("\nSystem test results:\n====================\n");
		while (num_sensors > 0) {
			pr_info("%02d:", num_sensors);
			CHECK_OBJECT("%s\n", num_sensors, info, SENSORINFO);
			--num_sensors;
		}
	}

private:
	BiosApi bbapi;

	template<size_t N>
	void test_generic(const std::string& nIndexOffset, const unsigned long offset, const void *const expectedValue, const std::string& msg)
	{
		uint8_t value[N];
		memset(value, 0, sizeof(value));
		fructose_loop_assert(nIndexOffset, -1 != bbapi.ioctl_read(offset, &value, sizeof(value)));
		if(expectedValue) {
			fructose_loop_assert(nIndexOffset, 0 == memcmp(expectedValue, &value, sizeof(value)));
			pr_info("%s", msg.c_str());
			for(size_t i = 0; i < N; ++i) {
				pr_info(" %02x", value[i]);
			}
			pr_info("\n");
			print_mem(value, 1);
		}
	}

	template<typename T>
	void test_object(const std::string& nIndexOffset, const unsigned long offset, const T expectedValue, const std::string& msg)
	{
		char text[256];
		T value {0};
		fructose_loop_assert(nIndexOffset, -1 != bbapi.ioctl_read(offset, &value, sizeof(value)));
		fructose_loop_assert(nIndexOffset, value == expectedValue);
		value.snprintf(text, sizeof(text));
		pr_info(msg.c_str(), text);
	}

	template<typename T>
	void test_range(const std::string& nIndexOffset, const unsigned long offset, const T lower, const T upper, const std::string& msg)
	{
		T value = 0;
		fructose_loop_assert(nIndexOffset, -1 != bbapi.ioctl_read(offset, &value, sizeof(value)));
		fructose_loop2_assert(nIndexOffset, lower, value, lower <= value);
		fructose_loop2_assert(nIndexOffset, upper, value, upper >= value);
		pr_info(msg.c_str(), value);
	}
};

int main(int argc, char *argv[])
{
	TestBBAPI bbapiTest;
	bbapiTest.add_test("test_General", &TestBBAPI::test_General);
	bbapiTest.add_test("test_PwrCtrl", &TestBBAPI::test_PwrCtrl);
	bbapiTest.add_test("test_SUPS", &TestBBAPI::test_SUPS);
	bbapiTest.add_test("test_System", &TestBBAPI::test_System);
	bbapiTest.add_test("test_CXPowerSupply", &TestBBAPI::test_CXPowerSupply);
	bbapiTest.add_test("test_CXUPS", &TestBBAPI::test_CXUPS);
	bbapiTest.run(argc, argv);
	return 0;
}
