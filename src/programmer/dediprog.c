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

#define DEBUG_MODULE "dediprog"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <chip.h>
#include <debug.h>
#include <programmer.h>
#include <spi_nor.h>
#include <spi_programmer.h>
#include <usb_util.h>

#if IS_WINDOWS
#include <lusb0_usb.h>
#else
#include <usb.h>
#endif

#define DEDI_SPI_CMD_PAGESWRITE	0x1
#define DEDI_SPI_CMD_PAGESREAD	0x2
#define DEDI_SPI_CMD_AAIWRITE	0x4

#define FIRMWARE_VERSION(x,y,z) ((x << 16) | (y << 8) | z)
#define DEFAULT_TIMEOUT 3000

struct dediprog_data;
struct dediprog_data {
	usb_dev_handle *dediprog_handle;
	int dediprog_firmwareversion;
	int millivolts;
	int speed_hz;
	int chip_select;
	int (*set_leds)(struct dediprog_data *ddata, int led);
	int (*send_command)(struct dediprog_data *ddata, unsigned int writecnt,
			    unsigned int readcnt, const unsigned char *writearr,
			    unsigned char *readarr);
	int (*prep_multi_cmd)(struct dediprog_data *ddata, int nb_pages,
			      off_t start, size_t pagesize,
			      unsigned char spi_cmd);
	int (*set_spi_speed)(struct dediprog_data *ddata, int selector);
	int (*set_spi_voltage)(struct dediprog_data *ddata, int selector);
};

/* Set/clear LEDs on dediprog */
#define PASS_ON		(0 << 0)
#define PASS_OFF	(1 << 0)
#define BUSY_ON		(0 << 1)
#define BUSY_OFF	(1 << 1)
#define ERROR_ON	(0 << 2)
#define ERROR_OFF	(1 << 2)

static int dediprog4_set_leds(struct dediprog_data *ddata, int leds)
{
	/*
	 * Older Dediprogs with 2.x.x and 3.x.x firmware only had
	 * two LEDs, and they were reversed. So map them around if
	 * we have an old device. On those devices the LEDs map as
	 * follows:
	 *   bit 2 == 0: green light is on.
	 *   bit 0 == 0: red light is on.
	 */
	leds = ((leds & ERROR_OFF) >> 2) | ((leds & PASS_OFF) << 2);
	return usb_vendor_ctrl_msg(ddata->dediprog_handle, 0x07,
				   0x0009, leds, NULL, 0, NULL, 0,
				   DEFAULT_TIMEOUT);
}

static int dediprog5_set_leds(struct dediprog_data *ddata, int leds)
{
	return usb_vendor_ctrl_msg(ddata->dediprog_handle, 0x07,
				   0x0009, leds, NULL, 0, NULL, 0,
				   DEFAULT_TIMEOUT);
}

static int dediprog55_set_leds(struct dediprog_data *ddata, int leds)
{
	return usb_vendor_ctrl_msg(ddata->dediprog_handle, 0x07,
				   (leds << 8) | 0x0009, 0x0000,
				   NULL, 0, NULL, 0, DEFAULT_TIMEOUT);
}

static int dediprog_set_leds(struct dediprog_data *ddata, int leds)
{
	int ret;

	if (leds < 0 || leds > 7)
		leds = 0; // Bogus value, enable all LEDs

	pr_dbg("set leds to pass=%s, busy=%s, error=%s\n",
	       leds & PASS_OFF ? "off" : "on", leds & BUSY_OFF ? "off" : "on",
	       leds & ERROR_OFF ? "off" : "on");
	ret = ddata->set_leds(ddata, leds);
	if (ret != 0) {
		pr_err("Command Set LED 0x%x failed (%s)!\n",
		       leds, usb_strerror());
		return ret;
	}

	return 0;
}

