/*
 * usb-vhci.h -- VHCI USB host controller driver header.
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

#ifndef _EVEUSB_VHCI_H
#define _EVEUSB_VHCI_H

#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/version.h>

#ifndef __KERNEL__

// wPortStatus bit field
// See USB 2.0 spec Table 11-21
#define USB_PORT_STAT_CONNECTION    0x0001
#define USB_PORT_STAT_ENABLE        0x0002
#define USB_PORT_STAT_SUSPEND       0x0004
#define USB_PORT_STAT_OVERCURRENT   0x0008
#define USB_PORT_STAT_RESET         0x0010
#define USB_PORT_STAT_POWER         0x0100
#define USB_PORT_STAT_LOW_SPEED     0x0200
#define USB_PORT_STAT_HIGH_SPEED    0x0400
//#define USB_PORT_STAT_TEST          0x0800
//#define USB_PORT_STAT_INDICATOR     0x1000

/*
 * Additions to wPortStatus bit field from USB 3.0
 * See USB 3.0 spec Table 10-10
 */
#define USB_PORT_STAT_LINK_STATE	0x01e0
#define USB_SS_PORT_STAT_POWER		0x0200
#define USB_SS_PORT_STAT_SPEED		0x1c00
#define USB_PORT_STAT_SPEED_5GBPS	0x0000
/* Valid only if port is enabled */
/* Bits that are the same from USB 2.0 */
#define USB_SS_PORT_STAT_MASK (USB_PORT_STAT_CONNECTION |	    \
				USB_PORT_STAT_ENABLE |	    \
				USB_PORT_STAT_OVERCURRENT | \
				USB_PORT_STAT_RESET)

/*
 * Definitions for PORT_LINK_STATE values
 * (bits 5-8) in wPortStatus
 */
#define USB_SS_PORT_LS_U0		0x0000
#define USB_SS_PORT_LS_U1		0x0020
#define USB_SS_PORT_LS_U2		0x0040
#define USB_SS_PORT_LS_U3		0x0060
#define USB_SS_PORT_LS_SS_DISABLED	0x0080
#define USB_SS_PORT_LS_RX_DETECT	0x00a0
#define USB_SS_PORT_LS_SS_INACTIVE	0x00c0
#define USB_SS_PORT_LS_POLLING		0x00e0
#define USB_SS_PORT_LS_RECOVERY		0x0100
#define USB_SS_PORT_LS_HOT_RESET	0x0120
#define USB_SS_PORT_LS_COMP_MOD		0x0140
#define USB_SS_PORT_LS_LOOPBACK		0x0160

// wPortChange bit field
// See USB 2.0 spec Table 11-22
#define USB_PORT_STAT_C_CONNECTION  0x0001
#define USB_PORT_STAT_C_ENABLE      0x0002
#define USB_PORT_STAT_C_SUSPEND     0x0004
#define USB_PORT_STAT_C_OVERCURRENT 0x0008
#define USB_PORT_STAT_C_RESET       0x0010
/*
 * USB 3.0 wPortChange bit fields
 * See USB 3.0 spec Table 10-11
 */
#define USB_PORT_STAT_C_BH_RESET	0x0020
#define USB_PORT_STAT_C_LINK_STATE	0x0040
#define USB_PORT_STAT_C_CONFIG_ERROR	0x0080

#endif

#if defined(RHEL_MAJOR) && defined(RHEL_MINOR)
#	if RHEL_MAJOR == 8 && RHEL_MINOR >= 1
#		define RHEL_81
#	endif
#endif

// structure for the EVEUSB_VHCI_HCD_IOCREGISTER ioctl
struct eveusb_vhci_ioc_register
{
	__s32 id;         // [out] identifier which was assigned by the kernel
	__s32 usb_hs_busnum; // [out] assigned USB High Speed bus number
	__s32 usb_ss_busnum; // [out] assigned USB Super Speed bus number
	char bus_id[20];  // [out] null-terminated bus-id of the controller
	                  //       (something similar to eveusb_vhci_hcd.<id>)
	__u8 port_count;  // [in]  number of ports the controller should have
};

struct eveusb_vhci_ioc_port_stat
{
	__u16 status;    // state of the port
	__u16 change;    // indicates changed status bits
	__u8 index;      // index of port
	__u8 flags;      // additional information:
#define USB_VHCI_PORT_STAT_FLAG_RESUMING   0x01 // indicates resuming
#define USB_VHCI_PORT_STAT_FLAG_SUPERSPEED 0x02 // indicates SuperSpeed device
	__u8 reserved1, reserved2; // size of the struct should be dividable by four
};

struct eveusb_vhci_ioc_setup_packet
{
	__u8 bmRequestType;
	__u8 bRequest;
	__u16 wValue;
	__u16 wIndex;
	__u16 wLength;
};

struct eveusb_vhci_ioc_urb
{
	struct eveusb_vhci_ioc_setup_packet setup_packet; // only for control urbs
	__s32 buffer_length;                           // number of bytes which were
	                                               // allocated for the buffer
	__s32 interval;
	__s32 packet_count;                            // number of iso packets
	__u16 flags;                                   // flags:
#define USB_VHCI_URB_FLAGS_SHORT_NOT_OK 0x0001     // IN: treat incomming short
                                                   // packets as an error
#define USB_VHCI_URB_FLAGS_ISO_ASAP     0x0002     // ISO: schedule as soon as
                                                   // possible
#define USB_VHCI_URB_FLAGS_ZERO_PACKET  0x0040     // BULK OUT: always send a
                                                   // short packet at the end
                                                   // (send a zero length packet
                                                   // if necessary)
	__u16 busnum;                                  // bus number of the usb device
	__u8 address;                                  // address of the usb device
	                                               // for which this urb is for
	__u8 endpoint;                                 // endpoint incl. direction
	__u8 type;
#define USB_VHCI_URB_TYPE_ISO     0
#define USB_VHCI_URB_TYPE_INT     1
#define USB_VHCI_URB_TYPE_CONTROL 2
#define USB_VHCI_URB_TYPE_BULK    3
};

