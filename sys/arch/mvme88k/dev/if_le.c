/*	$OpenBSD: if_le.c,v 1.16 2013/09/24 20:10:48 miod Exp $ */

/*-
 * Copyright (c) 1982, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)if_le.c	8.2 (Berkeley) 10/30/93
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <net/if.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif

#include <net/if_media.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include <mvme88k/dev/if_lereg.h>
#include <mvme88k/dev/if_levar.h>
#include <mvme88k/dev/vme.h>

/* autoconfiguration driver */
void  leattach(struct device *, struct device *, void *);
int   lematch(struct device *, void *, void *);

const struct cfattach le_ca = {
	sizeof(struct le_softc), lematch, leattach
};

void	vlewrcsr(struct lance_softc *, uint16_t, uint16_t);
uint16_t vlerdcsr(struct lance_softc *, uint16_t);
void	nvram_cmd(struct lance_softc *, uint8_t, uint16_t);
uint16_t nvram_read(struct lance_softc *, uint8_t);
void	vleetheraddr(struct lance_softc *);
void	vleinit(struct lance_softc *);
void	vlereset(struct lance_softc *);
int	vle_intr(void *);

/* send command to the nvram controller */
void
nvram_cmd(struct lance_softc *sc, uint8_t cmd, uint16_t addr)
{
	struct vlereg1 *reg1 = (struct vlereg1 *)((struct le_softc *)sc)->sc_r1;
	int i;

	for (i = 0; i < 8; i++) {
		reg1->ler1_ear = ((cmd | (addr << 1)) >> i); 
		CDELAY; 
	} 
}

/* read nvram one bit at a time */
uint16_t
nvram_read(struct lance_softc *sc, uint8_t nvram_addr)
{
	uint16_t val = 0, mask = 0x04000;
	uint16_t wbit;
	struct vlereg1 *reg1 = (struct vlereg1 *)((struct le_softc *)sc)->sc_r1;

	((struct le_softc *)sc)->sc_csr = 0x4f;
	ENABLE_NVRAM;
	nvram_cmd(sc, NVRAM_RCL, 0);
	DISABLE_NVRAM;
	CDELAY;
	ENABLE_NVRAM;
	nvram_cmd(sc, NVRAM_READ, nvram_addr);
	for (wbit = 0; wbit < 15; wbit++) {
		if (reg1->ler1_ear & 0x01)
			val |= mask;
		else
			val &= ~mask;
		mask = mask >> 1;
		CDELAY;
	}
	if (reg1->ler1_ear & 0x01)
		val |= 0x8000;
	else
		val &= 0x7fff;
	CDELAY;
	DISABLE_NVRAM;
	return (val);
}

void
vleetheraddr(struct lance_softc *sc)
{
	uint8_t *cp = sc->sc_arpcom.ac_enaddr;
	uint16_t ival[3];
	int i;

	for (i = 0; i < 3; i++) {
		ival[i] = nvram_read(sc, i);
	}
	memcpy(cp, &ival[0], 6);
}

void
vlewrcsr(struct lance_softc *sc, uint16_t port, uint16_t val)
{
	struct vlereg1 *ler1 = (struct vlereg1 *)((struct le_softc *)sc)->sc_r1;

	ler1->ler1_rap = port;
	ler1->ler1_rdp = val;
}

uint16_t
vlerdcsr(struct lance_softc *sc, uint16_t port)
{
	struct vlereg1 *ler1 = (struct vlereg1 *)((struct le_softc *)sc)->sc_r1;
	uint16_t val;

	ler1->ler1_rap = port;
	val = ler1->ler1_rdp;
	return (val);
}

/* init MVME376, set ipl and vec */
void
vleinit(struct lance_softc *sc)
{
	struct vlereg1 *reg1 = (struct vlereg1 *)((struct le_softc *)sc)->sc_r1;
	uint8_t vec = ((struct le_softc *)sc)->sc_vec;
	uint8_t ipl = ((struct le_softc *)sc)->sc_ipl;

	((struct le_softc *)sc)->sc_csr = 0x4f;
	WRITE_CSR_AND(ipl);
	SET_VEC(vec);
	return;
}