static int dediprog5_spi_send_command(struct dediprog_data *ddata,
				      unsigned int writecnt,
				     unsigned int readcnt,
				     const unsigned char *writearr,
				     unsigned char *readarr)
{
	int ret;

	ret = usb_vendor_ctrl_msg(ddata->dediprog_handle, 0x1, 0x00ff,
				  readcnt ? 0x0001 : 0x0000,
				  NULL, 0, writearr, writecnt,
				  DEFAULT_TIMEOUT);
	if (ret != writecnt)
		return -ENXIO;
	if (!readcnt)
		return 0;
	ret = usb_vendor_ctrl_msg(ddata->dediprog_handle, 0x01,
				  0xbb8, 0x0000,
				  readarr, readcnt, NULL, 0,
				  DEFAULT_TIMEOUT);
	if (ret != readcnt)
		return -ENXIO;
	return 0;
}

static int dediprog6_spi_send_command(struct dediprog_data *ddata,
				      unsigned int writecnt,
				     unsigned int readcnt,
				     const unsigned char *writearr,
				     unsigned char *readarr)
{
	int ret;

	ret = usb_vendor_ctrl_msg(ddata->dediprog_handle, 0x1,
				  readcnt ? 0x0001 : 0x0000, 0x0,
				  NULL, 0, writearr, writecnt,
				  DEFAULT_TIMEOUT);
	if (ret != writecnt)
		return -ENXIO;
	if (!readcnt)
		return 0;
	ret = usb_vendor_ctrl_msg(ddata->dediprog_handle, 0x01,
				  0x0001, 0x0000,
				  readarr, readcnt, NULL, 0,
				  DEFAULT_TIMEOUT);
	if (ret != readcnt)
		return -ENXIO;
	return 0;
}

static int dediprog_spi_send_command(struct context *ctxt,
				     unsigned int writecnt,
				     unsigned int readcnt,
				     const unsigned char *writearr,
				     unsigned char *readarr)
{
	struct dediprog_data *ddata = ctxt->programmer_data;
	int ret;

	pr_dbg("Send command: writecnt=%i, readcnt=%i\n", writecnt, readcnt);
	dediprog_set_leds(ddata, PASS_OFF | BUSY_ON | ERROR_OFF);

	/* Paranoid, but I don't want to be blamed if anything explodes. */
	if (writecnt > 16) {
		pr_err("Untested writecnt=%i, aborting.\n", writecnt);
		return -EINVAL;
	}
	/* 16 byte reads should work. */
	if (readcnt > 16) {
		pr_err("Untested readcnt=%i, aborting.\n", readcnt);
		return -EINVAL;
	}
	memset(readarr, 0, readcnt);

	ret = ddata->send_command(ddata, writecnt, readcnt, writearr, readarr);
	dediprog_set_leds(ddata, PASS_OFF | BUSY_OFF |
			  ((ret < 0) ? ERROR_ON : ERROR_OFF));
	return ret;
}

static struct dediprog_spispeed {
	const int speed_hz;
	const int dedi_value;
} spi_speeds[] = {
	{ 24 * MHz,	0x0 },
	{ 8 * MHz,	0x1 },
	{ 12 * MHz,	0x2 },
	{ 3 * MHz,	0x3 },
	{ 2180 * kHz,	0x4 },
	{ 1500 * kHz,	0x5 },
	{ 750 * kHz,	0x6 },
	{ 375 * kHz,	0x7 },
	{ -1,		-1 },
};

static struct dediprog_voltage {
	const int millivolts;
	const int dedi_value;
} spi_voltages[] = {
	{ 0,	0x0 },
	{ 3500,	0x10 },
	{ 2500,	0x11 },
	{ 1800,	0x12 },
	{ -1, -1 },
};

static int dediprog5_prep_multi_cmd(struct dediprog_data *ddata, int nb_pages,
				    off_t start, size_t pagesize,
				    unsigned char spi_cmd)
{
	struct firm5 {
		unsigned short nb_pages;
		unsigned char zero1;
		unsigned char spi_cmd;
	} __attribute__((packed)) seek_pre6;

	seek_pre6.nb_pages = nb_pages;
	seek_pre6.zero1 = 0;
	seek_pre6.spi_cmd = spi_cmd;

	return usb_vendor_ctrl_msg(ddata->dediprog_handle,
				   spi_cmd == DEDI_SPI_CMD_PAGESREAD ? 0x20 : 0x30,
				   start % 0x10000, start / 0x10000,
				   NULL, 0, (unsigned char *)&seek_pre6,
				   sizeof(seek_pre6), DEFAULT_TIMEOUT);
}

