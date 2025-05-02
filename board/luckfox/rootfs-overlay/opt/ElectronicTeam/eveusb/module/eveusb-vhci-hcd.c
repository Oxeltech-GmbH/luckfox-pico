/*
 * usb-vhci-hcd.c -- VHCI USB host controller driver.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/usb.h>
#include <linux/fs.h>
#include <linux/device.h>

#include <asm/atomic.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>

#include "eveusb-vhci-hcd.h"

#define DRIVER_NAME "eveusb_vhci_hcd"
#define DRIVER_DESC "EVEUSB Virtual Host Controller Interface"
#define DRIVER_VERSION USB_VHCI_HCD_VERSION " (" USB_VHCI_HCD_DATE ")"

#ifdef vhci_printk
#	undef vhci_printk
#endif
#define vhci_printk(level, fmt, args...) \
	printk(level DRIVER_NAME ": " fmt, ## args)
#ifdef vhci_dbg
#	undef vhci_dbg
#endif
#ifdef DEBUG
#	warning DEBUG is defined
#	define vhci_dbg(fmt, args...) \
		if(debug_output) vhci_printk(KERN_DEBUG, fmt, ## args)
#else
#	define vhci_dbg(fmt, args...) do {} while(0)
#endif
#ifdef trace_function
#	undef trace_function
#endif
#ifdef DEBUG
#	define trace_function(dev) \
		if(debug_output) dev_dbg((dev), "%s%s\n", \
			in_interrupt() ? "IN_INTERRUPT: " : "", __FUNCTION__)
#else
#	define trace_function(dev) do {} while(0)
#endif

static const char driver_name[] = DRIVER_NAME;
static const char driver_desc[] = DRIVER_DESC;
#ifdef DEBUG
static unsigned int debug_output = 0;
#endif

MODULE_DESCRIPTION(DRIVER_DESC " driver");
MODULE_AUTHOR("Michael Singer <michael@a-singer.de>");
MODULE_LICENSE("GPL");

static inline const char *vhci_dev_name(struct device *dev)
{
#ifdef OLD_DEV_BUS_ID
	return dev->bus_id;
#else
	return dev_name(dev);
#endif
}

const char *eveusb_vhci_dev_name(struct eveusb_vhci *vhci)
{
	if(likely(vhci->vhci_hcd_hs && vhci->vhci_hcd_ss))
	{
		struct platform_device *pdev = vhci_to_pdev(vhci);
		return vhci_dev_name(&pdev->dev);
	}

	return "<unknown>";
}
// EXPORT_SYMBOL_GPL(eveusb_vhci_dev_name);

int eveusb_vhci_dev_id(struct eveusb_vhci *vhci)
{
	return vhci_to_pdev(vhci)->id;
}
// EXPORT_SYMBOL_GPL(eveusb_vhci_dev_id);

int eveusb_vhci_dev_hs_busnum(struct eveusb_vhci *vhci)
{
	return vhci_to_hs_usbhcd(vhci)->self.busnum;
}
// EXPORT_SYMBOL_GPL(eveusb_vhci_dev_hs_busnum);

int eveusb_vhci_dev_ss_busnum(struct eveusb_vhci *vhci)
{
	return vhci_to_ss_usbhcd(vhci)->self.busnum;
}
// EXPORT_SYMBOL_GPL(eveusb_vhci_dev_ss_busnum);

void eveusb_vhci_maybe_set_status(struct eveusb_vhci_urb_priv *urbp, int status)
{
#ifdef OLD_GIVEBACK_MECH
	struct urb *const urb = urbp->urb;
	unsigned long flags;
	spin_lock_irqsave(&urb->lock, flags);
	if(urb->status == -EINPROGRESS)
		urb->status = status;
	spin_unlock_irqrestore(&urb->lock, flags);
#else
	(void)atomic_cmpxchg(&urbp->status, -EINPROGRESS, status);
#endif
}
// EXPORT_SYMBOL_GPL(eveusb_vhci_maybe_set_status);

#ifdef DEBUG
#include "eveusb-vhci-dump-urb.c"
#else
static inline void dump_urb(struct urb *urb) {/* do nothing */}
#endif

// caller has vhci->lock
// first port is port# 1 (not 0)
static void vhci_port_update(struct eveusb_vhci_hcd *vhci_hcd, u8 port)
{
	struct eveusb_vhci *vhci = vhcihcd_to_vhci(vhci_hcd);
	vhci_hcd->port_update |= 1 << port;
	vhci->ifc->wakeup(vhci);
}

// gives the urb back to its original owner/creator.
// caller owns vhci->lock and has irq disabled.
void eveusb_vhci_urb_giveback(struct eveusb_vhci_hcd *vhci_hcd, struct eveusb_vhci_urb_priv *urbp, unsigned long *lock_flags)
{
	struct device *dev;
	struct usb_hcd *hcd;
	struct eveusb_vhci *vhci;
	struct urb *const urb = urbp->urb;
	struct usb_device *const udev = urb->dev;
	unsigned long flags = *lock_flags;
#ifndef OLD_GIVEBACK_MECH
	int status;
#endif
	hcd = vhcihcd_to_usbhcd(vhci_hcd);
	dev = vhcihcd_to_dev(vhci_hcd);
	vhci = vhcihcd_to_vhci(vhci_hcd);
	trace_function(dev);
#ifndef OLD_GIVEBACK_MECH
	status = atomic_read(&urbp->status);
#endif
	urb->hcpriv = NULL;
	list_del(&urbp->urbp_list);
#ifndef OLD_GIVEBACK_MECH
	usb_hcd_unlink_urb_from_ep(hcd, urb);
#endif
	spin_unlock_irqrestore(&vhci->lock, flags);
	kfree(urbp);
	dump_urb(urb);
#ifdef OLD_GIVEBACK_MECH
	usb_hcd_giveback_urb(hcd, urb);
#else
#	ifdef DEBUG
	if(debug_output) vhci_printk(KERN_DEBUG, "eveusb_vhci_urb_giveback: status=%d(%s)\n", status, get_status_str(status));
#	endif
	usb_hcd_giveback_urb(hcd, urb, status);
#endif
	spin_lock_irqsave(&vhci->lock, flags);
	usb_put_dev(udev);

	*lock_flags = flags;
}
// EXPORT_SYMBOL_GPL(eveusb_vhci_urb_giveback);

#ifdef OLD_GIVEBACK_MECH
static int vhci_urb_enqueue(struct usb_hcd *hcd, struct usb_host_endpoint *ep, struct urb *urb, gfp_t mem_flags)
#else
static int vhci_urb_enqueue(struct usb_hcd *hcd, struct urb *urb, gfp_t mem_flags)
#endif
{
	struct eveusb_vhci_hcd *vhci_hcd;
	struct device *dev;
	struct eveusb_vhci_urb_priv *urbp;
	struct eveusb_vhci *vhci;
	unsigned long flags;
#ifndef OLD_GIVEBACK_MECH
	int retval;
#endif

	vhci_hcd = usbhcd_to_vhcihcd(hcd);
	dev = vhcihcd_to_dev(vhci_hcd);
	vhci = vhcihcd_to_vhci(vhci_hcd);

	trace_function(dev);

	if(unlikely(!urb->transfer_buffer && !urb->num_sgs && urb->transfer_buffer_length)) {
		vhci_printk(KERN_ERR, "-EINVAL\n");
		return -EINVAL;
	}

	urbp = kzalloc(sizeof *urbp, mem_flags);
	if(unlikely(!urbp)) {
		vhci_printk(KERN_ERR, "-NOMEM\n");
		return -ENOMEM;
	}
	urbp->urb = urb;
	atomic_set(&urbp->status, urb->status);

	vhci_dbg("vhci_urb_enqueue: urb->status = %d(%s)",urb->status,get_status_str(urb->status));

	spin_lock_irqsave(&vhci->lock, flags);
#ifndef OLD_GIVEBACK_MECH
	retval = usb_hcd_link_urb_to_ep(hcd, urb);
	if(unlikely(retval))
	{
		kfree(urbp);
		spin_unlock_irqrestore(&vhci->lock, flags);
		vhci_printk(KERN_ERR, "!usb_hcd_link_urb_to_ep(%d)\n", retval);
		return retval;
	}
#endif
	usb_get_dev(urb->dev);
	list_add_tail(&urbp->urbp_list, &vhci_hcd->urbp_list_inbox);
	urb->hcpriv = urbp;
	spin_unlock_irqrestore(&vhci->lock, flags);
	vhci->ifc->wakeup(vhci);
	return 0;
}

