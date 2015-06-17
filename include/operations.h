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
#ifndef __OPERATIONS_H__
#define __OPERATIONS_H__

#include <unistd.h>

#include <programmer.h>

int op_set_chip(struct context *context, char *programmer_args);
int op_set_programmer(struct context *context, char *programmer_args);
int op_read_chip(struct context *context, char *filename,
		 off_t where, size_t len);
int op_write_chip(struct context *context, char *filename,
		  off_t where, size_t len);
int op_verify_chip(struct context *context, char *filename,
		   off_t where, size_t len);

static inline int op_set_write_strategy(struct context *context,
					enum write_strategy strategy)
{
	context->write_strategy = strategy;
	return 0;
}

#endif
