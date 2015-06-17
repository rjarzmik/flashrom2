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

#ifndef __SPI_PROGRAMMER_H__
#define __SPI_PROGRAMMER_H__

#define kHz (1000)
#define MHz (1000 * 1000)

#include <bus_spi.h>

struct spi_programmer {
	unsigned int max_data_read;
	unsigned int max_data_write;
	int (*command)(struct context *flash, unsigned int writecnt,
		       unsigned int readcnt, const unsigned char *writearr,
		       unsigned char *readarr);
	int (*multicommand)(struct context *flash, struct spi_command *cmds);

	/* Optimized functions for this programmer */
	int (*read)(struct context *flash, uint8_t *buf, off_t start,
		    size_t len);
	int (*write_256)(struct context *flash, const uint8_t *buf,
			 off_t start, size_t len);
	int (*write_aai)(struct context *flash, const uint8_t *buf,
			 off_t start, size_t len);
};

void spi_programmer_extract_params(const char *programmer_args,
				   int *spi_speed_khz, int *voltage_mv);

#endif