#ifdef OLD_GIVEBACK_MECH
static int vhci_urb_dequeue(struct usb_hcd *hcd, struct urb *urb)
#else
static int vhci_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status)
#endif
{
	struct eveusb_vhci_hcd *vhci_hcd;
	struct device *dev;
	struct eveusb_vhci *vhci;
	unsigned long flags;
	struct eveusb_vhci_urb_priv *entry, *urbp = NULL;
#ifndef OLD_GIVEBACK_MECH
	int retval;
#endif

	vhci_hcd = usbhcd_to_vhcihcd(hcd);
	dev = vhcihcd_to_dev(vhci_hcd);
	vhci = vhcihcd_to_vhci(vhci_hcd);

	trace_function(dev);

	spin_lock_irqsave(&vhci->lock, flags);
#ifndef OLD_GIVEBACK_MECH
	retval = usb_hcd_check_unlink_urb(hcd, urb, status);
	if(retval)
	{
		spin_unlock_irqrestore(&vhci->lock, flags);
		return retval;
	}
#endif

	// search the queue of unprocessed urbs (inbox)
	list_for_each_entry(entry, &vhci_hcd->urbp_list_inbox, urbp_list)
	{
		if(entry->urb == urb)
		{
			urbp = entry;
			break;
		}
	}

	// if found in inbox
	if(urbp)
		eveusb_vhci_urb_giveback(vhci_hcd, urbp, &flags);
	else // if not found...
	{
		// ...then check if the urb is on a vacation through user space
		list_for_each_entry(entry, &vhci_hcd->urbp_list_fetched, urbp_list)
		{
			if(entry->urb == urb)
			{
				// move it into the cancel list
				list_move_tail(&entry->urbp_list, &vhci_hcd->urbp_list_cancel);
				vhci->ifc->wakeup(vhci);
				break;
			}
		}
	}

	spin_unlock_irqrestore(&vhci->lock, flags);
	return 0;
}

/*
static void vhci_timer(unsigned long _vhc)
{
	struct eveusb_vhci_hcd *vhci_hcd = (struct eveusb_vhci_hcd *)_vhc;
}
*/

 /* Check if a port is power on */
 static int port_is_power_on(struct usb_hcd *hcd, unsigned portstatus)
 {
 	int ret = 0;
 
 	if (hcd->speed == HCD_USB3) {
 		if (portstatus & USB_SS_PORT_STAT_POWER)
 			ret = 1;
 	} else {
 		if (portstatus & USB_PORT_STAT_POWER)
 			ret = 1;
 	}
 
 	return ret;
 }
 
 /* Check if a port is suspended(USB2.0 port) or in U3 state(USB3.0 port) */
 static int port_is_suspended(struct usb_hcd *hcd, unsigned portstatus)
 {
 	int ret = 0;
 
 	if (hcd->speed == HCD_USB3) {
 		if ((portstatus & USB_PORT_STAT_LINK_STATE) == USB_SS_PORT_LS_U3)
 			ret = 1;
 	} else {
 		if (portstatus & USB_PORT_STAT_SUSPEND)
 			ret = 1;
 	}
 
 	return ret;
 }

static int vhci_hub_status(struct usb_hcd *hcd, char *buf)
{
	struct eveusb_vhci_hcd *vhci_hcd;
	struct eveusb_vhci *vhci;
	struct device *dev;
	unsigned long flags;
	u8 port;
	int changed = 0;
	int idx, rel_bit, abs_bit;

	vhci_hcd = usbhcd_to_vhcihcd(hcd);
	vhci = vhcihcd_to_vhci(vhci_hcd);
	dev = vhcihcd_to_dev(vhci_hcd);

	trace_function(dev);

	memset(buf, 0, 1 + vhci_hcd->port_count / 8);

	spin_lock_irqsave(&vhci->lock, flags);
	if(!test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags))
	{
		spin_unlock_irqrestore(&vhci->lock, flags);
		return 0;
	}

	for(port = 0; port < vhci_hcd->port_count; port++)
	{
		if(vhci_hcd->ports[port].port_change)
		{
			abs_bit = port + 1;
			idx     = abs_bit / (sizeof *buf * 8);
			rel_bit = abs_bit % (sizeof *buf * 8);
			buf[idx] |= (1 << rel_bit);
			changed = 1;
		}
#ifdef DEBUG
		if(debug_output) dev_dbg(dev, "port %d status 0x%04x has changes at 0x%04x\n", (int)(port + 1), (int)vhci_hcd->ports[port].port_status, (int)vhci_hcd->ports[port].port_change);
#endif
	}

	if(vhci_hcd->rh_state == USB_VHCI_RH_SUSPENDED && changed)
		usb_hcd_resume_root_hub(hcd);

	spin_unlock_irqrestore(&vhci->lock, flags);
	return changed;
}

/* usb 3.0 root hub device descriptor */
static struct {
	struct usb_bos_descriptor bos;
	struct usb_ss_cap_descriptor ss_cap;
} __packed usb3_bos_desc = {

	.bos = {
		.bLength		= USB_DT_BOS_SIZE,
		.bDescriptorType	= USB_DT_BOS,
		.wTotalLength		= cpu_to_le16(sizeof(usb3_bos_desc)),
		.bNumDeviceCaps		= 1,
	},
	.ss_cap = {
		.bLength		= USB_DT_USB_SS_CAP_SIZE,
		.bDescriptorType	= USB_DT_DEVICE_CAPABILITY,
		.bDevCapabilityType	= USB_SS_CAP_TYPE,
		.wSpeedSupported	= cpu_to_le16(USB_5GBPS_OPERATION),
		.bFunctionalitySupport	= ilog2(USB_5GBPS_OPERATION),
	},
};

static inline void
ss_hub_descriptor(struct eveusb_vhci_hcd *vhci_hcd, struct usb_hub_descriptor *desc)
{
	memset(desc, 0, sizeof *desc);
	desc->bDescriptorType = USB_DT_SS_HUB;
	desc->bDescLength = 12;
	desc->wHubCharacteristics = cpu_to_le16(
		HUB_CHAR_INDV_PORT_LPSM | HUB_CHAR_COMMON_OCPM);
	desc->bNbrPorts = vhci_hcd->port_count;
	desc->u.ss.bHubHdrDecLat = 0x04;
	desc->u.ss.DeviceRemovable = 0xffff;
}

static inline void hub_descriptor(struct eveusb_vhci_hcd *vhci_hcd, struct usb_hub_descriptor *desc)
{
	int width;

	memset(desc, 0, sizeof(*desc));
	desc->bDescriptorType = USB_DT_HUB;
	desc->wHubCharacteristics = cpu_to_le16(
		HUB_CHAR_INDV_PORT_LPSM | HUB_CHAR_COMMON_OCPM);

	desc->bNbrPorts = vhci_hcd->port_count;
	width = desc->bNbrPorts / 8 + 1;
	desc->bDescLength = USB_DT_HUB_NONVAR_SIZE + 2 * width;
	memset(&desc->u.hs.DeviceRemovable[0], 0, width);
	memset(&desc->u.hs.DeviceRemovable[width], 0xff, width);
}

