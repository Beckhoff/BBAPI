/**
    Text display driver using the Beckhoff BIOS API
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

#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/kernel.h>

#include "../api.h"
#include "../TcBaDevDef_gpl.h"
#include "display.h"

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
					static const u8 enable = 0xff;
					bbapi_write(0x00009000, 0x00000060,
						    &enable, sizeof(enable));
					break;
				}
			case '\023':	/* disable backlight */
				{
					static const u8 disable = 0;
					bbapi_write(0x00009000, 0x00000060,
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

	bbapi_write(0x00009000, 0x00000061, g_fb[0], sizeof(g_fb[0]));
	bbapi_write(0x00009000, 0x00000062, g_fb[1], sizeof(g_fb[0]));
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
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
