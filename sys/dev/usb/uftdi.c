/*	$OpenBSD: uftdi.c,v 1.34 2007/05/27 04:00:25 jsg Exp $ 	*/
/*	$NetBSD: uftdi.c,v 1.14 2003/02/23 04:20:07 simonb Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * FTDI FT8U100AX serial adapter driver
 */

/*
 * XXX This driver will not support multiple serial ports.
 * XXX The ucom layer needs to be extended first.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/tty.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/ucomvar.h>

#include <dev/usb/uftdireg.h>

#ifdef UFTDI_DEBUG
#define DPRINTF(x)	do { if (uftdidebug) printf x; } while (0)
#define DPRINTFN(n,x)	do { if (uftdidebug>(n)) printf x; } while (0)
int uftdidebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define UFTDI_CONFIG_INDEX	0
#define UFTDI_IFACE_INDEX	0


/*
 * These are the maximum number of bytes transferred per frame.
 * The output buffer size cannot be increased due to the size encoding.
 */
#define UFTDIIBUFSIZE 64
#define UFTDIOBUFSIZE 64

struct uftdi_softc {
	USBBASEDEVICE		sc_dev;		/* base device */
	usbd_device_handle	sc_udev;	/* device */
	usbd_interface_handle	sc_iface;	/* interface */

	enum uftdi_type		sc_type;
	u_int			sc_hdrlen;

	u_char			sc_msr;
	u_char			sc_lsr;

	device_ptr_t		sc_subdev;

	u_char			sc_dying;

	u_int			last_lcr;
};

Static void	uftdi_get_status(void *, int portno, u_char *lsr, u_char *msr);
Static void	uftdi_set(void *, int, int, int);
Static int	uftdi_param(void *, int, struct termios *);
Static int	uftdi_open(void *sc, int portno);
Static void	uftdi_read(void *sc, int portno, u_char **ptr,
			   u_int32_t *count);
Static void	uftdi_write(void *sc, int portno, u_char *to, u_char *from,
			    u_int32_t *count);
Static void	uftdi_break(void *sc, int portno, int onoff);
Static int	uftdi_8u232am_getrate(speed_t speed, int *rate);

struct ucom_methods uftdi_methods = {
	uftdi_get_status,
	uftdi_set,
	uftdi_param,
	NULL,
	uftdi_open,
	NULL,
	uftdi_read,
	uftdi_write,
};

USB_DECLARE_DRIVER(uftdi);

