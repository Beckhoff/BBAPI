/**
    Character Driver for Beckhoff BIOS API
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

#ifndef __BBAPI_H_
#define __BBAPI_H_

#ifndef __user
#define __user
#endif

/* BBAPI specific constants */
#define BBIOSAPI_SIGNATURE_PHYS_START_ADDR 0xFFE00000	// Defining the Physical start address for the search
#define BBIOSAPI_SIGNATURE_SEARCH_AREA 0x1FFFFF	// Defining the Memory search area size
#define BBAPI_CMD							0x5000	// BIOS API Command number for IOCTL call
#define BBAPI_BUFFER_SIZE 256

#define FILE_PATH	"/dev/BBAPI" 	// Path to character Device

struct bbapi_struct {
	uint32_t nIndexGroup;
	uint32_t nIndexOffset;
	void __user *pInBuffer;
	uint32_t nInBufferSize;
	void __user *pOutBuffer;
	uint32_t nOutBufferSize;
};

struct bbapi_callback {
	const uint8_t name[8];
	const uint64_t func;
};

extern uint32_t __do_nop(uint32_t reg, uint32_t *hi, uint32_t *lo);
extern uint32_t reboot(uint32_t reg, uint32_t *hi, uint32_t *lo);
extern uint32_t ExtOsReadMSR(uint32_t reg, uint32_t *hi, uint32_t *lo);
#endif /* #ifndef __BBAPI_H_ */
