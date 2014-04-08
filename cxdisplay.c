/*
    Small command line utility to write to CX2100 Display
    Author: 	Heiko Wilke <h.wilke@beckhoff.com>
    Version: 	1.0
    Copyright (C) 2013  Beckhoff Automation GmbH

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

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

#define FILE_PATH	"/dev/BBAPI" 	// Path to character Device

#define STRING_SIZE 17				// Size of String for Display
#define BBAPI_CMD 0x5000			// BIOS API Command number for IOCTL call

#define CXPWRSUPP_BUTTON_STATE_OFF			0x00
#define CXPWRSUPP_BUTTON_STATE_RIGHT		0x01
#define CXPWRSUPP_BUTTON_STATE_LEFT			0x02
#define CXPWRSUPP_BUTTON_STATE_DOWN			0x04
#define CXPWRSUPP_BUTTON_STATE_UP			0x08
#define CXPWRSUPP_BUTTON_STATE_SELECT		0x10

// BBAPI struct which will be passed to IOCTL by reference
struct bbapi_struct
{
	unsigned long nIndexGroup;
	unsigned long nIndexOffset;
	void* pInBuffer;
	unsigned long nInBufferSize;
	void* pOutBuffer;
	unsigned long nOutBufferSize;
}bbstruct;


int main(int argc, char *argv[])
{
	unsigned char button;
	// Init BBAPI struct with values
	bbstruct.nIndexGroup = 0x9000;			// IndexGroup see BBAPI documentation
	bbstruct.nInBufferSize = STRING_SIZE;	
	bbstruct.nOutBufferSize = 0;
	bbstruct.pOutBuffer = NULL;
	int file = 0;							// file handle
	
	// Check number of arguments
	if ((argc != 3) && (argc !=1))
	{
		printf("Wrong number of Arguments \nUsage: %s -lx 'String'\nx = 1 or 2 for corresponding Display Line\nNo Argument will only show serial number of CX1100\n", argv[0]);
		return -1;
	}
	
	// Open BBAPI File
	if ((file = open(FILE_PATH, O_RDWR)) == 0) 
	{
		printf("Error open file\n");
		return -1;
	}
	
// Write Text to Display

	if(argc ==3) {
		if(strcmp(argv[1],"-l1")==0)
		{
			bbstruct.nIndexOffset = 0x61;
		}
		
		if(strcmp(argv[1],"-l2")==0)
		{
			bbstruct.nIndexOffset = 0x62;
		}
		// Check if string length matches for display
		if (strlen(argv[2]) > STRING_SIZE)
		{
			printf("String too large for Display, max. 17 characters \n");
			return -1;
		}
		// Set pointer for string to input argument
		bbstruct.pInBuffer = argv[2];
		
		// IOCTL Call
		if (ioctl(file, BBAPI_CMD, &bbstruct) == -1)
		{
			printf("IOCTL ERROR\n");
			if (file != 0) close(file);
			return -1;
		}
	}

// Read Button State from Power Supply
	bbstruct.nIndexGroup = 0x9000;
	bbstruct.nIndexOffset = 0x63;
	bbstruct.pInBuffer = NULL;
	bbstruct.nInBufferSize = 0;
	bbstruct.pOutBuffer = &button;
	bbstruct.nOutBufferSize = sizeof(button);
	
// Do not call the functions for CX2100 too frequently because commands are send over SMBus to the Power Supply
// And the SMBus has only a limited bandwidth
// Recommendation: 50ms cycle time for periodic updated of input buttons.
	// IOCTL call
	if (ioctl(file, BBAPI_CMD, &bbstruct) == -1)
	{
		printf("IOCTL ERROR\n");
		if (file != 0) close(file);
		return -1;
	}
	printf("CX1100 Button State: 0x%x\n", button);
		
	if (file != 0) close(file);
	return 0;
}
