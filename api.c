// SPDX-License-Identifier: MIT
/**
    Character Driver for Beckhoff BIOS API
    Author: 	Heiko Wilke <h.wilke@beckhoff.com>
    Author: 	Patrick Bruenn <p.bruenn@beckhoff.com>
    Copyright (C) 2013 - 2018  Beckhoff Automation GmbH & Co. KG
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <generated/utsrelease.h>
#include <asm/io.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,12,0))
#include <asm/uaccess.h>
#else
#include <linux/uaccess.h>
#endif

#include "api.h"
#include "TcBaDevDef.h"

#define DRV_VERSION "0.2.5"
#if BIOSAPIERR_OFFSET > 0
#define DRV_DESCRIPTION "Beckhoff BIOS API Driver"
#else
#define DRV_DESCRIPTION "Beckhoff BIOS API Driver (legacy mode)"
#endif

/* Global Variables */
static struct bbapi_object g_bbapi;

static unsigned long g_bbapi_search_area = BBIOSAPI_SIGNATURE_SEARCH_AREA;
module_param_named(search_area, g_bbapi_search_area, ulong, 0);
MODULE_PARM_DESC(search_area, "Size in bytes of the area to search for the BBAPI signature.");

#if defined(__i386__)
static const uint64_t BBIOSAPI_SIGNATURE = 0x495041534F494242LL;	// API-String "BBIOSAPI"

/**
 * This function is a wrapper to the Beckhoff BIOS API entry function,
 * which uses MS Windows calling convention.
 * I found no elegant and stable way to implement this for i386.
 * "__attribute__ ((stdcall))" + va_args was promissing but worked only
 * with optimization level "-O1". As long as ms_abi is not supported
 * on 32 bit x86 we stick with inline assembly...
 */
static unsigned int noinline bbapi_call(const void __kernel * const in,
			       void __kernel * const out, void *const entry,
			       const struct bbapi_struct *const cmd,
			       unsigned int *bytes_written)
{
	unsigned int ret;
__asm__("push %0": :"r"(bytes_written));
__asm__("push %0": :"r"(cmd->nOutBufferSize));
__asm__("push %0": :"r"(out));
__asm__("push %0": :"r"(cmd->nInBufferSize));
__asm__("push %0": :"r"(in));
__asm__("push %0": :"r"(cmd->nIndexOffset));
__asm__("push %0": :"r"(cmd->nIndexGroup));
__asm__("call *%0": :"r"(entry));
__asm__("mov %%eax, %0": "=m"(ret):);
	return ret;
}
#elif defined(__x86_64__)
static const uint64_t BBIOSAPI_SIGNATURE = 0x3436584950414242LL;	// API-String "BBAPIX64"

/**
 * This function is a wrapper to the Beckhoff BIOS API entry function,
 * which uses MS Windows calling convention.
 * On x86_64 all we need to do is to use gcc "__attribute__((ms_abi))"
 */
typedef
    __attribute__ ((ms_abi)) uint32_t(*PFN_BBIOSAPI_CALL) (uint32_t group,
							   uint32_t offset,
							   void *in,
							   uint32_t inSize,
							   void *out,
							   uint32_t outSize,
							   uint32_t * bytes);

static unsigned int bbapi_call(void __kernel * const in,
			       void __kernel * const out,
			       PFN_BBIOSAPI_CALL entry,
			       const struct bbapi_struct *const cmd,
			       unsigned int *bytes_written)
{
	return entry(cmd->nIndexGroup, cmd->nIndexOffset, in,
		     cmd->nInBufferSize, out, cmd->nOutBufferSize,
		     bytes_written);
}
#endif

unsigned int bbapi_rw(uint32_t group, uint32_t offset,
			     void __kernel * const in, uint32_t size_in,
			     void __kernel * const out, const uint32_t size_out, uint32_t *bytes_written)
{
	const struct bbapi_struct cmd = {
		.nIndexGroup = group,
		.nIndexOffset = offset,
		.pInBuffer = NULL,
		.nInBufferSize = size_in,
		.pOutBuffer = NULL,
		.nOutBufferSize = size_out
	};
	volatile unsigned int result = 0;

	if (!g_bbapi.entry)
		return BIOSAPI_SRVNOTSUPP;

	mutex_lock(&g_bbapi.mutex);
	result = bbapi_call(in, out, g_bbapi.entry, &cmd, bytes_written);
	mutex_unlock(&g_bbapi.mutex);
	if (result) {
		pr_debug("%s(0x%x:0x%x) failed with: 0x%x\n", __func__,
	         cmd.nIndexGroup, cmd.nIndexOffset, result);
		return -(result | BIOSAPIERR_OFFSET);
	}
	return result;
}