static int vhci_hub_control(struct usb_hcd *hcd,
                            u16 typeReq,
                            u16 wValue,
                            u16 wIndex,
                            char *buf,
                            u16 wLength)
{
	struct eveusb_vhci_hcd *vhci_hcd;
	struct eveusb_vhci *vhci;
	struct device *dev;
	int retval = 0;
	unsigned long flags;
	u16 *ps, *pc;
	u8 *pf;
	u8 port, has_changes = 0;

	vhci_hcd = usbhcd_to_vhcihcd(hcd);
	vhci = vhcihcd_to_vhci(vhci_hcd);
	dev = vhcihcd_to_dev(vhci_hcd);

	trace_function(dev);

	if(unlikely(!test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags)))
		return -ETIMEDOUT;

	spin_lock_irqsave(&vhci->lock, flags);

	switch(typeReq)
	{
	case ClearHubFeature:
	case SetHubFeature:
#ifdef DEBUG
		if(debug_output) dev_dbg(dev, "%s: %sHubFeature [wValue=0x%04x]\n", __FUNCTION__, (typeReq == ClearHubFeature) ? "Clear" : "Set", (int)wValue);
#endif
		if(unlikely(wIndex || wLength || (wValue != C_HUB_LOCAL_POWER && wValue != C_HUB_OVER_CURRENT)))
			goto err;
		break;
	case ClearPortFeature:
#ifdef DEBUG
		if(debug_output) dev_dbg(dev, "%s: ClearPortFeature [wValue=0x%04x, wIndex=%d]\n", __FUNCTION__, (int)wValue, (int)wIndex);
#endif
		if(unlikely(!wIndex || wIndex > vhci_hcd->port_count || wLength))
			goto err;
		ps = &vhci_hcd->ports[wIndex - 1].port_status;
		pc = &vhci_hcd->ports[wIndex - 1].port_change;
		pf = &vhci_hcd->ports[wIndex - 1].port_flags;
		switch(wValue)
		{
		case USB_PORT_FEAT_SUSPEND:
			if (hcd->speed == HCD_USB3) {
				dev_err(dev, "ClearPortFeature: USB_PORT_FEAT_SUSPEND req not supported for USB 3.0 roothub\n");
				goto err;
			}
			// (see USB 2.0 spec section 11.5 and 11.24.2.7.1.3)
			if(port_is_suspended(hcd, *ps))
			{
#ifdef DEBUG
				if(debug_output) dev_dbg(dev, "Port %d resuming\n", (int)wIndex);
#endif
				*pf |= USB_VHCI_PORT_STAT_FLAG_RESUMING;
				vhci_port_update(vhci_hcd, wIndex);
			}
			break;
		case USB_PORT_FEAT_POWER:
			// (see USB 2.0 spec section 11.11 and 11.24.2.7.1.6)
			if(port_is_power_on(hcd, *ps))
			{
#ifdef DEBUG
				if(debug_output) dev_dbg(dev, "Port %d power-off\n", (int)wIndex);
#endif
				// clear all status bits except overcurrent (see USB 2.0 spec section 11.24.2.7.1)
				*ps &= USB_PORT_STAT_OVERCURRENT;
				// clear all change bits except overcurrent (see USB 2.0 spec section 11.24.2.7.2)
				*pc &= USB_PORT_STAT_C_OVERCURRENT;
				// clear resuming flag
				*pf &= ~USB_VHCI_PORT_STAT_FLAG_RESUMING;
				vhci_port_update(vhci_hcd, wIndex);
			}
			break;
		case USB_PORT_FEAT_ENABLE:
			// (see USB 2.0 spec section 11.5.1.4 and 11.24.2.7.{1,2}.2)
			if(*ps & USB_PORT_STAT_ENABLE)
			{
#ifdef DEBUG
				if(debug_output) dev_dbg(dev, "Port %d disabled\n", (int)wIndex);
#endif
				// clear enable and suspend bits (see section 11.24.2.7.1.{2,3})
				*ps &= ~(USB_PORT_STAT_ENABLE | USB_PORT_STAT_SUSPEND);
				// i'm not quite sure if the suspend change bit should be cleared too (see section 11.24.2.7.2.{2,3})
				*pc &= ~(USB_PORT_STAT_C_ENABLE | USB_PORT_STAT_C_SUSPEND);
				// clear resuming flag
				*pf &= ~USB_VHCI_PORT_STAT_FLAG_RESUMING;
				// TODO: maybe we should clear the low/high speed bits here (section 11.24.2.7.1.{7,8})
				vhci_port_update(vhci_hcd, wIndex);
			}
			break;
		case USB_PORT_FEAT_CONNECTION:
		case USB_PORT_FEAT_OVER_CURRENT:
		case USB_PORT_FEAT_RESET:
		case USB_PORT_FEAT_LOWSPEED:
		case USB_PORT_FEAT_HIGHSPEED:
		case USB_PORT_FEAT_INDICATOR:
			break; // no-op
		case USB_PORT_FEAT_C_CONNECTION:
		case USB_PORT_FEAT_C_ENABLE:
		case USB_PORT_FEAT_C_SUSPEND:
		case USB_PORT_FEAT_C_OVER_CURRENT:
		case USB_PORT_FEAT_C_RESET:
			if(*pc & (1 << (wValue - 16)))
			{
				*pc &= ~(1 << (wValue - 16));
				vhci_port_update(vhci_hcd, wIndex);
			}
			break;
		//case USB_PORT_FEAT_TEST:
		default:
			*ps &= ~(1 << wValue);
			vhci_port_update(vhci_hcd, wIndex);
			break;
		}
		break;
	case GetHubDescriptor:
#ifdef DEBUG
		if(debug_output) dev_dbg(dev, "%s: GetHubDescriptor [wValue=0x%04x, wLength=%d]\n", __FUNCTION__, (int)wValue, (int)wLength);
#endif
		if(unlikely(wIndex))
			goto err;
		if (hcd->speed == HCD_USB3 &&
				(wLength < USB_DT_SS_HUB_SIZE ||
				 wValue != (USB_DT_SS_HUB << 8))) {
			dev_err(dev, "Wrong hub descriptor type for USB 3.0 roothub.\n");
			goto err;
		}
		if (hcd->speed == HCD_USB3 && wValue == (USB_DT_SS_HUB << 8))
			ss_hub_descriptor(vhci_hcd, (struct usb_hub_descriptor *) buf);
		else
			hub_descriptor(vhci_hcd, (struct usb_hub_descriptor *) buf);
		break;
	case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
		if (hcd->speed != HCD_USB3)
			goto err;

		if ((wValue >> 8) != USB_DT_BOS)
			goto err;

		memcpy(buf, &usb3_bos_desc, sizeof(usb3_bos_desc));
		retval = sizeof(usb3_bos_desc);
		break;
	case GetHubStatus:
#ifdef DEBUG
		if(debug_output) dev_dbg(dev, "%s: GetHubStatus\n", __FUNCTION__);
#endif
		if(unlikely(wValue || wIndex || wLength != 4))
			goto err;
		buf[0] = buf[1] = buf[2] = buf[3] = 0;
		break;
	case GetPortStatus:
#ifdef DEBUG
		if(debug_output) dev_dbg(dev, "%s: GetPortStatus [wIndex=%d]\n", __FUNCTION__, (int)wIndex);
#endif
		if(unlikely(wValue || !wIndex || wIndex > vhci_hcd->port_count || wLength != 4))
			goto err;
#ifdef DEBUG
		if(debug_output) dev_dbg(dev, "%s: ==> [port_status=0x%04x] [port_change=0x%04x]\n", __FUNCTION__, (int)vhc->ports[wIndex - 1].port_status, (int)vhc->ports[wIndex - 1].port_change);
#endif
		buf[0] = (u8)vhci_hcd->ports[wIndex - 1].port_status;
		buf[1] = (u8)(vhci_hcd->ports[wIndex - 1].port_status >> 8);
		buf[2] = (u8)vhci_hcd->ports[wIndex - 1].port_change;
		buf[3] = (u8)(vhci_hcd->ports[wIndex - 1].port_change >> 8);
		break;
	case SetPortFeature:
#ifdef DEBUG
		if(debug_output) dev_dbg(dev, "%s: SetPortFeature [wValue=0x%04x, wIndex=%d]\n", __FUNCTION__, (int)wValue, (int)wIndex);
#endif
		if(unlikely(!wIndex || wIndex > vhci_hcd->port_count || wLength))
			goto err;
		ps = &vhci_hcd->ports[wIndex - 1].port_status;
		pc = &vhci_hcd->ports[wIndex - 1].port_change;
		pf = &vhci_hcd->ports[wIndex - 1].port_flags;
		switch(wValue)
		{
		case USB_PORT_FEAT_LINK_STATE:
			if (hcd->speed != HCD_USB3) {
				dev_err(dev, "USB_PORT_FEAT_LINK_STATE not supported for USB 2.0 roothub\n");
				goto err;
			}
			break;
		case USB_PORT_FEAT_U1_TIMEOUT:
			/* Fall through */
		case USB_PORT_FEAT_U2_TIMEOUT:
			/* TODO: add suspend/resume support! */
			if (hcd->speed != HCD_USB3) {
				dev_err(dev, "USB_PORT_FEAT_U1/2_TIMEOUT req not supported for USB 2.0 roothub\n");
				goto err;
			}
			break;
		case USB_PORT_FEAT_SUSPEND:
			if (hcd->speed == HCD_USB3) {
				dev_err(dev, "USB_PORT_FEAT_SUSPEND req not supported for USB 3.0 roothub\n");
				goto err;
			}
			// USB 2.0 spec section 11.24.2.7.1.3:
			//  "This bit can be set only if the port’s PORT_ENABLE bit is set and the hub receives
			//  a SetPortFeature(PORT_SUSPEND) request."
			// The spec also says that the suspend bit has to be cleared whenever the enable bit is cleared.
			// (see also section 11.5)
			if((*ps & USB_PORT_STAT_ENABLE) && !port_is_suspended(hcd, *ps))
			{
#ifdef DEBUG
				if(debug_output) dev_dbg(dev, "Port %d suspended\n", (int)wIndex);
#endif
				*ps |= USB_PORT_STAT_SUSPEND;
				vhci_port_update(vhci_hcd, wIndex);
			}
			break;
		case USB_PORT_FEAT_POWER:
			// (see USB 2.0 spec section 11.11 and 11.24.2.7.1.6)
			if(!port_is_power_on(hcd, *ps))
			{
#ifdef DEBUG
				if(debug_output) dev_dbg(dev, "Port %d power-on\n", (int)wIndex);
#endif
			 	if (hcd->speed == HCD_USB3) {
					*ps |= USB_SS_PORT_STAT_POWER;
				} else {
					*ps |= USB_PORT_STAT_POWER;
				}
				vhci_port_update(vhci_hcd, wIndex);
			}
			break;
		case USB_PORT_FEAT_BH_PORT_RESET:
			/* Applicable only for USB3.0 hub */
			if (hcd->speed != HCD_USB3) {
				dev_err(dev, "USB_PORT_FEAT_BH_PORT_RESET req not supported for USB 2.0 roothub\n");
				goto err;
			}
			/* FALLS THROUGH */
		case USB_PORT_FEAT_RESET:
			// (see USB 2.0 spec section 11.24.2.7.1.5)
			// initiate reset only if there is a device plugged into the port and if there isn't already a reset pending
			if((*ps & USB_PORT_STAT_CONNECTION) && !(*ps & USB_PORT_STAT_RESET))
			{
#ifdef DEBUG
				if(debug_output) dev_dbg(dev, "Port %d resetting\n", (int)wIndex);
#endif

				// keep the state of these bits and clear all others
				*ps &= (hcd->speed == HCD_USB3 ? USB_SS_PORT_STAT_POWER : USB_PORT_STAT_POWER)
				     | USB_PORT_STAT_CONNECTION
				     | USB_PORT_STAT_LOW_SPEED
				     | USB_PORT_STAT_HIGH_SPEED
				     | USB_PORT_STAT_OVERCURRENT;

				*ps |= USB_PORT_STAT_RESET; // reset initiated

				// clear resuming flag
				*pf &= ~USB_VHCI_PORT_STAT_FLAG_RESUMING;

				vhci_port_update(vhci_hcd, wIndex);
			}
#ifdef DEBUG
			else if(debug_output) dev_dbg(dev, "Port %d reset not possible because of port_state=%04x\n", (int)wIndex, (int)*ps);
#endif
			break;
		case USB_PORT_FEAT_CONNECTION:
		case USB_PORT_FEAT_OVER_CURRENT:
		case USB_PORT_FEAT_LOWSPEED:
		case USB_PORT_FEAT_HIGHSPEED:
		case USB_PORT_FEAT_INDICATOR:
			break; // no-op
		case USB_PORT_FEAT_C_CONNECTION:
		case USB_PORT_FEAT_C_ENABLE:
		case USB_PORT_FEAT_C_SUSPEND:
		case USB_PORT_FEAT_C_OVER_CURRENT:
		case USB_PORT_FEAT_C_RESET:
			if(!(*pc & (1 << (wValue - 16))))
			{
				*pc |= 1 << (wValue - 16);
				vhci_port_update(vhci_hcd, wIndex);
			}
			break;
		//case USB_PORT_FEAT_ENABLE: // port can't be enabled without reseting (USB 2.0 spec section 11.24.2.7.1.2)
		//case USB_PORT_FEAT_TEST:
		default:
			if (port_is_power_on(hcd, *ps)) {
				*ps |= (1 << wValue);
				vhci_port_update(vhci_hcd, wIndex);
			}
			break;
		}
		break;
	default:
#ifdef DEBUG
		if(debug_output) dev_dbg(dev, "%s: +++UNHANDLED_REQUEST+++ [req=0x%04x, v=0x%04x, i=0x%04x, l=%d]\n", __FUNCTION__, (int)typeReq, (int)wValue, (int)wIndex, (int)wLength);
#endif
err:
#ifdef DEBUG
		if(debug_output) dev_dbg(dev, "%s: STALL\n", __FUNCTION__);
#endif
		// "protocol stall" on error
		retval = -EPIPE;
	}

	for(port = 0; port < vhci_hcd->port_count; port++)
		if(vhci_hcd->ports[port].port_change)
			has_changes = 1;

	spin_unlock_irqrestore(&vhci->lock, flags);

	if(has_changes)
		usb_hcd_poll_rh_status(hcd);
	return retval;
}

