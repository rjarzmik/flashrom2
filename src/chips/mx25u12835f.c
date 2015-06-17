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

static struct flashchip mx25u12835f = {
	.vendor		= "Macronix",
	.name		= "MX25U6435E/F",
	.bustype	= BUS_SPI,
	.manufacture_id	= MACRONIX_ID,
	.model_id	= MACRONIX_MX25U6435E,
	.total_size_kb	= 8192,
	.page_size	= 256,
	/* F model supports SFDP */
	/* OTP: 512B total; enter 0xB1, exit 0xC1 */
	/* QPI enable 0x35, disable 0xF5 (0xFF et al. work too) */
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
	.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
	.voltage	= {1650, 2000},
};

DECLARE_CHIP(mx25u12835f);
