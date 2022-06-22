// SPDX-License-Identifier: MIT
/**
    Beckhoff BIOS API driver to access hwmon sensors for Beckhoff IPCs
    Copyright (C) 2014 - 2018 Beckhoff Automation GmbH & Co. KG
    Author: Patrick Bruenn <p.bruenn@beckhoff.com>
*/
#ifndef _TCBADEVDEF_H_
#define _TCBADEVDEF_H_

#ifndef WINDOWS
typedef char TCHAR;
#define _T(x) x
#pragma GCC diagnostic ignored "-Wwrite-strings"

/** legacy mode can be enabled by predefining BIOSAPIERR_OFFSET to 0 */
#ifdef BIOSAPIERR_OFFSET
#pragma message "legacy mode is deprecated"
#define BBAPI_CMD							0x5000	// BIOS API Command number for IOCTL call
#else
#define BIOSAPIERR_OFFSET 0x20000000

#ifdef __FreeBSD__
#include <sys/ioccom.h>
#define BBAPI_CMD _IOWR('B', 0x5001, struct bbapi_struct)
#else
#define BBAPI_CMD_LEGACY						0x5000	// BIOS API Command number for IOCTL call
#define BBAPI_CMD							0x5001	// BIOS API Command number for IOCTL call
#endif
#endif
#define BBAPI_WATCHDOG_MAX_TIMEOUT_SEC (255 * 60) // BBAPI maximum timeout is 255 minutes

#ifndef __KERNEL__
#define __user
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif /* #ifndef __KERNEL__ */

struct bbapi_struct {
	uint32_t nIndexGroup;
	uint32_t nIndexOffset;
	const void __user *pInBuffer;
	uint32_t nInBufferSize;
	void __user *pOutBuffer;
	uint32_t nOutBufferSize;
#if BIOSAPIERR_OFFSET > 0
	uint32_t __user *pBytesReturned;
	void __user *pMode;
#endif
#ifdef __cplusplus
	bbapi_struct(uint32_t group, uint32_t offset, const void __user *pIn, uint32_t inSize, void __user *pOut, uint32_t outSize, uint32_t *bytesReturned = nullptr, void *mode = nullptr)
	: nIndexGroup(group),
	nIndexOffset(offset),
	pInBuffer(pIn),
	nInBufferSize(inSize),
	pOutBuffer(pOut),
	nOutBufferSize(outSize)
#if BIOSAPIERR_OFFSET > 0
	,pBytesReturned(bytesReturned),
	pMode(mode)
#endif
	{};
#endif /* #ifdef __cplusplus */
};
#endif /* #ifndef WINDOWS */

#define BADEVICE_MBINFO_snprintf(p, buffer, len) \
	snprintf(buffer, len, "%.8s hw: %d v%d.%d", (p)->MBName, (p)->MBRevision, (p)->biosMajVersion, (p)->biosMinVersion);

#define BADEVICE_VERSION_snprintf(p, buffer, len) \
	snprintf(buffer, len, "ver.: %d rev.: %d build: %d", (p)->version, (p)->revision, (p)->build);

#define Bapi_GpioInfoEx_snprintf(p, buffer, len) \
	snprintf(buffer, len, "0x%x, 0x%x, 0x%x, 0x%" PRIu64 "x, 0x%" PRIu64 "x [type, length, flags, address, bitmask]", \
		(p)->type, (p)->length, (p)->flags, (p)->address, (p)->bitmask);

#define SENSORINFO_snprintf(p, buffer, len) \
	snprintf(buffer, len, "%23s %14s val:%5u(0x%04x) min:%5u(0x%04x) max:%5u(0x%04x) nom:%5u(0x%04x) %12s", \
			LOCATIONCAPS[(int)((p)->eLocation)].name, PROBECAPS[(int)((p)->eType)].name, \
			(p)->readVal.value,(p)->readVal.status, (p)->minVal.value, (p)->minVal.status, \
			(p)->maxVal.value, (p)->maxVal.status, (p)->nomVal.value, (p)->nomVal.status, (p)->desc);

#define TSUps_GpioInfo_snprintf(p, buffer, len) \
	snprintf(buffer, len, "0x%04x, 0x%02x, 0x%02x [ioAddr, offset, params]", (p)->ioAddr, (p)->offset, (p)->params);