static int dediprog6_prep_multi_cmd(struct dediprog_data *ddata, int nb_pages,
				    off_t start, size_t pagesize,
				    unsigned char spi_cmd)
{
	struct firm6 {
		unsigned short nb_pages;
		unsigned char zero1;
		unsigned char spi_cmd;
		unsigned short pagesize;
		unsigned int offset;
	} __attribute__((packed)) seek_pre7;

	seek_pre7.nb_pages = nb_pages;
	seek_pre7.zero1 = 0;
	seek_pre7.spi_cmd = spi_cmd;
	seek_pre7.pagesize = pagesize - 1;
	seek_pre7.offset = (start / pagesize);
	return usb_vendor_ctrl_msg(ddata->dediprog_handle,
				   spi_cmd == DEDI_SPI_CMD_PAGESREAD ? 0x20 : 0x30,
				   0x0000, 0x0000,
				   NULL, 0, (unsigned char *)&seek_pre7,
				   sizeof(seek_pre7), DEFAULT_TIMEOUT);
}

static int dediprog_prep_multi_cmd(struct dediprog_data *ddata, int nb_pages,
				   off_t start, size_t pagesize,
				   unsigned char spi_cmd)
{
	return ddata->prep_multi_cmd(ddata, nb_pages, start, pagesize, spi_cmd);
}

#define DEDIPROG_MIN_ALIGN 512
/**
 * do_dediprog_spi_read_pages - read several pages from the chip
 * @ddata: the dediprog internal data
 * @buf: the buffer to read the chip into
 * @start: the address on the chip where to begin the read
 * @len: the length to read
 * @page_size: the number of bytes in one chip page
 *
 * Reads bytes from the NOR chip. Because the dediprog stores one page per bulk
 * transfer, each bulk transfer has the following constraints :
 *  - page_size < 512 (the usb bulk endpoint size)
 *  - in a 512 bytes transfer, only page_size are usefull, the remaining is filled with 0xff
 *
 * If start or (start + len) is not on a page boundary, the residue is read into
 * a temporary buffer before being copied back to buf, which implies a
 * performance loss.
 *
 * Returns the number of bytes read, or < 0 if an error occurred.
 */
static int do_dediprog_spi_read_pages(struct dediprog_data *ddata,
				      unsigned char *buf,  off_t start,
				      size_t len, size_t pagesize)
{
	unsigned char tmp_buf[DEDIPROG_MIN_ALIGN];
	int i, ret, nb_pages, skip_first;
	int first_page_bytes, nb_middle_pages, last_page_bytes;

	if (start % DEDIPROG_MIN_ALIGN)
		return -EINVAL;
	skip_first = start % pagesize;
	first_page_bytes = (pagesize - skip_first) % pagesize;
	last_page_bytes = (len - skip_first) % pagesize;
	nb_middle_pages = (len - first_page_bytes - last_page_bytes) / pagesize;

	nb_pages = nb_middle_pages + (first_page_bytes ? 1 : 0) +
		(last_page_bytes ? 1 : 0);
	ret = dediprog_prep_multi_cmd(ddata, nb_pages,
				      (start / pagesize) * pagesize,
				      pagesize, DEDI_SPI_CMD_PAGESREAD);

	memset(tmp_buf, 0xff, sizeof(tmp_buf));
	if (ret >= 0 && first_page_bytes) {
		ret = do_usb_bulk_read(ddata->dediprog_handle, 2,
				       tmp_buf, DEDIPROG_MIN_ALIGN,
				       DEFAULT_TIMEOUT);
		memcpy(buf, tmp_buf + skip_first, first_page_bytes);
		buf += first_page_bytes;
	}
	ret = DEDIPROG_MIN_ALIGN;
	for (i = 0; ret >= DEDIPROG_MIN_ALIGN && i < nb_middle_pages; i++) {
		ret = do_usb_bulk_read(ddata->dediprog_handle, 2, buf,
				       DEDIPROG_MIN_ALIGN, DEFAULT_TIMEOUT);
		buf += pagesize;
	}
	if (ret >= DEDIPROG_MIN_ALIGN && last_page_bytes) {
		ret = do_usb_bulk_read(ddata->dediprog_handle, 2, tmp_buf,
				       DEDIPROG_MIN_ALIGN, DEFAULT_TIMEOUT);
		memcpy(buf, tmp_buf, last_page_bytes);
		buf += last_page_bytes;
	}

	if (ret < 0)
		return ret;
	else
		return len;
}

