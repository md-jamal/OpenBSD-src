/* drmP.h -- Private header for Direct Rendering Manager -*- linux-c -*-
 * Created: Mon Jan  4 10:05:05 1999 by faith@precisioninsight.com
 */
/*-
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Rickard E. (Rik) Faith <faith@valinux.com>
 *    Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef _DRM_P_H_
#define _DRM_P_H_

#if defined(_KERNEL) || defined(__KERNEL__)


#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/proc.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/mutex.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/signalvar.h>
#include <sys/poll.h>
#include <sys/tree.h>
#include <sys/endian.h>
#include <sys/mman.h>
#include <sys/stdint.h>
#include <sys/agpio.h>
#include <sys/memrange.h>
#include <sys/extent.h>
#include <sys/vnode.h>
#include <uvm/uvm.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/agpvar.h>
#include <dev/pci/vga_pcivar.h>
#include <machine/param.h>
#include <machine/bus.h>

#include "drm.h"
#include "drm_linux_list.h"
#include "drm_atomic.h"
#include "drm_internal.h"

#define DRM_KERNEL_CONTEXT    0	 /* Change drm_resctx if changed	  */
#define DRM_RESERVED_CONTEXTS 1	 /* Change drm_resctx if changed	  */

#define DRM_MEM_DMA		0
#define DRM_MEM_SAREA		1
#define DRM_MEM_DRIVER		2
#define DRM_MEM_MAGIC		3
#define DRM_MEM_IOCTLS		4
#define DRM_MEM_MAPS		5
#define DRM_MEM_BUFS		6
#define DRM_MEM_SEGS		7
#define DRM_MEM_PAGES		8
#define DRM_MEM_FILES		9
#define DRM_MEM_QUEUES		10
#define DRM_MEM_CMDS		11
#define DRM_MEM_MAPPINGS	12
#define DRM_MEM_BUFLISTS	13
#define DRM_MEM_AGPLISTS	14
#define DRM_MEM_TOTALAGP	15
#define DRM_MEM_BOUNDAGP	16
#define DRM_MEM_CTXBITMAP	17
#define DRM_MEM_CTXLIST		18
#define DRM_MEM_STUB		19
#define DRM_MEM_SGLISTS		20
#define DRM_MEM_DRAWABLE	21
#define DRM_MEM_MM		22

#define DRM_MAX_CTXBITMAP (PAGE_SIZE * 8)

				/* Internal types and structures */
#define DRM_ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define DRM_MIN(a,b) ((a)<(b)?(a):(b))
#define DRM_MAX(a,b) ((a)>(b)?(a):(b))

#define DRM_IF_VERSION(maj, min) (maj << 16 | min)

#define __OS_HAS_AGP	1

#define wait_queue_head_t	atomic_t
#define DRM_WAKEUP(w)		wakeup((void *)w)
#define DRM_INIT_WAITQUEUE(queue) do {(void)(queue);} while (0)

#define DRM_CURPROC		curproc
#define DRM_CURRENTPID		curproc->p_pid
#define DRM_NOOP		do {} while(0)
#define DRM_SPINTYPE		struct mutex
#define DRM_SPININIT(l,name)	mtx_init(l,IPL_NONE)
#define DRM_SPINUNINIT(l)	DRM_NOOP
#define DRM_SPINLOCK(l)		mtx_enter(l)
#define DRM_SPINUNLOCK(l)	mtx_leave(l)
#define DRM_SPINLOCK_IRQSAVE(l, irqflags) do {		\
	DRM_SPINLOCK(l);				\
	(void)irqflags;					\
} while (0)
#define DRM_SPINUNLOCK_IRQRESTORE(u, irqflags) DRM_SPINUNLOCK(u)
#define DRM_SPINLOCK_ASSERT(l)	DRM_NOOP
#define DRM_LOCK()		rw_enter_write(&dev->dev_lock)
#define DRM_UNLOCK()		rw_exit_write(&dev->dev_lock)
#define DRM_MAXUNITS	8
extern struct drm_device *drm_units[];

/* Deal with netbsd code where only the print statements differ */
#define printk printf
#define __unused /* nothing */

