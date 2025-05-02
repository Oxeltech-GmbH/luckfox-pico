/*
 * usb-vhci-hcd.h -- VHCI USB host controller driver header.
 *
 * Copyright (C) 2007-2008 Conemis AG Karlsruhe Germany
 * Copyright (C) 2007-2010 Michael Singer <michael@a-singer.de>
 * Copyright (C) 2016-2024 Electronic Team, Inc. All Rights Reserved
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _EVEUSB_VHCI_HCD_H
#define _EVEUSB_VHCI_HCD_H

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/device.h>
#include <asm/atomic.h>

// this is undefined in linux >= 2.6.35
#ifndef USB_PORT_FEAT_HIGHSPEED
#	define USB_PORT_FEAT_HIGHSPEED (10)
#endif

#include "eveusb-vhci.h"
#include "conf/eveusb-vhci.config.h"

struct eveusb_vhci_port
{
	u16 port_status;
	u16 port_change;
	u8 port_flags;
};

enum eveusb_vhci_rh_state
{
	USB_VHCI_RH_RESET     = 0,
	USB_VHCI_RH_SUSPENDED = 1,
	USB_VHCI_RH_RUNNING   = 2
} __attribute__((packed));

struct eveusb_vhci;

struct eveusb_vhci_ifc
{
	const char *ifc_desc;
	struct module *owner;
	size_t ifc_priv_size;

	// callbacks for backend drivers
	int (*init)(void *context, void *ifc_priv);
	void (*destroy)(void *ifc_priv);
	void (*wakeup)(struct eveusb_vhci *vhci);
};

struct eveusb_vhci_urb_priv
{
	struct urb *urb;
	struct list_head urbp_list;
	atomic_t status;
};

struct eveusb_vhci {
	spinlock_t lock;

	const struct eveusb_vhci_ifc *ifc;
	struct platform_device *pdev;

	u8 port_count;

	struct eveusb_vhci_hcd *vhci_hcd_hs;
	struct eveusb_vhci_hcd *vhci_hcd_ss;

	// private data for backend drivers
	unsigned long ifc_priv[0] __attribute__((aligned(sizeof(unsigned long))));
};

struct eveusb_vhci_hcd
{
	struct eveusb_vhci *vhci;

	struct eveusb_vhci_port *ports;
	u32 port_update;

	atomic_t frame_num;
	enum eveusb_vhci_rh_state rh_state;

	// TODO: implement timer for incrementing frame_num every millisecond
	//struct timer_list timer;

	// urbs which are waiting to get fetched by user space are in this list
	struct list_head urbp_list_inbox;

	// urbs which were fetched by user space but not already given back are in this list
	struct list_head urbp_list_fetched;

	// urbs which were fetched by user space and not already given back, and which should be
	// canceled are in this list
	struct list_head urbp_list_cancel;

	// urbs which were fetched by user space and not already given back, and for which the
	// user space already knows about the cancelation state are in this list
	struct list_head urbp_list_canceling;

	u8 port_count;
};

int vhci_init_attr_group(void);
void vhci_finish_attr_group(void);

static inline struct eveusb_vhci *pdev_to_vhci(struct platform_device *pdev)
{
	return pdev->dev.platform_data;
}

static inline void *vhci_to_ifc(struct eveusb_vhci *vhci)
{
	return &vhci->ifc_priv;
}

static inline struct eveusb_vhci *ifc_to_vhci(void *ifc)
{
	return container_of(ifc, struct eveusb_vhci, ifc_priv);
}

static inline struct eveusb_vhci_hcd *vhci_to_hs_vhcihcd(struct eveusb_vhci *vhci)
{
	return vhci->vhci_hcd_hs;
}

static inline struct eveusb_vhci_hcd *vhci_to_ss_vhcihcd(struct eveusb_vhci *vhci)
{
	return vhci->vhci_hcd_ss;
}

static inline struct platform_device *vhci_to_pdev(struct eveusb_vhci *vhci)
{
	return vhci->pdev;
}

static inline struct usb_hcd *vhcihcd_to_usbhcd(struct eveusb_vhci_hcd *vhc)
{
	return container_of((void *)vhc, struct usb_hcd, hcd_priv);
}

static inline struct usb_hcd *vhci_to_hs_usbhcd(struct eveusb_vhci *vhci)
{
	return vhcihcd_to_usbhcd(vhci_to_hs_vhcihcd(vhci));
}

static inline struct usb_hcd *vhci_to_ss_usbhcd(struct eveusb_vhci *vhci)
{
	return vhcihcd_to_usbhcd(vhci_to_ss_vhcihcd(vhci));
}

static inline struct eveusb_vhci_hcd *usbhcd_to_vhcihcd(struct usb_hcd *hcd)
{
	return (struct eveusb_vhci_hcd *)&hcd->hcd_priv;
}

static inline struct device *usbhcd_to_dev(struct usb_hcd *hcd)
{
	return hcd->self.controller;
}

static inline struct device *vhcihcd_to_dev(struct eveusb_vhci_hcd *vhc)
{
	return usbhcd_to_dev(vhcihcd_to_usbhcd(vhc));
}

static inline struct platform_device *vhcihcd_to_pdev(struct eveusb_vhci_hcd *vhc)
{
	return to_platform_device(vhcihcd_to_dev(vhc));
}

static inline struct eveusb_vhci *vhcihcd_to_vhci(struct eveusb_vhci_hcd *vhc)
{
	return pdev_to_vhci(vhcihcd_to_pdev(vhc));
}

const char *eveusb_vhci_dev_name(struct eveusb_vhci *vhci);
int eveusb_vhci_dev_id(struct eveusb_vhci *vhci);
int eveusb_vhci_dev_hs_busnum(struct eveusb_vhci *vhci);
int eveusb_vhci_dev_ss_busnum(struct eveusb_vhci *vhci);
void eveusb_vhci_maybe_set_status(struct eveusb_vhci_urb_priv *urbp, int status);
void eveusb_vhci_urb_giveback(struct eveusb_vhci_hcd *vhc, struct eveusb_vhci_urb_priv *urbp, unsigned long *lock_flags);
int eveusb_vhci_hcd_register(const struct eveusb_vhci_ifc *ifc, void *context, u8 port_count, struct eveusb_vhci **vhci_ret);
int eveusb_vhci_hcd_unregister(struct eveusb_vhci *vhci);
int eveusb_vhci_hcd_has_work(struct eveusb_vhci *vhci);
int eveusb_vhci_apply_port_stat(struct eveusb_vhci *vhci, u16 port_status, u16 port_change, u8 port_index, u8 port_flags);

int vhci_iocifc_init(void);
void vhci_iocifc_exit(void);

#endif
