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
#define DEBUG_MODULE "chip-read"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chip.h>
#include <debug.h>
#include <programmer.h>

int op_read_chip(struct context *ctx, char *filename,
		 off_t where, size_t len)
{
	FILE *f;
	unsigned char *buf;
	int ret = 0;

	if (len == 0)
		len = ctx->chip->total_size_kb * 1024;
	buf = malloc(ctx->chip->total_size_kb * 1024);
	if (!buf)
		return -ENOMEM;
	memset(buf, 0xff, ctx->chip->total_size_kb * 1024);

	f = fopen(filename, "w");
	if (!f) {
		pr_err("Cannot open file %s to read chip into it\n",
		       filename);
		return errno;
	}

	ret = chip_read(ctx, buf, where, len);
	if (ret < 0)
		goto err;
	ret = fwrite(buf, len, 1, f);
	if (ret < 1) {
		pr_err("Couldn't write %zd bytes into %s\n",
		       len, filename);
		ret = -errno;
		goto err;
	}

	ret = 0;
err:
	free(buf);
	fclose(f);
	return ret;
}