static int vhci_bus_suspend(struct usb_hcd *hcd)
{
	struct eveusb_vhci_hcd *vhci_hcd;
	struct eveusb_vhci *vhci;
	struct device *dev;
	unsigned long flags;
	u8 port;

	vhci_hcd = usbhcd_to_vhcihcd(hcd);
	vhci = vhcihcd_to_vhci(vhci_hcd);
	dev = vhcihcd_to_dev(vhci_hcd);

	trace_function(dev);

	spin_lock_irqsave(&vhci->lock, flags);

	if (hcd->speed != HCD_USB3)
	{
		// suspend ports
		for(port = 0; port < vhci_hcd->port_count; port++)
		{
			if((vhci_hcd->ports[port].port_status & USB_PORT_STAT_ENABLE) &&
				!port_is_suspended(hcd, vhci_hcd->ports[port].port_status))
			{
				dev_dbg(dev, "Port %d suspended\n", (int)port + 1);
				vhci_hcd->ports[port].port_status |= USB_PORT_STAT_SUSPEND;
				vhci_hcd->ports[port].port_flags &= ~USB_VHCI_PORT_STAT_FLAG_RESUMING;
				vhci_port_update(vhci_hcd, port + 1);
			}
		}
	}

	// TODO: somehow we have to suppress the resuming of ports while the bus is suspended

	vhci_hcd->rh_state = USB_VHCI_RH_SUSPENDED;
	hcd->state = HC_STATE_SUSPENDED;

	spin_unlock_irqrestore(&vhci->lock, flags);

	return 0;
}

static int vhci_bus_resume(struct usb_hcd *hcd)
{
	struct eveusb_vhci_hcd *vhci_hcd;
	struct eveusb_vhci *vhci;
	struct device *dev;
	int rc = 0;
	unsigned long flags;

	vhci_hcd = usbhcd_to_vhcihcd(hcd);
	vhci = vhcihcd_to_vhci(vhci_hcd);
	dev = vhcihcd_to_dev(vhci_hcd);

	trace_function(dev);

	spin_lock_irqsave(&vhci->lock, flags);
	if(unlikely(!test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags)))
	{
		dev_warn(&hcd->self.root_hub->dev, "HC isn't running! You have to resume the host controller device before you resume the root hub.\n");
		rc = -ENODEV;
	}
	else
	{
		vhci_hcd->rh_state = USB_VHCI_RH_RUNNING;
		//set_link_state(vhc);
		hcd->state = HC_STATE_RUNNING;
	}
	spin_unlock_irqrestore(&vhci->lock, flags);

	return rc;
}

static inline ssize_t show_urb(char *buf, size_t size, struct urb *urb)
{
	int ep = usb_pipeendpoint(urb->pipe);

	return snprintf(buf, size,
		"urb/%p %s ep%d%s%s len %d/%d\n",
		urb,
		({
			char *s;
			switch(urb->dev->speed)
			{
			case USB_SPEED_LOW:  s = "ls"; break;
			case USB_SPEED_FULL: s = "fs"; break;
			case USB_SPEED_HIGH: s = "hs"; break;
			case USB_SPEED_SUPER:s = "ss"; break;
			default:             s = "?";  break;
			};
			s;
		}),
		ep, ep ? (usb_pipein(urb->pipe) ? "in" : "out") : "",
		({
			char *s;
			switch(usb_pipetype(urb->pipe))
			{
			case PIPE_CONTROL:   s = "";      break;
			case PIPE_BULK:      s = "-bulk"; break;
			case PIPE_INTERRUPT: s = "-int";  break;
			default:             s = "-iso";  break;
			};
			s;
		}),
		urb->actual_length, urb->transfer_buffer_length);
}

