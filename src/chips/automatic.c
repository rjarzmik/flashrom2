/*
 * This file is part of the flashrom2 project.
 *
 * Copyright (C) 2015 Robert Jarzmik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <errno.h>

#include <chip.h>
#include <debug.h>

static struct flashchip automatic;

static int auto_probe(struct context *ctxt, const char *chip_args)
{
	struct flashchip *chip;
	struct list_head head;
	int ret;

	for_each_chip(chip) {
		if (chip->probe == auto_probe)
			continue;
		ctxt->chip = chip;
		ret = chip->probe(ctxt, chip_args);
		pr_dbg("Probing chip %s: %d\n", chip->driver_name, ret);
		if (ret) {
			head = automatic.list;
			automatic = *chip;
			automatic.list = head;
			return 0;
		}
	}

	pr_err("Didn't find automatically any flash chip.\n");
	return -ENODEV;
}

static struct flashchip automatic = {
	.vendor		= "flashrom2",
	.name		= "automatic",
	.bustype	= BUS_SPI,
	.probe		= auto_probe,
};

DECLARE_CHIP(automatic);
