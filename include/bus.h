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

#ifndef __BUS_H__
#define __BUS_H__

enum chipbustype {
	BUS_NONE	= 0,
	BUS_PARALLEL	= 1 << 0,
	BUS_LPC		= 1 << 1,
	BUS_FWH		= 1 << 2,
	BUS_SPI		= 1 << 3,
	BUS_PROG	= 1 << 4,
	BUS_NONSPI	= BUS_PARALLEL | BUS_LPC | BUS_FWH,
};

#endif
