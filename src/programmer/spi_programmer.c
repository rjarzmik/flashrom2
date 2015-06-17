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

#include <stdio.h>
#include <string.h>

#include <programmer.h>
#include <spi_programmer.h>

static int parse_voltage(const char *s)
{
	float f = -1;
	char unit[3];

	memset(unit, 0, sizeof(unit));
	if (sscanf(s, "%f%2c", &f, unit) < 2)
		return -1;
	if (!strcasecmp(unit, "mv"))
		return (int)(f);
	if (!strcasecmp(unit, "v"))
		return (int)(f * 1000);
	if (!strlen(unit))
		return (int)f;
	return -1;
}

static int parse_freq(const char *s)
{
	float f = -1;
	char unit[4];

	memset(unit, 0, sizeof(unit));

	if (sscanf(s, "%f%2c", &f, unit) < 2)
		return -1;
	if (!strcasecmp(unit, "mhz"))
		return (int)(f * 1000 * 1000);
	if (!strcasecmp(unit, "khz"))
		return (int)(f *  1000);
	if (!strcasecmp(unit, "hz"))
		return (int)(f);
	if (!strlen(unit))
		return (int)f;
	return -1;
}

void spi_programmer_extract_params(const char *programmer_args,
				   int *spi_speed_hz, int *spi_voltage_mv)
{
	char *s;
	int speed_hz = -1;
	int voltage_mv = - 1;

	s = extract_programmer_param(programmer_args, "hz");
	if (s)
		speed_hz = parse_freq(s);
	if (speed_hz > 0)
		*spi_speed_hz = speed_hz;

	s = extract_programmer_param(programmer_args, "voltage");
	if (s)
		voltage_mv = parse_voltage(s);
	if (voltage_mv > 0)
		*spi_voltage_mv = voltage_mv;
}
