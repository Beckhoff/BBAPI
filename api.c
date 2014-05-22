/**
    Character Driver for Beckhoff BIOS API
    Author: 	Heiko Wilke <h.wilke@beckhoff.com>
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2013 - 2014  Beckhoff Automation GmbH

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

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <generated/utsrelease.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/msr.h>

#include "api.h"

#define DRV_VERSION      "1.0.1"
#define DRV_DESCRIPTION  "Beckhoff BIOS API Driver"

/* Global Variables */
static struct bbapi_object g_bbapi;

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
static unsigned int bbapi_call(const void __kernel * const in,
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
							   const void *in,
							   uint32_t inSize,
							   void *out,
							   uint32_t outSize,
							   uint32_t * bytes);

static unsigned int bbapi_call(const void __kernel * const in,
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

unsigned int bbapi_read(uint32_t group, uint32_t offset,
			 void __kernel * const out, const uint32_t size)
{
	const struct bbapi_struct cmd = {
		.nIndexGroup = group,
		.nIndexOffset = offset,
		.pInBuffer = NULL,
		.nInBufferSize = 0,
		.pOutBuffer = NULL,
		.nOutBufferSize = size
	};
	unsigned int bytes_written = 0;
	volatile unsigned int result = 0;
	mutex_lock(&g_bbapi.mutex);
	result = bbapi_call(NULL, out, g_bbapi.entry, &cmd, &bytes_written);
	mutex_unlock(&g_bbapi.mutex);
	return result;
}

EXPORT_SYMBOL_GPL(bbapi_read);

unsigned int bbapi_write(uint32_t group, uint32_t offset,
			 const void __kernel * const in, uint32_t size)
{
	const struct bbapi_struct cmd = {
		.nIndexGroup = group,
		.nIndexOffset = offset,
		.pInBuffer = NULL,
		.nInBufferSize = size,
		.pOutBuffer = NULL,
		.nOutBufferSize = 0
	};
	unsigned int bytes_written = 0;
	volatile unsigned int result = 0;
	mutex_lock(&g_bbapi.mutex);
	result = bbapi_call(in, NULL, g_bbapi.entry, &cmd, &bytes_written);
	mutex_unlock(&g_bbapi.mutex);
	return result;
}

EXPORT_SYMBOL_GPL(bbapi_write);

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
static int bbapi_copy_bios(struct bbapi_object *bbapi, void __iomem * pos)
{
	const uint32_t offset = ioread32(pos + 8);
	const size_t size = offset + 4096;
	bbapi->memory = __vmalloc(size, GFP_KERNEL, PAGE_KERNEL_EXEC);
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
static int bbapi_find_bios(struct bbapi_object *bbapi)
{
	int result = -EFAULT;
	// Try to remap IO Memory to search the BIOS API in the memory
	void __iomem *const start = ioremap(BBIOSAPI_SIGNATURE_PHYS_START_ADDR,
					    BBIOSAPI_SIGNATURE_SEARCH_AREA);
	void __iomem *pos = start;
	const void __iomem *const end = pos + BBIOSAPI_SIGNATURE_SEARCH_AREA;
	if (start == NULL) {
		pr_warn("Mapping Memory Search area for BIOS API failed\n");
		return -1;
	}
	// Search through the remapped memory and look for the BIOS API String
	for (; pos < end; pos += 0x10)	//Aligned Search 0x10
	{
		const uint32_t low = ioread32(pos);
		const uint32_t high = ioread32(pos + 4);
		const uint64_t lword = ((uint64_t) high << 32 | low);
		if (BBIOSAPI_SIGNATURE == lword) {
			result = bbapi_copy_bios(bbapi, pos);
			pr_info("BIOS found and copied from: %p + 0x%x\n",
				start, pos - start);
			break;
		}
	}
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
	if (bbapi_call(bbapi->in, bbapi->out, bbapi->entry, cmd, &written)) {
		pr_err("%s(): call to BIOS failed\n", __FUNCTION__);
		return -EIO;
	}
	// Copy the BIOS output to the output buffer in user space
	if (copy_to_user(cmd->pOutBuffer, bbapi->out, written)) {
		pr_err("%s(): copy_to_user() failed\n", __FUNCTION__);
		return -EFAULT;
	}
	return 0;
}

static long bbapi_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct bbapi_struct bbstruct;
	int result = -EINVAL;
	if (!g_bbapi.entry) {
		pr_warn("%s(): not initialized.\n", __FUNCTION__);
		return -EINVAL;
	}
	// Check if IOCTL CMD matches BBAPI Driver Command
	if (cmd != BBAPI_CMD) {
		pr_info("Wrong Command\n");
		return -EINVAL;
	}
	// Copy data (BBAPI struct) from User Space to Kernel Module - if it fails, return error
	if (copy_from_user
	    (&bbstruct, (const void __user *)arg, sizeof(bbstruct))) {
		pr_err("copy_from_user failed\n");
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

// This function remains for compatibility only.
// TODO test it with an old kernel!
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int bbapi_ioctl_old(struct inode *i, struct file *f, unsigned int cmd,
			   unsigned long arg)
{
	bbapi_ioctl(f, cmd, arg);
}
#endif

/* Declare file operation function in struct */
static struct file_operations file_ops = {
	.owner = THIS_MODULE,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	.ioctl = bbapi_ioctl_old
#else
	.unlocked_ioctl = bbapi_ioctl
#endif
};

static void update_display(void)
{
	char board[16+1] = "                ";
	bbapi_read(0x00000000, 0x00000001, &board, sizeof(board));
	board[sizeof(board) - 1] = 0;
	if (0 == strncmp(board, "CX20x0\0\0\0\0\0\0\0\0\0", sizeof(board))) {
		uint8_t enable = 0xff;
		char line1[16+1] = "Linux 7890123456";
		strncpy(line1 + 6, UTS_RELEASE, sizeof(line1) - 1 - 6);
		bbapi_write(0x00009000, 0x00000060, &enable, sizeof(enable));
		bbapi_write(0x00009000, 0x00000062, &line1, sizeof(line1));
		bbapi_write(0x00009000, 0x00000061, &board, sizeof(board));
	} else {
		pr_info("platform has no display or is not supported\n");
	}
}

static int __init bbapi_init_module(void)
{
	pr_info("%s, %s\n", DRV_DESCRIPTION, DRV_VERSION);
	memset(&g_bbapi, 0, sizeof(g_bbapi));
	mutex_init(&g_bbapi.mutex);
	if (bbapi_find_bios(&g_bbapi)) {
		pr_info("BIOS API not available on this System\n");
		return -1;
	}
	update_display();
	return simple_cdev_init(&g_bbapi.dev, "chardev", "BBAPI", &file_ops);
}

static void __exit bbapi_exit(void)
{
	vfree(g_bbapi.memory);
	simple_cdev_remove(&g_bbapi.dev);
	pr_info("unregistered\n");
}

module_init(bbapi_init_module);
module_exit(bbapi_exit);

MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR("Patrick Bruenn <p.bruenn@beckhoff.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