int
uftdi_match(struct device *parent, void *match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	if (uaa->iface != NULL) {
		if (uaa->vendor == USB_VENDOR_FTDI &&
		    (uaa->product == USB_PRODUCT_FTDI_SERIAL_2232C))
			return (UMATCH_VENDOR_IFACESUBCLASS);
		return (UMATCH_NONE);
	}

	DPRINTFN(20,("uftdi: vendor=0x%x, product=0x%x\n",
		     uaa->vendor, uaa->product));

	if (uaa->vendor == USB_VENDOR_FTDI &&
	    (uaa->product == USB_PRODUCT_FTDI_SERIAL_8U100AX ||
	     uaa->product == USB_PRODUCT_FTDI_SERIAL_8U232AM ||
	     uaa->product == USB_PRODUCT_FTDI_SERIAL_232BM ||
	     uaa->product == USB_PRODUCT_FTDI_SEMC_DSS20 ||
	     uaa->product == USB_PRODUCT_FTDI_MHAM_KW ||
	     uaa->product == USB_PRODUCT_FTDI_MHAM_YS ||
	     uaa->product == USB_PRODUCT_FTDI_MHAM_Y6 ||
	     uaa->product == USB_PRODUCT_FTDI_MHAM_Y8 ||
	     uaa->product == USB_PRODUCT_FTDI_MHAM_IC ||
	     uaa->product == USB_PRODUCT_FTDI_MHAM_DB9 ||
	     uaa->product == USB_PRODUCT_FTDI_MHAM_RS232 ||
	     uaa->product == USB_PRODUCT_FTDI_MHAM_Y9 ||
	     uaa->product == USB_PRODUCT_FTDI_COASTAL_TNCX ||
	     uaa->product == USB_PRODUCT_FTDI_LCD_LK202_24 ||
	     uaa->product == USB_PRODUCT_FTDI_LCD_LK204_24 ||
	     uaa->product == USB_PRODUCT_FTDI_LCD_MX200 ||
	     uaa->product == USB_PRODUCT_FTDI_LCD_CFA_631 ||
	     uaa->product == USB_PRODUCT_FTDI_LCD_CFA_632 ||
	     uaa->product == USB_PRODUCT_FTDI_LCD_CFA_633 ||
	     uaa->product == USB_PRODUCT_FTDI_LCD_CFA_634 ||
	     uaa->product == USB_PRODUCT_FTDI_MJS_SIRIUS_PC))
		return (UMATCH_VENDOR_PRODUCT);
	if (uaa->vendor == USB_VENDOR_SIIG2 &&
	    (uaa->product == USB_PRODUCT_SIIG2_US2308))
		return (UMATCH_VENDOR_PRODUCT);
	if (uaa->vendor == USB_VENDOR_INTREPIDCS &&
	    (uaa->product == USB_PRODUCT_INTREPIDCS_VALUECAN ||
	     uaa->product == USB_PRODUCT_INTREPIDCS_NEOVI))
		return (UMATCH_VENDOR_PRODUCT);
	if (uaa->vendor == USB_VENDOR_BBELECTRONICS &&
	    (uaa->product == USB_PRODUCT_BBELECTRONICS_USOTL4))
		return (UMATCH_VENDOR_PRODUCT);
	if (uaa->vendor == USB_VENDOR_FALCOM &&
	    (uaa->product == USB_PRODUCT_FALCOM_TWIST ||
	     uaa->product == USB_PRODUCT_FALCOM_SAMBA))
		 return (UMATCH_VENDOR_PRODUCT);
	if (uaa->vendor == USB_VENDOR_SEALEVEL &&
	    uaa->product == USB_PRODUCT_SEALEVEL_USBSERIAL)
		return (UMATCH_VENDOR_PRODUCT);

	return (UMATCH_NONE);
}