////////////////////////////////////////////////////////////////
// Max. mainboard type (plattform) string length  (inclusive null delimiter)
#define BAGEN_MAX_MAINBOARD_TYPE 16
////////////////////////////////////////////////////////////////
// Max. power controller serial number string length (inclusive null delimiter)
#define PWRCTRL_MAX_SERIAL_NUMBER 17
////////////////////////////////////////////////////////////////
// Max. power controller test number string length (inclusive null delimiter)
#define PWRCTRL_MAX_TEST_NUMBER 7
///////////////////////////////////////////////////////////
// Max. CX power supply display line length
#define CXPWRSUPP_MAX_DISPLAY_CHARS 16 // without null delimiter
#define CXPWRSUPP_MAX_DISPLAY_LINE (CXPWRSUPP_MAX_DISPLAY_CHARS + 1)  // 17 => inclusive null delimiter

////////////////////////////////////////////////////////////////
// max. length of device string (inclusive null delimiter)
#define BADEVICE_MAX_STRING  24

////////////////////////////////////////////////////////////////
// String description
typedef struct TStringResourceCap
{
	const TCHAR* name;
}PROBECAP, LOCATIONCAP, BOOTLOADER_FWUPDATE_ERRORSTRING, *PBOOTLOADER_FWUPDATE_ERRORSTRING;

////////////////////////////////////////////////////////////////
// Version information structure
typedef struct TBaDevice_Version
{
	uint8_t version;
	uint8_t revision;
	uint16_t build;
#ifdef __cplusplus
	TBaDevice_Version(uint8_t v = 0, uint8_t r = 0, uint16_t b = 0)
		: version(v), revision(r), build(b)
	{
	}

	bool operator==(const TBaDevice_Version& ref) const {
		return 0 == memcmp(this, &ref, sizeof(*this));
	}

	int snprintf(char* buffer, size_t len) const {
		return ::BADEVICE_VERSION_snprintf(this, buffer, len);
	};
#endif /* #ifdef __cplusplus */
}BADEVICE_VERSION, *PBADEVICE_VERSION;

// Collective version information
typedef struct TBaVersion
{
	BADEVICE_VERSION api;// BIOS (api implementation) version information
	BADEVICE_VERSION drv;// Driver version information
	BADEVICE_VERSION dll;// Api DLL version information
	BADEVICE_VERSION fun;// Function DLL version information
}BAVERSION,*PBAVERSION;

////////////////////////////////////////////////////////////////
// String parameter
typedef struct TBaDevice_String
{
	char s[BADEVICE_MAX_STRING];
}BADEVICE_STRING, *PBADEVICE_STRING;

////////////////////////////////////////////////////////////////
// Mainboard Information structure
typedef struct  TBaDevice_MBInfo
{
	char    MBName[8];//ascii string
	uint8_t MBRevision;
	uint8_t biosMajVersion;
	uint8_t biosMinVersion;
	uint8_t reserved;
#ifdef __cplusplus
	TBaDevice_MBInfo(const char* name = NULL, uint8_t revision = 0, uint8_t major = 0, uint8_t minor = 0)
		: MBRevision(revision), biosMajVersion(major), biosMinVersion(minor), reserved(0)
	{
		if(name) {
			strncpy(MBName, name, sizeof(MBName));
		} else {
			memset(MBName, 0, sizeof(MBName));
		}
	}

	bool operator==(const TBaDevice_MBInfo& ref) const {
		return 0 == memcmp(this, &ref, sizeof(*this));
	}

	int snprintf(char* buffer, size_t len) const {
		return ::BADEVICE_MBINFO_snprintf(this, buffer, len);
	};
#endif /* #ifdef __cplusplus */
}BADEVICE_MBINFO, *PBADEVICE_MBINFO;

////////////////////////////////////////////////////////////////
// Date structure
typedef struct TBDevice_Date
{
	uint16_t day; // 0 == not used
	uint16_t month; // 0 == not used
	uint16_t year; // 0 == not used
	uint16_t calWeek; // calendar week
}BADEVICE_DATE, *PBADEVICE_DATE;

////////////////////////////////////////////////////////////////
// Sensor types
typedef enum TProbeType
{
	PROBE_UNKNOWN			= 0,	// Unknown
	PROBE_TEMPERATURE		= 1,	// Temperature probe [°C];
	PROBE_VOLTAGE			= 2,	// Voltage probe [0.01V];
	PROBE_FAN				= 3,	// Fan tachometer input [RPM];
	PROBE_CASE_INTRUSION	= 4,	// Case intrusion input [0 = Closed, 1 = Open];
	PROBE_CURRENT			= 5,	// Current probe [0.001A];
	PROBE_COUNTER			= 6,	// Operating time counter (Betriebsstundenzähler)
	PROBE_POWER				= 7,	// Power consumption probe [0.001W]

	//add new types below
	PROBE_MAX
}PROBETYPE,*PPROBETYPE;