#define DRM_IRQ_ARGS		void *arg
typedef int			irqreturn_t;
#define IRQ_HANDLED		1
#define IRQ_NONE		0

enum {
	DRM_IS_NOT_AGP,
	DRM_IS_AGP,
	DRM_MIGHT_BE_AGP
};
#define DRM_AGP_MEM		struct agp_memory_info

/* D_CLONE only supports one device, this will be fixed eventually */
#define drm_get_device_from_kdev(_kdev)			\
	drm_units[0]

#if 0 /* D_CLONE only supports on device for now */
#define drm_get_device_from_kdev(_kdev) 		\
	(minor(kdev) < DRM_MAXUNITS) ?			\
	    drm_units[minor(kdev)] : NULL
#endif



/* DRM_SUSER returns true if the user is superuser */
#define DRM_SUSER(p)		(suser(p, p->p_acflag) == 0)
extern int ticks;		/* really should be in a header */
#define jiffies			ticks
#define DRM_MTRR_WC		MDF_WRITECOMBINE

#define PAGE_ALIGN(addr)	(((addr) + PAGE_SIZE - 1) & PAGE_MASK)

extern struct cfdriver drm_cd;

typedef unsigned long dma_addr_t;
typedef u_int64_t u64;
typedef u_int32_t u32;
typedef u_int16_t u16;
typedef u_int8_t u8;

/* DRM_READMEMORYBARRIER() prevents reordering of reads.
 * DRM_WRITEMEMORYBARRIER() prevents reordering of writes.
 * DRM_MEMORYBARRIER() prevents reordering of reads and writes.
 */
#if defined(__i386__)
#define DRM_READMEMORYBARRIER()		__asm __volatile( \
					"lock; addl $0,0(%%esp)" : : : "memory");
#define DRM_WRITEMEMORYBARRIER()	__asm __volatile("" : : : "memory");
#define DRM_MEMORYBARRIER()		__asm __volatile( \
					"lock; addl $0,0(%%esp)" : : : "memory");
#elif defined(__alpha__)
#define DRM_READMEMORYBARRIER()		alpha_mb();
#define DRM_WRITEMEMORYBARRIER()	alpha_wmb();
#define DRM_MEMORYBARRIER()		alpha_mb();
#elif defined(__amd64__)
#define DRM_READMEMORYBARRIER()		__asm __volatile( \
					"lock; addl $0,0(%%rsp)" : : : "memory");
#define DRM_WRITEMEMORYBARRIER()	__asm __volatile("" : : : "memory");
#define DRM_MEMORYBARRIER()		__asm __volatile( \
					"lock; addl $0,0(%%rsp)" : : : "memory");
#endif

#define DRM_READ8(map, offset) drm_read8(map, offset)
#define DRM_READ16(map, offset) drm_read16(map, offset)
#define DRM_READ32(map, offset) drm_read32(map, offset)
#define DRM_WRITE8(map, offset, val) drm_write8(map, offset, val)
#define DRM_WRITE16(map, offset, val) drm_write16(map, offset, val)
#define DRM_WRITE32(map, offset, val) drm_write32(map, offset, val)

#define DRM_VERIFYAREA_READ( uaddr, size )				\
	(!uvm_map_checkprot(&(curproc->p_vmspace->vm_map),		\
		    (vaddr_t)uaddr, (vaddr_t)uaddr+size, UVM_PROT_READ))

#define DRM_COPY_TO_USER(user, kern, size) \
	copyout(kern, user, size)
#define DRM_COPY_FROM_USER(kern, user, size) \
	copyin(user, kern, size)
#define DRM_COPY_FROM_USER_UNCHECKED(arg1, arg2, arg3) 	\
	copyin(arg2, arg1, arg3)
#define DRM_COPY_TO_USER_UNCHECKED(arg1, arg2, arg3)	\
	copyout(arg2, arg1, arg3)
#define DRM_GET_USER_UNCHECKED(val, uaddr)		\
	((val) = fuword(uaddr), 0)
#define le32_to_cpu(x) letoh32(x)
#define cpu_to_le32(x) htole32(x)

