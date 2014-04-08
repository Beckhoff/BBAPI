/*
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
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/msr.h>

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/* Global Variables */
static dev_t first; 				// First device number
static struct cdev char_dev; 		// Character device structure
static struct class *devclass; 		// Device class
static void *bbapi_memory;					// Pointer for BBAPI Memory area
static void *bbapi_entryPtr;				// Pointer for BBAPI entry Point
static unsigned int BytesReturned;  // Variable for number of returned Bytes by BBAPI function
static DEFINE_MUTEX(mut);					// Mutex for exclusive write access


/* BBAPI specific Variables */
static const unsigned long BBIOSAPI_SIGNATURE_PHYS_START_ADDR = 0xFFE00000;	// Defining the Physical start address for the search
static const unsigned long BBIOSAPI_SIGNATURE_SEARCH_AREA = 0x1FFFFF;	// Defining the Memory search area size
#define BBAPI_CMD							0x5000		// BIOS API Command number for IOCTL call

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
static unsigned int bbapi_call(unsigned int nIndexGroup,
							  unsigned int nIndexOffset,
							  void* pInBuffer,
							  unsigned int nInBufferSize,
							  void* pOutBuffer,
							  unsigned int nOutBufferSize,
							  unsigned int *pBytesReturned,
							  void* pEntry)
	{
		unsigned int ret;
		__asm__("push %0" : : "r" (pBytesReturned));
		__asm__("push %0" : : "r" (nOutBufferSize));
		__asm__("push %0" : : "r" (pOutBuffer));
		__asm__("push %0" : : "r" (nInBufferSize));
		__asm__("push %0" : : "r" (pInBuffer));
		__asm__("push %0" : : "r" (nIndexOffset));
		__asm__("push %0" : : "r" (nIndexGroup));
		__asm__("call *%0" : : "r" (pEntry));
		__asm__("mov %%eax, %0" :"=m" (ret): );
		return ret;
	}
#endif

#ifdef __x86_64__
// Currently not yet implemented
#endif


// Init function for BBAPI
static int init_bbapi(void)
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
			bbapi_memory = __vmalloc(offset + 4096,GFP_KERNEL, PAGE_KERNEL_EXEC);
			if (bbapi_memory == NULL) {
				pr_info("__vmalloc for Beckhoff BIOS API failed\n");
				break;
			}
			// copy BIOS API from SPI Flash into RAM to decrease performance impact on realtime applications
			memcpy_fromio(bbapi_memory, pos, offset + 4096);
			bbapi_entryPtr = bbapi_memory + offset;
			iounmap(start);
			pr_info("found and copied to: %p entry %p\n", bbapi_memory, bbapi_entryPtr);
			return 0;
		}
	}
	iounmap(start);
	return -1;
}

