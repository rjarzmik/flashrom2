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
#define DEBUG_MODULE "chip-verify"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chip.h>
#include <debug.h>
#include <programmer.h>

int op_verify_chip(struct context *ctx, char *filename,
		   off_t where, size_t len)
{
	FILE *f;
	unsigned char *buf = NULL, *buf_reference = NULL;
	int ret;

	buf_reference = malloc(ctx->chip->total_size_kb * 1024);
	if (!buf_reference)
		return -ENOMEM;
	buf = malloc(ctx->chip->total_size_kb * 1024);
	if (!buf)
		return -ENOMEM;

	f = fopen(filename, "r");
	if (!f) {
		pr_err("Cannot open file %s to verfiy the chip against\n",
		       filename);
		return errno;
	}
	if (!len) {
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		rewind(f);
	}
	ret = fread(buf_reference + where, len, 1, f);
	if (ret < 1) {
		pr_err("Couldn't read %zd bytes from %s\n",
		       len, filename);
		ret = -ENXIO;
		goto err;
	}

	ret = chip_read(ctx, buf, where, len);
	if (ret < len)
		goto err;
	if (!memcmp(buf_reference + where, buf, len)) {
		pr_info("Verification of chip against %s success.\n",
			filename);
	} else {
		pr_info("Verification of chip against %s failure.\n",
			filename);
	}
	ret = 0;

err:
	fclose(f);
	free(buf);
	free(buf_reference);
	return ret;
}
