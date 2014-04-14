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
.globl reboot
reboot:	movq $169, %rax                 #sys_reboot
        movq $0xfee1dead, %rdi          #magic_1
        movq $672274793, %rsi           #magic_2
        movq $0x1234567, %rdx           #cmd
        syscall

.globl __do_nop
__do_nop:
		ret

.globl ExtOsReadMSR
ExtOsReadMSR:
	movq $1, %rax                   #sys_write
	movq $2, %rdi                   #STDOUT
	movq $MSG_ExtOsReadMSR, %rsi    #buffer
	movq $15, %rdx                  #length
	syscall
	xor %rax, %rax
	ret

.globl ExtOsWriteMSR
ExtOsWriteMSR:
	movq $1, %rax                   #sys_write
	movq $2, %rdi                   #STDOUT
	movq $MSG_ExtOsWriteMSR, %rsi   #buffer
	movq $17, %rdx                  #length
	syscall
	xor %rax, %rax
	ret

	movq $60, %rax          #sys_exit
	xor %rdi, %rdi
	syscall

.section .data
	MSG_ExtOsReadMSR: .ascii "ExtOsReadMSR()\n"
	MSG_ExtOsWriteMSR: .ascii "ExtOsWriteMSR()\n"
