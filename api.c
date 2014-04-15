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
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/msr.h>

#include "api.h"

/* Global Variables */
static struct bbapi_object g_bbapi;


// BIOS API is called as with standard windows calling convention
// for Linux we have to rebuild it using assembler
#if defined(__i386__)
static const uint64_t BBIOSAPI_SIGNATURE = 0x495041534F494242LL;	// API-String "BBIOSAPI"

static unsigned int bbapi_call(struct bbapi_object *const bbapi,
			       const struct bbapi_struct *const cmd,
			       unsigned int *bytes_written)
{
	unsigned int ret;
__asm__("push %0": :"r"(bytes_written));
__asm__("push %0": :"r"(cmd->nOutBufferSize));
__asm__("push %0": :"r"(bbapi->out));
__asm__("push %0": :"r"(cmd->nInBufferSize));
__asm__("push %0": :"r"(bbapi->in));
__asm__("push %0": :"r"(cmd->nIndexOffset));
__asm__("push %0": :"r"(cmd->nIndexGroup));
__asm__("call *%0": :"r"(bbapi->entry));
__asm__("mov %%eax, %0": "=m"(ret):);
	return ret;
}
#elif defined(__x86_64__)
static const uint64_t BBIOSAPI_SIGNATURE = 0x3436584950414242LL;	// API-String "BBAPIX64"

static unsigned int bbapi_call(struct bbapi_object *const bbapi,
			       const struct bbapi_struct *const cmd,
			       unsigned int *bytes_written)
{
	unsigned int ret = 0;
__asm__("movq %0, 0x30(%%rsp)\n\t": :"r"(bytes_written));
__asm__("movl %0, 0x28(%%rsp)": :"r"(cmd->nOutBufferSize));
__asm__("movq %0, 0x20(%%rsp)": :"r"(bbapi->out));
__asm__("movq %0, %%r9": :"r"((uint64_t) cmd->nInBufferSize));
__asm__("movq %0, %%r8": :"r"(bbapi->in));
__asm__("movq %0, %%rdx": :"r"((uint64_t) cmd->nIndexOffset));
__asm__("movq %0, %%rcx": :"r"((uint64_t) cmd->nIndexGroup));
__asm__("call *%0": :"r"(bbapi->entry));
__asm__("mov %%rax, %0": "=m"(ret):);
	return ret;
}
#endif

/**
 * bbapi_copy_bios() - Copy BIOS from SPI flash into RAM
 * @bbapi: pointer to a not initialized bbapi_object
 * @pos: pointer to the BIOS identifier string in flash
 *
 * Directly behind the identifier string, the 32-Bit BIOS API
 * function offset follows. This offset is the location of
 * the BIOS API entry function and is located at most 4096
 * bytes in front of the BIOS memory end. So we calculate
 * the size of the BIOS and copy it from SPI Flash into RAM.
 * Accessing the BIOS directly would cause bad realtime
 * behaviour.
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
	// copy BIOS API from SPI Flash into RAM to decrease performance impact on realtime applications
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
 * Return: 0 if the bios was successfuly copied into RAM
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
			pr_info("BIOS found and copied.\n");
			break;
		}
	}
	iounmap(start);
	return result;
}

static struct bbapi_callback CALLBACKS[] = {
	{{"READMSR\0"}, (uint64_t)&__do_nop},
	{{"GETBUSDT"}, (uint64_t)&__do_nop},
	{{"MAPMEM\0\0"}, (uint64_t)&__do_nop},
	{{"UNMAPMEM"}, (uint64_t)&__do_nop},
	{{"WRITEMSR"}, (uint64_t)&__do_nop},
	{{"SETBUSDT"}, (uint64_t)&__do_nop},
	{{"\0\0\0\0\0\0\0\0"}, 0},
};

static void bbapi_init_callbacks(struct bbapi_object *const bbapi)
{
	static int initialized = 0;
	unsigned int bytes_written = 0;
	struct bbapi_struct cmd = {
		.nIndexGroup = 0x00000000, //BIOSIOFFS_GENERAL,
		.nIndexOffset = 0x000000FE, //BIOSIOFFS_GENERAL_LOADRESOURCEDATA
		.pInBuffer = NULL,
		.nInBufferSize = 5*sizeof(struct bbapi_callback),
		.pOutBuffer = NULL,
		.nOutBufferSize = 0,
	};
	BUILD_BUG_ON(sizeof(CALLBACKS) > sizeof(bbapi->in));
	BUILD_BUG_ON(5*sizeof(struct bbapi_callback) > sizeof(bbapi->in));
	if (initialized)
		return;

	mutex_lock(&bbapi->mutex);
	memcpy(bbapi->in, &CALLBACKS, sizeof(CALLBACKS));

	if (bbapi_call(bbapi, &cmd, &bytes_written)) {
		pr_err("%s(): call to BIOS failed\n", __FUNCTION__);
	}
	mutex_unlock(&bbapi->mutex);
	initialized = 1;
	pr_info("%s(): done.\n", __FUNCTION__);
}

/**
 * You have to hold the lock on bbapi->mutex when calling this function!!!
 */
static int bbapi_ioctl_mutexed(struct bbapi_object *const bbapi,
			       const struct bbapi_struct *const cmd)
{
	unsigned int bytes_written = 0;
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
	if (bbapi_call(bbapi, cmd, &bytes_written)) {
		pr_err("%s(): call to BIOS failed\n", __FUNCTION__);
		return -EIO;
	}
	// Copy the BIOS output to the output buffer in user space
	if (copy_to_user(cmd->pOutBuffer, bbapi->out, bytes_written)) {
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

	// Init callbacks in bbapi_init_module() would crash the kernel so we do it here
	//TODO reenable bbapi_init_callbacks(&g_bbapi);

	// Copy data (BBAPI struct) from User Space to Kernel Module - if it fails, return error
	if (copy_from_user
	    (&bbstruct, (const void __user *)arg, sizeof(bbstruct))) {
		pr_err("copy_from_user failed\n");
		return -EINVAL;
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
	.ioctl = bbapi_ioctl
#else
	.unlocked_ioctl = bbapi_ioctl
#endif
};

static int __init bbapi_init_module(void)
{
	memset(&g_bbapi, 0, sizeof(g_bbapi));
	mutex_init(&g_bbapi.mutex);
	if (bbapi_find_bios(&g_bbapi)) {
		pr_info("BIOS API not available on this System\n");
		return -1;
	}
	return simple_cdev_init(&g_bbapi.dev, "chardev", "BBAPI", &file_ops);
}

static void __exit bbapi_exit(void)
{
	vfree(g_bbapi.memory);
	simple_cdev_remove(&g_bbapi.dev);
	printk(KERN_INFO "Beckhoff BIOS API: BBAPI unregistered\n");
}

module_init(bbapi_init_module);
module_exit(bbapi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heiko Wilke");
MODULE_DESCRIPTION("Beckhoff BIOS API Driver");
MODULE_VERSION("1.1");
