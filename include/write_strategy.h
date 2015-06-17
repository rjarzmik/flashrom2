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
#ifndef _WRITE_STRATEGY
#define _WRITE_STRATEGY

enum write_strategy {
	UNKNOWN = 0,
	WIPE_IF_CHANGES,
	WIPE_BY_BIGGEST_ERASES
};

void print_available_write_strategies(void);
enum write_strategy parse_write_strategy(const char *strategy);
char *get_write_strategy_name(enum write_strategy strategy);

#endif
