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
#define DEBUG_MODULE "chip-core"

#include <stdio.h>

#include <chip.h>
#include <debug.h>
#include <hexdump.h>
#include <list.h>

LIST_HEAD(chips);

void register_chip(const char *chip_name, struct flashchip *chip)
{
	list_add(&chip->list, &chips);
}

void print_available_chips(void)
{
	struct flashchip *chip;
	list_for_each_entry(chip, &chips, list)
		printf(" - %s: %s %s\n", chip->driver_name, chip->vendor,
		       chip->name);
}

int chip_read(struct context *ctx, unsigned char *buf, off_t start, size_t len)
{
	int ret;

	ret = ctx->chip->read(ctx, buf, start, len);
	pr_dbg("read chip 0x%06x..0x%06x: %d\n", start, start + len, ret);
	hexdump_vdbg("\t\t Buf=[", buf, len, "]\n");

	if (ret < 0)
		pr_err("Couldn't read the %zd bytes from the chip: %d\n",
		       len, ret);

	return ret;
}

int chip_write(struct context *ctx, const unsigned char *buf,
	       off_t start, size_t len)
{
	int ret;

	ret = ctx->chip->write(ctx, buf, start, len);
	pr_dbg("wrote chip 0x%06x..0x%06x: %d\n", start, start + len, ret);
	hexdump_vdbg("\t\t Buf=[", buf, len, "]\n");

	if (ret < len)
		pr_err("Couldn't write the %zd bytes to the chip: %d\n",
		       len, ret);

	return ret;
}
