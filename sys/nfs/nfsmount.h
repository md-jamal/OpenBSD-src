/*	$OpenBSD: nfsmount.h,v 1.20 2009/06/03 00:12:34 thib Exp $	*/
/*	$NetBSD: nfsmount.h,v 1.10 1996/02/18 11:54:03 fvdl Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)nfsmount.h	8.3 (Berkeley) 3/30/95
 */


#ifndef _NFS_NFSMOUNT_H_
#define _NFS_NFSMOUNT_H_

/*
 * Mount structure.
 * One allocated on every NFS mount.
 * Holds NFS specific information for mount.
 */
struct	nfsmount {
	int	nm_flag;		/* Flags for soft/hard... */
	struct	mount *nm_mountp;	/* Vfs structure for this filesystem */
	int	nm_numgrps;		/* Max. size of groupslist */
	u_char	nm_fh[NFSX_V3FHMAX];	/* File handle of root dir */
	int	nm_fhsize;		/* Size of root file handle */
	struct	socket *nm_so;		/* Rpc socket */
	int	nm_sotype;		/* Type of socket */
	int	nm_soproto;		/* and protocol */
	int	nm_soflags;		/* pr_flags for socket protocol */
	struct	mbuf *nm_nam;		/* Addr of server */
	int	nm_timeo;		/* Init timer for NFSMNT_DUMBTIMR */
	int	nm_retry;		/* Max retries */
	int	nm_srtt[NFS_MAX_TIMER];	/* RTT Timers for RPCs */
	int	nm_sdrtt[NFS_MAX_TIMER];
	int	nm_sent;		/* Request send count */
	int	nm_cwnd;		/* Request send window */
	int	nm_timeouts;		/* Request timeouts */
	int	nm_rsize;		/* Max size of read rpc */
	int	nm_wsize;		/* Max size of write rpc */
	int	nm_readdirsize;		/* Size of a readdir rpc */
	int	nm_readahead;		/* Num. of blocks to readahead */
	u_char	nm_verf[NFSX_V3WRITEVERF]; /* V3 write verifier */
	u_short	nm_acregmin;		/* Attr cache file recently modified */
	u_short	nm_acregmax;		/* ac file not recently modified */
	u_short	nm_acdirmin;		/* ac for dir recently modified */
	u_short	nm_acdirmax;		/* ac for dir not recently modified */
};

#ifdef _KERNEL

/* Convert mount ptr to nfsmount ptr: */
#define VFSTONFS(mp)	((struct nfsmount *)((mp)->mnt_data))

/* Prototypes for NFS mount operations: */
int	nfs_mount(struct mount *, const char *, void *, struct nameidata *,
	    struct proc *);
int	mountnfs(struct nfs_args *, struct mount *, struct mbuf *, char *,
	    char *);
int	nfs_mountroot(void);
void	nfs_decode_args(struct nfsmount *, struct nfs_args *,
	    struct nfs_args *);
int	nfs_start(struct mount *, int, struct proc *);
int	nfs_unmount(struct mount *, int, struct proc *);
int	nfs_root(struct mount *, struct vnode **);
int	nfs_quotactl(struct mount *, int, uid_t, caddr_t, struct proc *);
int	nfs_statfs(struct mount *, struct statfs *, struct proc *);
int	nfs_sync(struct mount *, int, struct ucred *, struct proc *);
int	nfs_vget(struct mount *, ino_t, struct vnode **);
int	nfs_fhtovp(struct mount *, struct fid *, struct vnode **);
int	nfs_vptofh(struct vnode *, struct fid *);
int	nfs_fsinfo(struct nfsmount *, struct vnode *, struct ucred *,
	    struct proc *);
void	nfs_init(void);

#endif /* _KERNEL */

#endif