unsigned int bbapi_read(uint32_t group, uint32_t offset,
			void __kernel * const out, const uint32_t size)
{
	uint32_t bytes_written = 0;
	return bbapi_rw(group, offset, NULL, 0, out, size, &bytes_written);
}

EXPORT_SYMBOL(bbapi_read);

unsigned int bbapi_write(uint32_t group, uint32_t offset,
			 void __kernel * const in, uint32_t size)
{
	uint32_t bytes_written = 0;
	return bbapi_rw(group, offset, in, size, NULL, 0, &bytes_written);
}

EXPORT_SYMBOL(bbapi_write);

int bbapi_board_is(const char *const boardname)
{
	char board[CXPWRSUPP_MAX_DISPLAY_LINE] = { 0 };

	bbapi_read(BIOSIGRP_GENERAL, BIOSIOFFS_GENERAL_GETBOARDNAME, &board,
		   sizeof(board) - 1);
	return 0 == strncmp(board, boardname, sizeof(board));
}

EXPORT_SYMBOL(bbapi_board_is);

#include <linux/kprobes.h>
static struct kprobe kp = {
	.symbol_name = "kallsyms_lookup_name"
};

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
kallsyms_lookup_name_t fcn_kallsyms_lookup_name;

typedef void *(*fcn_vmalloc_node_range_t)(unsigned long size, unsigned long align,
		unsigned long start, unsigned long end, gfp_t gfp_mask,
		pgprot_t prot, unsigned long vm_flags, int node,
		const void *caller);
fcn_vmalloc_node_range_t fcn_vmalloc_node_range;

/**
 * bbapi_copy_bios() - Copy BIOS from SPI flash into RAM
 * @bbapi: pointer to a not initialized bbapi_object
 * @pos: pointer to the BIOS identifier string in flash
 *
 * We use BIOS shadowing to increase realtime performance.
 * The BIOS identifier string is followed by the 32-Bit BIOS API
 * function offset. This offset is the location of the BIOS API entry
 * function and is located at most 4096 bytes in front of the
 * BIOS memory end. So we calculate the size of the BIOS and copy it
 * from SPI Flash into RAM.
 * Accessing the BIOS in ROM while running realtime applications would
 * otherwise have bad effects on the realtime behaviour.
 *
 * Note: PAGE_KERNEL_EXEC omits the "no execute bit" exception
 *
 * Return: 0 for success, -ENOMEM if the allocation of kernel memory fails
 */
static int __init bbapi_copy_bios(struct bbapi_object *bbapi,
				  uint8_t __iomem * pos)
{
	const uint32_t offset = ioread32(pos + 8);
	const size_t size = offset + 4096;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
	bbapi->memory = fcn_vmalloc_node_range(size, 1, VMALLOC_START, VMALLOC_END,
			GFP_KERNEL, PAGE_KERNEL_EXEC, 0, NUMA_NO_NODE, __builtin_return_address(0));
#else
	bbapi->memory = __vmalloc(size, GFP_KERNEL, PAGE_KERNEL_EXEC);
#endif
	if (bbapi->memory == NULL) {
		pr_info("__vmalloc for Beckhoff BIOS API failed\n");
		return -ENOMEM;
	}
	memcpy_fromio(bbapi->memory, pos, size);
	bbapi->entry = bbapi->memory + offset;
	return 0;
}

/**
 * bbapi_find_bios() - Find BIOS in SPI flash and copy it into RAM
 * @bbapi: pointer to a not initialized bbapi_object
 *
 * If successful bbapi->memory and bbapi->entry point to the bios in RAM
 *
 * Return: 0 if the bios was successfully copied into RAM
 */
static int __init bbapi_find_bios(struct bbapi_object *bbapi)
{
	static const size_t STEP_SIZE = 0x10;

	// Try to remap IO Memory to search the BIOS API in the memory
	if (g_bbapi_search_area > BBIOSAPI_SIGNATURE_SEARCH_AREA) {
		pr_warn("Search area too big\n");
		return -EFAULT;
	}
	uint8_t __iomem *const start = ioremap(BBIOSAPI_SIGNATURE_PHYS_START_ADDR,
					       g_bbapi_search_area);
	const uint8_t __iomem *const end = start + g_bbapi_search_area;
	int result = -EFAULT;
	uint8_t __iomem *pos;
	size_t off;

	if (start == NULL) {
		pr_warn("Mapping memory search area for BIOS API failed\n");
		return -ENOMEM;
	}
	// Search through the remapped memory and look for the BIOS API String
	for (off = 0; off < STEP_SIZE; ++off) {
		for (pos = start + off; pos <= end - STEP_SIZE; pos += STEP_SIZE) {
			const uint32_t low = ioread32(pos);
			const uint32_t high = ioread32(pos + 4);
			const uint64_t lword = ((uint64_t) high << 32 | low);
			if (BBIOSAPI_SIGNATURE == lword) {
				result = bbapi_copy_bios(bbapi, pos);
				pr_info
				    ("BIOS found and copied from: %p + 0x%zx | %zu\n",
				     start, pos - start, off);
				goto cleanup;
			}
		}
	}
cleanup:
	iounmap(start);
	return result;
}

