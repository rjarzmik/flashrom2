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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <chip.h>
#include <debug.h>
#include <operation.h>
#include <write_strategy.h>

static void help(const char *pname)
{
	pr_warn("Usage : %s <list of operations> --programmer=<programmer with options>\n", pname);
	pr_warn("\t[--write-strategy=<strategy>] [--verbose] [--chip=<chipname>]\n");
	pr_warn("\t operation = { --read=<filename>, --write=<filename>, --verify=<filename> }\n");
	pr_warn("\t\t Operations order is important, they are carried out in order\n");
	pr_warn("Example1: write a file, verify it, and read back flash to another file\n");
	pr_warn("\t%s --programmer=dediprog:voltage=1.8v --write-strategy=wipe_by_biggest_erases --write=/tmp/rom.bin --verify=/tmp/rom.bin --read=/tmp/rom_reread.bin\n", pname);
	pr_warn("Example2: update a rom incrementaly, and then verify it\n");
	pr_warn("\t%s --programmer=dummy --write-strategy=wipe_if_changes --write=/tmp/rom.bin --verify=/tmp/rom.bin\n", pname);
	pr_warn("\nAvailable chips :\n");
	print_available_chips();
	pr_warn("Available programmers :\n");
	print_available_programmers();
	pr_warn("Available write strategies :\n");
	print_available_write_strategies();
}

int main(int argc, char **argv)
{
	const struct option long_options[] = {
		{ "read", required_argument, 0, 'r' },
		{ "write", required_argument, 0, 'w' },
		{ "verify", required_argument, 0, 'v' },
		{ "programmer",  required_argument, 0,  'p' },
		{ "chip", required_argument, 0, 'c' },
		{ "write-strategy", required_argument, 0, 's' },
		{ "verbose", no_argument, 0, 'V' },
		{NULL, 0, 0, 0 }
	};
	struct operation op, op_programmer, op_chip;
	enum write_strategy write_strategy = WIPE_BY_BIGGEST_ERASES;
	char c;

	if (argc == 1) {
		help(argv[0]);
		exit(1);
	}

	op_programmer.op = SET_PROGRAMMER;
	op_programmer.arg.programmer = "automatic";
	op_chip.op = SET_CHIP;
	op_chip.arg.chipname = "automatic";
	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "VEr:w:v:p:",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'V':
			dbg_level_inc();
			break;
		case 'r':
			op.op = READ;
			op.arg.filename = optarg;
			operation_add_tail(&op);
			break;
		case 'w':
			op.op = WRITE;
			op.arg.filename = optarg;
			operation_add_tail(&op);
			break;
		case 'v':
			op.op = VERIFY;
			op.arg.filename = optarg;
			operation_add_tail(&op);
			break;
		case 'p':
			op_programmer.arg.programmer = optarg;
			break;
		case 'c':
			op_chip.arg.chipname = optarg;
			break;
		case 's':
			write_strategy = parse_write_strategy(optarg);
			break;
		default:
			help(argv[0]);
		}
	}

	if (write_strategy == UNKNOWN) {
		pr_warn("Error: unknown write strategy, aborting !\n");
		exit(1);
	}

	op.op = SET_WRITE_STRATEGY;
	op.arg.write_strategy = write_strategy;
	operation_add(&op);
	operation_add(&op_chip);
	operation_add(&op_programmer);

	return operations_launch();
}