static ssize_t show_urbs(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(urbs_hs_inbox,     S_IRUSR, show_urbs, NULL);
static DEVICE_ATTR(urbs_hs_fetched,   S_IRUSR, show_urbs, NULL);
static DEVICE_ATTR(urbs_hs_cancel,    S_IRUSR, show_urbs, NULL);
static DEVICE_ATTR(urbs_hs_canceling, S_IRUSR, show_urbs, NULL);
static DEVICE_ATTR(urbs_ss_inbox,     S_IRUSR, show_urbs, NULL);
static DEVICE_ATTR(urbs_ss_fetched,   S_IRUSR, show_urbs, NULL);
static DEVICE_ATTR(urbs_ss_cancel,    S_IRUSR, show_urbs, NULL);
static DEVICE_ATTR(urbs_ss_canceling, S_IRUSR, show_urbs, NULL);

static ssize_t show_urbs(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct eveusb_vhci *vhci;
	struct eveusb_vhci_hcd *vhci_hcd;
	struct platform_device *pdev;
	struct eveusb_vhci_urb_priv *urbp;
	size_t size = 0;
	unsigned long flags;
	struct list_head *list;

	pdev = to_platform_device(dev);
	vhci = pdev_to_vhci(pdev);

	trace_function(dev);

	if(attr == &dev_attr_urbs_hs_inbox) {
		vhci_hcd = vhci_to_hs_vhcihcd(vhci);
		list = &vhci_hcd->urbp_list_inbox;
	} else if(attr == &dev_attr_urbs_hs_fetched) {
		vhci_hcd = vhci_to_hs_vhcihcd(vhci);
		list = &vhci_hcd->urbp_list_fetched;
	} else if(attr == &dev_attr_urbs_hs_cancel) {
		vhci_hcd = vhci_to_hs_vhcihcd(vhci);
		list = &vhci_hcd->urbp_list_cancel;
	} else if(attr == &dev_attr_urbs_hs_canceling) {
		vhci_hcd = vhci_to_hs_vhcihcd(vhci);
		list = &vhci_hcd->urbp_list_canceling;
	} else if(attr == &dev_attr_urbs_ss_inbox) {
		vhci_hcd = vhci_to_ss_vhcihcd(vhci);
		list = &vhci_hcd->urbp_list_inbox;
	} else if(attr == &dev_attr_urbs_ss_fetched) {
		vhci_hcd = vhci_to_ss_vhcihcd(vhci);
		list = &vhci_hcd->urbp_list_fetched;
	} else if(attr == &dev_attr_urbs_ss_cancel) {
		vhci_hcd = vhci_to_ss_vhcihcd(vhci);
		list = &vhci_hcd->urbp_list_cancel;
	} else if(attr == &dev_attr_urbs_ss_canceling) {
		vhci_hcd = vhci_to_ss_vhcihcd(vhci);
		list = &vhci_hcd->urbp_list_canceling;
	} else {
		dev_err(dev, "unreachable code reached... wtf?\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&vhci->lock, flags);
	list_for_each_entry(urbp, list, urbp_list)
	{
		size_t temp;

		temp = PAGE_SIZE - size;
		if(unlikely(temp <= 0)) break;

		temp = show_urb(buf, temp, urbp->urb);
		buf += temp;
		size += temp;
	}
	spin_unlock_irqrestore(&vhci->lock, flags);

	return size;
}

static int vhci_setup(struct usb_hcd *hcd)
{
	struct eveusb_vhci *vhci = (void *)dev_get_platdata(hcd->self.controller);
	if (usb_hcd_is_primary_hcd(hcd)) {
		vhci->vhci_hcd_hs = usbhcd_to_vhcihcd(hcd);
		vhci->vhci_hcd_hs->vhci = vhci;
		/*
		 * Mark the first roothub as being USB 2.0.
		 * The USB 3.0 roothub will be registered later by
		 * vhci_hcd_probe()
		 */
		hcd->speed = HCD_USB2;
		hcd->self.root_hub->speed = USB_SPEED_HIGH;
	} else {
		vhci->vhci_hcd_ss = usbhcd_to_vhcihcd(hcd);
		vhci->vhci_hcd_ss->vhci = vhci;
		hcd->speed = HCD_USB3;
		hcd->self.root_hub->speed = USB_SPEED_SUPER;
	}

	/*
	 * Support SG.
	 * sg_tablesize is an arbitrary value to alleviate memory pressure
	 * on the host.
	 */
	hcd->self.sg_tablesize = 32;
	hcd->self.no_sg_constraint = 1;

	return 0;
}

static int vhci_start(struct usb_hcd *hcd)
{
	struct eveusb_vhci_hcd *vhci_hcd;
	int retval;
	struct eveusb_vhci_port *ports;
	struct eveusb_vhci *vhci;
	struct device *dev;

	dev = usbhcd_to_dev(hcd);

	trace_function(dev);

	vhci_hcd = usbhcd_to_vhcihcd(hcd);
	vhci = vhcihcd_to_vhci(vhci_hcd);

	ports = kzalloc(vhci->port_count * sizeof(struct eveusb_vhci_port), GFP_KERNEL);
	if(unlikely(ports == NULL)) return -ENOMEM;

	if (usb_hcd_is_primary_hcd(hcd))
		spin_lock_init(&vhci->lock);

	//init_timer(&vhci_hcd->timer);
	//vhci_hcd->timer.function = vhci_timer;
	//vhci_hcd->timer.data = (unsigned long)vhci_hcd;
	vhci_hcd->ports = ports;
	vhci_hcd->port_count = vhci->port_count;
	vhci_hcd->port_update = 0;
	atomic_set(&vhci_hcd->frame_num, 0);
	INIT_LIST_HEAD(&vhci_hcd->urbp_list_inbox);
	INIT_LIST_HEAD(&vhci_hcd->urbp_list_fetched);
	INIT_LIST_HEAD(&vhci_hcd->urbp_list_cancel);
	INIT_LIST_HEAD(&vhci_hcd->urbp_list_canceling);
	vhci_hcd->rh_state = USB_VHCI_RH_RUNNING;

	hcd->power_budget = 0; // NOTE: practically we have unlimited power because this is a virtual device with... err... virtual power!
	hcd->state = HC_STATE_RUNNING;
	hcd->uses_new_polling = 1;

	if (usb_hcd_is_primary_hcd(hcd)) {
		retval = device_create_file(dev, &dev_attr_urbs_hs_inbox);
		if(unlikely(retval != 0)) goto kfree_port_arr;
		retval = device_create_file(dev, &dev_attr_urbs_hs_fetched);
		if(unlikely(retval != 0)) goto rem_file_inbox;
		retval = device_create_file(dev, &dev_attr_urbs_hs_cancel);
		if(unlikely(retval != 0)) goto rem_file_fetched;
		retval = device_create_file(dev, &dev_attr_urbs_hs_canceling);
		if(unlikely(retval != 0)) goto rem_file_cancel;
	} else {
		retval = device_create_file(dev, &dev_attr_urbs_ss_inbox);
		if(unlikely(retval != 0)) goto kfree_port_arr;
		retval = device_create_file(dev, &dev_attr_urbs_ss_fetched);
		if(unlikely(retval != 0)) goto rem_file_inbox;
		retval = device_create_file(dev, &dev_attr_urbs_ss_cancel);
		if(unlikely(retval != 0)) goto rem_file_fetched;
		retval = device_create_file(dev, &dev_attr_urbs_ss_canceling);
		if(unlikely(retval != 0)) goto rem_file_cancel;
	}

	return 0;

rem_file_cancel:
	if (usb_hcd_is_primary_hcd(hcd)) {
		device_remove_file(dev, &dev_attr_urbs_hs_cancel);
	} else {
		device_remove_file(dev, &dev_attr_urbs_ss_cancel);
	}

rem_file_fetched:
	if (usb_hcd_is_primary_hcd(hcd)) {
		device_remove_file(dev, &dev_attr_urbs_hs_fetched);
	} else {
		device_remove_file(dev, &dev_attr_urbs_ss_fetched);
	}

rem_file_inbox:
	if (usb_hcd_is_primary_hcd(hcd)) {
		device_remove_file(dev, &dev_attr_urbs_hs_inbox);
	} else {
		device_remove_file(dev, &dev_attr_urbs_ss_inbox);
	}

kfree_port_arr:
	kfree(ports);
	vhci_hcd->ports = NULL;
	vhci_hcd->port_count = 0;
	return retval;
}

static void vhci_stop(struct usb_hcd *hcd)
{
	struct eveusb_vhci_hcd *vhci_hcd;
	struct device *dev;

	dev = usbhcd_to_dev(hcd);

	trace_function(dev);

	vhci_hcd = usbhcd_to_vhcihcd(hcd);

	if (usb_hcd_is_primary_hcd(hcd)) {
		device_remove_file(dev, &dev_attr_urbs_hs_canceling);
		device_remove_file(dev, &dev_attr_urbs_hs_cancel);
		device_remove_file(dev, &dev_attr_urbs_hs_fetched);
		device_remove_file(dev, &dev_attr_urbs_hs_inbox);
	} else {
		device_remove_file(dev, &dev_attr_urbs_ss_canceling);
		device_remove_file(dev, &dev_attr_urbs_ss_cancel);
		device_remove_file(dev, &dev_attr_urbs_ss_fetched);
		device_remove_file(dev, &dev_attr_urbs_ss_inbox);
	}

	if(likely(vhci_hcd->ports))
	{
		kfree(vhci_hcd->ports);
		vhci_hcd->ports = NULL;
		vhci_hcd->port_count = 0;
	}

	vhci_hcd->rh_state = USB_VHCI_RH_RESET;
	dev_info(dev, "stopped\n");
}

static int vhci_get_frame(struct usb_hcd *hcd)
{
	struct eveusb_vhci_hcd *vhci_hcd;
	vhci_hcd = usbhcd_to_vhcihcd(hcd);
	trace_function(usbhcd_to_dev(hcd));
	return atomic_read(&vhci_hcd->frame_num);
}

static int vhci_alloc_streams(struct usb_hcd *hcd, struct usb_device *udev,
	struct usb_host_endpoint **eps, unsigned int num_eps,
	unsigned int num_streams, gfp_t mem_flags)
{
	dev_dbg(&hcd->self.root_hub->dev, "vhci_alloc_streams not implemented\n");
	return 0;
}

static int vhci_free_streams(struct usb_hcd *hcd, struct usb_device *udev,
	struct usb_host_endpoint **eps, unsigned int num_eps,
	gfp_t mem_flags)
{
	dev_dbg(&hcd->self.root_hub->dev, "vhci_free_streams not implemented\n");
	return 0;
}

static const struct hc_driver vhci_hc_driver = {
	.description      = driver_name,
	.product_desc     = driver_desc,
	.hcd_priv_size    = sizeof(struct eveusb_vhci_hcd),

	.flags            = HCD_USB3 | HCD_SHARED,

	.reset            = vhci_setup,
	.start            = vhci_start,
	.stop             = vhci_stop,

	.urb_enqueue      = vhci_urb_enqueue,
	.urb_dequeue      = vhci_urb_dequeue,

	.get_frame_number = vhci_get_frame,

	.hub_status_data  = vhci_hub_status,
	.hub_control      = vhci_hub_control,
	.bus_suspend      = vhci_bus_suspend,
	.bus_resume       = vhci_bus_resume,

	.alloc_streams	= vhci_alloc_streams,
	.free_streams	= vhci_free_streams,
};

static int vhci_hcd_probe(struct platform_device *pdev)
{
	struct eveusb_vhci	*vhci = (void *)dev_get_platdata(&pdev->dev);
	struct usb_hcd		*hcd_hs;
	struct usb_hcd		*hcd_ss;
	int			ret;

#ifdef DEBUG
	if(debug_output) dev_dbg(&pdev->dev, "%s\n", __FUNCTION__);
#endif
	dev_info(&pdev->dev, DRIVER_DESC " -- Version " DRIVER_VERSION "\n");
	dev_info(&pdev->dev, "--> Backend: %s\n", vhci->ifc->ifc_desc);

	/*
	 * Allocate and initialize hcd.
	 * Our private data is also allocated automatically.
	 */
	hcd_hs = usb_create_hcd(&vhci_hc_driver, &pdev->dev, vhci_dev_name(&pdev->dev));
	if (!hcd_hs) {
		dev_err(&pdev->dev, "create primary hcd failed\n");
		return -ENOMEM;
	}
	hcd_hs->has_tt = 1;

	/*
	 * Finish generic HCD structure initialization and register.
	 * Call the driver's reset() and start() routines.
	 */
	ret = usb_add_hcd(hcd_hs, 0, 0);
	if (ret != 0) {
		dev_err(&pdev->dev, "usb_add_hcd hs failed %d\n", ret);
		goto put_usb2_hcd;
	}

	hcd_ss = usb_create_shared_hcd(&vhci_hc_driver, &pdev->dev,
				       vhci_dev_name(&pdev->dev), hcd_hs);
	if (!hcd_ss) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "create shared hcd failed\n");
		goto remove_usb2_hcd;
	}

	ret = usb_add_hcd(hcd_ss, 0, 0);
	if (ret) {
		dev_err(&pdev->dev, "usb_add_hcd ss failed %d\n", ret);
		goto put_usb3_hcd;
	}

	return 0;

put_usb3_hcd:
	usb_put_hcd(hcd_ss);
remove_usb2_hcd:
	usb_remove_hcd(hcd_hs);
put_usb2_hcd:
	usb_put_hcd(hcd_hs);
	vhci->vhci_hcd_hs = NULL;
	vhci->vhci_hcd_ss = NULL;
	return ret;
}

static int vhci_hcd_remove(struct platform_device *pdev)
{
	int i;
	unsigned long flags;
	struct eveusb_vhci_urb_priv *urbp;
	struct eveusb_vhci *vhci = (void *)dev_get_platdata(&pdev->dev);
	struct eveusb_vhci_hcd *vhci_hcds[] = { vhci_to_ss_vhcihcd(vhci), vhci_to_hs_vhcihcd(vhci) };

	for(i = 0; i < sizeof(vhci_hcds)/sizeof(vhci_hcds[0]); i++)
	{
		struct eveusb_vhci_hcd *vhci_hcd = vhci_hcds[i];

		trace_function(vhcihcd_to_dev(vhci_hcd));

		spin_lock_irqsave(&vhci->lock, flags);
		while(!list_empty(&vhci_hcd->urbp_list_inbox))
		{
			urbp = list_entry(vhci_hcd->urbp_list_inbox.next, struct eveusb_vhci_urb_priv, urbp_list);
			eveusb_vhci_maybe_set_status(urbp, -ESHUTDOWN);
			eveusb_vhci_urb_giveback(vhci_hcd, urbp, &flags);
		}
		while(!list_empty(&vhci_hcd->urbp_list_fetched))
		{
			urbp = list_entry(vhci_hcd->urbp_list_fetched.next, struct eveusb_vhci_urb_priv, urbp_list);
			eveusb_vhci_maybe_set_status(urbp, -ESHUTDOWN);
			eveusb_vhci_urb_giveback(vhci_hcd, urbp, &flags);
		}
		while(!list_empty(&vhci_hcd->urbp_list_cancel))
		{
			urbp = list_entry(vhci_hcd->urbp_list_cancel.next, struct eveusb_vhci_urb_priv, urbp_list);
			eveusb_vhci_maybe_set_status(urbp, -ESHUTDOWN);
			eveusb_vhci_urb_giveback(vhci_hcd, urbp, &flags);
		}
		while(!list_empty(&vhci_hcd->urbp_list_canceling))
		{
			urbp = list_entry(vhci_hcd->urbp_list_canceling.next, struct eveusb_vhci_urb_priv, urbp_list);
			eveusb_vhci_maybe_set_status(urbp, -ESHUTDOWN);
			eveusb_vhci_urb_giveback(vhci_hcd, urbp, &flags);
		}
		spin_unlock_irqrestore(&vhci->lock, flags);

		/*
		 * Disconnects the root hub,
		 * then reverses the effects of usb_add_hcd(),
		 * invoking the HCD's stop() methods.
		 */
		usb_remove_hcd(vhcihcd_to_usbhcd(vhci_hcd));
		usb_put_hcd(vhcihcd_to_usbhcd(vhci_hcd));
	}

	vhci->vhci_hcd_hs = NULL;
	vhci->vhci_hcd_ss = NULL;

	if(vhci->ifc->destroy)
	{
		vhci_dbg("call ifc->destroy\n");
		vhci->ifc->destroy(vhci_to_ifc(vhci));
	}

	return 0;
}

static int vhci_hcd_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct usb_hcd *hcd;
	struct eveusb_vhci_hcd *vhci_hcd;
	int rc = 0;

	hcd = platform_get_drvdata(pdev);
	if (!hcd)
		return 0;

	vhci_hcd = usbhcd_to_vhcihcd(hcd);

	trace_function(vhcihcd_to_dev(vhci_hcd));

	if(unlikely(vhci_hcd->rh_state == USB_VHCI_RH_RUNNING))
	{
		dev_warn(&pdev->dev, "Root hub isn't suspended! You have to suspend the root hub before you suspend the host controller device.\n");
		rc = -EBUSY;
	}
	else
		clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	return rc;
}

