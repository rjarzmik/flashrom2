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
#include <string.h>

/*
 * It is common knowledge that this function leaks memory. As it is called only
 * upon initialization a finite number of times, it's a known ... liberty.
 */
char *extract_param(const char *input, const char *param_name,
		    const char *separators)
{
	char *s = strdup(input);
	int parlen = strlen(param_name);

	strtok(s, separators);
	do {
		if (parlen == 0)
			return s;
		if ((strlen(s) > parlen + 2) &&
		    !strncmp(param_name, s, parlen) &&
		    s[parlen] == '=') {
			return &s[parlen + 1];
		}
		s = strtok(NULL, separators);
	} while (s);

	free(s);
	return NULL;
}
