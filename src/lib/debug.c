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
#include <debug.h>
#include <stdarg.h>
#include <stdio.h>

int debug_level = MSG_INFO;

void print_leveled(const char *debug_module, enum msglevel level,
		   const char *format, ...)
{
	va_list ap;

	if (level > debug_level)
		return;

	if (*debug_module)
		printf("[%s]", debug_module);
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
}
