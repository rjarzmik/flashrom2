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
#ifndef _DEBUG
#define _DEBUG

#ifndef DEBUG_MODULE
#define DEBUG_MODULE ""
#endif

#define pr_err(...) print_leveled("", MSG_ERROR, __VA_ARGS__)
#define pr_warn(...) print_leveled("", MSG_WARN, __VA_ARGS__)
#define pr_info(...) print_leveled("", MSG_INFO, __VA_ARGS__)
#define pr_dbg(...) print_leveled(DEBUG_MODULE, MSG_DEBUG, __VA_ARGS__)
#define pr_vdbg(...) print_leveled(DEBUG_MODULE, MSG_VDEBUG, __VA_ARGS__)

#define pr_err_cont(...) print_leveled("", MSG_ERROR, __VA_ARGS__)
#define pr_warn_cont(...) print_leveled("", MSG_WARN, __VA_ARGS__)
#define pr_info_cont(...) print_leveled("", MSG_INFO, __VA_ARGS__)
#define pr_dbg_cont(...) print_leveled("", MSG_DEBUG, __VA_ARGS__)
#define pr_vdbg_cont(...) print_leveled("", MSG_VDEBUG, __VA_ARGS__)

enum msglevel {
	MSG_ERROR = 0,
	MSG_WARN = 1,
	MSG_INFO = 2,
	MSG_DEBUG = 3,
	MSG_VDEBUG = 4,
};

extern int debug_level;

static inline void dbg_level_inc(void)
{
	debug_level++;
}

void print_leveled(const char *debug_module, enum msglevel level,
		   const char *format, ...);

#endif

