/*
 * This file is part of the flashrom2 project.
 *
 * Copyright (C) 2015 Robert Jarzmik
 *
 * It is heavily inspired from flashrom project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define DEBUG_MODULE "chip-set"

#include <errno.h>
#include <string.h>

#include <chip.h>
#include <debug.h>

int op_set_chip(struct context *context, char *programmer_args)
{
	int ret;
	struct flashchip *chip;
	const char *chip_name = programmer_args;

	pr_dbg("Setting chip to %s\n", chip_name);
	for_each_chip(chip) {
		if (!strcmp(chip->driver_name, chip_name)) {
			context->chip = chip;
			ret = chip->probe(context, programmer_args);
			if (!ret)
				return -ENODEV;
			else
				return 0;
		}
	}

	pr_err("Chip %s not found in available chips.\n",
	       chip_name);
	return -ENODEV;
}
