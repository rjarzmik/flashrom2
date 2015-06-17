/*
 * This file is part of the flashrom2 project.
 *
 * It is inspired by dediprog.c from flashrom project.
 * Copyright (C) 2010 Carl-Daniel Hailfinger
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

#define DEBUG_MODULE "dummy_spi"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <bus_spi.h>
#include <chip.h>
#include <debug.h>
#include <programmer.h>
#include <spi_programmer.h>

static int dummy_spi_send_command(struct context *ctxt,
				  unsigned int writecnt,
				  unsigned int readcnt,
				  const unsigned char *writearr,
				  unsigned char *readarr)
{
	int i;

	if (readcnt) {
		pr_info("SPI dummy read command of %d bytes\n",
			readcnt);
	} else {
		pr_info("SPI dummy write command:");
		for (i = 0; i < writecnt; i++)
			pr_info_cont(" %02x", writearr[i]);
		pr_info_cont("\n");
	}

	return 0;
}

static int dummy_spi_read(struct context *ctxt, unsigned char *buf,
			  off_t start, size_t len)
{
	pr_info("SPI dummy read: 0x%06x..0x%06x (%d)\n",
		start, start + len, len);

	return len;
}

static int dummy_spi_write(struct context *ctxt, const unsigned char *buf,
			   off_t start, size_t len)
{
	pr_info("SPI dummy write: 0x%06x..0x%06x (%d)\n",
		start, start + len, len);

	return len;
}

int dummy_probe(const char *programmer_args, void **data)
{
	*data = NULL;
	pr_info("Dummy programmer ready: echoeing every SPI access without doing it\n");

	return 0;
}

static struct programmer dummy_spi = {
	.buses_supported = BUS_SPI,
	.spi = {
		.max_data_read = 0,
		.max_data_write = 0,
		.command = dummy_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = dummy_spi_read,
		.write_256 = dummy_spi_write,
	},
	.probe = dummy_probe,
	.desc = "dummy programmer to test spi layers",
};

DECLARE_PROGRAMMER(dummy_spi);
