/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Carl-Daniel Hailfinger
 * Copyright (C) 2008 coresystems GmbH
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/*
 * Contains the generic SPI framework
 */

#include <stdlib.h>
#include <strings.h>
#include <sys/param.h>
#include <unistd.h>

#include <bus_spi.h>
#include <chip.h>
#include <debug.h>
#include <programmer.h>
#include <spi_programmer.h>
#include <spi_nor.h>

#define programmer_delay(ms) usleep(ms)

int spi_send_command(struct context *flash, unsigned int writecnt,
		     unsigned int readcnt, const unsigned char *writearr,
		     unsigned char *readarr)
{
	return flash->mst->spi.command(flash, writecnt, readcnt, writearr,
				       readarr);
}

int spi_send_multicommand(struct context *flash, struct spi_command *cmds)
{
	return flash->mst->spi.multicommand(flash, cmds);
}

int default_spi_send_command(struct context *flash, unsigned int writecnt,
			     unsigned int readcnt,
			     const unsigned char *writearr,
			     unsigned char *readarr)
{
	struct spi_command cmd[] = {
	{
		.writecnt = writecnt,
		.readcnt = readcnt,
		.writearr = writearr,
		.readarr = readarr,
	}, {
		.writecnt = 0,
		.writearr = NULL,
		.readcnt = 0,
		.readarr = NULL,
	}};

	return spi_send_multicommand(flash, cmd);
}

int default_spi_send_multicommand(struct context *flash,
				  struct spi_command *cmds)
{
	int result = 0;
	for (; (cmds->writecnt || cmds->readcnt) && !result; cmds++) {
		result = spi_send_command(flash, cmds->writecnt, cmds->readcnt,
					  cmds->writearr, cmds->readarr);
	}
	return result;
}

size_t spi_nbyte_read(struct context *flash, off_t address, uint8_t *bytes,
		      size_t len)
{
	const unsigned char cmd[JEDEC_READ_OUTSIZE] = {
		JEDEC_READ,
		(address >> 16) & 0xff,
		(address >> 8) & 0xff,
		(address >> 0) & 0xff,
	};

	/* Send Read */
	return spi_send_command(flash, sizeof(cmd), len, cmd, bytes);
}

/*
 * Read a part of the flash chip.
 * FIXME: Use the chunk code from Michael Karcher instead.
 * Each page is read separately in chunks with a maximum size of chunksize.
 */
size_t spi_read_chunked(struct context *flash, uint8_t *buf,
			off_t start, size_t len,
			size_t chunksize)
{
	int rc = 0;
	unsigned int i, j, starthere, lenhere, toread;
	unsigned int page_size = flash->chip->page_size;

	/* Warning: This loop has a very unusual condition and body.
	 * The loop needs to go through each page with at least one affected
	 * byte. The lowest page number is (start / page_size) since that
	 * division rounds down. The highest page number we want is the page
	 * where the last byte of the range lives. That last byte has the
	 * address (start + len - 1), thus the highest page number is
	 * (start + len - 1) / page_size. Since we want to include that last
	 * page as well, the loop condition uses <=.
	 */
	for (i = start / page_size; i <= (start + len - 1) / page_size; i++) {
		/* Byte position of the first byte in the range in this page. */
		/* starthere is an offset to the base address of the chip. */
		starthere = MAX(start, i * page_size);
		/* Length of bytes in the range in this page. */
		lenhere = MIN(start + len, (i + 1) * page_size) - starthere;
		for (j = 0; j < lenhere; j += chunksize) {
			toread = MIN(chunksize, lenhere - j);
			rc = spi_nbyte_read(flash, starthere + j, buf + starthere - start + j, toread);
			if (rc)
				break;
		}
		if (rc)
			break;
	}

	return rc;
}

/*
 * Write a part of the flash chip.
 * FIXME: Use the chunk code from Michael Karcher instead.
 * Each page is written separately in chunks with a maximum size of chunksize.
 */
