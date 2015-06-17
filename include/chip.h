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
#ifndef __CHIP_H__
#define __CHIP_H__

#include <stdint.h>
#include <unistd.h>

#include <bus.h>
#include <list.h>
#include <programmer.h>

/* Feature bits used for non-SPI only */
#define FEATURE_REGISTERMAP	(1 << 0)
#define FEATURE_LONG_RESET	(0 << 4)
#define FEATURE_SHORT_RESET	(1 << 4)
#define FEATURE_EITHER_RESET	FEATURE_LONG_RESET
#define FEATURE_RESET_MASK	(FEATURE_LONG_RESET | FEATURE_SHORT_RESET)
#define FEATURE_ADDR_FULL	(0 << 2)
#define FEATURE_ADDR_MASK	(3 << 2)
#define FEATURE_ADDR_2AA	(1 << 2)
#define FEATURE_ADDR_AAA	(2 << 2)
#define FEATURE_ADDR_SHIFTED	(1 << 5)

/* Feature bits used for SPI only */
#define FEATURE_WRSR_EWSR	(1 << 6)
#define FEATURE_WRSR_WREN	(1 << 7)
#define FEATURE_WRSR_EITHER	(FEATURE_WRSR_EWSR | FEATURE_WRSR_WREN)
#define FEATURE_OTP		(1 << 8)
#define FEATURE_QPI		(1 << 9)

/* Timing used in probe routines. ZERO is -2 to differentiate between an unset
 * field and zero delay.
 *
 * SPI devices will always have zero delay and ignore this field.
 */
#define TIMING_FIXME	-1
/* this is intentionally same value as fixme */
#define TIMING_IGNORED	-1
#define TIMING_ZERO	-2

/*
 * How many different contiguous runs of erase blocks with one size each do
 * we have for a given erase function?
 */
#define NUM_ERASEREGIONS 5

/*
 * How many different erase functions do we have per chip?
 * Atmel AT25FS010 has 6 different functions.
 */
#define NUM_ERASEFUNCTIONS 6

typedef int (erasefunc_t)(struct context *flash, off_t addr, size_t blocklen);

struct flashchip {
	char *driver_name;	/* Filled in by register_chip() */

	const char *vendor;
	const char *name;

	enum chipbustype bustype;

	/*
	 * With 32bit manufacture_id and model_id we can cover IDs up to
	 * (including) the 4th bank of JEDEC JEP106W Standard Manufacturer's
	 * Identification code.
	 */
	uint32_t manufacture_id;
	uint32_t model_id;

	/* Total chip size in kilobytes */
	unsigned int total_size_kb;
	/* Chip page size in bytes */
	unsigned int page_size;
	int feature_bits;

	int (*probe)(struct context *ctxt, const char *chip_args);

	/* Delay after "enter/exit ID mode" commands in microseconds.
	 * NB: negative values have special meanings, see TIMING_* below.
	 */
	signed int probe_timing;

	/*
	 * Erase blocks and associated erase function. Any chip erase function
	 * is stored as chip-sized virtual block together with said function.
	 */
	struct block_eraser {
		off_t start;
		size_t size;
		size_t count;
		erasefunc_t *block_erase;
		struct list_head list;
	} erasers[NUM_ERASEFUNCTIONS];

	int (*printlock)(struct context *ctx);
	int (*unlock)(struct context *ctx);
	size_t (*write)(struct context *ctx, const uint8_t *buf, off_t start,
			size_t len);
	size_t (*read)(struct context *ctx, uint8_t *buf, off_t start,
		       size_t len);
	struct voltage {
		uint16_t min;
		uint16_t max;
	} voltage;

	struct list_head list;
};

extern struct list_head chips;

#define for_each_chip(chip)	list_for_each_entry(chip, &chips, list)
void register_chip(const char *chip_name, struct flashchip *chip);
void print_available_chips();

int chip_read(struct context *context, unsigned char *buf,
	      off_t where, size_t len);
int chip_write(struct context *context, const unsigned char *buf,
	       off_t where, size_t len);
int chip_erase(struct context *ctx, off_t start, size_t len,
	       int required_exact_fit, off_t *real_start, off_t *real_len);
int compute_list_erases(struct block_eraser erasers[],
			off_t start, size_t len, struct list_head *ops);

/*
 * Register function for each chip.
 */
#define DECLARE_CHIP(chipname)							\
	static void __attribute__((constructor(210))) init_##chipname(void)	\
	{									\
		(chipname).driver_name = #chipname;				\
		register_chip(#chipname , &(chipname));				\
	}

#endif
