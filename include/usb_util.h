/*
 * This file is part of the flashrom2 project.
 *
 * Copyright (C) 2015 Robert Jarzmik
 *
 * It is inspired by dediprog.c from flashrom project.
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

#ifndef __USB_UTIL_H__
#define __USB_UTIL_H__

#include <usb.h>

struct usb_device *get_device_by_vid_pid(uint16_t vid, uint16_t pid,
					 unsigned int device);
int do_usb_control_msg(usb_dev_handle *dev, int requesttype, int request,
		       int value, int idx, unsigned char *bytes, size_t size,
		       int timeout);
int usb_vendor_ctrl_msg(usb_dev_handle *dev, int request, int value, int index,
			unsigned char *receive_bytes, size_t nb_receive_bytes,
			const unsigned char *send_bytes, size_t nb_send_bytes,
			int timeout);
int do_usb_bulk_read(usb_dev_handle *dev, int endpoint, unsigned char *buf,
		     size_t len, int timeout);
int do_usb_bulk_write(usb_dev_handle *dev, int endpoint, unsigned char *buf,
		      size_t len, int timeout);

#endif
