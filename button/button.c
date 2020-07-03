// SPDX-License-Identifier: MIT
/**
    CX2100 button driver using the Beckhoff BIOS API
    Author: 	Patrick Brünn <p.bruenn@beckhoff.com>
    Copyright (C) 2016 - 2018 Beckhoff Automation GmbH & Co. KG
*/

#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/workqueue.h>

#include "../api.h"
#include "../TcBaDevDef.h"

#define DRV_VERSION      "0.2"
#define DRV_DESCRIPTION  "Beckhoff BIOS API button driver"

#define POLL_TIME ktime_set(0, 125000 * NSEC_PER_USEC)

static struct input_dev *input_dev;
static struct hrtimer poll_timer;

static void button_poll(struct work_struct *work)
{
	u8 btn_state = 0;

	bbapi_read(BIOSIGRP_CXPWRSUPP, BIOSIOFFS_CXPWRSUPP_GETBUTTONSTATE,
		   &btn_state, sizeof(btn_state));
	input_report_abs(input_dev, ABS_X,
			 ((btn_state >> 0) & 1) - ((btn_state >> 1) & 1));
	input_report_abs(input_dev, ABS_Y,
			 ((btn_state >> 3) & 1) - ((btn_state >> 2) & 1));
	input_report_key(input_dev, BTN_0, ((btn_state >> 4) & 1));
	input_sync(input_dev);
}
DECLARE_WORK(work, button_poll);

static enum hrtimer_restart poll_timer_callback(struct hrtimer *timer)
{
	schedule_work(&work);
	hrtimer_forward_now(timer, POLL_TIME);
	return HRTIMER_RESTART;
}

static int button_open(struct input_dev *dev)
{
	hrtimer_init(&poll_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	poll_timer.function = poll_timer_callback;
	hrtimer_start(&poll_timer, POLL_TIME, HRTIMER_MODE_REL);
	return 0;
}

static void button_close(struct input_dev *dev)
{
	hrtimer_cancel(&poll_timer);
}

static int __init button_init(void)
{
	int error;

	input_dev = input_allocate_device();
	if (!input_dev) {
		return -ENOMEM;
	}

	input_dev->name = "Beckhoff CX2100 Buttons";
	input_dev->open = button_open;
	input_dev->close = button_close;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_0)] = BIT_MASK(BTN_0);
	input_set_abs_params(input_dev, ABS_X, -1, 1, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, -1, 1, 0, 0);

	error = input_register_device(input_dev);
	if (error) {
		input_free_device(input_dev);
	}
	return error;
}

static void __exit button_exit(void)
{
	input_unregister_device(input_dev);
}

module_init(button_init);
module_exit(button_exit);

MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR("Patrick Bruenn <p.bruenn@beckhoff.com>");
MODULE_LICENSE("GPL and additional rights");
MODULE_VERSION(DRV_VERSION);