size_t spi_write_chunked(struct context *flash, const uint8_t *buf,
			 off_t start, size_t len,
			 size_t chunksize)
{
	int rc = 0;
	unsigned int i, j, starthere, lenhere, towrite;
	/* FIXME: page_size is the wrong variable. We need max_writechunk_size
	 * in struct context to do this properly. All chips using
	 * spi_chip_write_256 have page_size set to max_writechunk_size, so
	 * we're OK for now.
	 */
	unsigned int page_size = flash->chip->page_size;

	/* Warning: This loop has a very unusual condition and body.
	 * The loop needs to go through each page with at least one affected
	 * byte. The lowest page number is (start / page_size) since that
	 * division rounds down. The highest page number we want is the page
	 * where the last byte of the range lives. That last byte has the
	 * address (start + len - 1), thus the highest page number is
	 * (start + len - 1) / page_size. Since we want to include that last
	 * page as well, the loop condition uses <=.
	 */
	for (i = start / page_size; i <= (start + len - 1) / page_size; i++) {
		/* Byte position of the first byte in the range in this page. */
		/* starthere is an offset to the base address of the chip. */
		starthere = MAX(start, i * page_size);
		/* Length of bytes in the range in this page. */
		lenhere = MIN(start + len, (i + 1) * page_size) - starthere;
		for (j = 0; j < lenhere; j += chunksize) {
			towrite = MIN(chunksize, lenhere - j);
			rc = spi_nbyte_program(flash, starthere + j, buf + starthere - start + j, towrite);
			if (rc)
				break;
			while (spi_read_status_register(flash) & SPI_SR_WIP)
				programmer_delay(10);
		}
		if (rc)
			break;
	}

	return rc;
}

size_t default_spi_read(struct context *flash, uint8_t *buf, off_t start,
			size_t len)
{
	unsigned int max_data = flash->mst->spi.max_data_read;
	if (max_data == MAX_DATA_UNSPECIFIED) {
		pr_err("%s called, but SPI read chunk size not defined "
			 "on this hardware. Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		return 1;
	}
	return spi_read_chunked(flash, buf, start, len, max_data);
}

size_t default_spi_write_256(struct context *flash, const uint8_t *buf, off_t start, size_t len)
{
	unsigned int max_data = flash->mst->spi.max_data_write;
	if (max_data == MAX_DATA_UNSPECIFIED) {
		pr_err("%s called, but SPI write chunk size not defined "
			 "on this hardware. Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		return 1;
	}
	return spi_write_chunked(flash, buf, start, len, max_data);
}

size_t spi_chip_read(struct context *flash, uint8_t *buf, off_t start,
		  size_t len)
{
	unsigned int addrbase = 0;

	if (addrbase + flash->chip->total_size_kb * 1024 > (1 << 24)) {
		pr_err("Flash chip size exceeds the allowed access window. ");
		pr_err("Read will probably fail.\n");
		/* Try to get the best alignment subject to constraints. */
		addrbase = (1 << 24) - flash->chip->total_size_kb * 1024;
	}
	/* Check if alignment is native (at least the largest power of two which
	 * is a factor of the mapped size of the chip).
	 */
	if (ffs(flash->chip->total_size_kb * 1024) > (ffs(addrbase) ? : 33)) {
		pr_err("Flash chip is not aligned natively in the allowed "
			 "access window.\n");
		pr_err("Read will probably return garbage.\n");
	}
	return flash->mst->spi.read(flash, buf, addrbase + start, len);
}

/*
 * Program chip using page (256 bytes) programming.
 * Some SPI masters can't do this, they use single byte programming instead.
 * The redirect to single byte programming is achieved by setting
 * .write_256 = spi_chip_write_1
 */
/* real chunksize is up to 256, logical chunksize is 256 */
size_t spi_chip_write_256(struct context *flash, const uint8_t *buf, off_t start,
		       size_t len)
{
	return flash->mst->spi.write_256(flash, buf, start, len);
}

size_t spi_aai_write(struct context *flash, const uint8_t *buf, off_t start,
		  size_t len)
{
	return flash->mst->spi.write_aai(flash, buf, start, len);
}

int register_spi_programmer(const struct spi_programmer *mst)
{
	struct programmer *rmst;

	rmst = malloc(sizeof(*rmst));
	if (!rmst)
		return 1;

	if (!mst->write_aai || !mst->write_256 || !mst->read || !mst->command ||
	    !mst->multicommand ||
	    ((mst->command == default_spi_send_command) &&
	     (mst->multicommand == default_spi_send_multicommand))) {
		pr_err("%s called with incomplete master definition.\n",
		       __func__);
		return ERROR_FLASHROM_BUG;
	}

	rmst->buses_supported = BUS_SPI;
	rmst->spi = *mst;
	return register_programmer(rmst);
}

uint8_t spi_read_status_register(struct context *flash)
{
	static const unsigned char cmd[JEDEC_RDSR_OUTSIZE] = { JEDEC_RDSR };
	/* FIXME: No workarounds for driver/hardware bugs in generic code. */
	unsigned char readarr[2]; /* JEDEC_RDSR_INSIZE=1 but wbsio needs 2 */
	int ret;

	/* Read Status Register */
	ret = spi_send_command(flash, sizeof(cmd), sizeof(readarr), cmd, readarr);
	if (ret)
		pr_err("RDSR status read failed: %d\n", ret);

	return readarr[0];
}
