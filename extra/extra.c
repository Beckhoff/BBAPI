// SPDX-License-Identifier: MIT
/**
    BBAPI extra driver to initialize BIOS function pointer table
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2017 -2018 Beckhoff Automation GmbH & Co. KG
*/

#include <linux/module.h>
#include <linux/kernel.h>

#include "../api.h"
#include "../TcBaDevDef.h"

#define DRV_VERSION      "0.1"
#define DRV_DESCRIPTION  "Beckhoff BIOS API extension"

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#ifdef __i386__
typedef void __iomem *(*map_func) (int64_t, uint32_t, ...);
static void __iomem *ExtOsMapPhysAddr(int64_t physAddr, uint32_t memSize, ...)
#else
typedef __attribute__ ((ms_abi))
void __iomem *(*map_func) (int64_t, uint32_t);
static __attribute__ ((ms_abi))
void __iomem *ExtOsMapPhysAddr(int64_t physAddr, uint32_t memSize)
#endif
{
	return ioremap((unsigned long)physAddr, memSize);
}

#ifdef __i386__
typedef void (*unmap_func) (void *pLinMem, uint32_t memSize, ...);
static void ExtOsUnMapPhysAddr(void *pLinMem, uint32_t memSize, ...)
#else
typedef __attribute__ ((ms_abi))
void (*unmap_func) (void *pLinMem, uint32_t memSize);
static __attribute__ ((ms_abi))
void ExtOsUnMapPhysAddr(void *pLinMem, uint32_t memSize)
#endif
{
	iounmap((void __iomem *)pLinMem);
}

struct EXTOS_FUNCTION_ENTRY {
	uint8_t name[8];
	union {
		map_func map;
		unmap_func unmap;
		uint64_t placeholder;
	};
};

static struct EXTOS_FUNCTION_ENTRY extOsOps[] = {
	{"READMSR", {NULL}},
	{"GETBUSDT", {NULL}},
	{"MAPMEM", {.map = &ExtOsMapPhysAddr}},
	{"UNMAPMEM", {.unmap = &ExtOsUnMapPhysAddr}},
	{"WRITEMSR", {NULL}},
	{"SETBUSDT", {NULL}},

	/** MARK END OF TABLE */
	{"\0\0\0\0\0\0\0\0", {NULL}},
};

static int __init bbapi_init_bios(void)
{
	const unsigned int bios_status =
	    bbapi_write(0, 0xFE, &extOsOps, sizeof(extOsOps));
	if (bios_status) {
		pr_err("Initializing BIOS failed with: 0x%x\n", bios_status);
		return -EINVAL;
	}

	pr_info("%s, %s\n", DRV_DESCRIPTION, DRV_VERSION);
	return 0;
}

static void __exit bbapi_exit_bios(void)
{
	const unsigned int bios_status = bbapi_write(0, 0xFF, NULL, 0);
	if (bios_status) {
		pr_err("Unload BIOS failed with: 0x%x\n", bios_status);
	}
	pr_info("%s, %s unloaded.\n", DRV_DESCRIPTION, DRV_VERSION);
}

module_init(bbapi_init_bios);
module_exit(bbapi_exit_bios);

MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR("Patrick Bruenn <p.bruenn@beckhoff.com>");
MODULE_LICENSE("GPL and additional rights");
MODULE_VERSION(DRV_VERSION);