static int vhci_hcd_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	struct eveusb_vhci_hcd *vhci_hcd;

	hcd = platform_get_drvdata(pdev);
	if (!hcd)
		return 0;

	vhci_hcd = usbhcd_to_vhcihcd(hcd);

	trace_function(vhcihcd_to_dev(vhci_hcd));

	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	usb_hcd_poll_rh_status(hcd);
	return 0;
}

static struct platform_driver vhci_driver = {
	.probe      = vhci_hcd_probe,
	.remove     = vhci_hcd_remove,
	.suspend    = vhci_hcd_suspend,
	.resume     = vhci_hcd_resume,
	.driver     = {
		.name   = driver_name,
		.owner  = THIS_MODULE
	}
};

// Callback function for driver_for_each_device(..) in eveusb_vhci_hcd_register(...).
// Data points to the device-id we're looking for.
// This funktion returns an error (-EINVAL), if the device has the given id assigned to it.
// (Enumeration stops/finishes on errors.)
static int device_enum(struct device *dev, void *data)
{
	struct platform_device *pdev;
	pdev = to_platform_device(dev);
	return unlikely(*((const int *)data) == pdev->id) ? -EINVAL : 0;
}

static DEFINE_MUTEX(dev_enum_lock);

int eveusb_vhci_hcd_register(const struct eveusb_vhci_ifc *ifc, void *context, u8 port_count, struct eveusb_vhci **vhci_ret)
{
	int retval, i;
	struct eveusb_vhci vhci, *vhci_ptr;

	if(unlikely(port_count > 31))
		return -EINVAL;

	// search for free device-id
	mutex_lock(&dev_enum_lock);
	for(i = 0; i < 10000; i++)
	{
		retval = driver_for_each_device(&vhci_driver.driver, NULL, &i, device_enum);
		if(unlikely(!retval)) break;
	}
	if(unlikely(i >= 10000))
	{
		mutex_unlock(&dev_enum_lock);
		vhci_printk(KERN_ERR, "there are too many devices!\n");
		return -EBUSY;
	}

	vhci.ifc = ifc;
	vhci.port_count = port_count;
	vhci.vhci_hcd_hs = NULL;
	vhci.vhci_hcd_ss = NULL;

	vhci_dbg("allocate platform_device %s.%d\n", driver_name, i);
	vhci.pdev = platform_device_alloc(driver_name, i);
	if (!vhci.pdev) {
		mutex_unlock(&dev_enum_lock);
		return -ENOMEM;
	}

	if(!try_module_get(ifc->owner))
	{
		mutex_unlock(&dev_enum_lock);
		vhci_printk(KERN_ERR, "ifc module died\n");
		retval = -ENODEV;
		goto pdev_put;
	}

	vhci_dbg("install eveusb_vhci_device structure within pdev->dev.platform_data\n");
	retval = platform_device_add_data(vhci.pdev, &vhci, sizeof vhci + ifc->ifc_priv_size);
	if (unlikely(retval < 0)) {
		mutex_unlock(&dev_enum_lock);
		goto pdev_put;
	}
	vhci_ptr = pdev_to_vhci(vhci.pdev);

	if(ifc->init)
	{
		vhci_dbg("call ifc->init\n");
		retval = ifc->init(context, vhci_to_ifc(vhci_ptr));
		if(unlikely(retval < 0))
		{
			mutex_unlock(&dev_enum_lock);
			goto mod_put;
		}
	}

	vhci_dbg("add platform_device %s.%d\n", vhci.pdev->name, vhci.pdev->id);
	retval = platform_device_add(vhci.pdev);
	mutex_unlock(&dev_enum_lock);
	if (unlikely(retval < 0)) {
		vhci_printk(KERN_ERR, "add platform_device %s.%d failed\n", vhci.pdev->name, vhci.pdev->id);
		if(ifc->destroy)
		{
			vhci_dbg("call ifc->destroy\n");
			ifc->destroy(vhci_to_ifc(vhci_ptr));
		}
		goto mod_put;
	}

	try_module_get(THIS_MODULE);
	*vhci_ret = vhci_ptr;
	return 0;

mod_put:
	module_put(ifc->owner);

pdev_put:
	platform_device_put(vhci.pdev);
	return retval;
}
// EXPORT_SYMBOL_GPL(eveusb_vhci_hcd_register);

