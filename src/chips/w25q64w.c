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

#include <chip.h>
#include <chip_ids.h>
#include <spi_nor.h>

static struct flashchip w25q64w = {
	.vendor		= "Winbond",
	.name		= "W25Q64.W",
	.bustype	= BUS_SPI,
	.manufacture_id	= WINBOND_NEX_ID,
	.model_id	= WINBOND_NEX_W25Q64_W,
	.total_size_kb	= 8192,
	.page_size	= 256,
	/* OTP: 256B total; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
	/* QPI enable 0x38, disable 0xFF */
	.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
	.probe		= probe_spi_rdid,
	.probe_timing	= TIMING_ZERO,
	.erasers	= {
		{ 0, 4 * 1024, 2048, spi_block_erase_20 },
		{ 0, 32 * 1024, 256, spi_block_erase_52 },
		{ 0, 64 * 1024, 128, spi_block_erase_d8 },
		{ 0, 8 * 1024 * 1024, 1, spi_block_erase_60 },
		{ 0, 8 * 1024 * 1024, 1, spi_block_erase_c7 },
	},
	.write		= spi_chip_write_256,
	.read		= spi_chip_read,
	.voltage	= {1700, 1950}, /* Fast read (0x0B) and multi I/O supported */
};

DECLARE_CHIP(w25q64w);