/**
 * You have to hold the lock on bbapi->mutex when calling this function!!!
 */
static int bbapi_ioctl_mutexed(struct bbapi_object *const bbapi,
			       const struct bbapi_struct *const cmd)
{
	unsigned int written = 0;
	unsigned int ret;

	if (cmd->nInBufferSize > sizeof(bbapi->in)) {
		pr_err("%s(): nInBufferSize invalid\n", __FUNCTION__);
		return -EINVAL;
	}
	if (cmd->nOutBufferSize > sizeof(bbapi->out)) {
		pr_err("%s(): nOutBufferSize: %d invalid\n", __FUNCTION__,
		       cmd->nOutBufferSize);
		return -EINVAL;
	}
	// BIOS can operate on kernel space buffers only -> make a temporary copy
	if (copy_from_user(bbapi->in, cmd->pInBuffer, cmd->nInBufferSize)) {
		pr_err("%s(): copy_from_user() failed\n", __FUNCTION__);
		return -EFAULT;
	}
	// Call the BIOS API
	ret = bbapi_call(bbapi->in, bbapi->out, bbapi->entry, cmd, &written);
	if (ret) {
		pr_debug("%s(0x%x:0x%x) failed with: 0x%x\n", __func__,
		         cmd->nIndexGroup, cmd->nIndexOffset, ret);
		return -(ret | BIOSAPIERR_OFFSET);
	}
	// Copy the BIOS output to the output buffer in user space
	if (copy_to_user(cmd->pOutBuffer, bbapi->out, written)) {
		pr_err("%s(): copy_to_user() failed\n", __FUNCTION__);
		return -EFAULT;
	}

	if (cmd->pBytesReturned) {
		put_user(written, cmd->pBytesReturned);
	}
	return 0;
}

static long bbapi_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct bbapi_struct bbstruct;
	size_t size = sizeof(bbstruct);
	int result = -EINVAL;
	if (!g_bbapi.entry) {
		pr_warn("%s(): not initialized.\n", __FUNCTION__);
		return -EINVAL;
	}
	// Check if IOCTL CMD matches BBAPI Driver Command
#ifdef BBAPI_CMD_LEGACY
	if (cmd == BBAPI_CMD_LEGACY) {
		size -= sizeof(bbstruct.pBytesReturned) + sizeof(bbstruct.pMode);
		bbstruct.pBytesReturned = NULL;
		bbstruct.pMode = NULL;
	} else
#endif
	if (cmd != BBAPI_CMD) {
		pr_info("Wrong Command\n");
		return -EINVAL;
	}
	// Copy data (BBAPI struct) from User Space to Kernel Module - if it fails, return error
	if (copy_from_user
	    (&bbstruct, (const void __user *)arg, size)) {
		pr_err("copy_from_user failed\n");
		return -EINVAL;
	}
	// pMode is reserved for future use
	if (bbstruct.pMode) {
		pr_info("Setting pMode to nullptr is mandatory!\n");
		return -EINVAL;
	}

	if (bbstruct.nIndexOffset >= 0xB0) {
		pr_info("cmd: 0x%x : 0x%x not available from user mode\n",
			bbstruct.nIndexGroup, bbstruct.nIndexOffset);
		return -EACCES;
	}

	mutex_lock(&g_bbapi.mutex);
	result = bbapi_ioctl_mutexed(&g_bbapi, &bbstruct);
	mutex_unlock(&g_bbapi.mutex);
	return result;
}

static int bbapi_release(struct inode *i, struct file *f)
{
	return 0;
}

static struct file_operations file_ops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = bbapi_ioctl,
	.release = bbapi_release,
};