int eveusb_vhci_hcd_unregister(struct eveusb_vhci *vhci)
{
	struct platform_device *pdev;
	struct device *dev;
	struct module *ifc_owner = vhci->ifc->owner; // we need a copy, because vhci gets destroyed on platform_device_unregister

	pdev = vhci_to_pdev(vhci);
	dev = &pdev->dev;

	vhci_dbg("unregister platform_device %s\n", vhci_dev_name(dev));
	platform_device_unregister(pdev); // calls vhci_hcd_remove which calls ifc->destroy

	module_put(ifc_owner);
	module_put(THIS_MODULE);
	return 0;
}
// EXPORT_SYMBOL_GPL(eveusb_vhci_hcd_unregister);

int eveusb_vhci_hcd_has_work(struct eveusb_vhci *vhci)
{
	int i;
	unsigned long flags;
	int y = 0;
	struct eveusb_vhci_hcd *vhci_hcds[] = { vhci_to_ss_vhcihcd(vhci), vhci_to_hs_vhcihcd(vhci) };

	for(i = 0; i < sizeof(vhci_hcds)/sizeof(vhci_hcds[0]); i++)
	{
		struct eveusb_vhci_hcd *vhci_hcd = vhci_hcds[i];

		spin_lock_irqsave(&vhci->lock, flags);
		if(vhci_hcd->port_update ||
		   !list_empty(&vhci_hcd->urbp_list_cancel) ||
		   !list_empty(&vhci_hcd->urbp_list_inbox))
			y += 1;
		spin_unlock_irqrestore(&vhci->lock, flags);
	}
	return y;
}
// EXPORT_SYMBOL_GPL(eveusb_vhci_hcd_has_work);

