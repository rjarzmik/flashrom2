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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <debug.h>
#include <list.h>
#include <programmer.h>
#include <params.h>

LIST_HEAD(programmers);

int register_programmer(struct programmer *mst)
{
	list_add(&mst->list, &programmers);

	return 0;
}

void print_available_programmers(void)
{
	struct programmer *mst;
	list_for_each_entry(mst, &programmers, list)
		pr_warn(" - %s : %s\n", mst->name, mst->desc);
}

char *extract_programmer_param(const char *programmer_args,
			       const char *param_name)
{
	return extract_param(programmer_args, param_name, ",:");
}

char *extract_programmer_name(const char *programmer_args)
{
	return extract_param(programmer_args, "", ",:");
}

void programmer_shutdown(struct context *ctx)
{
	if (ctx->mst && ctx->mst->shutdown)
		ctx->mst->shutdown(ctx->programmer_data);
}