#define DRM_HZ			hz
#define DRM_UDELAY(udelay)	DELAY(udelay)

#define LOCK_TEST_WITH_RETURN(dev, file_priv)				\
do {									\
	if (!_DRM_LOCK_IS_HELD(dev->lock.hw_lock->lock) ||		\
	     dev->lock.file_priv != file_priv) {			\
		DRM_ERROR("%s called without lock held\n",		\
			   __FUNCTION__);				\
		return EINVAL;						\
	}								\
} while (0)

#define DRM_WAIT_ON( ret, queue, timeout, condition )	\
DRM_SPINLOCK(&dev->irq_lock);				\
while ( ret == 0 ) {					\
	if (condition)					\
		break;					\
	ret = msleep(&(queue), &dev->irq_lock,	 	\
	     PZERO | PCATCH, "drmwtq", (timeout));	\
}							\
DRM_SPINUNLOCK(&dev->irq_lock)

#define DRM_ERROR(fmt, arg...) \
	printf("error: [" DRM_NAME ":pid%d:%s] *ERROR* " fmt,		\
	    DRM_CURRENTPID, __func__ , ## arg)

#define DRM_INFO(fmt, arg...)  printf("%s: " fmt, drm_units[0]->device.dv_xname, ## arg)

#undef DRM_DEBUG
#define DRM_DEBUG(fmt, arg...) do {					\
	if (drm_debug_flag)						\
		printf("[" DRM_NAME ":pid%d:%s] " fmt, DRM_CURRENTPID,	\
			__func__ , ## arg);				\
} while (0)

typedef struct drm_pci_id_list
{
	int vendor;
	int device;
	long driver_private;
	char *name;
} drm_pci_id_list_t;

struct drm_file;

#define DRM_AUTH	0x1
#define DRM_MASTER	0x2
#define DRM_ROOT_ONLY	0x4
typedef struct drm_ioctl_desc {
	unsigned long cmd;
	int (*func)(struct drm_device *, void *, struct drm_file *);
	int flags;
} drm_ioctl_desc_t;

struct drm_magic_entry {
	drm_magic_t	       magic;
	struct drm_file	       *priv;
	SPLAY_ENTRY(drm_magic_entry) node;
};

typedef struct drm_buf {
	int		  idx;	       /* Index into master buflist	     */
	int		  total;       /* Buffer size			     */
	int		  used;	       /* Amount of buffer in use (for DMA)  */
	unsigned long	  offset;      /* Byte offset (used internally)	     */
	void 		  *address;    /* KVA of buffer			     */
	unsigned long	  bus_address; /* Bus address of buffer		     */
	__volatile__ int  pending;     /* On hardware DMA queue		     */
	struct drm_file   *file_priv;  /* Unique identifier of holding process */
	void		  *dev_private;  /* Per-buffer private storage       */
} drm_buf_t;

typedef struct drm_dma_handle {
	void *vaddr;
	bus_addr_t busaddr;
	bus_dmamap_t	dmamap;
	bus_dma_segment_t seg;
	void *addr;
	size_t size;
} drm_dma_handle_t;

typedef struct drm_buf_entry {
	int		  buf_size;
	int		  buf_count;
	drm_buf_t	  *buflist;
	int		  seg_count;
	drm_dma_handle_t  **seglist;
	int		  page_order;
} drm_buf_entry_t;

typedef TAILQ_HEAD(drm_file_list, drm_file) drm_file_list_t;
struct drm_file {
	TAILQ_ENTRY(drm_file)	 link;
	void			*driver_priv;
	int			 authenticated;
	unsigned long		 ioctl_count;
	dev_t			 kdev;
	drm_magic_t		 magic;
	int			 flags;
	int			 master;
	int			 minor;
};

struct drm_lock_data {
	struct drm_hw_lock	*hw_lock;	/* Hardware lock */
	/* Unique identifier of holding process (NULL is kernel) */
	struct drm_file		*file_priv;
	unsigned long	 	 lock_time;	/* Time of last lock */
	struct mutex		 spinlock;
};

/* This structure, in the struct drm_device, is always initialized while
 * the device is open.  dev->dma_lock protects the incrementing of
 * dev->buf_use, which when set marks that no further bufs may be allocated
 * until device teardown occurs (when the last open of the device has closed).
 * The high/low watermarks of bufs are only touched by the X Server, and thus
 * not concurrently accessed, so no locking is needed.
 */
typedef struct drm_device_dma {
	drm_buf_entry_t	  bufs[DRM_MAX_ORDER+1];
	int		  buf_count;
	drm_buf_t	  **buflist;	/* Vector of pointers info bufs	   */
	int		  seg_count;
	int		  page_count;
	unsigned long	  *pagelist;
	unsigned long	  byte_count;
	enum {
		_DRM_DMA_USE_AGP = 0x01,
		_DRM_DMA_USE_SG  = 0x02
	} flags;
} drm_device_dma_t;

struct drm_agp_mem {
	void               *handle;
	unsigned long      bound; /* address */
	int                pages;
	TAILQ_ENTRY(drm_agp_mem) link;
};

struct drm_agp_head {
	struct device				*agpdev;
	const char				*chipset;
	TAILQ_HEAD(agp_memlist, drm_agp_mem)	 memory;
	struct agp_info				 info;
	unsigned long				 base;
	unsigned long				 mode;
	unsigned long				 page_mask;
	int					 acquired;
	int					 cant_use_aperture;
	int					 enabled;
   	int					 mtrr;
};

struct drm_sg_dmamem {
	bus_dma_tag_t		sg_tag;
	bus_dmamap_t		sg_map;
	bus_dma_segment_t	*sg_segs;
	int			sg_nsegs;
	size_t			sg_size;
	caddr_t			sg_kva;
};

typedef struct drm_sg_mem {
	unsigned long   handle;
	void            *virtual;
	int             pages;
	dma_addr_t	*busaddr;
	drm_dma_handle_t *dmah;	/* Handle to PCI memory for ATI PCIGART table */
	struct drm_sg_dmamem *mem;
} drm_sg_mem_t;

typedef TAILQ_HEAD(drm_map_list, drm_local_map) drm_map_list_t;

typedef struct drm_local_map {
	TAILQ_ENTRY(drm_local_map)	 link;	/* Link for map list */
	struct vga_pci_bar		*bsr;	/* Vga BAR, if applicable */
	drm_dma_handle_t		*dmah;	/* Handle to DMA mem */
	void				*handle;/* KVA, if mapped */
	bus_space_tag_t			 bst;	/* Tag for mapped pci mem */
	bus_space_handle_t		 bsh;	/* Handle to mapped pci mem */
	u_long				 ext;	/* extent for mmap */
	enum drm_map_flags		 flags;	/* Flags */
	int				 mtrr;	/* Boolean: MTRR used */
	unsigned long			 offset;/* Physical address */
	unsigned long			 size;	/* Physical size (bytes) */
	enum drm_map_type		 type;	/* Type of memory mapped */
} drm_local_map_t;

struct drm_vblank {
	u_int32_t	last_vblank;	/* Last vblank we recieved */
	atomic_t	vbl_count;	/* Number of interrupts */
	atomic_t	vbl_refcount;	/* Number of users */
	int		vbl_enabled;	/* Enabled? */
	int		vbl_inmodeset;	/* is the DDX currently modesetting */
};

/* location of GART table */
#define DRM_ATI_GART_MAIN 1
#define DRM_ATI_GART_FB   2

#define DRM_ATI_GART_PCI  1
#define DRM_ATI_GART_PCIE 2
#define DRM_ATI_GART_IGP  3

#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : (1ULL<<(n)) -1)
#define upper_32_bits(_val) ((u_int32_t)(((_val) >> 16) >> 16))

struct drm_ati_pcigart_info {
	int gart_table_location;
	int gart_reg_if;
	void *addr;
	dma_addr_t bus_addr;
	dma_addr_t table_mask;
	drm_local_map_t mapping;
	int table_size;
};

struct drm_driver_info {
	int	(*load)(struct drm_device *, unsigned long);
	int	(*firstopen)(struct drm_device *);
	int	(*open)(struct drm_device *, struct drm_file *);
	int	(*ioctl)(struct drm_device*, u_long, caddr_t,
		    struct drm_file *);
	void	(*preclose)(struct drm_device *, struct drm_file *);
	void	(*postclose)(struct drm_device *, struct drm_file *);
	void	(*lastclose)(struct drm_device *);
	int	(*unload)(struct drm_device *);
	void	(*reclaim_buffers_locked)(struct drm_device *,
		    struct drm_file *);
	int	(*dma_ioctl)(struct drm_device *, void *, struct drm_file *);
	int	(*dma_quiescent)(struct drm_device *);
	int	(*context_ctor)(struct drm_device *, int);
	int	(*context_dtor)(struct drm_device *, int);
	void	(*irq_preinstall)(struct drm_device *);
	int	(*irq_postinstall)(struct drm_device *);
	void	(*irq_uninstall)(struct drm_device *);
	irqreturn_t	(*irq_handler)(DRM_IRQ_ARGS);
	u_int32_t (*get_vblank_counter)(struct drm_device *, int);
	int	(*enable_vblank)(struct drm_device *, int);
	void	(*disable_vblank)(struct drm_device *, int);

	/**
	 * Called by \c drm_device_is_agp.  Typically used to determine if a
	 * card is really attached to AGP or not.
	 *
	 * \param dev  DRM device handle
	 *
	 * \returns 
	 * One of three values is returned depending on whether or not the
	 * card is absolutely \b not AGP (return of 0), absolutely \b is AGP
	 * (return of 1), or may or may not be AGP (return of 2).
	 */
	int	(*device_is_agp) (struct drm_device * dev);

	int	buf_priv_size;

	int	major;
	int	minor;
	int	patchlevel;
	const char *name;		/* Simple driver name		   */
	const char *desc;		/* Longer driver name		   */
	const char *date;		/* Date of last major changes.	   */

#define DRIVER_AGP		0x1
#define DRIVER_AGP_REQUIRE	0x2
#define DRIVER_MTRR		0x4
#define DRIVER_DMA		0x8
#define DRIVER_PCI_DMA		0x10
#define DRIVER_SG		0x20
#define DRIVER_IRQ		0x40

	u_int	flags;
};

/* Length for the array of resource pointers for drm_get_resource_*. */
#define DRM_MAX_PCI_RESOURCE	3

/** 
 * DRM device functions structure
 */
struct drm_device {
	struct device	  device; /* softc is an extension of struct device */

	const struct drm_driver_info *driver;
	drm_pci_id_list_t *id_entry;	/* PCI ID, name, and chipset private */

	u_int16_t pci_device;		/* PCI device id */
	u_int16_t pci_vendor;		/* PCI vendor id */

	char		  *unique;	/* Unique identifier: e.g., busid  */
	int		  unique_len;	/* Length of unique field	   */
	struct vga_pci_softc *vga_softc;
	
	int		  if_version;	/* Highest interface version set */
				/* Locks */
	DRM_SPINTYPE	  dma_lock;	/* protects dev->dma */
	DRM_SPINTYPE	  irq_lock;	/* protects irq condition checks */
	struct rwlock	  dev_lock;	/* protects everything else */
	DRM_SPINTYPE	  drw_lock;

				/* Usage Counters */
	int		  open_count;	/* Outstanding files open	   */
	int		  buf_use;	/* Buffers in use -- cannot alloc  */

				/* Authentication */
	drm_file_list_t   files;
	drm_magic_t	  magicid;
	SPLAY_HEAD(drm_magic_tree, drm_magic_entry)	magiclist;

	/* Linked list of mappable regions. Protected by dev_lock */
	struct extent	*handle_ext;
	drm_map_list_t	  maplist;

	struct drm_lock_data  lock;	/* Information on hardware lock	*/

				/* DMA queues (contexts) */
	drm_device_dma_t  *dma;		/* Optional pointer for DMA support */

				/* Context support */
	int		  irq;		/* Interrupt used by board	   */
	int		  irq_enabled;	/* True if the irq handler is enabled */
	struct pci_attach_args  pa;
	int		  unit;		/* drm unit number */
	void		  *irqh;	/* Handle from bus_setup_intr      */

	/* Storage of resource pointers for drm_get_resource_* */
	struct vga_pci_bar	  *pcir[DRM_MAX_PCI_RESOURCE];

	int		  pci_domain;
	int		  pci_bus;
	int		  pci_slot;
	int		  pci_func;

	/* VBLANK support */
	int			 num_crtcs;		/* number of crtcs */
	u_int32_t		 max_vblank_count;	/* size of counter reg*/
	DRM_SPINTYPE		 vbl_lock;		/* VBLANK data lock */
	int			 vblank_disable_allowed;
	struct timeout		 vblank_disable_timer;	/* timer for disable */
	struct drm_vblank	*vblank;		/* One per ctrc */

	pid_t		  buf_pgid;

	struct drm_agp_head	*agp;
	drm_sg_mem_t		*sg;  /* Scatter gather memory */
	atomic_t		*ctx_bitmap;
	void			*dev_private;
	unsigned int		 agp_buffer_token;
	drm_local_map_t		*agp_buffer_map;

	u_int		  drw_no;
	/* RB tree of drawable infos */
	RB_HEAD(drawable_tree, bsd_drm_drawable_info) drw_head;
};

extern int	drm_debug_flag;

/* Device setup support (drm_drv.c) */
int	drm_probe(struct pci_attach_args *, drm_pci_id_list_t * );
void	drm_attach(struct device *, struct device *,
	    struct pci_attach_args *, drm_pci_id_list_t *);
int	drm_detach(struct device *, int );
int	drm_activate(struct device *, enum devact);
dev_type_ioctl(drmioctl);
dev_type_open(drmopen);
dev_type_close(drmclose);
dev_type_read(drmread);
dev_type_poll(drmpoll);
dev_type_mmap(drmmmap);
extern drm_local_map_t	*drm_getsarea(struct drm_device *);

/* File operations helpers (drm_fops.c) */
struct drm_file	*drm_find_file_by_minor(struct drm_device *, int);

/* Memory management support (drm_memory.c) */
void	drm_mem_init(void);
void	drm_mem_uninit(void);
void	*drm_alloc(size_t, int);
void	*drm_calloc(size_t, size_t, int);
void	*drm_realloc(void *, size_t, size_t, int);
void	drm_free(void *, size_t, int);
void	*drm_ioremap(struct drm_device *, drm_local_map_t *);
void	drm_ioremapfree(drm_local_map_t *);
int	drm_mtrr_add(unsigned long, size_t, int);
int	drm_mtrr_del(int, unsigned long, size_t, int);

int	drm_ctxbitmap_init(struct drm_device *);
void	drm_ctxbitmap_cleanup(struct drm_device *);
void	drm_ctxbitmap_free(struct drm_device *, int);
int	drm_ctxbitmap_next(struct drm_device *);
u_int8_t	drm_read8(drm_local_map_t *, unsigned long);
u_int16_t drm_read16(drm_local_map_t *, unsigned long);
u_int32_t drm_read32(drm_local_map_t *, unsigned long);
void	drm_write8(drm_local_map_t *, unsigned long, u_int8_t);
void	drm_write16(drm_local_map_t *, unsigned long, u_int16_t);
void	drm_write32(drm_local_map_t *, unsigned long, u_int32_t);

/* Locking IOCTL support (drm_lock.c) */
int	drm_lock_take(struct drm_lock_data *, unsigned int);
int	drm_lock_free(struct drm_lock_data *, unsigned int);

/* Buffer management support (drm_bufs.c) */
unsigned long drm_get_resource_start(struct drm_device *, unsigned int);
unsigned long drm_get_resource_len(struct drm_device *, unsigned int);
void	drm_rmmap(struct drm_device *, drm_local_map_t *);
void	drm_rmmap_locked(struct drm_device *, drm_local_map_t *);
int	drm_order(unsigned long);
drm_local_map_t
	*drm_find_matching_map(struct drm_device *, drm_local_map_t *);
int	drm_addmap(struct drm_device *, unsigned long, unsigned long,
	    enum drm_map_type, enum drm_map_flags, drm_local_map_t **);
int	drm_addbufs_pci(struct drm_device *, struct drm_buf_desc *);
int	drm_addbufs_sg(struct drm_device *, struct drm_buf_desc *);
int	drm_addbufs_agp(struct drm_device *, struct drm_buf_desc *);

/* DMA support (drm_dma.c) */
int	drm_dma_setup(struct drm_device *);
void	drm_dma_takedown(struct drm_device *);
void	drm_cleanup_buf(struct drm_device *, drm_buf_entry_t *);
void	drm_free_buffer(struct drm_device *, drm_buf_t *);
void	drm_reclaim_buffers(struct drm_device *, struct drm_file *);
#define drm_core_reclaim_buffers drm_reclaim_buffers

/* IRQ support (drm_irq.c) */
int	drm_irq_install(struct drm_device *);
int	drm_irq_uninstall(struct drm_device *);
irqreturn_t drm_irq_handler(DRM_IRQ_ARGS);
void	drm_driver_irq_preinstall(struct drm_device *);
void	drm_driver_irq_postinstall(struct drm_device *);
void	drm_driver_irq_uninstall(struct drm_device *);
void	drm_vblank_cleanup(struct drm_device *);
int	drm_vblank_init(struct drm_device *, int);
u_int32_t drm_vblank_count(struct drm_device *, int);
int	drm_vblank_get(struct drm_device *, int);
void	drm_vblank_put(struct drm_device *, int);
int	drm_modeset_ctl(struct drm_device *, void *, struct drm_file *);
void	drm_handle_vblank(struct drm_device *, int);

/* AGP/PCI Express/GART support (drm_agpsupport.c) */
int	drm_device_is_agp(struct drm_device *);
int	drm_device_is_pcie(struct drm_device *);
struct drm_agp_head *drm_agp_init(void);
void	drm_agp_takedown(struct drm_device *);
int	drm_agp_acquire(struct drm_device *);
int	drm_agp_release(struct drm_device *);
int	drm_agp_info(struct drm_device *, struct drm_agp_info *);
int	drm_agp_enable(struct drm_device *, struct drm_agp_mode);
void	*drm_agp_allocate_memory(size_t, u32);
int	drm_agp_free_memory(void *);
int	drm_agp_bind_memory(void *, off_t);
int	drm_agp_unbind_memory(void *);
int	drm_agp_alloc(struct drm_device *, struct drm_agp_buffer *);
int	drm_agp_free(struct drm_device *, struct drm_agp_buffer *);
int	drm_agp_bind(struct drm_device *, struct drm_agp_binding *);
int	drm_agp_unbind(struct drm_device *, struct drm_agp_binding *);

/* Scatter Gather Support (drm_scatter.c) */
void	drm_sg_cleanup(drm_sg_mem_t *);
int	drm_sg_alloc(struct drm_device *, struct drm_scatter_gather *);

/* ATI PCIGART support (ati_pcigart.c) */
int	drm_ati_pcigart_init(struct drm_device *,
	    struct drm_ati_pcigart_info *);
int	drm_ati_pcigart_cleanup(struct drm_device *,
	    struct drm_ati_pcigart_info *);

/* Locking IOCTL support (drm_drv.c) */
int	drm_lock(struct drm_device *, void *, struct drm_file *);
int	drm_unlock(struct drm_device *, void *, struct drm_file *);
int	drm_version(struct drm_device *, void *, struct drm_file *);
int	drm_setversion(struct drm_device *, void *, struct drm_file *);

/* Misc. IOCTL support (drm_ioctl.c) */
int	drm_irq_by_busid(struct drm_device *, void *, struct drm_file *);
int	drm_getunique(struct drm_device *, void *, struct drm_file *);
int	drm_setunique(struct drm_device *, void *, struct drm_file *);
int	drm_getmap(struct drm_device *, void *, struct drm_file *);
int	drm_getclient(struct drm_device *, void *, struct drm_file *);
int	drm_getstats(struct drm_device *, void *, struct drm_file *);
int	drm_noop(struct drm_device *, void *, struct drm_file *);

/* Context IOCTL support (drm_context.c) */
int	drm_resctx(struct drm_device *, void *, struct drm_file *);
int	drm_addctx(struct drm_device *, void *, struct drm_file *);
int	drm_getctx(struct drm_device *, void *, struct drm_file *);
int	drm_rmctx(struct drm_device *, void *, struct drm_file *);

/* Drawable IOCTL support (drm_drawable.c) */
int	drm_adddraw(struct drm_device *, void *, struct drm_file *);
int	drm_rmdraw(struct drm_device *, void *, struct drm_file *);
int	drm_update_draw(struct drm_device *, void *, struct drm_file *);
void	drm_drawable_free_all(struct drm_device *);
struct drm_drawable_info	*drm_get_drawable_info(struct drm_device *,
				    unsigned int);

/* Authentication IOCTL support (drm_auth.c) */
int	drm_getmagic(struct drm_device *, void *, struct drm_file *);
int	drm_authmagic(struct drm_device *, void *, struct drm_file *);
int	drm_magic_cmp(struct drm_magic_entry *, struct drm_magic_entry *);
SPLAY_PROTOTYPE(drm_magic_tree, drm_magic_entry, node, drm_magic_cmp);

/* Buffer management support (drm_bufs.c) */
int	drm_addmap_ioctl(struct drm_device *, void *, struct drm_file *);
int	drm_rmmap_ioctl(struct drm_device *, void *, struct drm_file *);
int	drm_addbufs_ioctl(struct drm_device *, void *, struct drm_file *);
int	drm_freebufs(struct drm_device *, void *, struct drm_file *);
int	drm_mapbufs(struct drm_device *, void *, struct drm_file *);

/* DMA support (drm_dma.c) */
int	drm_dma(struct drm_device *, void *, struct drm_file *);

/* IRQ support (drm_irq.c) */
int	drm_control(struct drm_device *, void *, struct drm_file *);
int	drm_wait_vblank(struct drm_device *, void *, struct drm_file *);

/* AGP/GART support (drm_agpsupport.c) */
int	drm_agp_acquire_ioctl(struct drm_device *, void *, struct drm_file *);
int	drm_agp_release_ioctl(struct drm_device *, void *, struct drm_file *);
int	drm_agp_enable_ioctl(struct drm_device *, void *, struct drm_file *);
int	drm_agp_info_ioctl(struct drm_device *, void *, struct drm_file *);
int	drm_agp_alloc_ioctl(struct drm_device *, void *, struct drm_file *);
int	drm_agp_free_ioctl(struct drm_device *, void *, struct drm_file *);
int	drm_agp_unbind_ioctl(struct drm_device *, void *, struct drm_file *);
int	drm_agp_bind_ioctl(struct drm_device *, void *, struct drm_file *);

/* Scatter Gather Support (drm_scatter.c) */
int	drm_sg_alloc_ioctl(struct drm_device *, void *, struct drm_file *);
int	drm_sg_free(struct drm_device *, void *, struct drm_file *);

/* consistent PCI memory functions (drm_pci.c) */
drm_dma_handle_t *drm_pci_alloc(struct drm_device *, size_t, size_t,
		      dma_addr_t);
void	drm_pci_free(struct drm_device *, drm_dma_handle_t *);

/* Inline replacements for DRM_IOREMAP macros */
#define drm_core_ioremap_wc drm_core_ioremap
static __inline__ void drm_core_ioremap(struct drm_local_map *map, struct drm_device *dev)
{
	map->handle = drm_ioremap(dev, map);
}
static __inline__ void drm_core_ioremapfree(struct drm_local_map *map, struct drm_device *dev)
{
	if ( map->handle && map->size )
		drm_ioremapfree(map);
}

static __inline__ struct drm_local_map *drm_core_findmap(struct drm_device *dev, unsigned long offset)
{
	drm_local_map_t *map;

	DRM_LOCK();
	TAILQ_FOREACH(map, &dev->maplist, link) {
		if (offset == map->ext)
			break;
	}
	DRM_UNLOCK();
	return (map);
}

#endif /* __KERNEL__ */
#endif /* _DRM_P_H_ */
