/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Created: Mon Mar 20 22:09:17 1995 ylo
 *
 * Versions of malloc and friends that check their results, and never return
 * failure (they call fatal if they encounter an error).
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

/* RCSID("$OpenBSD: xmalloc.h,v 1.6 2001/06/26 06:33:07 itojun Exp $"); */

#ifndef XMALLOC_H
#define XMALLOC_H

/* Like malloc, but calls fatal() if out of memory. */
void   *xmalloc(size_t);

/* Like realloc, but calls fatal() if out of memory. */
void   *xrealloc(void *, size_t);

/* Frees memory allocated using xmalloc or xrealloc. */
void    xfree(void *);

/* Allocates memory using xmalloc, and copies the string into that memory. */
char   *xstrdup(const char *);

#endif				/* XMALLOC_H */