/**
 * do_dediprog_spi_write_pages - write several pages to the chip
 * @ddata: the dediprog internal data
 * @buf: the buffer to read the chip into
 * @start: the address on the chip where to begin the read
 * @len: the length to read
 * @page_size: the number of bytes in one chip page
 *
 * Write bytes from the NOR chip. Because the dediprog stores one page per bulk
 * transfer, each bulk transfer has the following constraints :
 *  - page_size < 512 (the usb bulk endpoint size)
 *  - in a 512 bytes transfer, only page_size are usefull, the remaining is filled with 0xff
 *
 * If start or (start + len) is not on a page boundary, the residue is filled
 * with 0xff and written as a whole page.
 *
 * Returns the number of bytes written, or < 0 if an error occurred.
 */
static int do_dediprog_spi_write_pages(struct dediprog_data *ddata,
				       const unsigned char *buf,  off_t start,
				       size_t len, size_t pagesize)
{
	unsigned char tmp_buf[DEDIPROG_MIN_ALIGN];
	int i, ret, nb_pages, skip_first;
	int first_page_bytes, nb_middle_pages, last_page_bytes;

	if (start % DEDIPROG_MIN_ALIGN)
		return -EINVAL;
	skip_first = start % pagesize;
	first_page_bytes = (pagesize - skip_first) % pagesize;
	last_page_bytes = (len - skip_first) % pagesize;
	nb_middle_pages = (len - first_page_bytes - last_page_bytes) / pagesize;

	nb_pages = nb_middle_pages + (first_page_bytes ? 1 : 0) +
		(last_page_bytes ? 1 : 0);
	ret = dediprog_prep_multi_cmd(ddata, nb_pages,
				      (start / pagesize) * pagesize,
				      pagesize, DEDI_SPI_CMD_PAGESWRITE);

	memset(tmp_buf, 0xff, sizeof(tmp_buf));
	if (ret >= 0 && first_page_bytes) {
		memset(tmp_buf, 0xff, sizeof(tmp_buf));
		memcpy(tmp_buf + skip_first, buf, first_page_bytes);
		ret = do_usb_bulk_write(ddata->dediprog_handle, 2,
				       tmp_buf, DEDIPROG_MIN_ALIGN,
				       DEFAULT_TIMEOUT);
		buf += first_page_bytes;
	}
	memset(tmp_buf, 0xff, sizeof(tmp_buf));
	ret = DEDIPROG_MIN_ALIGN;
	for (i = 0; ret >= DEDIPROG_MIN_ALIGN && i < nb_middle_pages; i++) {
		memcpy(tmp_buf, buf, pagesize);
		ret = do_usb_bulk_write(ddata->dediprog_handle, 2, tmp_buf,
				       DEDIPROG_MIN_ALIGN, DEFAULT_TIMEOUT);
		buf += pagesize;
	}
	memset(tmp_buf, 0xff, sizeof(tmp_buf));
	if (ret >= DEDIPROG_MIN_ALIGN && last_page_bytes) {
		memcpy(tmp_buf, buf, last_page_bytes);
		ret = do_usb_bulk_read(ddata->dediprog_handle, 2, tmp_buf,
				       DEDIPROG_MIN_ALIGN, DEFAULT_TIMEOUT);
		buf += last_page_bytes;
	}

	if (ret < DEDIPROG_MIN_ALIGN)
		return ret;
	else
		return len;
}

