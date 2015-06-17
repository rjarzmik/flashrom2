/*
 * This file is part of the flashrom2 project.
 *
 * Copyright (C) 2015 Robert Jarzmik
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

#include <stdio.h>
#include <string.h>

#include <debug.h>
#include <write_strategy.h>

static const struct {
	char *name;
	enum write_strategy strategy;
	const char *comment;
} write_strategies[] = {
	{ "unknown", UNKNOWN },
	{ "wipe_if_changes", WIPE_IF_CHANGES,
	"erase a sector/block/chip only if the new content from the file is different => poor flashing performance, but flash wears slower"},
	{ "wipe_by_biggest_erases", WIPE_BY_BIGGEST_ERASES,
	"unconditionnaly try to erase a block where a write will be done => good flashing performance, but flash wears quicker"},
	{ NULL, 0 }
};

void print_available_write_strategies(void)
{
	int i;

	for (i = 0; write_strategies[i].name; i++)
		if (write_strategies[i].strategy != UNKNOWN)
			pr_warn(" - %s : %s\n", write_strategies[i].name,
				write_strategies[i].comment);
	pr_warn("\n");
}

enum write_strategy parse_write_strategy(const char *strategy)
{
	int i;

	for (i = 0; write_strategies[i].name; i++)
		if (!strcmp(strategy, write_strategies[i].name))
			return write_strategies[i].strategy;

	pr_warn("Error: unknown write strategy %s\n", strategy);
	print_available_write_strategies();
	return UNKNOWN;
}

char *get_write_strategy_name(enum write_strategy strategy)
{
	return write_strategies[strategy].name;
}
