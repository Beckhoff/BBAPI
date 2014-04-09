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
static DEFINE_MUTEX(mut);					// Mutex for exclusive write access

// Variables for 32 Bit System
#ifdef __i386__
static const char BBIOSAPI_SIGNATURE[8] = {'B','B','I','O','S','A','P','I'};
static const unsigned int BBIOSAPI_SEARCHBSTR_LOW = 0x4F494242; 	// first 4 Byte of API-String "BBIOSAPI"
static const unsigned int BBIOSAPI_SEARCHBSTR_HIGH = 0x49504153;	// last 4 Byte of API-String "BBIOSAPI"
#endif

// Variables for 64 Bit System
#ifdef __x86_64__
	const char BBIOSAPI_SIGNATURE[8] = {'B','B','A','P','I','X','6','4'};
	const unsigned int BBIOSAPI_SEARCHBSTR_LOW = 0x50414242; 	// first 4 Byte of API-String "BBAPIX64"
	const unsigned int BBIOSAPI_SEARCHBSTR_HIGH = 0x34365849;	// last 4 Byte of API-String "BBAPIX64"
#endif

// Beckhoff BIOS API call function
// BIOS API is called as stdcall (standard windows calling convention)
// Calling convention in Linux is cdecl so it has to be rebuild using assembler
#ifdef __i386__
static unsigned int bbapi_call(struct bbapi_object *const bbapi,
							  const struct bbapi_struct *const cmd,
							  unsigned int *bytes_written)
	{
		unsigned int ret;
		__asm__("push %0" : : "r" (bytes_written));
		__asm__("push %0" : : "r" (cmd->nOutBufferSize));
		__asm__("push %0" : : "r" (bbapi->out));
		__asm__("push %0" : : "r" (cmd->nInBufferSize));
		__asm__("push %0" : : "r" (bbapi->in));
		__asm__("push %0" : : "r" (cmd->nIndexOffset));
		__asm__("push %0" : : "r" (cmd->nIndexGroup));
		__asm__("call *%0" : : "r" (bbapi->entry));
		__asm__("mov %%eax, %0" :"=m" (ret): );
		return ret;
	}
#endif

#ifdef __x86_64__
// Currently not yet implemented
#endif


/** bbapi_find_bios() - Find BIOS and copy it into RAM
 * @bbapi: pointer to a not initialized bbapi_object
 *
 * If successful bbapi->memory and bbapi->entry point to the bios in RAM
 *
 * Return: 0 if the bios was successfuly copied into RAM
 */
static int bbapi_find_bios(struct bbapi_object *bbapi)
{
	// Try to remap IO Memory to search the BIOS API in the memory
	void __iomem *const start = ioremap(BBIOSAPI_SIGNATURE_PHYS_START_ADDR, BBIOSAPI_SIGNATURE_SEARCH_AREA);
	void __iomem *pos = start;
	const void __iomem *const end = pos + BBIOSAPI_SIGNATURE_SEARCH_AREA;
	if(start == NULL) {
		printk(KERN_ERR "Beckhoff BIOS API: Mapping Memory Search area for Beckhoff BIOS API failed\n");
		return -1;
	}

	// Search through the remapped memory and look for the BIOS API String
	for (;pos<end; pos+=0x10)	//Aligned Search 0x10
	{
		// Read 4 Bytes of memory using ioread and compare it with the low bytes of the BIOS API String
		const uint32_t low = ioread32(pos);
		const uint32_t high = ioread32(pos+4);
		if ((low == BBIOSAPI_SEARCHBSTR_LOW) && (high == BBIOSAPI_SEARCHBSTR_HIGH)) {
			// Set the BIOS API offset which is stored in 0x08
			const unsigned int offset = ioread32(pos+8);

			// Try to allocate memory in the kernel module to copy the BIOS API
			// Memory ha to be marked executable (PAGE_KERNEL_EXEC) otherwise you may get an exception (No Execute Bit)
			bbapi->memory = __vmalloc(offset + 4096,GFP_KERNEL, PAGE_KERNEL_EXEC);
			if (bbapi->memory == NULL) {
				pr_info("__vmalloc for Beckhoff BIOS API failed\n");
				break;
			}
			// copy BIOS API from SPI Flash into RAM to decrease performance impact on realtime applications
			memcpy_fromio(bbapi->memory, pos, offset + 4096);
			bbapi->entry = bbapi->memory + offset;
			iounmap(start);
			pr_info("found and copied to: %p entry %p\n", bbapi->memory, bbapi->entry);
			return 0;
		}
	}
	iounmap(start);
	return -1;
}