static int dediprog_spi_read(struct context *ctxt, unsigned char *buf,
			     off_t start, size_t len)
{
	int ret;
	struct dediprog_data *ddata = ctxt->programmer_data;

	pr_dbg("read dediprog(buf=%p, start=%u, len=%zu)\n", buf, start, len);
	dediprog_set_leds(ddata, PASS_OFF|BUSY_ON|ERROR_OFF);

	ret = do_dediprog_spi_read_pages(ddata, buf, start, len,
					 DEDIPROG_MIN_ALIGN);
	if (ret < (int)len) {
		dediprog_set_leds(ddata, PASS_OFF|BUSY_OFF|ERROR_ON);
		pr_err("dediprog read error: wrote %d bytes while expected %d\n",
		       ret, len);
	} else {
		dediprog_set_leds(ddata, PASS_ON|BUSY_OFF|ERROR_OFF);
	}

	return ret;
}

static int dediprog_spi_write(struct context *ctxt, const unsigned char *buf,
			      off_t start, size_t len)
{
	int ret, timeout = 10;
	struct dediprog_data *ddata = ctxt->programmer_data;

	pr_dbg("write dediprog(buf=%p, start=%u, len=%zu)\n", buf, start, len);
	dediprog_set_leds(ddata, PASS_OFF|BUSY_ON|ERROR_OFF);

	ret = do_dediprog_spi_write_pages(ddata, buf, start, len,
					  ctxt->chip->page_size);
	while (ret > (int)len && timeout-- &&
	       spi_read_status_register(ctxt) & SPI_SR_WIP)
		programmer_delay(10);

	if (!timeout || (ret < (int)len)) {
		dediprog_set_leds(ddata, PASS_OFF|BUSY_OFF|ERROR_ON);
		pr_err("dediprog write error: wrote %d bytes while expected %d%s\n",
		       ret, len, timeout ? "" : " timeout to clear 'write in processing' occurred");
	} else {
		dediprog_set_leds(ddata, PASS_ON|BUSY_OFF|ERROR_OFF);
	}

	return ret;
}

static int dediprog_spi_speed_value(int speed)
{
	struct dediprog_spispeed *sp;

	for (sp = &spi_speeds[0]; sp->speed_hz >= 0; sp++)
		if (sp->speed_hz == speed)
			return sp->dedi_value;
	return -1;
}

static int dediprog_spi_voltage_value(int millivolts)
{
	struct dediprog_voltage *v;

	for (v = &spi_voltages[0]; v->millivolts >= 0; v++)
		if (v->millivolts == millivolts)
			return v->dedi_value;
	return -1;
}

static int dediprog4_set_spi_speed(struct dediprog_data *ddata,
				   int selector)
{
	pr_warn("Skipping to set SPI speed because firmware is too old.\n");
	return 0;
}

static int dediprog5_set_spi_speed(struct dediprog_data *ddata,
				   int selector)
{
	return usb_vendor_ctrl_msg(ddata->dediprog_handle, 0x61,
				   selector, 0x00ff,
				   NULL, 0, NULL, 0, DEFAULT_TIMEOUT);
}

static int dediprog6_set_spi_speed(struct dediprog_data *ddata,
				   int selector)
{
	return usb_vendor_ctrl_msg(ddata->dediprog_handle, 0x61,
				   selector, 0x000,
				   NULL, 0, NULL, 0, DEFAULT_TIMEOUT);
}

/* After dediprog_set_spi_speed, the original app always calls
 * dediprog_set_spi_voltage(0) and then
 * dediprog_check_devicestring() four times in a row.
 * After that, dediprog_command_a() is called.
 * This looks suspiciously like the microprocessor in the SF100 has to be
 * restarted/reinitialized in case the speed changes.
 */