static void __init update_display(void)
{
	char line[CXPWRSUPP_MAX_DISPLAY_LINE];
	uint8_t enable = 0xff;

	snprintf(line, sizeof(line), "%s %s", UNAME_S, UTS_RELEASE);
	bbapi_write(BIOSIGRP_CXPWRSUPP,
		    BIOSIOFFS_CXPWRSUPP_DISPLAYLINE2, line, sizeof(line));

	bbapi_read(BIOSIGRP_GENERAL, BIOSIOFFS_GENERAL_GETBOARDNAME, line,
		   sizeof(line));
	bbapi_write(BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_DISPLAYLINE1, line,
		    sizeof(line));

	bbapi_write(BIOSIGRP_CXPWRSUPP,
		    BIOSIOFFS_CXPWRSUPP_ENABLEBACKLIGHT, &enable,
		    sizeof(enable));
}

static void dev_release_nop(struct device *dev)
{
}

static struct platform_device bbapi_power = {
	.name = "bbapi_power",
	.id = -1,
	.dev = {.release = dev_release_nop},
};

static struct platform_device bbapi_sups = {
	.name = "bbapi_sups",
	.id = -1,
	.dev = {.release = dev_release_nop},
};

inline static bool bbapi_supports(uint32_t group, uint32_t offset)
{

	switch (-bbapi_read(group, offset, NULL, 0)) {
	case BIOSAPI_INVALIDSIZE:
	case BIOSAPI_INVALIDPARM:
		return true;
	default:
		return false;
	}
}

#define bbapi_supports_display() \
	bbapi_supports(BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_ENABLEBACKLIGHT)

#define bbapi_supports_power() \
	bbapi_supports(BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETTYPE)

#define bbapi_supports_sups() \
	(bbapi_supports(BIOSIGRP_SUPS, BIOSIOFFS_SUPS_GPIO_PIN_EX) \
	 || bbapi_supports(BIOSIGRP_SUPS, BIOSIOFFS_SUPS_GPIO_PIN))

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
		pr_warn("Initializing BIOS failed with: 0x%x\n", bios_status);
	}
	return 0;
}

static void __exit bbapi_exit_bios(void)
{
	const unsigned int bios_status = bbapi_write(0, 0xFF, NULL, 0);
	if (bios_status) {
		pr_warn("Unload BIOS failed with: 0x%x\n", bios_status);
	}
}

static int __init bbapi_init_module(void)
{
	int result;

	pr_info("%s, %s\n", DRV_DESCRIPTION, DRV_VERSION);
	mutex_init(&g_bbapi.mutex);

	register_kprobe(&kp);
	fcn_kallsyms_lookup_name = (kallsyms_lookup_name_t)kp.addr;
	unregister_kprobe(&kp);

	fcn_vmalloc_node_range = (fcn_vmalloc_node_range_t)fcn_kallsyms_lookup_name("__vmalloc_node_range");

	result = bbapi_find_bios(&g_bbapi);
	if (result) {
		pr_info("BIOS API not available on this System\n");
		return result;
	}

	if (bbapi_supports_power()) {
		result = platform_device_register(&bbapi_power);
		if (result) {
			pr_info("register %s failed\n", bbapi_power.name);
			goto rollback_memory;
		}
	}

	if (bbapi_supports_sups()) {
		result = platform_device_register(&bbapi_sups);
		if (result) {
			pr_info("register %s failed\n", bbapi_sups.name);
			goto rollback_power;
		}
	}

	result =
	    simple_cdev_init(&g_bbapi.dev, "chardev", KBUILD_MODNAME,
			     &file_ops);
	if (result) {
		goto rollback_sups;
	}

	if (bbapi_supports_display()) {
		update_display();
	}
	return bbapi_init_bios();

rollback_sups:
	if (bbapi_supports_sups()) {
		platform_device_unregister(&bbapi_sups);
	}

rollback_power:
	if (bbapi_supports_power()) {
		platform_device_unregister(&bbapi_power);
	}

rollback_memory:
	vfree(g_bbapi.memory);
	return result;
}

static void __exit bbapi_exit(void)
{
	if (!g_bbapi.memory)
		return;

	bbapi_exit_bios();
	simple_cdev_remove(&g_bbapi.dev);

	if (bbapi_supports_sups()) {
		platform_device_unregister(&bbapi_sups);
	}

	if (bbapi_supports_power()) {
		platform_device_unregister(&bbapi_power);
	}
	vfree(g_bbapi.memory);
}

module_init(bbapi_init_module);
module_exit(bbapi_exit);

MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR("Patrick Bruenn <p.bruenn@beckhoff.com>");
MODULE_LICENSE("GPL and additional rights");
#ifndef __FreeBSD__
MODULE_VERSION(DRV_VERSION);
#else
MODULE_VERSION(bbapi, 1);
MODULE_DEPEND(bbapi, linuxkpi, 1, 1, 1);
#endif