static const PROBECAP PROBECAPS[PROBE_MAX+1] =
{
	{_T("UNKNOWN\0")},
	{_T("TEMPERATURE\0")},
	{_T("VOLTAGE\0")},
	{_T("FAN\0")},
	{_T("CASE INTRUSION\0")},
	{_T("CURRENT\0")},
	{_T("COUNTER\0")},
	{_T("POWER\0")},
	{_T("\0")}
};
#define PROBENAME(x) (PROBECAPS[min(x,PROBE_MAX)].name)

////////////////////////////////////////////////////////////////
// Sensor locations
typedef enum TLocationType
{
	LOCATION_UNKNOWN = 0,
	LOCATION_OTHER = 1,
	LOCATION_PROCESSOR = 2,
	LOCATION_DISK = 3,
	LOCATION_SYSTEM_MANAGEMENT_MODULE = 4,
	LOCATION_MOTHERBOARD = 5,
	LOCATION_MEMORY_MODULE = 6,
	LOCATION_POWER_SUPPLY = 7,
	LOCATION_ADDIN_CARD = 8,
	LOCATION_FRONT_PANEL_BOARD = 9,
	LOCATION_BACK_PANEL_BOARD = 10,
	LOCATION_PERIPHERIE = 11,
	LOCATION_CHASSIS = 12,
	LOCATION_BATTERY = 13,
	LOCATION_UPS = 14,
	LOCATION_GRAFFIC_BOARD = 15,
	LOCATION_SUPERIO = 16,
	LOCATION_CHIPSET = 17,
	LOCATION_PWRCTRL = 18,

	// add new types below
	LOCATION_MAX
}LOCATIONTYPE, *PLOCATIONTYPE;

static const LOCATIONCAP LOCATIONCAPS[LOCATION_MAX+1] =
{
	{_T("UNKNOWN\0")},
	{_T("OTHER\0")},
	{_T("PROCESSOR\0")},
	{_T("DISK\0")},
	{_T("SYSTEM MANAGEMENT MODULE\0")},
	{_T("MOTHERBOARD\0")},
	{_T("MEMORY MODULE\0")},
	{_T("POWER SUPPLY\0")},
	{_T("ADDIN CARD\0")},
	{_T("FRONT PANEL BOARD\0")},
	{_T("BACK PANEL BOARD\0")},
	{_T("PERIPHERIE\0")},
	{_T("CHASSIS\0")},
	{_T("BATTERY\0")},
	{_T("UPS\0")},
	{_T("GRAFFIC BOARD\0")},
	{_T("SUPERIO\0")},
	{_T("CHIPSET\0")},
	{_T("PWRCTRL\0")},
	{_T("\0")}
};
#define LOCATIONNAME(x) (LOCATIONCAPS[min(x,LOCATION_MAX)].name)

////////////////////////////////////////////////////////////////
// Infovalue structure
typedef struct TInfoValue
{
	int16_t value;	// -32000 .. +32000
	uint16_t status;// sensor/probe measure/value status
	uint32_t reserved;
}INFOVALUE, *PINFOVALUE;

// Info value status bits
#define INFOVALUE_STATUS_OK			0x0000 // value is valid and can be used
#define INFOVALUE_STATUS_UNUSED		0x8000 // invalid/unused/unknown value
#define INFOVALUE_STATUS_RELVALUE	0x4000 // used by temperature == relative temperature if set