/* MVME376 hardware reset */
void
vlereset(struct lance_softc *sc)
{
	struct vlereg1 *reg1 = (struct vlereg1 *)((struct le_softc *)sc)->sc_r1;

	RESET_HW;
#ifdef LEDEBUG
	if (sc->sc_debug) {
		printf("%s: hardware reset\n", sc->sc_dev.dv_xname);
	}
#endif
	SYSFAIL_CL;
}

int
vle_intr(void *sc)
{
	struct le_softc *lesc = (struct le_softc *)sc;
	struct vlereg1 *reg1 = (struct vlereg1 *)lesc->sc_r1;
	int rc;

	rc = am7990_intr(sc);
	ENABLE_INTR;
	return (rc);
}

int
lematch(struct device *parent, void *vcf, void *args)
{
	struct confargs *ca = args;
	bus_space_tag_t iot = ca->ca_iot;
	bus_space_handle_t ioh;
	int rc;

	if (bus_space_map(iot, ca->ca_paddr, PAGE_SIZE, 0, &ioh) != 0)
		return 0;
	rc = badaddr((vaddr_t)bus_space_vaddr(iot, ioh), 2);
	bus_space_unmap(iot, ioh, PAGE_SIZE);

	return rc == 0;
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
void
leattach(struct device *parent, struct device *self, void *aux)
{
	struct le_softc *lesc = (struct le_softc *)self;
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	struct confargs *ca = aux;
	paddr_t paddr;
	int card;
	bus_space_tag_t iot = ca->ca_iot;
	bus_space_handle_t ioh, memh;

	if (ca->ca_vec < 0) {
		printf(": no more interrupts!\n");
		return;
	}
	if (ca->ca_ipl < 0)
		ca->ca_ipl = IPL_NET;

	/* 
	 * MVME376 uses its own dual-ported RAM for buffers.
	 * Its address is set by DIP switches on board.  We support the six
	 * Motorola address locations; however, the board can be set up at
	 * any other address.
	 */
	switch (ca->ca_paddr) {
	case 0xffff1200:
		card = 0;
		break;
	case 0xffff1400:
		card = 1;
		break;
	case 0xffff1600:
		card = 2;
		break;
	case 0xffff5400:
		card = 3;
		break;
	case 0xffff5600:
		card = 4;
		break;
	case 0xffffa400:
		card = 5;
		break;
	default:
		printf(": unsupported address\n");
		return;
	}

	if (bus_space_map(iot, ca->ca_paddr, PAGE_SIZE, 0, &ioh) != 0) {
		printf(": can't map registers!\n");
		return;
	}

	paddr = VLEMEMBASE - (card * VLEMEMSIZE);
	if (bus_space_map(iot, paddr, VLEMEMSIZE, BUS_SPACE_MAP_LINEAR,
	    &memh) != 0) {
		printf(": can't map buffers!\n");
		bus_space_unmap(iot, ioh, PAGE_SIZE);
		return;
	}
	lesc->sc_r1 = (void *)bus_space_vaddr(iot, ioh);
	lesc->sc_ipl = ca->ca_ipl;
	lesc->sc_vec = ca->ca_vec;

	sc->sc_mem = (void *)bus_space_vaddr(iot, memh);
	sc->sc_memsize = VLEMEMSIZE;
	sc->sc_addr = paddr & 0x00ffffff;
	sc->sc_conf3 = LE_C3_BSWP;
	sc->sc_hwreset = vlereset;
	sc->sc_rdcsr = vlerdcsr;
	sc->sc_wrcsr = vlewrcsr;
	sc->sc_hwinit = vleinit;
	sc->sc_copytodesc = lance_copytobuf_contig;
	sc->sc_copyfromdesc = lance_copyfrombuf_contig;
	sc->sc_copytobuf = lance_copytobuf_contig;
	sc->sc_copyfrombuf = lance_copyfrombuf_contig;
	sc->sc_zerobuf = lance_zerobuf_contig;

	/* get Ethernet address */
	vleetheraddr(sc);

	am7990_config(&lesc->sc_am7990);

	/* connect the interrupt */
	lesc->sc_ih.ih_fn = vle_intr;
	lesc->sc_ih.ih_arg = sc;
	lesc->sc_ih.ih_wantframe = 0;
	lesc->sc_ih.ih_ipl = ca->ca_ipl;
	vmeintr_establish(ca->ca_vec, &lesc->sc_ih, self->dv_xname);
}
