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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <debug.h>
#include <programmer.h>
#include <operation.h>
#include <operations.h>
#include <write_strategy.h>

static LIST_HEAD(operations);

static int programmer_chip_available(struct context *ctx)
{
	if (!ctx->chip) {
		pr_err("No chip found, not performing operation.\n");
		return 0;
	}
	if (!ctx->mst) {
		pr_err("No programmer found, not performing operation.\n");
		return 0;
	}
	return 1;
}

static char *get_operation_desc(struct operation *op)
{
	static char msg[80 + PATH_MAX];

	switch (op->op) {
	case READ:
		sprintf(msg, "read chip into %s", op->arg.filename);
		break;
	case WRITE:
		sprintf(msg, "write chip from %s", op->arg.filename);
		break;
	case VERIFY:
		sprintf(msg, "verify chip against %s", op->arg.filename);
		break;
	case SET_WRITE_STRATEGY:
		sprintf(msg, "set write strategy to %s",
			get_write_strategy_name(op->arg.write_strategy));
		break;
	case SET_CHIP:
		sprintf(msg, "set chip to %s", op->arg.chipname);
		break;
	case SET_PROGRAMMER:
		sprintf(msg, "set programmer to %s", op->arg.programmer);
		break;
	default:
		sprintf(msg, "unknown action");
	}

	return msg;
}

static struct operation *operation_clone(struct operation *op)
{
	struct operation *new;

	new = malloc(sizeof(*new));
	if (!new) {
		pr_warn("%s:%d(): allocation failed\n", __func__, __LINE__);
		exit(1);
	}
	*new = *op;

	return new;
}

void operation_add_tail(struct operation *op)
{
	struct operation *new = operation_clone(op);

	list_add_tail(&new->list, &operations);
}

void operation_add(struct operation *op)
{
	struct operation *new = operation_clone(op);

	list_add(&new->list, &operations);
}

int operations_launch(void)
{
	struct operation *op;
	struct context ctx;
	int num_op = 1, ret = 0;

	list_for_each_entry(op, &operations, list) {
		pr_dbg("Operation %d: %s\n", num_op, get_operation_desc(op));
		switch(op->op) {
		case READ:
			if (programmer_chip_available(&ctx))
				ret = op_read_chip(&ctx, op->arg.filename, 0, 0);
			break;
		case WRITE:
			if (programmer_chip_available(&ctx))
				ret = op_write_chip(&ctx, op->arg.filename, 0, 0);
			break;
		case VERIFY:
			if (programmer_chip_available(&ctx))
				ret = op_verify_chip(&ctx, op->arg.filename, 0, 0);
			break;
		case SET_PROGRAMMER:
			ret = op_set_programmer(&ctx, op->arg.programmer);
			break;
		case SET_CHIP:
			ret = op_set_chip(&ctx, op->arg.chipname);
			break;
		case SET_WRITE_STRATEGY:
			ret = op_set_write_strategy(&ctx, op->arg.write_strategy);
			break;
		default:
			ret = 0;
		}
		if (ret)
			return ret;
		num_op++;
	}

	if (programmer_chip_available(&ctx))
		programmer_shutdown(&ctx);

	return 0;
}