union eveusb_vhci_ioc_work_union
{
	struct eveusb_vhci_ioc_urb urb;         // for USB_VHCI_IOC_WORK_TYPE_PROCESS_URB
	struct eveusb_vhci_ioc_port_stat port;  // for USB_VHCI_IOC_WORK_TYPE_PORT_STAT
};

struct eveusb_vhci_ioc_work
{
	__u64 handle;                        // for USB_VHCI_IOC_WORK_TYPE_PROCESS_URB
	                                     // and USB_VHCI_IOC_WORK_TYPE_CANCEL_URB;
	                                     // handle which identifies the urb
	                                     // (it is just a pointer to the urb
	                                     // in kernel space)
	union eveusb_vhci_ioc_work_union work;
	__s16 timeout;                       // timeout in milliseconds (max. 1000)
#define USB_VHCI_TIMEOUT_INFINITE     -1
	__u8 type;
#define USB_VHCI_WORK_TYPE_PORT_STAT   0 // the state of a port has changed
#define USB_VHCI_WORK_TYPE_PROCESS_URB 1 // hand an urb to the (virtual)
                                         // hardware
#define USB_VHCI_WORK_TYPE_CANCEL_URB  2 // cancel urb if it isn't processed
                                         // already
};

struct eveusb_vhci_ioc_iso_packet_data
{
	__u32 offset;
	__u32 packet_length;
};

struct eveusb_vhci_ioc_urb_data
{
	__u64 handle;        // handle which identifies the urb
	void *buffer;        // points to the beginning of the data buffer
	struct eveusb_vhci_ioc_iso_packet_data *iso_packets; // points to the beginning of
	                                                  // the iso packet array
	__s32 buffer_length; // number of bytes which were allocated for the buffer
	__s32 packet_count;  // number of iso packets
};

struct eveusb_vhci_ioc_iso_packet_giveback
{
	__u32 packet_actual;
	__s32 status;
};

struct eveusb_vhci_ioc_giveback
{
	__u64 handle;
	void *buffer;        // only for IN URBs: the received data (for OUT URBs
	                     // always a null pointer)
	struct eveusb_vhci_ioc_iso_packet_giveback *iso_packets; // for ISO
	__s32 status;        // (ignored for ISO URBs)
	__s32 buffer_actual; // number of bytes which were actually transfered
	                     // (for IN-ISOs buffer_actual has to be equal to
	                     // buffer_length; for OUT-ISOs this value will be
	                     // ignored)
	__s32 packet_count;  // for ISO (has to match with the value from the urb)
	__s32 error_count;   // for ISO
};

#ifdef __KERNEL__
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
struct eveusb_vhci_ioc_urb_data32
{
	__u64 handle;
	compat_caddr_t buffer;
	compat_caddr_t iso_packets;
	__s32 buffer_length;
	__s32 packet_count;
};

struct eveusb_vhci_ioc_giveback32
{
	__u64 handle;
	compat_caddr_t buffer;
	compat_caddr_t iso_packets;
	__s32 status;
	__s32 buffer_actual;
	__s32 packet_count;
	__s32 error_count;
};
#endif
#endif

#define EVEUSB_VHCI_HCD_IOC_MAGIC       138
#define EVEUSB_VHCI_HCD_IOCREGISTER     _IOWR(EVEUSB_VHCI_HCD_IOC_MAGIC, 0, \
                                       struct eveusb_vhci_ioc_register)
#define EVEUSB_VHCI_HCD_IOCPORTSTAT     _IOW (EVEUSB_VHCI_HCD_IOC_MAGIC, 1, \
                                       struct eveusb_vhci_ioc_port_stat)
// read-only FETCH_WORK: timeout defaults to 100 ms
#define EVEUSB_VHCI_HCD_IOCFETCHWORK_RO _IOR (EVEUSB_VHCI_HCD_IOC_MAGIC, 2, \
                                       struct eveusb_vhci_ioc_work)
#define EVEUSB_VHCI_HCD_IOCFETCHWORK    _IOWR(EVEUSB_VHCI_HCD_IOC_MAGIC, 2, \
                                       struct eveusb_vhci_ioc_work)
#define EVEUSB_VHCI_HCD_IOCGIVEBACK     _IOW (EVEUSB_VHCI_HCD_IOC_MAGIC, 3, \
                                       struct eveusb_vhci_ioc_giveback)
#define EVEUSB_VHCI_HCD_IOCGIVEBACK32   _IOW (EVEUSB_VHCI_HCD_IOC_MAGIC, 3, \
                                       struct eveusb_vhci_ioc_giveback32)
#define EVEUSB_VHCI_HCD_IOCFETCHDATA    _IOW (EVEUSB_VHCI_HCD_IOC_MAGIC, 4, \
                                       struct eveusb_vhci_ioc_urb_data)
#define EVEUSB_VHCI_HCD_IOCFETCHDATA32  _IOW (EVEUSB_VHCI_HCD_IOC_MAGIC, 4, \
                                       struct eveusb_vhci_ioc_urb_data32)
#define EVEUSB_VHCI_HCD_IOC_MAXNR       4

#endif