////////////////////////////////////////////////////////////////
// Sensor information structure (new size == 56 byte (0x38), old size == 48 byte (0x30)
typedef struct TSensorInfo
{
	PROBETYPE 		eType;		// sensor type
	LOCATIONTYPE 	eLocation;	// Sensor location
	INFOVALUE		readVal;		// Current value
	INFOVALUE		nomVal;  	// Nominal value
	INFOVALUE		minVal;		// Min. value
	INFOVALUE		maxVal;		// Max. value
	uint32_t reserved;
	char				desc[12]; 	// description of sensor as ASCII string (inkludes null termination)
#ifdef __cplusplus
	TSensorInfo(int status = 0)
	{
		memset(this, 0, sizeof(*this));
		readVal.status = INFOVALUE_STATUS_OK;
	}

	bool operator==(const TSensorInfo& ref) const {
		return (INFOVALUE_STATUS_UNUSED != readVal.status) && (INFOVALUE_STATUS_UNUSED != ref.readVal.status);
	}

	int snprintf(char* buffer, size_t len) const {
		return ::SENSORINFO_snprintf(this, buffer, len);
	};
#endif  /* #ifdef __cplusplus */
}SENSORINFO, *PSENSORINFO;

//*********************************************************
// SUPS data types
//*********************************************************
////////////////////////////////////////////////////////////////
// SUPS state
typedef enum TSUps_State
{
	SUPS_STATE_OTHER = 0x00,
	SUPS_STATE_OFF = 0xAF,// UPS off
	SUPS_STATE_DCIN_FAIL = 0xBB,// DCin fail
	SUPS_STATE_CHARGING = 0xC0,// Charging
	SUPS_STATE_DISCHARGING = 0xD0,// Discharging
	SUPS_STATE_BAL_ERR = 0xEE,// Balance error
	SUPS_STATE_NOT_SUPP = 0xF0,// Not supported
	SUPS_STATE_NO_COMM = 0xF1,// Not supported
	SUPS_STATE_NACK = 0xF2,// NACK
	SUPS_STATE_CHKSUM_ERR = 0xF3,// Checksum error
	SUPS_STATE_ID_ERR = 0xF4,// ID error, communication failed
	SUPS_STATE_CTRL_PWRF = 0xFF// µC power fail
}SUPS_STATE,*PSUPS_STATE;

////////////////////////////////////////////////////////////////
// SUPS or watchdog GPIO pin info
typedef struct TSUps_GpioInfo
{
	uint16_t ioAddr;
	uint8_t offset;
	uint8_t params;
	const uint32_t reserved;
#ifdef __cplusplus
	TSUps_GpioInfo(uint16_t addr = 0, uint8_t off = 0, uint8_t param = 0)
		: ioAddr(addr), offset(off), params(param), reserved(0)
	{
	}

	bool operator==(const TSUps_GpioInfo& ref) const {
		return (ioAddr == ref.ioAddr) && (offset == ref.offset) && (params == ref.params);
	}

	int snprintf(char* buffer, size_t len) const {
		return ::TSUps_GpioInfo_snprintf(this, buffer, len);
	};
#endif /* #ifdef __cplusplus */
}SUPS_GPIO_INFO, *PSUPS_GPIO_INFO, WATCHDOG_GPIO_INFO, *PWATCHDOG_GPIO_INFO;

typedef struct Bapi_GpioInfoEx
{
	uint16_t type; //0=Port access, 1=MemoryMappedIo,...
	uint16_t length;
	uint16_t flags; //Bit0-1: 00 Not valid, 01=RO, 10=WO, 11=RW;Bit2-3: 00=lowActive, 01=highActive,11=Toggle;Bit4-15:TBD
	uint16_t reserved;
	uint64_t address;
	uint64_t bitmask;
#ifdef __cplusplus
	Bapi_GpioInfoEx(uint16_t t = 0, uint16_t len = 0, uint16_t f = 0, uint64_t addr = 0, uint64_t mask = 0)
		: type(t),
		length(len),
		flags(f),
		reserved(0),
		address(addr),
		bitmask(mask)
	{
	}

	bool operator==(const Bapi_GpioInfoEx& ref) const {
		return !memcmp(this, &ref, sizeof(*this));
	}

	int snprintf(char* buffer, size_t len) const {
		return ::Bapi_GpioInfoEx_snprintf(this, buffer, len);
	};
#endif /* #ifdef __cplusplus */
}BAPI_GPIO_INFO_EX;

///////////////////////////////////////////////////////////
// Index-Group/Index-Offset specification
///////////////////////////////////////////////////////////
#define BIOSIGRP_GENERAL	0x00000000	// 0x00000000..0x00001FFF General BIOS functions
	#define BIOSIOFFS_GENERAL_VERSION						0x00000000	// Return BIOS API version, W:0, R:4
	#define BIOSIOFFS_GENERAL_GETBOARDNAME					0x00000001	// Return mainboard plattform, W:0, R:16
	#define BIOSIOFFS_GENERAL_GETBOARDINFO					0x00000002	// Return mainboard info (BiosVersion, Mainboard, Revision), W:0, R:SIZEOF(BADEVICE_MBINFO)
	#define BIOSIOFFS_GENERAL_GETPLATFORMINFO				0x00000003	// Returns the BIOS API platform information (32, 64 bit?). W:0, R:1 ( <0x00> := 32 bit API, <0x01> := 64 bit API )

