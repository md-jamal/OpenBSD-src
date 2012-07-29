/*	$OpenBSD: if_ix.h,v 1.16 2012/07/29 13:49:03 mikeb Exp $	*/

/******************************************************************************

  Copyright (c) 2001-2008, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
/*$FreeBSD: src/sys/dev/ixgbe/ixgbe.h,v 1.4 2008/05/16 18:46:30 jfv Exp $*/

#ifndef _IX_H_
#define _IX_H_

#include <dev/pci/ixgbe.h>

/* Tunables */

/*
 * TxDescriptors Valid Range: 64-4096 Default Value: 256 This value is the
 * number of transmit descriptors allocated by the driver. Increasing this
 * value allows the driver to queue more transmits. Each descriptor is 16
 * bytes. Performance tests have show the 2K value to be optimal for top
 * performance.
 */
#define DEFAULT_TXD	256
#define PERFORM_TXD	2048
#define MAX_TXD		4096
#define MIN_TXD		64

/*
 * RxDescriptors Valid Range: 64-4096 Default Value: 256 This value is the
 * number of receive descriptors allocated for each RX queue. Increasing this
 * value allows the driver to buffer more incoming packets. Each descriptor
 * is 16 bytes.  A receive buffer is also allocated for each descriptor.
 *
 * Note: with 8 rings and a dual port card, it is possible to bump up
 *	against the system mbuf pool limit, you can tune nmbclusters
 *	to adjust for this.
 */
#define DEFAULT_RXD	256
#define PERFORM_RXD	2048
#define MAX_RXD		4096
#define MIN_RXD		64

/* Alignment for rings */
#define DBA_ALIGN	128

/*
 * This parameter controls the duration of transmit watchdog timer.
 */
#define IXGBE_TX_TIMEOUT                   5	/* set to 5 seconds */

/*
 * This parameters control when the driver calls the routine to reclaim
 * transmit descriptors.
 */
#define IXGBE_TX_CLEANUP_THRESHOLD	(sc->num_tx_desc / 16)
#define IXGBE_TX_OP_THRESHOLD		(sc->num_tx_desc / 32)

#define IXGBE_MAX_FRAME_SIZE	0x3F00

/* Flow control constants */
#define IXGBE_FC_PAUSE		0xFFFF
#define IXGBE_FC_HI		0x20000
#define IXGBE_FC_LO		0x10000

/* Defines for printing debug information */
#define DEBUG_INIT  0
#define DEBUG_IOCTL 0
#define DEBUG_HW    0

#define INIT_DEBUGOUT(S)            if (DEBUG_INIT)  printf(S "\n")
#define INIT_DEBUGOUT1(S, A)        if (DEBUG_INIT)  printf(S "\n", A)
#define INIT_DEBUGOUT2(S, A, B)     if (DEBUG_INIT)  printf(S "\n", A, B)
#define IOCTL_DEBUGOUT(S)           if (DEBUG_IOCTL) printf(S "\n")
#define IOCTL_DEBUGOUT1(S, A)       if (DEBUG_IOCTL) printf(S "\n", A)
#define IOCTL_DEBUGOUT2(S, A, B)    if (DEBUG_IOCTL) printf(S "\n", A, B)
#define HW_DEBUGOUT(S)              if (DEBUG_HW) printf(S "\n")
#define HW_DEBUGOUT1(S, A)          if (DEBUG_HW) printf(S "\n", A)
#define HW_DEBUGOUT2(S, A, B)       if (DEBUG_HW) printf(S "\n", A, B)

#define MAX_NUM_MULTICAST_ADDRESSES     128
#define IXGBE_82598_SCATTER		100
#define IXGBE_82599_SCATTER		32
#define IXGBE_MSIX_BAR			3
#define IXGBE_TSO_SIZE			65535
#define IXGBE_TX_BUFFER_SIZE		((uint32_t) 1514)
#define IXGBE_RX_HDR                    128
#define IXGBE_VFTA_SIZE                 128
#define IXGBE_BR_SIZE                   4096
#define IXGBE_QUEUE_IDLE                0
#define IXGBE_QUEUE_WORKING             1
#define IXGBE_QUEUE_HUNG                2

/*
 * Interrupt Moderation parameters
 */
#define IXGBE_INTS_PER_SEC		8000

/* Used for auto RX queue configuration */
extern int mp_ncpus;

struct ixgbe_tx_buf {
	uint32_t	eop_index;
	struct mbuf	*m_head;
	bus_dmamap_t	map;
};

struct ixgbe_rx_buf {
	struct mbuf	*m_head;
	struct mbuf	*m_pack;
	struct mbuf	*fmp;
	bus_dmamap_t	hmap;
	bus_dmamap_t	pmap;
};

/*
 * Bus dma allocation structure used by ixgbe_dma_malloc and ixgbe_dma_free.
 */
