// SPDX-License-Identifier: MIT
/**
    Compile test to ensure __vmalloc_node_range is a symbol and its arguments
    Author: 	Bjarne von Horn <vh@igh.de>
    Copyright (C) 2024 IgH GmbH
*/

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)

#include "api_internals.h"
#include <linux/vmalloc.h>


__attribute__((__used__)) fcn_vmalloc_node_range_t fcn_vmalloc_node_range_test = &__vmalloc_node_range;

#endif
