/*

ttymodes.h

Author: Tatu Ylonen <ylo@cs.hut.fi>
	SGTTY stuff contributed by Janne Snabb <snabb@niksula.hut.fi>

Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
                   All rights reserved

Created: Tue Mar 21 15:42:09 1995 ylo

*/

/* RCSID("$Id: ttymodes.h,v 1.2 1999/09/30 05:03:05 deraadt Exp $"); */

/* The tty mode description is a stream of bytes.  The stream consists of
   opcode-arguments pairs.  It is terminated by opcode TTY_OP_END (0).
   Opcodes 1-127 have one-byte arguments.  Opcodes 128-159 have integer
   arguments.  Opcodes 160-255 are not yet defined, and cause parsing to
   stop (they should only be used after any other data).

   The client puts in the stream any modes it knows about, and the
   server ignores any modes it does not know about.  This allows some degree
   of machine-independence, at least between systems that use a posix-like
   tty interface.  The protocol can support other systems as well, but might
   require reimplementing as mode names would likely be different. */
     
/* Some constants and prototypes are defined in packet.h; this file
   is only intended for including from ttymodes.h. */

/* termios macro */		/* sgtty macro */
/* name, op */
TTYCHAR(VINTR, 1) 		SGTTYCHAR(tiotc.t_intrc, 1)
TTYCHAR(VQUIT, 2)		SGTTYCHAR(tiotc.t_quitc, 2)
TTYCHAR(VERASE, 3)		SGTTYCHAR(tio.sg_erase, 3)
TTYCHAR(VEOF, 5)		SGTTYCHAR(tiotc.t_eofc, 5)
#ifdef VEOL2			/* n/a */
TTYCHAR(VEOL2, 7)
#endif /* VEOL2 */
TTYCHAR(VSTART, 8)		SGTTYCHAR(tiotc.t_startc, 8)
TTYCHAR(VSTOP, 9)		SGTTYCHAR(tiotc.t_stopc, 9)
#ifdef VSWTCH
TTYCHAR(VSWTCH, 16)		/* n/a */
#endif /* VSWTCH */
#ifdef VDISCARD
TTYCHAR(VDISCARD, 18)		/* n/a */
#endif /* VDISCARD */

/* name, field, op */
TTYMODE(IGNPAR,	c_iflag, 30)	/* n/a */
TTYMODE(PARMRK,	c_iflag, 31)	/* n/a */
TTYMODE(INPCK, 	c_iflag, 32)	SGTTYMODEN(ANYP, tio.sg_flags, 32)
TTYMODE(ISTRIP,	c_iflag, 33)	SGTTYMODEN(LPASS8, tiolm, 33)
TTYMODE(INLCR, 	c_iflag, 34)	/* n/a */
TTYMODE(IGNCR, 	c_iflag, 35)	/* n/a */
TTYMODE(ICRNL, 	c_iflag, 36)	SGTTYMODE(CRMOD, tio.sg_flags, 36)
TTYMODE(IXON,  	c_iflag, 38)	/* n/a */
TTYMODE(IXANY, 	c_iflag, 39)	SGTTYMODEN(LDECCTQ, tiolm, 39)
TTYMODE(IXOFF, 	c_iflag, 40)	SGTTYMODE(TANDEM, tio.sg_flags, 40)
#ifdef IMAXBEL
TTYMODE(IMAXBEL,c_iflag, 41)	/* n/a */
#endif /* IMAXBEL */

TTYMODE(ISIG,	c_lflag, 50)	/* n/a */
TTYMODE(ICANON,	c_lflag, 51)	SGTTYMODEN(CBREAK, tio.sg_flags, 51)
#ifdef XCASE
TTYMODE(XCASE,	c_lflag, 52)	/* n/a */
#endif
TTYMODE(ECHO,	c_lflag, 53)	SGTTYMODE(ECHO, tio.sg_flags, 53)
TTYMODE(ECHOE,	c_lflag, 54)	SGTTYMODE(LCRTERA, tiolm, 54)
TTYMODE(ECHOK,	c_lflag, 55)	SGTTYMODE(LCRTKIL, tiolm, 55)
TTYMODE(ECHONL,	c_lflag, 56)	/* n/a */
TTYMODE(NOFLSH,	c_lflag, 57)	SGTTYMODE(LNOFLSH, tiolm, 57)
TTYMODE(TOSTOP,	c_lflag, 58)	SGTTYMODE(LTOSTOP, tiolm, 58)
#ifdef IEXTEN
TTYMODE(IEXTEN, c_lflag, 59)	/* n/a */
#endif /* IEXTEN */
#ifdef ECHOKE
TTYMODE(ECHOKE,	c_lflag, 61)	/* n/a */
#endif /* ECHOKE */

TTYMODE(OPOST,	c_oflag, 70)	/* n/a */
TTYMODE(ONLCR,	c_oflag, 72)	SGTTYMODE(CRMOD, tio.sg_flags, 72)
#ifdef OCRNL
TTYMODE(OCRNL,	c_oflag, 73)	/* n/a */
#endif
#ifdef ONOCR
TTYMODE(ONOCR,	c_oflag, 74)	/* n/a */
#endif
#ifdef ONLRET
TTYMODE(ONLRET,	c_oflag, 75)	/* n/a */
#endif

TTYMODE(CS7,	c_cflag, 90)	/* n/a */
TTYMODE(CS8,	c_cflag, 91)	SGTTYMODE(LPASS8, tiolm, 91)
TTYMODE(PARENB,	c_cflag, 92)	/* n/a */
TTYMODE(PARODD,	c_cflag, 93)	SGTTYMODE(ODDP, tio.sg_flags, 93)