static int dediprog_set_spi_speed(struct dediprog_data *ddata,
				  unsigned int spi_hz)
{
	int ret = -1;
	uint16_t speed_selector = dediprog_spi_speed_value(spi_hz);

	pr_info("Setting SPI speed to %d Hz\n", spi_hz);
	if (ddata->dediprog_firmwareversion < FIRMWARE_VERSION(5, 0, 0)) {
		pr_warn("Skipping to set SPI speed because firmware is too old.\n");
		return 0;
	}

	ret = ddata->set_spi_speed(ddata, speed_selector);
	if (ret != 0x0) {
		pr_err("Command Set SPI Speed %d Hz failed!\n", spi_hz);
		return 1;
	}
	return 0;
}

static int dediprog5_set_spi_voltage(struct dediprog_data *ddata, int selector)
{
	return usb_vendor_ctrl_msg(ddata->dediprog_handle, 0x9,
				   selector, 0x00ff,
				   NULL, 0, NULL, 0, DEFAULT_TIMEOUT);
}

static int dediprog6_set_spi_voltage(struct dediprog_data *ddata, int selector)
{
	return usb_vendor_ctrl_msg(ddata->dediprog_handle, 0x9,
				   selector, 0x0000,
				   NULL, 0, NULL, 0, DEFAULT_TIMEOUT);
}

static int dediprog_set_spi_voltage(struct dediprog_data *ddata, int millivolt)
{
	int ret = -1;
	uint16_t voltage_selector = dediprog_spi_voltage_value(millivolt);
	pr_info("Setting SPI voltage to %u.%03u V\n", millivolt / 1000,
		millivolt % 1000);

	if (voltage_selector == 0) {
		/* Wait some time as the original driver does. */
		programmer_delay(200 * 1000);
	}
	ret = ddata->set_spi_voltage(ddata, voltage_selector);
	if (ret != 0x0) {
		pr_err("Command Set SPI Voltage 0x%x failed!\n",
			 voltage_selector);
		return -ENXIO;
	}
	if (voltage_selector != 0) {
		/* Wait some time as the original driver does. */
		programmer_delay(200 * 1000);
	}
	return 0;
}

/* Command A seems to be some sort of device init. It is either followed by
 * dediprog_check_devicestring (often) or Command A (often) or
 * Command F (once).
 */
static int dediprog_command_a(struct dediprog_data *ddata)
{
	int ret;
	unsigned char buf[1];

	memset(buf, 0, sizeof(buf));
	ret = usb_vendor_ctrl_msg(ddata->dediprog_handle, 4, 0x0000, 0x0000,
				  NULL, 0, NULL, 0, DEFAULT_TIMEOUT);
	if (ret < 0) {
		pr_err("Command A failed (%s)!\n", usb_strerror());
		return ret;
	}
	ret = do_usb_control_msg(ddata->dediprog_handle, 0xc3, 0xb,
				 0x0000, 0x0000,
				 buf, sizeof(buf), DEFAULT_TIMEOUT);
	if (ret < 0) {
		pr_err("Command A failed (%s)!\n", usb_strerror());
		return ret;
	}
	if ((ret != 0x1) || (buf[0] != 0x6f)) {
		pr_err("Unexpected response to Command A!\n");
		return -ENXIO;
	}
	return 0;
}

static int dediprog_check_devicestring(struct dediprog_data *ddata)
{
	int ret;
	int fw[3];
	unsigned char buf[17];

	/* Command Prepare Receive Device String. */
	memset(buf, 0, sizeof(buf));
	ret = do_usb_control_msg(ddata->dediprog_handle, 0xc3, 0x7,
				 0x0000, 0xef03, buf, 1, DEFAULT_TIMEOUT);
	if ((ret != 0x1) || (buf[0] != 0xff)) {
		pr_err("Unexpected response to Command Prepare Receive Device"
		       " String!\n");
		return -EINVAL;
	}
	/* Command Receive Device String. */
	memset(buf, 0, sizeof(buf));
	ret = do_usb_control_msg(ddata->dediprog_handle, 0xc2, 0x8,
				 0x00ff, 0x00ff, buf, 16, DEFAULT_TIMEOUT);
	if (ret != 0x10) {
		pr_err("Incomplete/failed Command Receive Device String!\n");
		return 1;
	}
	buf[0x10] = '\0';
	if (memcmp(buf, "SF100", 0x5)) {
		pr_err("Device not a SF100!\n");
		return -ENODEV;
	}
	if (sscanf((char *)buf,
		   "SF100 V:%d.%d.%d ", &fw[0], &fw[1], &fw[2]) != 3) {
		pr_err("Unexpected firmware version string!\n");
		return -ENODEV;
	}
	pr_warn("Found a %s (fw_version=%d.%d.%d)\n", buf, fw[0], fw[1], fw[2]);
	/* Only these versions were tested. */
	if (fw[0] < 2 || fw[0] > 6) {
		pr_err("Unexpected firmware version %d.%d.%d!\n", fw[0],
			 fw[1], fw[2]);
		return -ENODEV;
	}
	ddata->dediprog_firmwareversion = FIRMWARE_VERSION(fw[0], fw[1], fw[2]);
	return 0;
}

