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

#define DEBUG_MODULE "programmer-automatic"

#include <errno.h>

#include <debug.h>
#include <programmer.h>

static struct programmer automatic;
static int auto_probe(const char *programmer_args, void **data)
{
	struct programmer *programmer;
	struct list_head head;
	int ret;

	for_each_programmer(programmer) {
		pr_dbg("Probing programmer %s\n", programmer->name);
		if (programmer->probe == auto_probe)
			continue;
		ret = programmer->probe(programmer_args, data);
		if (!ret) {
			head = automatic.list;
			automatic = *programmer;
			automatic.list = head;
			return 0;
		}
	}

	return -ENODEV;
}

static struct programmer automatic = {
	.buses_supported = 0,
	.probe = auto_probe,
	.desc = "probes each programmer until one is found",
};

DECLARE_PROGRAMMER(automatic);