void
uftdi_attach(struct device *parent, struct device *self, void *aux)
{
	struct uftdi_softc *sc = (struct uftdi_softc *)self;
	struct usb_attach_arg *uaa = aux;
	usbd_device_handle dev = uaa->device;
	usbd_interface_handle iface;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	char *devname = USBDEVNAME(sc->sc_dev);
	int i;
	usbd_status err;
	struct ucom_attach_args uca;

	DPRINTFN(10,("\nuftdi_attach: sc=%p\n", sc));

	if (uaa->iface == NULL) {
		/* Move the device into the configured state. */
		err = usbd_set_config_index(dev, UFTDI_CONFIG_INDEX, 1);
		if (err) {
			printf("\n%s: failed to set configuration, err=%s\n",
			       devname, usbd_errstr(err));
			goto bad;
		}

		err = usbd_device2interface_handle(dev, UFTDI_IFACE_INDEX, &iface);
		if (err) {
			printf("\n%s: failed to get interface, err=%s\n",
			       devname, usbd_errstr(err));
			goto bad;
		}
	} else
		iface = uaa->iface;

	devinfop = usbd_devinfo_alloc(dev, 0);
	printf("\n%s: %s\n", devname, devinfop);
	usbd_devinfo_free(devinfop);

	id = usbd_get_interface_descriptor(iface);

	sc->sc_udev = dev;
	sc->sc_iface = iface;

	switch (uaa->vendor) {
	case USB_VENDOR_FTDI:
		switch (uaa->product) {
		case USB_PRODUCT_FTDI_SERIAL_8U100AX:
			sc->sc_type = UFTDI_TYPE_SIO;
			sc->sc_hdrlen = 1;
			break;

		case USB_PRODUCT_FTDI_SEMC_DSS20:
		case USB_PRODUCT_FTDI_SERIAL_8U232AM:
		case USB_PRODUCT_FTDI_SERIAL_2232C:
		case USB_PRODUCT_FTDI_SERIAL_232BM:
		case USB_PRODUCT_FTDI_COASTAL_TNCX:
		case USB_PRODUCT_FTDI_LCD_LK202_24:
		case USB_PRODUCT_FTDI_LCD_LK204_24:
		case USB_PRODUCT_FTDI_LCD_MX200:
		case USB_PRODUCT_FTDI_LCD_CFA_631:
		case USB_PRODUCT_FTDI_LCD_CFA_632:
		case USB_PRODUCT_FTDI_LCD_CFA_633:
		case USB_PRODUCT_FTDI_LCD_CFA_634:
		case USB_PRODUCT_FTDI_MHAM_KW:
		case USB_PRODUCT_FTDI_MHAM_YS:
		case USB_PRODUCT_FTDI_MHAM_Y6:
		case USB_PRODUCT_FTDI_MHAM_Y8:
		case USB_PRODUCT_FTDI_MHAM_IC:
		case USB_PRODUCT_FTDI_MHAM_DB9:
		case USB_PRODUCT_FTDI_MHAM_RS232:
		case USB_PRODUCT_FTDI_MHAM_Y9:
		case USB_PRODUCT_SEALEVEL_USBSERIAL:
		case USB_PRODUCT_FTDI_MJS_SIRIUS_PC:
			sc->sc_type = UFTDI_TYPE_8U232AM;
			sc->sc_hdrlen = 0;
			break;
		
		default:		/* Can't happen */
			goto bad;
		}
		break;

	case USB_VENDOR_INTREPIDCS:
		switch (uaa->product) {
		case USB_PRODUCT_INTREPIDCS_VALUECAN:
		case USB_PRODUCT_INTREPIDCS_NEOVI:
		sc->sc_type = UFTDI_TYPE_8U232AM;
			sc->sc_hdrlen = 0;
			break;

		default:                /* Can't happen */
			goto bad;
		}
		break;

	case USB_VENDOR_SIIG2:
		switch (uaa->product) {
		case USB_PRODUCT_SIIG2_US2308:
			sc->sc_type = UFTDI_TYPE_8U232AM;
			sc->sc_hdrlen = 0;
			break;

		default:		/* Can't happen */
			goto bad;
		}
		break;

	case USB_VENDOR_BBELECTRONICS:
		switch( uaa->product ){
		case USB_PRODUCT_BBELECTRONICS_USOTL4:
			sc->sc_type = UFTDI_TYPE_8U232AM;
			sc->sc_hdrlen = 0;
			break;
		default:		/* Can't happen */
			goto bad;
		}
		break;

	case USB_VENDOR_FALCOM:
		switch( uaa->product ){
		case USB_PRODUCT_FALCOM_TWIST:
		case USB_PRODUCT_FALCOM_SAMBA:
			sc->sc_type = UFTDI_TYPE_8U232AM;
			sc->sc_hdrlen = 0;
			break;
		default:		/* Can't happen */
			goto bad;
		}
		break;
	}		


	uca.bulkin = uca.bulkout = -1;
	for (i = 0; i < id->bNumEndpoints; i++) {
		int addr, dir, attr;
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			printf("%s: could not read endpoint descriptor\n",
			    devname);
			goto bad;
		}

		addr = ed->bEndpointAddress;
		dir = UE_GET_DIR(ed->bEndpointAddress);
		attr = ed->bmAttributes & UE_XFERTYPE;
		if (dir == UE_DIR_IN && attr == UE_BULK)
			uca.bulkin = addr;
		else if (dir == UE_DIR_OUT && attr == UE_BULK)
			uca.bulkout = addr;
		else {
			printf("%s: unexpected endpoint\n", devname);
			goto bad;
		}
	}
	if (uca.bulkin == -1) {
		printf("%s: Could not find data bulk in\n",
		       USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	if (uca.bulkout == -1) {
		printf("%s: Could not find data bulk out\n",
		       USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	if (uaa->iface == NULL)
		uca.portno = FTDI_PIT_SIOA;
	else
		uca.portno = FTDI_PIT_SIOA + id->bInterfaceNumber;
	/* bulkin, bulkout set above */
	uca.ibufsize = UFTDIIBUFSIZE;
	uca.obufsize = UFTDIOBUFSIZE - sc->sc_hdrlen;
	uca.ibufsizepad = UFTDIIBUFSIZE;
	uca.opkthdrlen = sc->sc_hdrlen;
	uca.device = dev;
	uca.iface = iface;
	uca.methods = &uftdi_methods;
	uca.arg = sc;
	uca.info = NULL;

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	DPRINTF(("uftdi: in=0x%x out=0x%x\n", uca.bulkin, uca.bulkout));
	sc->sc_subdev = config_found_sm(self, &uca, ucomprint, ucomsubmatch);

	return;

bad:
	DPRINTF(("uftdi_attach: ATTACH ERROR\n"));
	sc->sc_dying = 1;
}

int
uftdi_activate(device_ptr_t self, enum devact act)
{
	struct uftdi_softc *sc = (struct uftdi_softc *)self;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_subdev != NULL)
			rv = config_deactivate(sc->sc_subdev);
		sc->sc_dying = 1;
		break;
	}
	return (rv);
}

int
uftdi_detach(device_ptr_t self, int flags)
{
	struct uftdi_softc *sc = (struct uftdi_softc *)self;

	DPRINTF(("uftdi_detach: sc=%p flags=%d\n", sc, flags));
	sc->sc_dying = 1;
	if (sc->sc_subdev != NULL) {
		config_detach(sc->sc_subdev, flags);
		sc->sc_subdev = NULL;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	return (0);
}

Static int
uftdi_open(void *vsc, int portno)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	usbd_status err;
	struct termios t;

	DPRINTF(("uftdi_open: sc=%p\n", sc));

	if (sc->sc_dying)
		return (EIO);

	/* Perform a full reset on the device */
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_RESET;
	USETW(req.wValue, FTDI_SIO_RESET_SIO);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	/* Set 9600 baud, 2 stop bits, no parity, 8 bits */
	t.c_ospeed = 9600;
	t.c_cflag = CSTOPB | CS8;
	(void)uftdi_param(sc, portno, &t);

	/* Turn on RTS/CTS flow control */
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_FLOW_CTRL;
	USETW(req.wValue, 0);
	USETW2(req.wIndex, FTDI_SIO_RTS_CTS_HS, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	return (0);
}

Static void
uftdi_read(void *vsc, int portno, u_char **ptr, u_int32_t *count)
{
	struct uftdi_softc *sc = vsc;
	u_char msr, lsr;

	DPRINTFN(15,("uftdi_read: sc=%p, port=%d count=%d\n", sc, portno,
		     *count));

	msr = FTDI_GET_MSR(*ptr);
	lsr = FTDI_GET_LSR(*ptr);

#ifdef UFTDI_DEBUG
	if (*count != 2)
		DPRINTFN(10,("uftdi_read: sc=%p, port=%d count=%d data[0]="
			    "0x%02x\n", sc, portno, *count, (*ptr)[2]));
#endif

	if (sc->sc_msr != msr ||
	    (sc->sc_lsr & FTDI_LSR_MASK) != (lsr & FTDI_LSR_MASK)) {
		DPRINTF(("uftdi_read: status change msr=0x%02x(0x%02x) "
			 "lsr=0x%02x(0x%02x)\n", msr, sc->sc_msr,
			 lsr, sc->sc_lsr));
		sc->sc_msr = msr;
		sc->sc_lsr = lsr;
		ucom_status_change((struct ucom_softc *)sc->sc_subdev);
	}

	/* Pick up status and adjust data part. */
	*ptr += 2;
	*count -= 2;
}

Static void
uftdi_write(void *vsc, int portno, u_char *to, u_char *from, u_int32_t *count)
{
	struct uftdi_softc *sc = vsc;

	DPRINTFN(10,("uftdi_write: sc=%p, port=%d count=%u data[0]=0x%02x\n",
		     vsc, portno, *count, from[0]));

	/* Make length tag and copy data */
	if (sc->sc_hdrlen > 0)
		*to = FTDI_OUT_TAG(*count, portno);

	memcpy(to + sc->sc_hdrlen, from, *count);
	*count += sc->sc_hdrlen;
}

Static void
uftdi_set(void *vsc, int portno, int reg, int onoff)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	int ctl;

	DPRINTF(("uftdi_set: sc=%p, port=%d reg=%d onoff=%d\n", vsc, portno,
		 reg, onoff));

	switch (reg) {
	case UCOM_SET_DTR:
		ctl = onoff ? FTDI_SIO_SET_DTR_HIGH : FTDI_SIO_SET_DTR_LOW;
		break;
	case UCOM_SET_RTS:
		ctl = onoff ? FTDI_SIO_SET_RTS_HIGH : FTDI_SIO_SET_RTS_LOW;
		break;
	case UCOM_SET_BREAK:
		uftdi_break(sc, portno, onoff);
		return;
	default:
		return;
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_MODEM_CTRL;
	USETW(req.wValue, ctl);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_set: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	(void)usbd_do_request(sc->sc_udev, &req, NULL);
}

Static int
uftdi_param(void *vsc, int portno, struct termios *t)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	usbd_status err;
	int rate, data, flow;

	DPRINTF(("uftdi_param: sc=%p\n", sc));

	if (sc->sc_dying)
		return (EIO);

	switch (sc->sc_type) {
	case UFTDI_TYPE_SIO:
		switch (t->c_ospeed) {
		case 300: rate = ftdi_sio_b300; break;
		case 600: rate = ftdi_sio_b600; break;
		case 1200: rate = ftdi_sio_b1200; break;
		case 2400: rate = ftdi_sio_b2400; break;
		case 4800: rate = ftdi_sio_b4800; break;
		case 9600: rate = ftdi_sio_b9600; break;
		case 19200: rate = ftdi_sio_b19200; break;
		case 38400: rate = ftdi_sio_b38400; break;
		case 57600: rate = ftdi_sio_b57600; break;
		case 115200: rate = ftdi_sio_b115200; break;
		default:
			return (EINVAL);
		}
		break;

	case UFTDI_TYPE_8U232AM:
		if (uftdi_8u232am_getrate(t->c_ospeed, &rate) == -1)
			return (EINVAL);
		break;
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_BAUD_RATE;
	USETW(req.wValue, rate);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_param: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	if (ISSET(t->c_cflag, CSTOPB))
		data = FTDI_SIO_SET_DATA_STOP_BITS_2;
	else
		data = FTDI_SIO_SET_DATA_STOP_BITS_1;
	if (ISSET(t->c_cflag, PARENB)) {
		if (ISSET(t->c_cflag, PARODD))
			data |= FTDI_SIO_SET_DATA_PARITY_ODD;
		else
			data |= FTDI_SIO_SET_DATA_PARITY_EVEN;
	} else
		data |= FTDI_SIO_SET_DATA_PARITY_NONE;
	switch (ISSET(t->c_cflag, CSIZE)) {
	case CS5:
		data |= FTDI_SIO_SET_DATA_BITS(5);
		break;
	case CS6:
		data |= FTDI_SIO_SET_DATA_BITS(6);
		break;
	case CS7:
		data |= FTDI_SIO_SET_DATA_BITS(7);
		break;
	case CS8:
		data |= FTDI_SIO_SET_DATA_BITS(8);
		break;
	}
	sc->last_lcr = data;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_DATA;
	USETW(req.wValue, data);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	DPRINTFN(2,("uftdi_param: reqtype=0x%02x req=0x%02x value=0x%04x "
		    "index=0x%04x len=%d\n", req.bmRequestType, req.bRequest,
		    UGETW(req.wValue), UGETW(req.wIndex), UGETW(req.wLength)));
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	if (ISSET(t->c_cflag, CRTSCTS)) {
		flow = FTDI_SIO_RTS_CTS_HS;
		USETW(req.wValue, 0);
	} else if (ISSET(t->c_iflag, IXON|IXOFF)) {
		flow = FTDI_SIO_XON_XOFF_HS;
		USETW2(req.wValue, t->c_cc[VSTOP], t->c_cc[VSTART]);
	} else {
		flow = FTDI_SIO_DISABLE_FLOW_CTRL;
		USETW(req.wValue, 0);
	}
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_FLOW_CTRL;
	USETW2(req.wIndex, flow, portno);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->sc_udev, &req, NULL);
	if (err)
		return (EIO);

	return (0);
}

void
uftdi_get_status(void *vsc, int portno, u_char *lsr, u_char *msr)
{
	struct uftdi_softc *sc = vsc;

	DPRINTF(("uftdi_status: msr=0x%02x lsr=0x%02x\n",
		 sc->sc_msr, sc->sc_lsr));

	if (msr != NULL)
		*msr = sc->sc_msr;
	if (lsr != NULL)
		*lsr = sc->sc_lsr;
}

void
uftdi_break(void *vsc, int portno, int onoff)
{
	struct uftdi_softc *sc = vsc;
	usb_device_request_t req;
	int data;

	DPRINTF(("uftdi_break: sc=%p, port=%d onoff=%d\n", vsc, portno,
		  onoff));

	if (onoff) {
		data = sc->last_lcr | FTDI_SIO_SET_BREAK;
	} else {
		data = sc->last_lcr;
	}

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = FTDI_SIO_SET_DATA;
	USETW(req.wValue, data);
	USETW(req.wIndex, portno);
	USETW(req.wLength, 0);
	(void)usbd_do_request(sc->sc_udev, &req, NULL);
}

Static int
uftdi_8u232am_getrate(speed_t speed, int *rate)
{
	/* Table of the nearest even powers-of-2 for values 0..15. */
	static const unsigned char roundoff[16] = {
		0, 2, 2, 4,  4,  4,  8,  8,
		8, 8, 8, 8, 16, 16, 16, 16,
	};

	unsigned int d, freq;
	int result;

	if (speed <= 0)
		return (-1);

	/* Special cases for 2M and 3M. */
	if (speed >= 3000000 * 100 / 103 &&
	    speed <= 3000000 * 100 / 97) {
		result = 0;
		goto done;
	}
	if (speed >= 2000000 * 100 / 103 &&
	    speed <= 2000000 * 100 / 97) {
		result = 1;
		goto done;
	}

	d = (FTDI_8U232AM_FREQ << 4) / speed;
	d = (d & ~15) + roundoff[d & 15];

	if (d < FTDI_8U232AM_MIN_DIV)
		d = FTDI_8U232AM_MIN_DIV;
	else if (d > FTDI_8U232AM_MAX_DIV)
		d = FTDI_8U232AM_MAX_DIV;

	/* 
	 * Calculate the frequency needed for d to exactly divide down
	 * to our target speed, and check that the actual frequency is
	 * within 3% of this.
	 */
	freq = speed * d;
	if (freq < (quad_t)(FTDI_8U232AM_FREQ << 4) * 100 / 103 ||
	    freq > (quad_t)(FTDI_8U232AM_FREQ << 4) * 100 / 97)
		return (-1);

	/* 
	 * Pack the divisor into the resultant value.  The lower
	 * 14-bits hold the integral part, while the upper 2 bits
	 * encode the fractional component: either 0, 0.5, 0.25, or
	 * 0.125.
	 */
	result = d >> 4;
	if (d & 8)
		result |= 0x4000;
	else if (d & 4)
		result |= 0x8000;
	else if (d & 2)
		result |= 0xc000;

done:
	*rate = result;
	return (0);
}
