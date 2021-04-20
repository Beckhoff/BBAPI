## Beckhoff BIOS API

### Linux Kernel 5.8 and BBAPI
Since Linux Kernel 5.8 it is not possible to dynamically load a Kernel module which allocates executable memory. 
Therefore the BBAPI doesn't work as a loadable Kernel module anymore. 
If you want to use it with Kernel Version>=5.8 you need to compile your own kernel and statically compile the BBAPI into your kernel.

### General information about BBAPI
The “BIOS-API” is a piece of software which is part of the BIOS in Beckhoff industrial motherboards. 
It offers a one-stop solution for communicating with several components on the board, 
such as temperature and voltage sensors, the S-USV microcontroller, the PWRCTRL microcontroller, 
the Watchdog and other components (if installed). 
It also offers access to a small memory area in the EEPROM reserved for user data.
the API is integrated into the BIOS. 
The OS which is running on the board needs to have a special Device Driver installed to access the API functions. 
Through this driver the user software can take advantage of the API functionality.

### BBAPI Driver
The Beckhoff BIOS API linux driver is implemented in two layers.
The kernel module 'bbapi' represents the bottom layer, which communicates
directly to the BIOS. The upper interface of 'bbapi' is based on ioctl's
and IndexGroups and IndexOffsets defined in the "Beckhoff BIOS-API manual".

On top of 'bbapi' a second layer is implemented to provide a linux interface
based on device files. 'bbapi_display' and 'bbapi_wdt' implement this second
layer. Both depend on an installed 'bbapi' module.
'bbapi_display' implements virtual terminal like interface to the CX2100 text display
'bbapi_wdt' implements a common watchdog interface to the CX hw watchdog


### How to build and install the kernel modules
#### Install 'bbapi'

1. cd into bbapi <src_dir>
2. make && make install

#### Install 'bbapi_display'

1. make sure 'bbapi' is already installed
2. cd into <src_dir>/display
3. make && make install

#### Install 'bbapi_power'

1. make sure 'bbapi' is already installed
2. cd into <src_dir>/power
3. make && make install

#### Install 'bbapi_sups'

1. make sure 'bbapi' is already installed
2. cd into <src_dir>/sups
3. make && make install

#### Install 'bbapi_wdt'

1. make sure 'bbapi' is already installed
2. cd into <src_dir>/wdt
3. make && make install


### How to access the bbapi
`/dev/bbapi` is the device file to access the low level BBAPI<br/>
see "Beckhoff BIOS-API manual" and unittest.cpp for more details.

`/dev/cx_display` is the device file to access the CX2100 text display.<br/>
see display_example.cpp for detailed information

`/dev/watchdog` is the device file to access the CX hardware watchdog.<br/>
See https://www.kernel.org/doc/Documentation/watchdog/watchdog-api.txt

`/sys/class/gpio/sups_pwrfail/value` shows the power fail state on devices with S-UPS.<br/>
See scripts/poll_pwrfail.sh for detailed information

### History
See [CHANGES](CHANGES)
