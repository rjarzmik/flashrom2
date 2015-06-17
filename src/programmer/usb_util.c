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

#define DEBUG_MODULE "usb"

#include <errno.h>
#include <usb.h>
#include <stdint.h>

#include <debug.h>
#include <usb_util.h>

static void hexdump_vdbg(const char *prefix, const void *buf, size_t len,
	const char *postfix)
{
	size_t i;

	pr_vdbg("%s", prefix);
	for (i = 0; i < len; i++)
		pr_vdbg_cont("%s%02x", i ? " " : "", ((uint8_t *)buf)[i]);
	pr_vdbg_cont("%s", postfix);
}

struct usb_device *get_device_by_vid_pid(uint16_t vid, uint16_t pid,
					 unsigned int device)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	for (bus = usb_get_busses(); bus; bus = bus->next)
		for (dev = bus->devices; dev; dev = dev->next)
			if ((dev->descriptor.idVendor == vid) &&
			    (dev->descriptor.idProduct == pid)) {
				if (device == 0)
					return dev;
				device--;
			}

	return NULL;
}

int do_usb_control_msg(usb_dev_handle *dev, int requesttype, int request,
		       int value, int idx, unsigned char *bytes, size_t size,
		       int timeout)
{
	int ret;

	ret = usb_control_msg(dev, requesttype, request, value, idx,
			      (char *)bytes, size, timeout);
	pr_vdbg("\tusb_control_msg(rqtype=0x%x, request=0x%x, value=0x%x, idx=0x%0x, buflen=%d): %d\n",
	       requesttype, request, value, idx, size, ret);
	hexdump_vdbg("\t\t Buf=[", bytes, size, "]\n");

	return ret;
}

int usb_vendor_ctrl_msg(usb_dev_handle *dev, int request, int value, int index,
			unsigned char *receive_bytes, size_t nb_receive_bytes,
			const unsigned char *send_bytes, size_t nb_send_bytes,
			int timeout)
{
	int ret, request_type = USB_TYPE_VENDOR | USB_RECIP_ENDPOINT;
	size_t nb_bytes;

	if (nb_receive_bytes > 0 && nb_send_bytes > 0)
		return -EINVAL;

	nb_bytes = (nb_receive_bytes > 0) ? nb_receive_bytes : nb_send_bytes;
	if (nb_receive_bytes > 0)
		ret = do_usb_control_msg(dev, request_type | USB_ENDPOINT_IN,
					 request, value, index, receive_bytes,
					 nb_receive_bytes, timeout);
	else
		ret = do_usb_control_msg(dev, request_type | USB_ENDPOINT_OUT,
					 request, value, index,
					 (unsigned char *)send_bytes,
					 nb_send_bytes, timeout);
	if (ret != nb_bytes)
		pr_err("read/write usb failed, expected %i, got %i %s!\n",
		       nb_bytes, ret, usb_strerror());
	return ret;
}

int do_usb_bulk_read(usb_dev_handle *dev, int endpoint, unsigned char *buf,
		     size_t len, int timeout)
{
	int ret;

	ret = usb_bulk_read(dev, endpoint | USB_ENDPOINT_IN, (char *)buf,
			    len, timeout);
	pr_vdbg("\tusb_bulk_read(endpoint=%d, buflen=%zu): %d\n",
	       endpoint, len);
	hexdump_vdbg("\t\t Buf=[", buf, len, "]\n");
	return ret;
}

int do_usb_bulk_write(usb_dev_handle *dev, int endpoint, unsigned char *buf,
		     size_t len, int timeout)
{
	int ret;

	ret = usb_bulk_write(dev, endpoint | USB_ENDPOINT_OUT, (char *)buf,
			     len, timeout);
	pr_vdbg("\tusb_bulk_write(endpoint=%d, buflen=%zu): %d\n", endpoint,
	       len, ret);
	hexdump_vdbg("\t\t Buf=[", buf, len, "]\n");
	return ret;
}
