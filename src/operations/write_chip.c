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
#define DEBUG_MODULE "chip-write"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chip.h>
#include <debug.h>
#include <programmer.h>

static int chip_write_by_biggest_erases(struct context *context,
					unsigned char *buf, off_t start,
					size_t len)
{
	int ret;

	pr_info("Erasing zone 0x%06x..0x%06x\n", start, start + len);
	ret = chip_erase(context, start, len, 0, NULL, NULL);
	if (ret) {
		pr_err("Erase of zone 0x%06x..0x%06x failed:%d\n",
		       start, start + len, ret);
		return ret;
	}
	pr_info("Writing zone 0x%06x..0x%06x\n", start, start + len);
	ret = chip_write(context, buf, start, len);
	return ret;
}

static int chip_write_if_changes(struct context *context,
				 unsigned char *buf, off_t start,
				 size_t len, unsigned char *chip_ref)
{
	LIST_HEAD(erases);
	struct block_eraser *eraser;
	off_t end, bstart, copy_skip_first, copy_skip_last;
	size_t blen;
	int ret, skip_it;

	end = start + len;
	ret = compute_list_erases(context->chip->erasers, start, len, &erases);
	if (ret)
		return ret;

	bstart = list_first_entry(&erases, struct block_eraser, list)->start;
	list_for_each_entry(eraser, &erases, list) {
		blen = eraser->size;
		skip_it = (bstart >= start && (bstart + blen < len) &&
			   !memcmp(chip_ref + bstart, buf + bstart, blen));
		pr_vdbg("%s: considering 0x%06x..0x%06x(%d): %s\n",
			__func__, bstart, bstart + blen, blen,
			skip_it ? "won't touch" : "erasing and writing");
		if (skip_it)
			continue;
		ret = eraser->block_erase(context, bstart, blen);
		if (ret < 0) {
			pr_err("Erase of zone 0x%06x..0x%06x failed: %d\n",
			       bstart, bstart + blen, ret);
			break;
		}
		copy_skip_first = (start > bstart ? start - bstart : 0);
		copy_skip_last = (end < bstart + blen ? bstart + blen - end
				  : 0);
		memcpy(chip_ref + bstart + copy_skip_first,
		       buf + (bstart - start),
		       blen - copy_skip_first - copy_skip_last);
		ret = chip_write(context, chip_ref + bstart, bstart, blen);
		if (ret < blen) {
			pr_err("Write of zone 0x%06x..0x%06x failed: %d\n",
			       bstart, bstart + blen, ret);
			break;
		}
		bstart += blen;
	}
	return 0;
}

int op_write_chip(struct context *context, char *filename,
		  off_t where, size_t len)
{
	struct flashchip *chip = context->chip;
	FILE *f;
	unsigned char *buf, *chip_ref;
	size_t ret;

	buf = malloc(chip->total_size_kb * 1024);
	chip_ref = malloc(chip->total_size_kb * 1024);
	memset(buf, 0xff, chip->total_size_kb * 1024);
	memset(chip_ref, 0xff, chip->total_size_kb * 1024);
	if (!buf || !chip_ref)
		return -ENOMEM;

	f = fopen(filename, "r");
	if (!f) {
		pr_err("Cannot open file %s to write the chip\n",
		       filename);
		return errno;
	}
	if (!len) {
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		rewind(f);
	}
	if (len > chip->total_size_kb * 1024) {
		pr_warn("File %s is bigger that chip total size %d, truncating\n",
			filename, chip->total_size_kb * 1024);
		len = chip->total_size_kb * 1024;
	}
	ret = fread(buf, len, 1, f);
	if (ret < 1) {
		pr_err("Couldn't read %zd bytes from %s\n",
		       len, filename);
		ret = -ENXIO;
		goto err;
	}

	switch(context->write_strategy) {
	case WIPE_BY_BIGGEST_ERASES:
		ret = chip_write_by_biggest_erases(context, buf, where, len);
		break;
	case WIPE_IF_CHANGES:
		ret = chip_read(context, chip_ref, 0,
				chip->total_size_kb * 1024);
		if (ret >= chip->total_size_kb * 1024)
			ret = chip_write_if_changes(context, buf, where, len,
						    chip_ref);
		fclose(f);
		break;
	default:
		ret = -ENODEV;
	}

	if (ret < 0) {
		pr_err("Couldn't write the %z bytes into the chip\n",
		       ret);
		goto err;
	}

	ret = 0;
	pr_warn("Write operation succeeded.\n");
err:
	free(buf);
	return ret;
}