#define BIOSIGRP_SYSTEM		0x00002000	// 0x00002000..0x00002FFF System information
	#define BIOSIOFFS_SYSTEM_COUNT_SENSORS					0x00000000	// Return max. number of available sensors, W:0, R:4 (count)
	#define BIOSIOFFS_SYSTEM_SENSOR_MIN						0x00000001	// Return info of first sensor IO:Sensor index, W:0, R:SIZEOF(SENSORINFO)
//	#define BIOSIOFFS_SYSTEM_SENSOR_MAX						0x000000FF	// Return info of max. sensor, W:0, R:56

#define BIOSIGRP_SERVICES		0x00003000	// 0x00003000..0x00003FFF := Device services
	#define BIOSIOFFS_SERVICES_GPIOREADIN0					0x00000000	// Reads GPIO0 input, W:0, R:1
	#define BIOSIOFFS_SERVICES_GPIOREADIN1					0x00000001	// Reads GPIO1 input, W:0, R:1
	#define BIOSIOFFS_SERVICES_GPIOREADOUT0					0x00000002	// Reads GPIO0 output, W:0, R:1
	#define BIOSIOFFS_SERVICES_GPIOREADOUT1					0x00000003	// Reads GPIO1 output, W:0, R:1
	#define BIOSIOFFS_SERVICES_GPIOREADMASK0				0x00000004	// Reads GPIO0 mask, W:0, R:1
	#define BIOSIOFFS_SERVICES_GPIOREADMASK1				0x00000005	// Reads GPIO1 mask, W:0, R:1
	#define BIOSIOFFS_SERVICES_GPIOWRITEOUT0				0x00000006	// Writes GPIO0 output, W:1, R:0
	#define BIOSIOFFS_SERVICES_GPIOWRITEOUT1				0x00000007	// Writes GPIO1 output, W:1, R:0
	#define BIOSIOFFS_SERVICES_GPIOWRITEMASK0				0x00000008	// Writes GPIO0 mask, W:1, R:0
	#define BIOSIOFFS_SERVICES_GPIOWRITEMASK1				0x00000009	// Writes GPIO1 mask, W:1, R:0

#define BIOSIGRP_PWRCTRL	0x00004000	// Power controller functions
	#define BIOSIOFFS_PWRCTRL_BOOTLDR_REV					0x00000000	// Bootloader revision, W:0, R:3 ([xx].[yy]-[zz])
	#define BIOSIOFFS_PWRCTRL_FIRMWARE_REV					0x00000001	// Firmware revision, W:0, R:3 ([xx].[yy]-[zz])
	#define BIOSIOFFS_PWRCTRL_DEVICE_ID						0x00000002	// Device ID, W:0, R:1
	#define BIOSIOFFS_PWRCTRL_OPERATING_TIME				0x00000003	// Operating time counter (Betriebsstundenzähler), W:0, R:4 (minutes)
	#define BIOSIOFFS_PWRCTRL_BOARD_TEMP					0x00000004	// Board temperature: min, max, W:0, R:2
	#define BIOSIOFFS_PWRCTRL_INPUT_VOLTAGE				0x00000005	// Input voltage: min, max, W:0, R:2
	#define BIOSIOFFS_PWRCTRL_SERIAL_NUMBER				0x00000006	// Serial number, W:0, R:16
	#define BIOSIOFFS_PWRCTRL_BOOT_COUNTER					0x00000007	// Boot counter, W:0, R:2
	#define BIOSIOFFS_PWRCTRL_PRODUCTION_DATE				0x00000008	// Manufacturing date, W:0, R:2, ([KW].[JJ])
	#define BIOSIOFFS_PWRCTRL_BOARD_POSITION				0x00000009	// Board position, W:0, R:1
	#define BIOSIOFFS_PWRCTRL_SHUTDOWN_REASON				0x0000000A	// Last Shutdown reason, W:0, R:1
	#define BIOSIOFFS_PWRCTRL_TEST_COUNTER					0x0000000B	// Test counter, W:0, R:1
	#define BIOSIOFFS_PWRCTRL_TEST_NUMBER					0x0000000C	// Test number, W:0, R:6

