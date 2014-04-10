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

#ifndef __API_H_
#define __API_H_

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/mutex.h>
#include "simple_cdev.h"

/* BBAPI specific constants */
#define BBIOSAPI_SIGNATURE_PHYS_START_ADDR 0xFFE00000	// Defining the Physical start address for the search
#define BBIOSAPI_SIGNATURE_SEARCH_AREA 0x1FFFFF	// Defining the Memory search area size
#define BBAPI_CMD							0x5000	// BIOS API Command number for IOCTL call
#define BBAPI_BUFFER_SIZE 256

struct bbapi_struct {
	unsigned int nIndexGroup;
	unsigned int nIndexOffset;
	void __user *pInBuffer;
	unsigned int nInBufferSize;
	void __user *pOutBuffer;
	unsigned int nOutBufferSize;
};

/**
 * struct bbapi_object - manage access to Beckhoff BIOS functions
 * @memory: pointer to a BIOS copy in RAM
 * @entry: function pointer to the BIOS API function in RAM
 * @in: buffer to exchange data between user space and BIOS
 * @out: buffer to exchange data between BIOS and user space
 * @dev: meta data for the character device interface
 *
 * The size of the output buffer should be large enough to satisfy the
 * largest BIOS command. Right now this is: BIOSIOFFS_UEEPROM_READ.
 */
struct bbapi_object {
	void *memory;
	void *entry;
	char in[BBAPI_BUFFER_SIZE];
	char out[BBAPI_BUFFER_SIZE];
	struct simple_cdev dev;
	struct mutex mutex;
};
#endif /* #ifndef __API_H_ */
