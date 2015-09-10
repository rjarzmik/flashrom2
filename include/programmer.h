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
#ifndef __PROGRAMMER_H__
#define __PROGRAMMER_H__

#include <list.h>

#include <bus.h>
#include <spi_programmer.h>
#include <write_strategy.h>

#define ERROR_FLASHROM_BUG 17
#define programmer_delay(ms) usleep(ms)
#define for_each_programmer(programmer)	\
	list_for_each_entry(programmer, &programmers, list)

struct programmer {
	const char *name;
	const char *desc;
	enum chipbustype buses_supported;
	struct spi_programmer spi;
	struct list_head list;

	int (*probe)(const char *programmer_args, void **pdata);
	void (*shutdown)(void *pdata);
};

struct chip;
struct flashchip;

struct context {
	struct flashchip *chip;
	struct programmer *mst;
	void *programmer_data;
	enum write_strategy write_strategy;

	struct list_head list;
};

extern struct list_head programmers;

char *extract_programmer_name(const char *programmer_args);
char *extract_programmer_param(const char *programmer_args,
			       const char *param_name);
void print_available_programmers(void);
int register_programmer(struct programmer *mst);
void programmer_shutdown(struct context *ctx);

/*
 * Register function for each programmer.
 */
#define DECLARE_PROGRAMMER(prog_name)						\
	static void __attribute__((constructor(210))) init_##prog_name(void)	\
	{									\
		(prog_name).name = #prog_name;					\
		register_programmer(&(prog_name));				\
	}

#endif
