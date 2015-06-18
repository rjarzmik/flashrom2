/*
 * This file is part of the flashrom2 project.
 *
 * It is inspired by dediprog.c from flashrom project.
 * Copyright (C) 2010 Carl-Daniel Hailfinger
 * Copyright (C) 2015 Robert Jarzmik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

static inline void hexdump_vdbg(const char *prefix, const void *buf, size_t len,
					const char *postfix)
{
	size_t i;
	int repeat = 1;
	const unsigned char *s = buf;

	if (len <= 0)
		return;
	pr_vdbg("%s", prefix);
	for (i = 0; i < len; i++) {
		if (i + 1 == len) {
			pr_vdbg_cont("%s%02x*%d", (i - repeat + 1) ? " " : "", s[i], repeat);
		} else if (s[i] == s[i + 1]) {
			repeat++;
		} else {
			if (repeat > 1)
				pr_vdbg_cont("%s%02x*%d", (i - repeat + 1) ? " " : "",
					     s[i], repeat);
			else
				pr_vdbg_cont("%s%02x", (i - repeat +1) ? " " : "", s[i]);
			repeat = 1;
		}
	}
	pr_vdbg_cont("%s", postfix);
}