#define BIOSIGRP_SUPS		0x00005000	// SUPS functions
	#define BIOSIOFFS_SUPS_ENABLE								0x00000000	// Enable/disable SUPS, W:1, R:0
	#define BIOSIOFFS_SUPS_STATUS								0x00000001	// SUPS status, W:0, R:1
	#define BIOSIOFFS_SUPS_REVISION							0x00000002	// SUPS revision, W:0, R:2
	#define BIOSIOFFS_SUPS_PWRFAIL_COUNTER					0x00000003	// Power fail counter, W:0, R:2
	#define BIOSIOFFS_SUPS_PWRFAIL_TIMES					0x00000004	// Get latest power fail time stamps, W:0, R:12
	#define BIOSIOFFS_SUPS_SET_SHUTDOWN_TYPE				0x00000005	// Set the Shutdown behavior, W:1, R:0
	#define BIOSIOFFS_SUPS_GET_SHUTDOWN_TYPE				0x00000006	// Get the Shutdown behavior and reset, W:0, R:1
	#define BIOSIOFFS_SUPS_ACTIVE_COUNT						0x00000007	// Get the SUSV Active Count and reset, W:0, R:1
	#define BIOSIOFFS_SUPS_INTERNAL_PWRF_STATUS			0x00000008	// Get the number of Pwr-Fail in the SUSV, W:0, R:1
	#define BIOSIOFFS_SUPS_CAPACITY_TEST					0x00000009	// Capacitator test, W:0, R:0
	#define BIOSIOFFS_SUPS_TEST_RESULT						0x0000000a	// Get SUPS test result, W:0, R:1
	#define BIOSIOFFS_SUPS_GPIO_PIN							0x000000a0	// Get the Address and the GPIO-Pin from PWR-Fail PIN, W:0, R:4
	#define BIOSIOFFS_SUPS_GPIO_PIN_EX						0x000000a1	// Get the Address and the GPIO-Pin from PWR-Fail PIN, W:0, R:24

#define BIOSIGRP_WATCHDOG	0x00006000	// Watchdog functions
	#define BIOSIOFFS_WATCHDOG_ENABLE_TRIGGER				0x00000000	// Enable/trigger watchdog, W:1 (0 == Disable, 1..255 == Enable + set internal, R:0
	#define BIOSIOFFS_WATCHDOG_CONFIG						0x00000001	// Configure watchdog, W:1 (sets interval timebase, 0 == seconds, 1 == minutes), R:0
	#define BIOSIOFFS_WATCHDOG_GETCONFIG					0x00000002	// Get WD config, W:0, R:1
	#define BIOSIOFFS_WATCHDOG_SETCONFIG					0x00000003	// Set WD config, W:1, R:0
	#define BIOSIOFFS_WATCHDOG_ACTIVATE_PWRCTRL			0x00000004	// Activate PwrCtrl IO-Watchdog, W:1, R:0
	#define BIOSIOFFS_WATCHDOG_TRIGGER_TIMESPAN			0x00000005	// Enables/trigger the watchdog with TimeSpan, W:2, R:0
	#define BIOSIOFFS_WATCHDOG_IORETRIGGER					0x00000006	// IO-Retrigger, W:0, R:0
	#define BIOSIOFFS_WATCHDOG_GPIO_PIN						0x00000007	// Get IO Addr for direct retrigger, W:0, R:4 
	#define BIOSIOFFS_WATCHDOG_GPIO_PIN_EX					0x00000008	// Get IO Addr for direct retrigger, W:0, R:24

#define BIOSIGRP_LED			0x00008000	// TwinCAT and user LED functions
	#define BIOSIOFFS_LED_SET_TC								0x00000000	// Sets TwinCAT LED, W:1, R:0 (write value 0=off, 1=red, 2=blue, 3=green)
	#define BIOSIOFFS_LED_SET_USER							0x00000001	// Sets user LED, W:1, R:0 (write value 0=off, 1=red, 2=blue, 3=green)
	#define BIOSIOFFS_LED_SET_PWR                                                    0x00000002   //Sets Pwr LED, W:1, R:0 (write value 0=off, 1=red, 2=blue, 3=green, 4=yellow, 5=magenta, 6=cyan, 7=white)

