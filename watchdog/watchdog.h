/**
    Watchdog driver using the Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2014  Beckhoff Automation GmbH

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

#ifndef __WATCHDOG_H_
#define __WATCHDOG_H_

#define DRV_VERSION      "0.1"
#define DRV_DESCRIPTION  "Beckhoff BIOS API watchdog driver"

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#endif /* #ifndef __WATCHDOG_H_ */