// IOCTL function which is called from user space to access BBAPI functions
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int bbapi_ioctl(struct inode *i, struct file *f, unsigned int cmd, unsigned long arg)
#else
static long bbapi_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
#endif
{
	struct bbapi_struct {
		unsigned int nIndexGroup;
		unsigned int nIndexOffset;
		void __user* pInBuffer;
		unsigned int nInBufferSize;
		void __user* pOutBuffer;
		unsigned int nOutBufferSize;
	} bbstruct;
	void *pInBuffer = NULL;			// Local Kernel Pointer to InBuffer
	void *pOutBuffer = NULL;		// Local Kernel Pointer to OutBuffer
	unsigned int nOutBufferSize =0; // Local OutBufferSize

	// Check if IOCTL CMD matches BBAPI Driver Command
	if (cmd != BBAPI_CMD)
	{
		printk(KERN_ERR "Beckhoff BIOS API: Wrong Command\n");
		return -EINVAL;
	}
	
	// Mutex to protect critical section against concurrent access
	mutex_lock(&mut);
	// Copy data (BBAPI struct) from User Space to Kernel Module - if it fails, return error
	if (copy_from_user(&bbstruct, (const void __user *) arg, sizeof(bbstruct)))
	{
			printk(KERN_ERR "Beckhoff BIOS API: copy_from_user failed\n");
			mutex_unlock(&mut);
			return -EINVAL;
	}
	
	// Copy InBuffer data from user space to Kernel Module - if it fails, return error
	if (bbstruct.nInBufferSize>0)
	{
		if ((pInBuffer = kmalloc(bbstruct.nInBufferSize, GFP_KERNEL)) == NULL)
		{
			printk(KERN_ERR "Beckhoff BIOS API: Kmalloc failed\n");
			mutex_unlock(&mut);
			return -EINVAL;
		}
		if (copy_from_user(pInBuffer, bbstruct.pInBuffer, bbstruct.nInBufferSize))
		{
			printk(KERN_ERR "Beckhoff BIOS API: copy_from_user failed\n");
			mutex_unlock(&mut);
			return -EINVAL;
		}
	}
	// Allocate memory for OutBuffer
	if (bbstruct.nOutBufferSize>0)
	{
		nOutBufferSize = bbstruct.nOutBufferSize;
		if ((pOutBuffer = kmalloc(bbstruct.nOutBufferSize, GFP_KERNEL)) == NULL)
		{
			printk(KERN_ERR "Beckhoff BIOS API: Kmalloc failed\n");
			mutex_unlock(&mut);
			return -EINVAL;
		}
	}
	
	// Call the BIOS API
	//printk(KERN_INFO "Beckhoff BIOS API: Call API with IndexGroup: %X  IndexOffset: %X\n", bbstruct.nIndexGroup, bbstruct.nIndexOffset);
	if (bbapi_call(bbstruct.nIndexGroup, bbstruct.nIndexOffset, pInBuffer, bbstruct.nInBufferSize, pOutBuffer, bbstruct.nOutBufferSize, &BytesReturned, bbapi_entryPtr)!=0)
	{
		printk(KERN_ERR "Beckhoff BIOS API: ERROR\n");
		mutex_unlock(&mut);
		return -EINVAL;
	}
	
	// Copy OutBuffer to User Space
	if ((nOutBufferSize==BytesReturned) > 0)
	{
		if (copy_to_user(bbstruct.pOutBuffer, pOutBuffer, nOutBufferSize))
		{
			printk(KERN_ERR "Beckhoff BIOS API: copy to user failed\n");
			mutex_unlock(&mut);
			return -EINVAL;		
		}
	}
	
	// Free Allocated Kernel Memory
	if (pOutBuffer != NULL) kfree(pOutBuffer);
	if (pInBuffer != NULL) kfree(pInBuffer); 
	// Release Mutex
	mutex_unlock(&mut);
	return 0;
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
	// Init BBAPI
	if (init_bbapi() < 0) 
	{
		printk(KERN_ERR "Beckhoff BIOS API: BIOS API not available on this System\n");
		return -1;
	}
	
	// ToDo: Implement OS Function Pointer for different Functions (e.g. Read MSR)
	
	//Allocation of character driver numbers
	if (alloc_chrdev_region(&first, 0, 1, "BBAPI")<0)
	{
		return -1;
	}
	printk(KERN_INFO "Beckhoff BIOS API: Character Device allocated <Major, Minor> <%d, %d>\n", MAJOR(first), MINOR(first));
	
	//Create device class
	if ((devclass = class_create(THIS_MODULE, "chardrv")) == NULL)
	{
		pr_warn("class_create() failed!\n");
		unregister_chrdev_region(first, 1);
		return -1;
    }
    
    //Create device File
    if (device_create(devclass, NULL, first, NULL, "BBAPI") == NULL)
    {
		pr_warn("device_create() failed!\n");
		class_destroy(devclass);
		unregister_chrdev_region(first, 1);
		return -1;
    }
    
    //Init Device File
    cdev_init(&char_dev, &file_ops);
    if (cdev_add(&char_dev, first, 1) == -1)
    {
		pr_warn("cdev_add() failed!\n");
		device_destroy(devclass, first);
		class_destroy(devclass);
		unregister_chrdev_region(first, 1);
		return -1;
	}

    return 0;
}
 
static void __exit bbapi_exit(void) /* Destructor */
{
	if (bbapi_memory != NULL) vfree(bbapi_memory);
	cdev_del(&char_dev);
	device_destroy(devclass, first);
	class_destroy(devclass);
	unregister_chrdev_region(first, 1);

    printk(KERN_INFO "Beckhoff BIOS API: BBAPI unregistered\n");
}
 
module_init(bbapi_init_module);
module_exit(bbapi_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heiko Wilke");
MODULE_DESCRIPTION("Beckhoff BIOS API Driver");
MODULE_VERSION("1.1");