struct ixgbe_dma_alloc {
	caddr_t			dma_vaddr;
	bus_dma_tag_t		dma_tag;
	bus_dmamap_t		dma_map;
	bus_dma_segment_t	dma_seg;
	bus_size_t		dma_size;
	int			dma_nseg;
};

/*
 * Driver queue struct: this is the interrupt container
 *  for the associated tx and rx ring.
 */
struct ix_queue {
	struct ix_softc         *sc;
	uint32_t		msix;           /* This queue's MSIX vector */
	uint32_t		eims;           /* This queue's EIMS bit */
	uint32_t		eitr_setting;
	/* struct resource	*res; */
	void			*tag;
	struct tx_ring		*txr;
	struct rx_ring		*rxr;
	uint64_t		irqs;
};

/*
 * The transmit ring, one per tx queue
 */
struct tx_ring {
	struct ix_softc		*sc;
	struct mutex		tx_mtx;
	uint32_t		me;
	int			queue_status;
	uint32_t		watchdog_timer;
	union ixgbe_adv_tx_desc	*tx_base;
	struct ixgbe_dma_alloc	txdma;
	uint32_t		next_avail_desc;
	uint32_t		next_to_clean;
	struct ixgbe_tx_buf	*tx_buffers;
	volatile uint16_t	tx_avail;
	uint32_t		txd_cmd;
	bus_dma_tag_t		txtag;
	uint32_t		bytes; /* Used for AIM calc */
	uint32_t		packets;
	/* Soft Stats */
	uint64_t		tx_packets;
};


/*
 * The Receive ring, one per rx queue
 */
struct rx_ring {
	struct ix_softc		*sc;
	struct mutex		rx_mtx;
	uint32_t		me;
	union ixgbe_adv_rx_desc	*rx_base;
	struct ixgbe_dma_alloc	rxdma;
#if 0
	struct lro_ctrl		lro;
#endif
	int			lro_enabled;
	int			hdr_split;
	int			hw_rsc;
	int			discard;
	unsigned int		next_to_refresh;
	unsigned int		next_to_check;
	unsigned int		last_desc_filled;
	int			rx_ndescs;
	struct ixgbe_rx_buf	*rx_buffers;

	uint32_t		bytes; /* Used for AIM calc */
	uint32_t		packets;

	/* Soft stats */
	uint64_t		rx_irq;
	uint64_t		rx_split_packets;
	uint64_t		rx_packets;
	uint64_t		rx_bytes;
	uint64_t		rx_discarded;
	uint64_t		rsc_num;
};

/* Our adapter structure */
struct ix_softc {
	struct device		dev;
	struct arpcom		arpcom;

	struct ixgbe_hw	hw;
	struct ixgbe_osdep	osdep;

	/* struct resource	*pci_mem; */
	/* struct resource	*msix_mem; */

	void			*tag;
	/* struct resource 	*res; */

	struct ifmedia		media;
	struct timeout		timer;
	struct timeout		rx_refill;
	int			msix;
	int			if_flags;

	struct mutex		core_mtx;

	uint16_t		num_vlans;
	uint16_t		num_queues;

	/*
	 * Shadow VFTA table, this is needed because
	 * the real vlan filter table gets cleared during
	 * a soft reset and the driver needs to be able
	 * to repopulate it.
	 */
	uint32_t		shadow_vfta[IXGBE_VFTA_SIZE];

	/* Info about the interface */
	uint			optics;
	int			advertise;  /* link speeds */
	int			link_active;
	uint16_t		max_frame_size;
	uint16_t		num_segs;
	uint32_t		link_speed;
	int			link_up;
	uint32_t		linkvec;

	/* Mbuf cluster size */
	uint32_t		rx_mbuf_sz;

	/* Support for pluggable optics */
	int			sfp_probe;
	workq_fn		link_task;	/* Link tasklet */
	workq_fn		mod_task;	/* SFP tasklet */
	workq_fn		msf_task;	/* Multispeed Fiber */

	/*
	 * Queues:
	 *   This is the irq holder, it has
	 *   and RX/TX pair or rings associated
	 *   with it.
	 */
	struct ix_queue		*queues;

	/*
	 * Transmit rings:
	 *	Allocated at run time, an array of rings.
	 */
	struct tx_ring	*tx_rings;
	int		num_tx_desc;

	/*
	 * Receive rings:
	 *	Allocated at run time, an array of rings.
	 */
	struct rx_ring	*rx_rings;
	uint64_t	que_mask;
	int		num_rx_desc;
	uint32_t	rx_process_limit;

	/* Multicast array memory */
	uint8_t		*mta;

	/* Misc stats maintained by the driver */
	unsigned long   dropped_pkts;
	unsigned long   mbuf_defrag_failed;
	unsigned long   mbuf_header_failed;
	unsigned long   mbuf_packet_failed;
	unsigned long   no_tx_map_avail;
	unsigned long   no_tx_dma_setup;
	unsigned long   watchdog_events;
	unsigned long   tso_tx;
	unsigned long	link_irq;

	struct ixgbe_hw_stats stats;
};

#endif /* _IX_H_ */
