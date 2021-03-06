#	$OpenBSD: Makefile.inc,v 1.11 2013/05/12 08:10:07 miod Exp $

MDEC_DIR?=/usr/mdec
CFLAGS+=-Wall -Werror -fno-stack-protector
CFLAGS+=-fno-builtin-printf -fno-builtin-putchar -fno-builtin-vprintf
CFLAGS+=-DHEAP_START=${HEAP_START}

# Load addresses for first and second stage bootstraps
STAGE1_RELOC=	0x00100000 
STAGE2_RELOC=	0x005f0000 

# Base of the heap (ethernet buffers will be allocated below it)
HEAP_START=    0x00700000