/**
 * You have to hold the lock on bbapi->mutex when calling this function!!!
 */
static int bbapi_ioctl_mutexed(struct bbapi_object *const bbapi, const struct bbapi_struct *const cmd)
{
	unsigned int bytes_written;
	if (cmd->nInBufferSize > sizeof(bbapi->in)) {
		return -EINVAL;
	}
	if (cmd->nOutBufferSize > sizeof(bbapi->out)) {
		return -EINVAL;
	}

	// BIOS can operate on kernel space buffers only -> make a temporary copy
	if (copy_from_user(bbapi->in, cmd->pInBuffer, cmd->nInBufferSize)) {
		printk(KERN_ERR "Beckhoff BIOS API: copy_from_user failed\n");
		return -EFAULT;
	}

	// Call the BIOS API
	if (bbapi_call(bbapi, cmd, &bytes_written)) {
		printk(KERN_ERR "Beckhoff BIOS API: ERROR\n");
		return -EIO;
	}

	// Copy the BIOS output to the output buffer in user space
	if (copy_to_user(cmd->pOutBuffer, bbapi->out, bytes_written))
	{
		printk(KERN_ERR "Beckhoff BIOS API: copy to user failed\n");
		return -EFAULT;
	}
	return 0;
}

// IOCTL function which is called from user space to access BBAPI functions
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int bbapi_ioctl(struct inode *i, struct file *f, unsigned int cmd, unsigned long arg)
#else
static long bbapi_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
#endif
{
	struct bbapi_struct bbstruct;
	int result = -EINVAL;

	// Check if IOCTL CMD matches BBAPI Driver Command
	if (cmd != BBAPI_CMD)
	{
		printk(KERN_ERR "Beckhoff BIOS API: Wrong Command\n");
		return -EINVAL;
	}

	// Copy data (BBAPI struct) from User Space to Kernel Module - if it fails, return error
	if (copy_from_user(&bbstruct, (const void __user *) arg, sizeof(bbstruct)))
	{
		printk(KERN_ERR "Beckhoff BIOS API: copy_from_user failed\n");
		return -EINVAL;
	}

	mutex_lock(&mut);
	result = bbapi_ioctl_mutexed(&g_bbapi, &bbstruct);
	mutex_unlock(&mut);
	return result;
}

/* Declare file operation function in struct */
static struct file_operations file_ops =
{
	.owner = THIS_MODULE,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
    .ioctl = bbapi_ioctl
#else
    .unlocked_ioctl = bbapi_ioctl
#endif
};


static int __init bbapi_init_module(void)
{
	if (bbapi_find_bios(&g_bbapi))  {
		pr_info("BIOS API not available on this System\n");
		return -1;
	}

	// ToDo: Implement OS Function Pointer for different Functions (e.g. Read MSR)

	//Allocation of character driver numbers
	return simple_cdev_init(&g_bbapi.dev, "chardev", "BBAPI", &file_ops);
}

static void __exit bbapi_exit(void) /* Destructor */
{
	if (g_bbapi.memory != NULL) vfree(g_bbapi.memory);
	simple_cdev_remove(&g_bbapi.dev);
    printk(KERN_INFO "Beckhoff BIOS API: BBAPI unregistered\n");
}

module_init(bbapi_init_module);
module_exit(bbapi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heiko Wilke");
MODULE_DESCRIPTION("Beckhoff BIOS API Driver");
MODULE_VERSION("1.1");