#define BIOSIGRP_CXPWRSUPP		0x00009000	// CX Power Supply functions
	#define BIOSIOFFS_CXPWRSUPP_GETTYPE						0x00000010	// Get type, W:0, R:4 (DWORD)
	#define BIOSIOFFS_CXPWRSUPP_GETSERIALNO				0x00000011	// Get serial number, W:0, R:4 (DWORD := decimal1)
	#define BIOSIOFFS_CXPWRSUPP_GETFWVERSION				0x00000012	// Get FW version, W:0, R:2 (BYTE[2] := xx.yy)
	#define BIOSIOFFS_CXPWRSUPP_GETBOOTCOUNTER			0x00000013	// Get boot counter, W:0, R:4 (DWORD)
	#define BIOSIOFFS_CXPWRSUPP_GETOPERATIONTIME			0x00000014	// Get operation time, [minutes] since production time, W:0, R:4 (DWORD)
	#define BIOSIOFFS_CXPWRSUPP_GET5VOLT					0x00000030	// Get 5V sensor [mV], W:0, R:2 (WORD)
	#define BIOSIOFFS_CXPWRSUPP_GETMAX5VOLT				0x00000031	// Get max. 5V sensor [mV], W:0, R:2 (WORD)
	#define BIOSIOFFS_CXPWRSUPP_GET12VOLT					0x00000032	// Get 12V sensor [mV], W:0, R:2 (WORD)
	#define BIOSIOFFS_CXPWRSUPP_GETMAX12VOLT				0x00000033	// Get max. 12V sensor [mV], W:0, R:2 (WORD)
	#define BIOSIOFFS_CXPWRSUPP_GET24VOLT					0x00000034	// Get 24V sensor [mV], W:0, R:2 (WORD)
	#define BIOSIOFFS_CXPWRSUPP_GETMAX24VOLT				0x00000035	// Get max. 24V sensor [mV], W:0, R:2 (WORD)
	#define BIOSIOFFS_CXPWRSUPP_GETTEMP						0x00000036	// Get temperature [°C], W:0, R:1 (Signed BYTE)
	#define BIOSIOFFS_CXPWRSUPP_GETMINTEMP					0x00000037	// Get min. temperature [°C], W:0, R:1 (Signed BYTE)
	#define BIOSIOFFS_CXPWRSUPP_GETMAXTEMP					0x00000038	// Get max. temperature [°C], W:0, R:1 (Signed BYTE)
	#define BIOSIOFFS_CXPWRSUPP_GETCURRENT					0x00000039	// Get current [mA], W:0, R:2 (WORD)
	#define BIOSIOFFS_CXPWRSUPP_GETMAXCURRENT				0x0000003A	// Get max. current [mA], W:0, R:2 (WORD)
	#define BIOSIOFFS_CXPWRSUPP_GETPOWER					0x0000003B	// Get power [mW], W:0, R:4 (DWORD)
	#define BIOSIOFFS_CXPWRSUPP_GETMAXPOWER				0x0000003C	// Get max. power [mW], W:0, R:4 (DWORD)
	#define BIOSIOFFS_CXPWRSUPP_ENABLEBACKLIGHT			0x00000060	// Set display backlight, W:1 (0x00 := OFF, 0xFF := ON), R:0
	#define BIOSIOFFS_CXPWRSUPP_DISPLAYLINE1				0x00000061	// Set display line 1, W:17(BYTE[17]), R:0
	#define BIOSIOFFS_CXPWRSUPP_DISPLAYLINE2				0x00000062	// Set display line 2, W:17(BYTE[17]), R:0
	#define BIOSIOFFS_CXPWRSUPP_GETBUTTONSTATE			0x00000063	// Get button state, W:0, R:1 (0x01:=right, 0x02:=left, 0x04:=down, 0x08:=up, 0x10:=state select)

#define BIOSIGRP_CXUPS		0x00009001	// CX (condensator/battery) UPS functions
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

#define BIOSAPIERR_NOERR 0x0
#define BIOSAPI_SRVNOTSUPP (BIOSAPIERR_OFFSET + 0x701)
#define BIOSAPI_INVALIDSIZE (BIOSAPIERR_OFFSET + 0x705)
#define BIOSAPI_BUSY (BIOSAPIERR_OFFSET + 0x708)
#define BIOSAPI_INVALIDPARM (BIOSAPIERR_OFFSET + 0x70B)

#endif /* #ifndef _TCBADEVDEF_H_ */
