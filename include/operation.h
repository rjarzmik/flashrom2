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
#ifndef __OPERATION_H__
#define __OPERATION_H__

#include <list.h>

#include <write_strategy.h>

enum operation_type {
	READ = 0,
	WRITE,
	VERIFY,
	SET_WRITE_STRATEGY,
	SET_CHIP,
	SET_PROGRAMMER,
	LAST_OPERATION_TYPE,
};

struct operation {
	enum operation_type op;
	union {
		char *filename;
		enum write_strategy write_strategy;
		char *programmer;
		char *chipname;
	} arg;
	struct list_head list;
};

void operation_add_tail(struct operation *op);
void operation_add(struct operation *op);
int operations_launch(void);

#endif