/* Command Chip Select is only sent after dediprog_check_devicestring, but not
 * after every invocation of dediprog_check_devicestring. It is only sent after
 * the first dediprog_command_a(); dediprog_check_devicestring() sequence in
 * each session.  Bit #1 of the value changes the chip select: 0 is target 1, 1
 * is target 2 and parameter target can be 1 or 2 respectively. We don't know
 * how to encode "3, Socket" and "0, reference card" yet. On SF100 the vendor
 * software "DpCmd 6.0.4.06" selects target 2 when requesting 3 (which is
 * unavailable on that hardware).
 */
static int dediprog_chip_select(struct dediprog_data *ddata)
{
	int ret;
	uint16_t value = ((ddata->chip_select) & 1) << 1;

	pr_dbg("Selecting target chip %i\n", ddata->chip_select);
	ret = do_usb_control_msg(ddata->dediprog_handle, 0x42, 0x4, value, 0x0,
				 NULL, 0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		pr_err("Command Chip Select failed (%s)!\n", usb_strerror());
		return ret;
	}
	return 0;
}
static int dediprog_setup(struct dediprog_data *ddata)
{
	/* URB 6. Command A. */
	if (dediprog_command_a(ddata)) {
		return 1;
	}
	/* URB 7. Command A. */
	if (dediprog_command_a(ddata)) {
		return 1;
	}
	/* URB 8. Command Prepare Receive Device String. */
	/* URB 9. Command Receive Device String. */
	if (dediprog_check_devicestring(ddata)) {
		return 1;
	}
	/* URB 10. Command Chip Select */
	if (dediprog_chip_select(ddata)) {
		return 1;
	}

	ddata->send_command = dediprog5_spi_send_command;
	ddata->prep_multi_cmd = dediprog5_prep_multi_cmd;
	ddata->set_spi_speed = dediprog5_set_spi_speed;
	ddata->set_spi_voltage = dediprog5_set_spi_voltage;
	if (ddata->dediprog_firmwareversion < FIRMWARE_VERSION(5, 0, 0)) {
		ddata->set_leds = dediprog4_set_leds;
		ddata->set_spi_speed = dediprog4_set_spi_speed;
	} else if (ddata->dediprog_firmwareversion >= FIRMWARE_VERSION(5, 5, 0)) {
		ddata->set_leds = dediprog55_set_leds;
		ddata->send_command = dediprog6_spi_send_command;
		ddata->prep_multi_cmd = dediprog6_prep_multi_cmd;
		ddata->set_spi_speed = dediprog6_set_spi_speed;
		ddata->set_spi_voltage = dediprog6_set_spi_voltage;
	} else if (ddata->dediprog_firmwareversion >= FIRMWARE_VERSION(5, 0, 0)) {
		ddata->set_leds = dediprog5_set_leds;
	}

	return 0;
}