int eveusb_vhci_apply_port_stat(struct eveusb_vhci *vhci, u16 port_status, u16 port_change, u8 port_index, u8 port_flags)
{
	struct eveusb_vhci_hcd *vhci_hcd;
	struct usb_hcd *hcd;
	struct device *dev;
	unsigned long flags;
	u16 overcurrent;

	if(vhci_to_ss_vhcihcd(vhci)->ports[port_index - 1].port_status & USB_PORT_STAT_CONNECTION) {
		vhci_hcd = vhci_to_ss_vhcihcd(vhci);
	} else if(vhci_to_hs_vhcihcd(vhci)->ports[port_index - 1].port_status & USB_PORT_STAT_CONNECTION) {
		vhci_hcd = vhci_to_hs_vhcihcd(vhci);
	} else if((port_change & USB_PORT_STAT_C_CONNECTION) && (port_status & USB_PORT_STAT_CONNECTION)) {
		vhci_hcd = (port_flags & USB_VHCI_PORT_STAT_FLAG_SUPERSPEED) ? vhci_to_ss_vhcihcd(vhci) : vhci_to_hs_vhcihcd(vhci);
	} else {
		return -EINVAL;
	}

	if(unlikely(!port_index || port_index > vhci_hcd->port_count))
		return -EINVAL;

	if(unlikely(port_change != USB_PORT_STAT_C_CONNECTION &&
	            port_change != USB_PORT_STAT_C_ENABLE &&
	            port_change != USB_PORT_STAT_C_SUSPEND &&
	            port_change != USB_PORT_STAT_C_OVERCURRENT &&
	            port_change != USB_PORT_STAT_C_RESET &&
	            port_change != (USB_PORT_STAT_C_RESET | USB_PORT_STAT_C_ENABLE)))
		return -EINVAL;

	hcd = vhcihcd_to_usbhcd(vhci_hcd);
	dev = vhcihcd_to_dev(vhci_hcd);

	spin_lock_irqsave(&vhci->lock, flags);
	if(unlikely(!port_is_power_on(hcd, vhci_hcd->ports[port_index - 1].port_status)))
	{
		spin_unlock_irqrestore(&vhci->lock, flags);
		return -EPROTO;
	}

#ifdef DEBUG
	if(debug_output) dev_dbg(dev, "performing PORT_STAT [port=%d ~status=0x%04x ~change=0x%04x]\n", (int)index, (int)status, (int)change);
#endif

	switch(port_change)
	{
	case USB_PORT_STAT_C_CONNECTION:
		overcurrent = vhci_hcd->ports[port_index - 1].port_status & USB_PORT_STAT_OVERCURRENT;
		vhci_hcd->ports[port_index - 1].port_change |= USB_PORT_STAT_C_CONNECTION;
		if(port_status & USB_PORT_STAT_CONNECTION)
			vhci_hcd->ports[port_index - 1].port_status = (hcd->speed == HCD_USB3 ? USB_SS_PORT_STAT_POWER : USB_PORT_STAT_POWER) |
				USB_PORT_STAT_CONNECTION |
				((port_status & USB_PORT_STAT_LOW_SPEED) ? USB_PORT_STAT_LOW_SPEED :
				((port_status & USB_PORT_STAT_HIGH_SPEED) ? USB_PORT_STAT_HIGH_SPEED : 0)) |
				overcurrent;
		else
			vhci_hcd->ports[port_index - 1].port_status = (hcd->speed == HCD_USB3 ? USB_SS_PORT_STAT_POWER : USB_PORT_STAT_POWER) | 
				overcurrent;
		vhci_hcd->ports[port_index - 1].port_flags &= ~USB_VHCI_PORT_STAT_FLAG_RESUMING;
		vhci_hcd->ports[port_index - 1].port_flags |= (port_flags & USB_VHCI_PORT_STAT_FLAG_SUPERSPEED);
		break;

	case USB_PORT_STAT_C_ENABLE:
		if(unlikely(!(vhci_hcd->ports[port_index - 1].port_status & USB_PORT_STAT_CONNECTION) ||
			(vhci_hcd->ports[port_index - 1].port_status & USB_PORT_STAT_RESET) ||
			(port_status & USB_PORT_STAT_ENABLE)))
		{
			spin_unlock_irqrestore(&vhci->lock, flags);
			return -EPROTO;
		}
		vhci_hcd->ports[port_index - 1].port_change |= USB_PORT_STAT_C_ENABLE;
		vhci_hcd->ports[port_index - 1].port_status &= ~USB_PORT_STAT_ENABLE;
		vhci_hcd->ports[port_index - 1].port_flags &= ~USB_VHCI_PORT_STAT_FLAG_RESUMING;
		vhci_hcd->ports[port_index - 1].port_status &= ~USB_PORT_STAT_SUSPEND;
		break;

	case USB_PORT_STAT_C_SUSPEND:
		if(unlikely(!(vhci_hcd->ports[port_index - 1].port_status & USB_PORT_STAT_CONNECTION) ||
			!(vhci_hcd->ports[port_index - 1].port_status & USB_PORT_STAT_ENABLE) ||
			(vhci_hcd->ports[port_index - 1].port_status & USB_PORT_STAT_RESET) ||
			port_is_suspended(hcd, port_status)))
		{
			spin_unlock_irqrestore(&vhci->lock, flags);
			return -EPROTO;
		}
		vhci_hcd->ports[port_index - 1].port_flags &= ~USB_VHCI_PORT_STAT_FLAG_RESUMING;
		vhci_hcd->ports[port_index - 1].port_change |= USB_PORT_STAT_C_SUSPEND;
		vhci_hcd->ports[port_index - 1].port_status &= ~USB_PORT_STAT_SUSPEND;
		break;

	case USB_PORT_STAT_C_OVERCURRENT:
		vhci_hcd->ports[port_index - 1].port_change |= USB_PORT_STAT_C_OVERCURRENT;
		vhci_hcd->ports[port_index - 1].port_status &= ~USB_PORT_STAT_OVERCURRENT;
		vhci_hcd->ports[port_index - 1].port_status |= port_status & USB_PORT_STAT_OVERCURRENT;
		break;

	default: // USB_PORT_STAT_C_RESET [| USB_PORT_STAT_C_ENABLE]
		if(unlikely(!(vhci_hcd->ports[port_index - 1].port_status & USB_PORT_STAT_CONNECTION) ||
			!(vhci_hcd->ports[port_index - 1].port_status & USB_PORT_STAT_RESET) ||
			(port_status & USB_PORT_STAT_RESET)))
		{
			spin_unlock_irqrestore(&vhci->lock, flags);
			return -EPROTO;
		}
		if(port_change & USB_PORT_STAT_C_ENABLE)
		{
			if(port_status & USB_PORT_STAT_ENABLE)
			{
				spin_unlock_irqrestore(&vhci->lock, flags);
				return -EPROTO;
			}
			vhci_hcd->ports[port_index - 1].port_change |= USB_PORT_STAT_C_ENABLE;
		}
		else
			vhci_hcd->ports[port_index - 1].port_status |= port_status & USB_PORT_STAT_ENABLE;
		vhci_hcd->ports[port_index - 1].port_change |= USB_PORT_STAT_C_RESET;
		vhci_hcd->ports[port_index - 1].port_status &= ~USB_PORT_STAT_RESET;
		break;
	}

	vhci_port_update(vhci_hcd, port_index);
	spin_unlock_irqrestore(&vhci->lock, flags);

	usb_hcd_poll_rh_status(vhcihcd_to_usbhcd(vhci_hcd));
	return 0;
}
// EXPORT_SYMBOL_GPL(eveusb_vhci_apply_port_stat);

#ifdef DEBUG
static ssize_t show_debug_output(struct device_driver *drv, char *buf)
{
	if(buf != NULL)
	{
		switch(debug_output)
		{
		case 0:  *buf = '0'; break; // No debug output
		case 1:  *buf = '1'; break; // Debug output without data buffer dumps
		case 2:  *buf = '2'; break; // Debug output with short data buffer dumps
		default: *buf = '3'; break; // Debug output with full data buffer dumps
		}
	}
	return 1;
}

static ssize_t store_debug_output(struct device_driver *drv, const char *buf, size_t count)
{
	if(count != 1 || buf == NULL) return -EINVAL;
	switch(*buf)
	{
	case '0': debug_output = 0; return 1;
	case '1': debug_output = 1; return 1;
	case '2': debug_output = 2; return 1;
	case '3': debug_output = 3; return 1;
	}
	return -EINVAL;
}

static DRIVER_ATTR(debug_output, S_IRUSR | S_IWUSR, show_debug_output, store_debug_output);
#endif

static void do_cleanup(void)
{
#ifdef DEBUG
	driver_remove_file(&vhci_driver.driver, &driver_attr_debug_output);
#endif
	vhci_dbg("unregister platform_driver %s\n", driver_name);
	platform_driver_unregister(&vhci_driver);
	vhci_dbg("gone\n");
}

static int __init vhci_hcd_init(void)
{
	int retval;

	if(usb_disabled()) return -ENODEV;

	vhci_printk(KERN_INFO, DRIVER_DESC " -- Version " DRIVER_VERSION "\n");

#ifdef DEBUG
	vhci_printk(KERN_DEBUG, "register platform_driver %s\n", driver_name);
#endif
	retval = platform_driver_register(&vhci_driver);
	if(unlikely(retval < 0))
	{
		vhci_printk(KERN_ERR, "register platform_driver failed\n");
		return retval;
	}

#ifdef DEBUG
	retval = driver_create_file(&vhci_driver.driver, &driver_attr_debug_output);
	if(unlikely(retval != 0))
	{
		vhci_printk(KERN_DEBUG, "driver_create_file(&vhci_driver, &driver_attr_debug_output) failed\n");
		vhci_printk(KERN_DEBUG, "==> ignoring\n");
	}
#endif

	retval = vhci_iocifc_init();
	if(unlikely(retval != 0))
	{
		do_cleanup();
	}

	return retval;
}
module_init(vhci_hcd_init);

static void __exit vhci_hcd_exit(void)
{
	vhci_iocifc_exit();
	do_cleanup();
}
module_exit(vhci_hcd_exit);
