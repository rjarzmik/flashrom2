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

#ifndef __BUS_SPI_H__
#define __BUS_SPI_H__

#include <stdint.h>
#include <unistd.h>

/*
 * When no maximum for read/write, use 0.
 */
#define MAX_DATA_UNSPECIFIED 0

struct context;

struct spi_command {
	unsigned int writecnt;
	unsigned int readcnt;
	const unsigned char *writearr;
	unsigned char *readarr;
};

int spi_send_command(struct context *flash, unsigned int writecnt,
		     unsigned int readcnt, const unsigned char *writearr,
		     unsigned char *readarr);
int spi_send_multicommand(struct context *flash, struct spi_command *cmds);
uint8_t spi_read_status_register(struct context *flash);
size_t spi_nbyte_read(struct context *flash, off_t address, uint8_t *bytes,
		   size_t len);
size_t spi_chip_write_256(struct context *flash, const uint8_t *buf,
			  off_t start, size_t len);
size_t spi_chip_read(struct context *flash, uint8_t *buf, off_t start, size_t len);
size_t spi_read_chunked(struct context *flash, uint8_t *buf, off_t start,
			size_t len, size_t chunksize);
size_t spi_write_chunked(struct context *flash, const uint8_t *buf,
			 off_t start, size_t len, size_t chunksize);

int default_spi_send_multicommand(struct context *flash,
				  struct spi_command *cmds);

#endif
