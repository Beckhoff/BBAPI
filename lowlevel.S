#
#    Character Driver for Beckhoff BIOS API
#    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
#    Copyright (C) 2013 - 2014  Beckhoff Automation GmbH
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


.section .text

#if defined(__x86_64__)
.globl reboot
reboot:
		movq $169, %rax                 #sys_reboot
        movq $0xfee1dead, %rdi          #magic_1
        movq $672274793, %rsi           #magic_2
        movq $0x1234567, %rdx           #cmd
        syscall
#endif

.globl __do_nop
__do_nop:
		ret

.globl ExtOsReadMSR
# extern uint32_t ExtOsReadMSR(uint32_t reg, uint32_t *hi, uint32_t *lo);
# RCX:reg, RDX:&hi, R8:&lo
ExtOsReadMSR:
					rdmsr
#if defined(__i386__)
					xor %eax, %eax
#else
					movl %eax, (%r8)
					xor %rax, %rax
					cmp %rax, %rdx				# check %rdx for NULL
					je ExtOsReadMSR_exit
					movl %eax, (%rdx)
#endif
ExtOsReadMSR_exit:
					ret
