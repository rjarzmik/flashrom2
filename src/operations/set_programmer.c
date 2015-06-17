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

#define DEBUG_MODULE "set-programmer"

#include <errno.h>
#include <string.h>

#include <debug.h>
#include <programmer.h>

int op_set_programmer(struct context *context, char *programmer_args)
{
	int ret;
	struct programmer *programmer;
	const char *programmer_name = extract_programmer_name(programmer_args);
	void *pdata;

	pr_dbg("Setting programmer to %s\n", programmer_args);
	for_each_programmer(programmer) {
		if (!strcmp(programmer->name, programmer_name)) {
			ret = programmer->probe(programmer_args, &pdata);
			if (!ret) {
				context->mst = programmer;
				context->programmer_data = pdata;
			}
			return ret;
		}
	}

	pr_err("Programmer %s not found in available programmers.\n",
	       programmer_name);
	return -ENODEV;
}
