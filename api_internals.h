// SPDX-License-Identifier: MIT
/**
    Author:     Bjarne von Horn <vh@igh.de>
    Copyright (C) 2024 IgH GmbH
*/
#ifndef __API_INTERNALS_H
#define __API_INTERNALS_H

#include <linux/vmalloc.h>

typedef void *(*fcn_vmalloc_node_range_t)(unsigned long size, unsigned long align,
		unsigned long start, unsigned long end, gfp_t gfp_mask,
		pgprot_t prot, unsigned long vm_flags, int node,
		const void *caller);

#endif