int dediprog_probe(const char *programmer_args, void **data)
{
	struct usb_device *dev;
	char *device;
	long usedevice = 0;
	int ret;
	struct dediprog_data *ddata;

	ddata = malloc(sizeof(*ddata));
	if (!ddata)
		return -ENOMEM;
	ddata->speed_hz = 12 * MHz;
	ddata->millivolts = 3500;
	ddata->chip_select = 0;

	spi_programmer_extract_params(programmer_args, &ddata->speed_hz,
				  &ddata->millivolts);
	if (dediprog_spi_speed_value(ddata->speed_hz) < 0) {
		pr_err("Sorry, requested spi speed %d Hz not possible, aborting ...\n",
		       ddata->speed_hz);
		return -EINVAL;
	}
	if (dediprog_spi_voltage_value(ddata->millivolts) < 0) {
		pr_err("Sorry, requested spi voltage %dmv not possible, aborting ...\n",
		       ddata->millivolts);
		return -EINVAL;
	}

	device = extract_programmer_param(programmer_args, "device");
	if (device) {
		char *dev_suffix;
		errno = 0;
		usedevice = strtol(device, &dev_suffix, 10);
		if (errno != 0 || device == dev_suffix) {
			pr_err("Error: Could not convert 'device'.\n");
			free(device);
			return -EINVAL;
		}
		if (usedevice < 0 || usedevice > UINT_MAX) {
			pr_err("Error: Value for 'device' is out of range.\n");
			free(device);
			return -EINVAL;
		}
		if (strlen(dev_suffix) > 0) {
			pr_err("Error: Garbage following 'device' value.\n");
			free(device);
			return -EINVAL;
		}
		pr_info("Using device %li.\n", usedevice);
	}
	free(device);

	/* Here comes the USB stuff. */
	usb_init();
	usb_find_busses();
	usb_find_devices();
	dev = get_device_by_vid_pid(0x0483, 0xdada, (unsigned int) usedevice);
	if (!dev) {
		pr_err("Could not find a Dediprog SF100 on USB!\n");
		return 1;
	}
	pr_dbg("Found USB device (%04x:%04x).\n",
		 dev->descriptor.idVendor, dev->descriptor.idProduct);
	ddata->dediprog_handle = usb_open(dev);
	if (!ddata->dediprog_handle) {
		pr_err("Could not open USB device: %s\n", usb_strerror());
		return -ENODEV;
	}
	ret = usb_set_configuration(ddata->dediprog_handle, 1);
	if (ret < 0) {
		pr_err("Could not set USB device configuration: %i %s\n",
		       ret, usb_strerror());
		if (usb_close(ddata->dediprog_handle))
			pr_err("Could not close USB device!\n");
		return -ENODEV;
	}
	ret = usb_claim_interface(ddata->dediprog_handle, 0);
	if (ret < 0) {
		pr_err("Could not claim USB device interface %i: %i %s\n",
		       0, ret, usb_strerror());
		if (usb_close(ddata->dediprog_handle))
			pr_err("Could not close USB device!\n");
		return -ENODEV;
	}

	/* Perform basic setup. */
	if (dediprog_setup(ddata))
		return -ENXIO;

	dediprog_set_leds(ddata, PASS_ON|BUSY_ON|ERROR_ON);

	/* After setting voltage and speed, perform setup again. */
	if (dediprog_set_spi_voltage(ddata, 0) ||
	    dediprog_set_spi_speed(ddata, ddata->speed_hz) ||
	    dediprog_setup(ddata)) {
		dediprog_set_leds(ddata, PASS_OFF|BUSY_OFF|ERROR_ON);
		return -ENXIO;
	}

	if (dediprog_set_spi_voltage(ddata, ddata->millivolts)) {
		dediprog_set_leds(ddata, PASS_OFF|BUSY_OFF|ERROR_ON);
		return -ENXIO;
	}

	dediprog_set_leds(ddata, PASS_OFF|BUSY_OFF|ERROR_OFF);
	*data = ddata;

	return 0;
}

static struct programmer dediprog = {
	.buses_supported = BUS_SPI,
	.spi = {
		.max_data_read = 0,
		.max_data_write = 0,
		.command = dediprog_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = dediprog_spi_read,
		.write_256 = dediprog_spi_write,
	},
	.probe = dediprog_probe,
	.desc = "[voltage={1.8v,2.5v,3.5v}] [hz={12MHz,...}]",
};

DECLARE_PROGRAMMER(dediprog);
