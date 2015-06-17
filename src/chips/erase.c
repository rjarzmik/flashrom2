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
#define DEBUG_MODULE "chip-erase"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <chip.h>
#include <debug.h>
#include <list.h>

#define false 0
#define true 1

struct erase_one {
	off_t where;
	size_t count;
	struct block_eraser *eraser;
	struct list_head list;
};

struct erase_metric {
	int usable;
	size_t pre_erase;
	size_t over_erase;
	off_t start;
	size_t len;
	struct block_eraser *eraser;
};

#define print_chosen_eraser(start, end, eraser)					\
	do {									\
		if (eraser) {							\
			pr_dbg("%s(0x%06x..0x%06x) -> chosen = 0x%06x..0x%06x(%d)\n", \
			       __func__, start, end,				\
			       eraser->start +					\
			       find_erase_block(eraser, start) * eraser->size,	\
			       eraser->start +					\
			       (find_erase_block(eraser, start) + 1) * eraser->size, \
			       eraser->size);					\
		} else {							\
			pr_dbg("%s(0x%06x..0x%06x) -> not found\n", __func__,	\
			       start, end);					\
		}								\
	} while (0);



static int find_erase_block(struct block_eraser *eraser, off_t where)
{
	if (!eraser->block_erase)
		return -1;
	if (where < eraser->start)
		return -1;
	if (where > eraser->start + eraser->size * eraser->count)
		return -1;
	return (where - eraser->start) / eraser->size;
}

static struct block_eraser *
find_smallest_eraser_before_point(struct block_eraser erasers[],
				  off_t start, off_t end)
{
	size_t e_start, e_size;
	struct erase_metric metric[NUM_ERASEFUNCTIONS];
	struct erase_metric *chosen;
	struct block_eraser *eraser;
	int i, blk;

	for (i = 0; i < NUM_ERASEFUNCTIONS; i++) {
		metric[i].usable = false;
		eraser = &erasers[i];
		blk = find_erase_block(eraser, start);
		if (blk < 0)
			continue;

		metric[i].usable = true;
		e_size = eraser->size;
		e_start = eraser->start + blk * e_size;
		metric[i].start = e_start;
		metric[i].pre_erase = start - e_start;
		if (e_start + e_size >= end)
			metric[i].over_erase = (e_start + e_size) - end;
		else
			metric[i].over_erase = 0;
		metric[i].len = e_size;
		metric[i].eraser = eraser;
	}

	chosen = NULL;
	for (i = 0; i < NUM_ERASEFUNCTIONS; i++) {
		if (!metric[i].usable)
			continue;
		if (!chosen)
			chosen = &metric[i];
		pr_vdbg("%s: considering for 0x%06x..0x%06x eraser 0x%06x..0x%06x(%d) : pre_erase=%d, over_erase=%d\n",
			__func__, start, end,
		       metric[i].start, metric[i].start + metric[i].len,
		       metric[i].len, metric[i].pre_erase, metric[i].over_erase);
		if (metric[i].pre_erase + metric[i].over_erase <
		    chosen->pre_erase + chosen->over_erase)
			chosen = &metric[i];
		else if (((metric[i].pre_erase + metric[i].over_erase) ==
			  (chosen->pre_erase + chosen->over_erase)) &&
			 (metric[i].len > chosen->len))
			chosen = &metric[i];
	}

	eraser = chosen ? chosen->eraser : NULL;
	print_chosen_eraser(start, end , eraser);

	return eraser;
}

static struct block_eraser *
find_biggest_eraser_within(struct block_eraser erasers[],
			   off_t start, off_t end)
{
	struct block_eraser *eraser, *chosen = NULL;
	int i, blk;

	for (i = 0; i < NUM_ERASEFUNCTIONS; i++) {
		eraser = &erasers[i];
		blk = find_erase_block(eraser, start);
		if (blk < 0)
			continue;
		pr_vdbg("%s: considering for 0x%06x..0x%06x eraser 0x%06x..0x%06x(%d)\n",
			__func__,
			start, end,
			eraser->start + blk * eraser->size,
			eraser->start + (blk + 1) * eraser->size,
			eraser->size);
		if ((start - eraser->start) % eraser->size)
			continue;
		if ((eraser->start + blk *eraser->size) != start)
			continue;
		if ((eraser->start + (blk + 1) * eraser->size) > end)
			continue;
		if (!chosen) {
			chosen = eraser;
			continue;
		}
		if (chosen->size < eraser->size)
			chosen = eraser;
	}

	print_chosen_eraser(start, end , chosen);
	return chosen;
}

