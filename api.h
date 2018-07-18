/**
    Character Driver for Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2013 - 2015  Beckhoff Automation GmbH

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

#define BBIOSAPI_SIGNATURE_PHYS_START_ADDR 0xFFE00000	// Defining the Physical start address for the search
#define BBIOSAPI_SIGNATURE_SEARCH_AREA     0x001FFFFF	// Defining the Memory search area size
#define BBAPI_BUFFER_SIZE 256	// maximum size of a buffer shared between user and kernel space

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
	uint8_t *memory;
	void *entry;
	char in[BBAPI_BUFFER_SIZE];
	char out[BBAPI_BUFFER_SIZE];
	struct simple_cdev dev;
	struct mutex mutex;
};

extern unsigned int bbapi_read(uint32_t group, uint32_t offset,
			       void __kernel * out, uint32_t size);

extern unsigned int bbapi_write(uint32_t group, uint32_t offset,
				void __kernel * in, uint32_t size);

extern unsigned int bbapi_rw(uint32_t group, uint32_t offset,
				void __kernel * in, uint32_t size_in,
				void __kernel * out, uint32_t size_out, uint32_t *bytes_written);

extern int bbapi_board_is(const char *boardname);
#endif /* #ifndef __API_H_ */
