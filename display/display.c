// SPDX-License-Identifier: MIT
/**
    Text display driver using the Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2014-2018 Beckhoff Automation GmbH & Co. KG
*/

#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/kernel.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,12,0))
#include <asm/uaccess.h>
#else
#include <linux/uaccess.h>
#endif

#include "../api.h"
#include "../TcBaDevDef.h"

#define DRV_VERSION      "0.3"
#define DRV_DESCRIPTION  "Beckhoff BIOS API text display driver"

struct display_buffer {
	size_t row;
	size_t col;
};

#define NUM_ROWS ((size_t)2)
#define NUM_COLS  ((size_t)16)
static char g_fb[NUM_ROWS][NUM_COLS + 1];
static DEFINE_MUTEX(g_mutex);

static void fb_init(void)
{
	mutex_lock(&g_mutex);
	memset(g_fb, ' ', sizeof(g_fb));
	g_fb[0][NUM_COLS] = '\0';
	g_fb[1][NUM_COLS] = '\0';
	mutex_unlock(&g_mutex);
}

static int display_open(struct inode *const i, struct file *const f)
{
	f->private_data = kzalloc(sizeof(struct display_buffer), GFP_KERNEL);
	return NULL == f->private_data;
}

static int display_release(struct inode *const i, struct file *const f)
{
	kfree(f->private_data);
	return 0;
}

static ssize_t display_write(struct file *const f, const char __user * buf,
			     size_t len, loff_t * off)
{
	struct display_buffer *const db = f->private_data;
	size_t pos = 0;
	char next;

	while (pos < len) {
		get_user(next, buf + pos);
		if (isprint(next) && db->row < NUM_ROWS) {
			g_fb[db->row][db->col] = next;
			db->col++;
		} else {
			switch (next) {
			case '\b':	/* move cursor to previous character */
				if (db->col) {
					db->col--;
				} else if (db->row) {
					db->col = NUM_COLS - 1;
					db->row--;
				}
				break;
			case '\t':	/* move cursor to next character */
				db->col++;
				break;
			case '\n':	/* move cursor to start of next row */
				db->col = 0;
				db->row++;
				break;
			case '\f':	/* clear screen and move cursor to column 0 of row 0 */
				db->col = 0;
				db->row = 0;
				fb_init();
				break;
			case '\r':	/* move cursor to first character in row */
				db->col = 0;
				break;
			case '\021':	/* enable backlight */
				{
					u8 enable = 0xff;
					bbapi_write(BIOSIGRP_CXPWRSUPP,
						    BIOSIOFFS_CXPWRSUPP_ENABLEBACKLIGHT,
						    &enable, sizeof(enable));
					break;
				}
			case '\023':	/* disable backlight */
				{
					u8 disable = 0;
					bbapi_write(BIOSIGRP_CXPWRSUPP,
						    BIOSIOFFS_CXPWRSUPP_ENABLEBACKLIGHT,
						    &disable, sizeof(disable));
					break;
				}
			default:
				break;
			}
		}

		/* calculate new positions */
		if (db->col >= NUM_COLS) {
			db->col = 0;
			db->row = min(NUM_ROWS, db->row + 1);
		}
		++pos;
	}

	bbapi_write(BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_DISPLAYLINE1,
		    g_fb[0], sizeof(g_fb[0]));
	bbapi_write(BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_DISPLAYLINE2,
		    g_fb[1], sizeof(g_fb[0]));
	return len;
}

static struct file_operations display_ops = {
	.owner = THIS_MODULE,
	.open = display_open,
	.release = display_release,
	.write = display_write,
};

static struct miscdevice display_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cx_display",
	.fops = &display_ops,
};

static int __init bbapi_display_init_module(void)
{
	pr_info("%s, %s\n", DRV_DESCRIPTION, DRV_VERSION);
	fb_init();
	return misc_register(&display_device);
}

static void __exit bbapi_display_exit(void)
{
	misc_deregister(&display_device);
	pr_info("Text display unregistered\n");
}

module_init(bbapi_display_init_module);
module_exit(bbapi_display_exit);

MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR("Patrick Bruenn <p.bruenn@beckhoff.com>");
MODULE_LICENSE("GPL and additional rights");
MODULE_VERSION(DRV_VERSION);
