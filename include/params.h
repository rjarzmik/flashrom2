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
#ifndef __PARAMS_H__
#define __PARAMS_H__

/**
 * extract_param - Extract a parameter from then command line
 * @input: the part of the commandline to parse
 * @param_name: the parameter name, might be ""
 * @separators: parameters separator
 *
 * A command line part should be of the form :
 *   <name><separator><param1>=<value1>...<paramN>=<valueN>
 * This function returns valueX for param_name = paramX.
 * This function return name for param_name = "".
 * If param_name is not found, return NULL.
 *
 * Returns the paramater value if found, or NULL if not
 */
char *extract_param(const char *input, const char *param_name,
		    const char *separators);

#endif