static struct block_eraser *
find_smallest_eraser_after_point(struct block_eraser erasers[],
				 off_t start, off_t end)
{
	struct block_eraser *eraser, *chosen = NULL;
	int i, blk;

	for (i = 0; i < NUM_ERASEFUNCTIONS; i++) {
		eraser = &erasers[i];
		blk = find_erase_block(eraser, start);
		if (blk < 0)
			continue;
		pr_vdbg("%s: considering for 0x%06x..0x%06x eraser 0x%06x..0x%06x(%d)\n",
			__func__,
			start, end, eraser->start + blk * eraser->size,
			eraser->start + (blk + 1) * eraser->size,
			eraser->size);
		if (!((start - eraser->start) % eraser->size))
			continue;
		if (eraser->start + blk * eraser->size <= end)
			continue;
		if (!chosen) {
			chosen = eraser;
			continue;
		}
		if (chosen->size > eraser->size)
			chosen = eraser;
	}
	print_chosen_eraser(start, end , chosen);

	return chosen;
}

static void eraser_add(struct block_eraser *eraser, struct list_head *head)
{
	struct block_eraser *e;

	e = malloc(sizeof(*e));
	if (!e) {
		pr_err("%f(): allocation failed, aborting ...\n");
		exit(1);
	}

	*e = *eraser;
	list_add(&e->list, head);
}

int compute_list_erases(struct block_eraser erasers[],
			off_t start, size_t len, struct list_head *ops)
{
	struct block_eraser *eraser;
	off_t where = start;
	off_t end = where + len;
	LIST_HEAD(erases);

	eraser = find_smallest_eraser_before_point(erasers, where, end);
	if (!eraser)
		return -EINVAL;
	eraser_add(eraser, &erases);
	where = eraser->start + find_erase_block(eraser, where) * eraser->size
		+ eraser->size;

	while (where < end && eraser) {
		eraser = find_biggest_eraser_within(erasers, where, end);
		if (!eraser)
			continue;
		eraser_add(eraser, &erases);
		where += eraser->size;
	}
	if (where < start + len)
		return -ENODEV;

	if (where < end) {
		eraser = find_smallest_eraser_after_point(erasers, where, end);
		if (eraser) {
			eraser_add(eraser, &erases);
			where += eraser->size;
		} else {
			return -ENODEV;
		}
	}

	list_splice(&erases, ops);
	return 0;
}

int chip_erase(struct context *ctx, off_t start, size_t len,
	       int required_exact_fit, off_t *real_start, off_t *real_len)
{
	struct flashchip *chip = ctx->chip;
	LIST_HEAD(erases);
	struct block_eraser *first, *last, *eraser;
	off_t where;
	int ret;

	ret = compute_list_erases(chip->erasers, start, len, &erases);
	if (ret)
		return ret;
	if (list_empty(&erases))
		return -ENODEV;
	first = list_first_entry(&erases, struct block_eraser, list);
	last = list_last_entry(&erases, struct block_eraser, list);

	if (required_exact_fit &&
	    (first->start + find_erase_block(first, start) * first->size)
	    != start)
		return -ENODEV;
	if (required_exact_fit &&
	    (last->start + find_erase_block(last, start + len - 1) * last->size
	     + last->size != start))
		return -ENODEV;

	where = first->start + find_erase_block(first, start) * first->size;
	if (real_start)
		*real_start = where;

	list_for_each_entry(eraser, &erases, list) {
		ret = eraser->block_erase(ctx, where, eraser->size);
		pr_dbg("Erased 0x%06x..0x%06x: %d\n",
		       where, where + eraser->size, ret);
		if (ret)
			break;
		where += eraser->size;
	}

	if (real_len)
		*real_len = where - *real_start;
	return ret;
}
