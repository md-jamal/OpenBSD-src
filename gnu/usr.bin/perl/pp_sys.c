/*    pp_sys.c
 *
 *    Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
 *    2004, 2005, 2006, 2007 by Larry Wall and others
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

/*
 * But only a short way ahead its floor and the walls on either side were
 * cloven by a great fissure, out of which the red glare came, now leaping
 * up, now dying down into darkness; and all the while far below there was
 * a rumour and a trouble as of great engines throbbing and labouring.
 */

/* This file contains system pp ("push/pop") functions that
 * execute the opcodes that make up a perl program. A typical pp function
 * expects to find its arguments on the stack, and usually pushes its
 * results onto the stack, hence the 'pp' terminology. Each OP structure
 * contains a pointer to the relevant pp_foo() function.
 *
 * By 'system', we mean ops which interact with the OS, such as pp_open().
 */

#include "EXTERN.h"
#define PERL_IN_PP_SYS_C
#include "perl.h"

#ifdef I_SHADOW
/* Shadow password support for solaris - pdo@cs.umd.edu
 * Not just Solaris: at least HP-UX, IRIX, Linux.
 * The API is from SysV.
 *
 * There are at least two more shadow interfaces,
 * see the comments in pp_gpwent().
 *
 * --jhi */
#   ifdef __hpux__
/* There is a MAXINT coming from <shadow.h> <- <hpsecurity.h> <- <values.h>
 * and another MAXINT from "perl.h" <- <sys/param.h>. */
#       undef MAXINT
#   endif
#   include <shadow.h>
#endif

#ifdef I_SYS_WAIT
# include <sys/wait.h>
#endif

#ifdef I_SYS_RESOURCE
# include <sys/resource.h>
#endif

#ifdef NETWARE
NETDB_DEFINE_CONTEXT
#endif

#ifdef HAS_SELECT
# ifdef I_SYS_SELECT
#  include <sys/select.h>
# endif
#endif

/* XXX Configure test needed.
   h_errno might not be a simple 'int', especially for multi-threaded
   applications, see "extern int errno in perl.h".  Creating such
   a test requires taking into account the differences between
   compiling multithreaded and singlethreaded ($ccflags et al).
   HOST_NOT_FOUND is typically defined in <netdb.h>.
*/
#if defined(HOST_NOT_FOUND) && !defined(h_errno) && !defined(__CYGWIN__)
extern int h_errno;
#endif

#ifdef HAS_PASSWD
# ifdef I_PWD
#  include <pwd.h>
# else
#  if !defined(VMS)
    struct passwd *getpwnam (char *);
    struct passwd *getpwuid (Uid_t);
#  endif
# endif
# ifdef HAS_GETPWENT
#ifndef getpwent
  struct passwd *getpwent (void);
#elif defined (VMS) && defined (my_getpwent)
  struct passwd *Perl_my_getpwent (pTHX);
#endif
# endif
#endif

#ifdef HAS_GROUP
# ifdef I_GRP
#  include <grp.h>
# else
    struct group *getgrnam (char *);
    struct group *getgrgid (Gid_t);
# endif
# ifdef HAS_GETGRENT
#ifndef getgrent
    struct group *getgrent (void);
#endif
# endif
#endif

#ifdef I_UTIME
#  if defined(_MSC_VER) || defined(__MINGW32__)
#    include <sys/utime.h>
#  else
#    include <utime.h>
#  endif
#endif

#ifdef HAS_CHSIZE
# ifdef my_chsize  /* Probably #defined to Perl_my_chsize in embed.h */
#   undef my_chsize
# endif
# define my_chsize PerlLIO_chsize
#else
# ifdef HAS_TRUNCATE
#   define my_chsize PerlLIO_chsize
# else
I32 my_chsize(int fd, Off_t length);
# endif
#endif

#ifdef HAS_FLOCK
#  define FLOCK flock
#else /* no flock() */

   /* fcntl.h might not have been included, even if it exists, because
      the current Configure only sets I_FCNTL if it's needed to pick up
      the *_OK constants.  Make sure it has been included before testing
      the fcntl() locking constants. */
#  if defined(HAS_FCNTL) && !defined(I_FCNTL)
#    include <fcntl.h>
#  endif

#  if defined(HAS_FCNTL) && defined(FCNTL_CAN_LOCK)
#    define FLOCK fcntl_emulate_flock
#    define FCNTL_EMULATE_FLOCK
#  else /* no flock() or fcntl(F_SETLK,...) */
#    ifdef HAS_LOCKF
#      define FLOCK lockf_emulate_flock
#      define LOCKF_EMULATE_FLOCK
#    endif /* lockf */
#  endif /* no flock() or fcntl(F_SETLK,...) */

#  ifdef FLOCK
     static int FLOCK (int, int);

    /*
     * These are the flock() constants.  Since this sytems doesn't have
     * flock(), the values of the constants are probably not available.
     */
#    ifndef LOCK_SH
#      define LOCK_SH 1
#    endif
#    ifndef LOCK_EX
#      define LOCK_EX 2
#    endif
#    ifndef LOCK_NB
#      define LOCK_NB 4
#    endif
#    ifndef LOCK_UN
#      define LOCK_UN 8
#    endif
#  endif /* emulating flock() */

#endif /* no flock() */

#define ZBTLEN 10
static const char zero_but_true[ZBTLEN + 1] = "0 but true";

#if defined(I_SYS_ACCESS) && !defined(R_OK)
#  include <sys/access.h>
#endif

#if defined(HAS_FCNTL) && defined(F_SETFD) && !defined(FD_CLOEXEC)
#  define FD_CLOEXEC 1		/* NeXT needs this */
#endif

#include "reentr.h"

#ifdef __Lynx__
/* Missing protos on LynxOS */
void sethostent(int);
void endhostent(void);
void setnetent(int);
void endnetent(void);
void setprotoent(int);
void endprotoent(void);
void setservent(int);
void endservent(void);
#endif

#undef PERL_EFF_ACCESS	/* EFFective uid/gid ACCESS */

/* AIX 5.2 and below use mktime for localtime, and defines the edge case
 * for time 0x7fffffff to be valid only in UTC. AIX 5.3 provides localtime64
 * available in the 32bit environment, which could warrant Configure
 * checks in the future.
 */
#ifdef  _AIX
#define LOCALTIME_EDGECASE_BROKEN
#endif

/* F_OK unused: if stat() cannot find it... */

#if !defined(PERL_EFF_ACCESS) && defined(HAS_ACCESS) && defined(EFF_ONLY_OK) && !defined(NO_EFF_ONLY_OK)
    /* Digital UNIX (when the EFF_ONLY_OK gets fixed), UnixWare */
#   define PERL_EFF_ACCESS(p,f) (access((p), (f) | EFF_ONLY_OK))
#endif

#if !defined(PERL_EFF_ACCESS) && defined(HAS_EACCESS)
#   ifdef I_SYS_SECURITY
#       include <sys/security.h>
#   endif
#   ifdef ACC_SELF
        /* HP SecureWare */
#       define PERL_EFF_ACCESS(p,f) (eaccess((p), (f), ACC_SELF))
#   else
        /* SCO */
#       define PERL_EFF_ACCESS(p,f) (eaccess((p), (f)))
#   endif
#endif

#if !defined(PERL_EFF_ACCESS) && defined(HAS_ACCESSX) && defined(ACC_SELF)
    /* AIX */
#   define PERL_EFF_ACCESS(p,f) (accessx((p), (f), ACC_SELF))
#endif


#if !defined(PERL_EFF_ACCESS) && defined(HAS_ACCESS)	\
    && (defined(HAS_SETREUID) || defined(HAS_SETRESUID)		\
	|| defined(HAS_SETREGID) || defined(HAS_SETRESGID))
/* The Hard Way. */
STATIC int
S_emulate_eaccess(pTHX_ const char* path, Mode_t mode)
{
    const Uid_t ruid = getuid();
    const Uid_t euid = geteuid();
    const Gid_t rgid = getgid();
    const Gid_t egid = getegid();
    int res;

    LOCK_CRED_MUTEX;
#if !defined(HAS_SETREUID) && !defined(HAS_SETRESUID)
    Perl_croak(aTHX_ "switching effective uid is not implemented");
#else
#ifdef HAS_SETREUID
    if (setreuid(euid, ruid))
#else
#ifdef HAS_SETRESUID
    if (setresuid(euid, ruid, (Uid_t)-1))
#endif
#endif
	Perl_croak(aTHX_ "entering effective uid failed");
#endif

#if !defined(HAS_SETREGID) && !defined(HAS_SETRESGID)
    Perl_croak(aTHX_ "switching effective gid is not implemented");
#else
#ifdef HAS_SETREGID
    if (setregid(egid, rgid))
#else
#ifdef HAS_SETRESGID
    if (setresgid(egid, rgid, (Gid_t)-1))
#endif
#endif
	Perl_croak(aTHX_ "entering effective gid failed");
#endif

    res = access(path, mode);

#ifdef HAS_SETREUID
    if (setreuid(ruid, euid))
#else
#ifdef HAS_SETRESUID
    if (setresuid(ruid, euid, (Uid_t)-1))
#endif
#endif
	Perl_croak(aTHX_ "leaving effective uid failed");

#ifdef HAS_SETREGID
    if (setregid(rgid, egid))
#else
#ifdef HAS_SETRESGID
    if (setresgid(rgid, egid, (Gid_t)-1))
#endif
#endif
	Perl_croak(aTHX_ "leaving effective gid failed");
    UNLOCK_CRED_MUTEX;

    return res;
}
#   define PERL_EFF_ACCESS(p,f) (S_emulate_eaccess(aTHX_ (p), (f)))
#endif

PP(pp_backtick)
{
    dVAR; dSP; dTARGET;
    PerlIO *fp;
    const char * const tmps = POPpconstx;
    const I32 gimme = GIMME_V;
    const char *mode = "r";

    TAINT_PROPER("``");
    if (PL_op->op_private & OPpOPEN_IN_RAW)
	mode = "rb";
    else if (PL_op->op_private & OPpOPEN_IN_CRLF)
	mode = "rt";
    fp = PerlProc_popen(tmps, mode);
    if (fp) {
        const char * const type = Perl_PerlIO_context_layers(aTHX_ NULL);
	if (type && *type)
	    PerlIO_apply_layers(aTHX_ fp,mode,type);

	if (gimme == G_VOID) {
	    char tmpbuf[256];
	    while (PerlIO_read(fp, tmpbuf, sizeof tmpbuf) > 0)
		NOOP;
	}
	else if (gimme == G_SCALAR) {
	    ENTER;
	    SAVESPTR(PL_rs);
	    PL_rs = &PL_sv_undef;
	    sv_setpvn(TARG, "", 0);	/* note that this preserves previous buffer */
	    while (sv_gets(TARG, fp, SvCUR(TARG)) != NULL)
		NOOP;
	    LEAVE;
	    XPUSHs(TARG);
	    SvTAINTED_on(TARG);
	}
	else {
	    for (;;) {
		SV * const sv = newSV(79);
		if (sv_gets(sv, fp, 0) == NULL) {
		    SvREFCNT_dec(sv);
		    break;
		}
		XPUSHs(sv_2mortal(sv));
		if (SvLEN(sv) - SvCUR(sv) > 20) {
		    SvPV_shrink_to_cur(sv);
		}
		SvTAINTED_on(sv);
	    }
	}
	STATUS_NATIVE_CHILD_SET(PerlProc_pclose(fp));
	TAINT;		/* "I believe that this is not gratuitous!" */
    }
    else {
	STATUS_NATIVE_CHILD_SET(-1);
	if (gimme == G_SCALAR)
	    RETPUSHUNDEF;
    }

    RETURN;
}

PP(pp_glob)
{
    dVAR;
    OP *result;
    tryAMAGICunTARGET(iter, -1);

    /* Note that we only ever get here if File::Glob fails to load
     * without at the same time croaking, for some reason, or if
     * perl was built with PERL_EXTERNAL_GLOB */

    ENTER;

#ifndef VMS
    if (PL_tainting) {
	/*
	 * The external globbing program may use things we can't control,
	 * so for security reasons we must assume the worst.
	 */
	TAINT;
	taint_proper(PL_no_security, "glob");
    }
#endif /* !VMS */

    SAVESPTR(PL_last_in_gv);	/* We don't want this to be permanent. */
    PL_last_in_gv = (GV*)*PL_stack_sp--;

    SAVESPTR(PL_rs);		/* This is not permanent, either. */
    PL_rs = sv_2mortal(newSVpvs("\000"));
#ifndef DOSISH
#ifndef CSH
    *SvPVX(PL_rs) = '\n';
#endif	/* !CSH */
#endif	/* !DOSISH */

    result = do_readline();
    LEAVE;
    return result;
}

PP(pp_rcatline)
{
    dVAR;
    PL_last_in_gv = cGVOP_gv;
    return do_readline();
}

PP(pp_warn)
{
    dVAR; dSP; dMARK;
    SV *tmpsv;
    const char *tmps;
    STRLEN len;
    if (SP - MARK > 1) {
	dTARGET;
	do_join(TARG, &PL_sv_no, MARK, SP);
	tmpsv = TARG;
	SP = MARK + 1;
    }
    else if (SP == MARK) {
	tmpsv = &PL_sv_no;
	EXTEND(SP, 1);
	SP = MARK + 1;
    }
    else {
	tmpsv = TOPs;
    }
    tmps = SvPV_const(tmpsv, len);
    if ((!tmps || !len) && PL_errgv) {
  	SV * const error = ERRSV;
	SvUPGRADE(error, SVt_PV);
	if (SvPOK(error) && SvCUR(error))
	    sv_catpvs(error, "\t...caught");
	tmpsv = error;
	tmps = SvPV_const(tmpsv, len);
    }
    if (!tmps || !len)
	tmpsv = sv_2mortal(newSVpvs("Warning: something's wrong"));

    Perl_warn(aTHX_ "%"SVf, SVfARG(tmpsv));
    RETSETYES;
}

PP(pp_die)
{
    dVAR; dSP; dMARK;
    const char *tmps;
    SV *tmpsv;
    STRLEN len;
    bool multiarg = 0;
#ifdef VMS
    VMSISH_HUSHED  = VMSISH_HUSHED || (PL_op->op_private & OPpHUSH_VMSISH);
#endif
    if (SP - MARK != 1) {
	dTARGET;
	do_join(TARG, &PL_sv_no, MARK, SP);
	tmpsv = TARG;
	tmps = SvPV_const(tmpsv, len);
	multiarg = 1;
	SP = MARK + 1;
    }
    else {
	tmpsv = TOPs;
        tmps = SvROK(tmpsv) ? (const char *)NULL : SvPV_const(tmpsv, len);
    }
    if (!tmps || !len) {
	SV * const error = ERRSV;
	SvUPGRADE(error, SVt_PV);
	if (multiarg ? SvROK(error) : SvROK(tmpsv)) {
	    if (!multiarg)
		SvSetSV(error,tmpsv);
	    else if (sv_isobject(error)) {
		HV * const stash = SvSTASH(SvRV(error));
		GV * const gv = gv_fetchmethod(stash, "PROPAGATE");
		if (gv) {
		    SV * const file = sv_2mortal(newSVpv(CopFILE(PL_curcop),0));
		    SV * const line = sv_2mortal(newSVuv(CopLINE(PL_curcop)));
		    EXTEND(SP, 3);
		    PUSHMARK(SP);
		    PUSHs(error);
		    PUSHs(file);
 		    PUSHs(line);
		    PUTBACK;
		    call_sv((SV*)GvCV(gv),
			    G_SCALAR|G_EVAL|G_KEEPERR);
		    sv_setsv(error,*PL_stack_sp--);
		}
	    }
	    DIE(aTHX_ NULL);
	}
	else {
	    if (SvPOK(error) && SvCUR(error))
		sv_catpvs(error, "\t...propagated");
	    tmpsv = error;
	    if (SvOK(tmpsv))
		tmps = SvPV_const(tmpsv, len);
	    else
		tmps = NULL;
	}
    }
    if (!tmps || !len)
	tmpsv = sv_2mortal(newSVpvs("Died"));

    DIE(aTHX_ "%"SVf, SVfARG(tmpsv));
}

/* I/O. */

PP(pp_open)
{
    dVAR; dSP;
    dMARK; dORIGMARK;
    dTARGET;
    SV *sv;
    IO *io;
    const char *tmps;
    STRLEN len;
    bool  ok;

    GV * const gv = (GV *)*++MARK;

    if (!isGV(gv))
	DIE(aTHX_ PL_no_usym, "filehandle");

    if ((io = GvIOp(gv))) {
	MAGIC *mg;
	IoFLAGS(GvIOp(gv)) &= ~IOf_UNTAINT;

	if (IoDIRP(io) && ckWARN2(WARN_IO, WARN_DEPRECATED))
	    Perl_warner(aTHX_ packWARN2(WARN_IO, WARN_DEPRECATED),
		    "Opening dirhandle %s also as a file", GvENAME(gv));

	mg = SvTIED_mg((SV*)io, PERL_MAGIC_tiedscalar);
	if (mg) {
	    /* Method's args are same as ours ... */
	    /* ... except handle is replaced by the object */
	    *MARK-- = SvTIED_obj((SV*)io, mg);
	    PUSHMARK(MARK);
	    PUTBACK;
	    ENTER;
	    call_method("OPEN", G_SCALAR);
	    LEAVE;
	    SPAGAIN;
	    RETURN;
	}
    }

    if (MARK < SP) {
	sv = *++MARK;
    }
    else {
	sv = GvSVn(gv);
    }

    tmps = SvPV_const(sv, len);
    ok = do_openn(gv, tmps, len, FALSE, O_RDONLY, 0, NULL, MARK+1, (SP-MARK));
    SP = ORIGMARK;
    if (ok)
	PUSHi( (I32)PL_forkprocess );
    else if (PL_forkprocess == 0)		/* we are a new child */
	PUSHi(0);
    else
	RETPUSHUNDEF;
    RETURN;
}

PP(pp_close)
{
    dVAR; dSP;
    GV * const gv = (MAXARG == 0) ? PL_defoutgv : (GV*)POPs;

    if (gv) {
	IO * const io = GvIO(gv);
	if (io) {
	    MAGIC * const mg = SvTIED_mg((SV*)io, PERL_MAGIC_tiedscalar);
	    if (mg) {
		PUSHMARK(SP);
		XPUSHs(SvTIED_obj((SV*)io, mg));
		PUTBACK;
		ENTER;
		call_method("CLOSE", G_SCALAR);
		LEAVE;
		SPAGAIN;
		RETURN;
	    }
	}
    }
    EXTEND(SP, 1);
    PUSHs(boolSV(do_close(gv, TRUE)));
    RETURN;
}

PP(pp_pipe_op)
{
#ifdef HAS_PIPE
    dVAR;
    dSP;
    register IO *rstio;
    register IO *wstio;
    int fd[2];

    GV * const wgv = (GV*)POPs;
    GV * const rgv = (GV*)POPs;

    if (!rgv || !wgv)
	goto badexit;

    if (SvTYPE(rgv) != SVt_PVGV || SvTYPE(wgv) != SVt_PVGV)
	DIE(aTHX_ PL_no_usym, "filehandle");
    rstio = GvIOn(rgv);
    wstio = GvIOn(wgv);

    if (IoIFP(rstio))
	do_close(rgv, FALSE);
    if (IoIFP(wstio))
	do_close(wgv, FALSE);

    if (PerlProc_pipe(fd) < 0)
	goto badexit;

    IoIFP(rstio) = PerlIO_fdopen(fd[0], "r"PIPE_OPEN_MODE);
    IoOFP(wstio) = PerlIO_fdopen(fd[1], "w"PIPE_OPEN_MODE);
    IoOFP(rstio) = IoIFP(rstio);
    IoIFP(wstio) = IoOFP(wstio);
    IoTYPE(rstio) = IoTYPE_RDONLY;
    IoTYPE(wstio) = IoTYPE_WRONLY;

    if (!IoIFP(rstio) || !IoOFP(wstio)) {
	if (IoIFP(rstio))
	    PerlIO_close(IoIFP(rstio));
	else
	    PerlLIO_close(fd[0]);
	if (IoOFP(wstio))
	    PerlIO_close(IoOFP(wstio));
	else
	    PerlLIO_close(fd[1]);
	goto badexit;
    }
#if defined(HAS_FCNTL) && defined(F_SETFD)
    fcntl(fd[0],F_SETFD,fd[0] > PL_maxsysfd);	/* ensure close-on-exec */
    fcntl(fd[1],F_SETFD,fd[1] > PL_maxsysfd);	/* ensure close-on-exec */
#endif
    RETPUSHYES;

badexit:
    RETPUSHUNDEF;
#else
    DIE(aTHX_ PL_no_func, "pipe");
#endif
}

PP(pp_fileno)
{
    dVAR; dSP; dTARGET;
    GV *gv;
    IO *io;
    PerlIO *fp;
    MAGIC  *mg;

    if (MAXARG < 1)
	RETPUSHUNDEF;
    gv = (GV*)POPs;

    if (gv && (io = GvIO(gv))
	&& (mg = SvTIED_mg((SV*)io, PERL_MAGIC_tiedscalar)))
    {
	PUSHMARK(SP);
	XPUSHs(SvTIED_obj((SV*)io, mg));
	PUTBACK;
	ENTER;
	call_method("FILENO", G_SCALAR);
	LEAVE;
	SPAGAIN;
	RETURN;
    }

    if (!gv || !(io = GvIO(gv)) || !(fp = IoIFP(io))) {
	/* Can't do this because people seem to do things like
	   defined(fileno($foo)) to check whether $foo is a valid fh.
	  if (ckWARN2(WARN_UNOPENED,WARN_CLOSED))
	      report_evil_fh(gv, io, PL_op->op_type);
	    */
	RETPUSHUNDEF;
    }

    PUSHi(PerlIO_fileno(fp));
    RETURN;
}

PP(pp_umask)
{
    dVAR;
    dSP;
#ifdef HAS_UMASK
    dTARGET;
    Mode_t anum;

    if (MAXARG < 1) {
	anum = PerlLIO_umask(022);
	/* setting it to 022 between the two calls to umask avoids
	 * to have a window where the umask is set to 0 -- meaning
	 * that another thread could create world-writeable files. */
	if (anum != 022)
	    (void)PerlLIO_umask(anum);
    }
    else
	anum = PerlLIO_umask(POPi);
    TAINT_PROPER("umask");
    XPUSHi(anum);
#else
    /* Only DIE if trying to restrict permissions on "user" (self).
     * Otherwise it's harmless and more useful to just return undef
     * since 'group' and 'other' concepts probably don't exist here. */
    if (MAXARG >= 1 && (POPi & 0700))
	DIE(aTHX_ "umask not implemented");
    XPUSHs(&PL_sv_undef);
#endif
    RETURN;
}

PP(pp_binmode)
{
    dVAR; dSP;
    GV *gv;
    IO *io;
    PerlIO *fp;
    SV *discp = NULL;

    if (MAXARG < 1)
	RETPUSHUNDEF;
    if (MAXARG > 1) {
	discp = POPs;
    }

    gv = (GV*)POPs;

    if (gv && (io = GvIO(gv))) {
	MAGIC * const mg = SvTIED_mg((SV*)io, PERL_MAGIC_tiedscalar);
	if (mg) {
	    PUSHMARK(SP);
	    XPUSHs(SvTIED_obj((SV*)io, mg));
	    if (discp)
		XPUSHs(discp);
	    PUTBACK;
	    ENTER;
	    call_method("BINMODE", G_SCALAR);
	    LEAVE;
	    SPAGAIN;
	    RETURN;
	}
    }

    EXTEND(SP, 1);
    if (!(io = GvIO(gv)) || !(fp = IoIFP(io))) {
	if (ckWARN2(WARN_UNOPENED,WARN_CLOSED))
	    report_evil_fh(gv, io, PL_op->op_type);
	SETERRNO(EBADF,RMS_IFI);
        RETPUSHUNDEF;
    }

    PUTBACK;
    {
	const int mode = mode_from_discipline(discp);
	const char *const d = (discp ? SvPV_nolen_const(discp) : NULL);
	if (PerlIO_binmode(aTHX_ fp, IoTYPE(io), mode, d)) {
	    if (IoOFP(io) && IoOFP(io) != IoIFP(io)) {
		if (!PerlIO_binmode(aTHX_ IoOFP(io), IoTYPE(io), mode, d)) {
		    SPAGAIN;
		    RETPUSHUNDEF;
		}
	    }
	    SPAGAIN;
	    RETPUSHYES;
	}
	else {
	    SPAGAIN;
	    RETPUSHUNDEF;
	}
    }
}

PP(pp_tie)
{
    dVAR; dSP; dMARK;
    HV* stash;
    GV *gv;
    SV *sv;
    const I32 markoff = MARK - PL_stack_base;
    const char *methname;
    int how = PERL_MAGIC_tied;
    U32 items;
    SV *varsv = *++MARK;

    switch(SvTYPE(varsv)) {
	case SVt_PVHV:
	    methname = "TIEHASH";
	    HvEITER_set((HV *)varsv, 0);
	    break;
	case SVt_PVAV:
	    methname = "TIEARRAY";
	    break;
	case SVt_PVGV:
#ifdef GV_UNIQUE_CHECK
	    if (GvUNIQUE((GV*)varsv)) {
                Perl_croak(aTHX_ "Attempt to tie unique GV");
	    }
#endif
	    methname = "TIEHANDLE";
	    how = PERL_MAGIC_tiedscalar;
	    /* For tied filehandles, we apply tiedscalar magic to the IO
	       slot of the GP rather than the GV itself. AMS 20010812 */
	    if (!GvIOp(varsv))
		GvIOp(varsv) = newIO();
	    varsv = (SV *)GvIOp(varsv);
	    break;
	default:
	    methname = "TIESCALAR";
	    how = PERL_MAGIC_tiedscalar;
	    break;
    }
    items = SP - MARK++;
    if (sv_isobject(*MARK)) {
	ENTER;
	PUSHSTACKi(PERLSI_MAGIC);
	PUSHMARK(SP);
	EXTEND(SP,(I32)items);
	while (items--)
	    PUSHs(*MARK++);
	PUTBACK;
	call_method(methname, G_SCALAR);
    }
    else {
	/* Not clear why we don't call call_method here too.
	 * perhaps to get different error message ?
	 */
	stash = gv_stashsv(*MARK, 0);
	if (!stash || !(gv = gv_fetchmethod(stash, methname))) {
	    DIE(aTHX_ "Can't locate object method \"%s\" via package \"%"SVf"\"",
		 methname, SVfARG(*MARK));
	}
	ENTER;
	PUSHSTACKi(PERLSI_MAGIC);
	PUSHMARK(SP);
	EXTEND(SP,(I32)items);
	while (items--)
	    PUSHs(*MARK++);
	PUTBACK;
	call_sv((SV*)GvCV(gv), G_SCALAR);
    }
    SPAGAIN;

    sv = TOPs;
    POPSTACK;
    if (sv_isobject(sv)) {
	sv_unmagic(varsv, how);
	/* Croak if a self-tie on an aggregate is attempted. */
	if (varsv == SvRV(sv) &&
	    (SvTYPE(varsv) == SVt_PVAV ||
	     SvTYPE(varsv) == SVt_PVHV))
	    Perl_croak(aTHX_
		       "Self-ties of arrays and hashes are not supported");
	sv_magic(varsv, (SvRV(sv) == varsv ? NULL : sv), how, NULL, 0);
    }
    LEAVE;
    SP = PL_stack_base + markoff;
    PUSHs(sv);
    RETURN;
}

PP(pp_untie)
{
    dVAR; dSP;
    MAGIC *mg;
    SV *sv = POPs;
    const char how = (SvTYPE(sv) == SVt_PVHV || SvTYPE(sv) == SVt_PVAV)
		? PERL_MAGIC_tied : PERL_MAGIC_tiedscalar;

    if (SvTYPE(sv) == SVt_PVGV && !(sv = (SV *)GvIOp(sv)))
	RETPUSHYES;

    if ((mg = SvTIED_mg(sv, how))) {
	SV * const obj = SvRV(SvTIED_obj(sv, mg));
        if (obj) {
	    GV * const gv = gv_fetchmethod_autoload(SvSTASH(obj), "UNTIE", FALSE);
	    CV *cv;
	    if (gv && isGV(gv) && (cv = GvCV(gv))) {
	       PUSHMARK(SP);
	       XPUSHs(SvTIED_obj((SV*)gv, mg));
	       XPUSHs(sv_2mortal(newSViv(SvREFCNT(obj)-1)));
	       PUTBACK;
	       ENTER;
	       call_sv((SV *)cv, G_VOID);
	       LEAVE;
	       SPAGAIN;
            }
	    else if (mg && SvREFCNT(obj) > 1 && ckWARN(WARN_UNTIE)) {
		  Perl_warner(aTHX_ packWARN(WARN_UNTIE),
		      "untie attempted while %"UVuf" inner references still exist",
		       (UV)SvREFCNT(obj) - 1 ) ;
	    }
        }
    }
    sv_unmagic(sv, how) ;
    RETPUSHYES;
}

PP(pp_tied)
{
    dVAR;
    dSP;
    const MAGIC *mg;
    SV *sv = POPs;
    const char how = (SvTYPE(sv) == SVt_PVHV || SvTYPE(sv) == SVt_PVAV)
		? PERL_MAGIC_tied : PERL_MAGIC_tiedscalar;

    if (SvTYPE(sv) == SVt_PVGV && !(sv = (SV *)GvIOp(sv)))
	RETPUSHUNDEF;

    if ((mg = SvTIED_mg(sv, how))) {
	SV *osv = SvTIED_obj(sv, mg);
	if (osv == mg->mg_obj)
	    osv = sv_mortalcopy(osv);
	PUSHs(osv);
	RETURN;
    }
    RETPUSHUNDEF;
}

PP(pp_dbmopen)
{
    dVAR; dSP;
    dPOPPOPssrl;
    HV* stash;
    GV *gv;

    HV * const hv = (HV*)POPs;
    SV * const sv = sv_2mortal(newSVpvs("AnyDBM_File"));
    stash = gv_stashsv(sv, 0);
    if (!stash || !(gv = gv_fetchmethod(stash, "TIEHASH"))) {
	PUTBACK;
	require_pv("AnyDBM_File.pm");
	SPAGAIN;
	if (!(gv = gv_fetchmethod(stash, "TIEHASH")))
	    DIE(aTHX_ "No dbm on this machine");
    }

    ENTER;
    PUSHMARK(SP);

    EXTEND(SP, 5);
    PUSHs(sv);
    PUSHs(left);
    if (SvIV(right))
	PUSHs(sv_2mortal(newSVuv(O_RDWR|O_CREAT)));
    else
	PUSHs(sv_2mortal(newSVuv(O_RDWR)));
    PUSHs(right);
    PUTBACK;
    call_sv((SV*)GvCV(gv), G_SCALAR);
    SPAGAIN;

    if (!sv_isobject(TOPs)) {
	SP--;
	PUSHMARK(SP);
	PUSHs(sv);
	PUSHs(left);
	PUSHs(sv_2mortal(newSVuv(O_RDONLY)));
	PUSHs(right);
	PUTBACK;
	call_sv((SV*)GvCV(gv), G_SCALAR);
	SPAGAIN;
    }

    if (sv_isobject(TOPs)) {
	sv_unmagic((SV *) hv, PERL_MAGIC_tied);
	sv_magic((SV*)hv, TOPs, PERL_MAGIC_tied, NULL, 0);
    }
    LEAVE;
    RETURN;
}

PP(pp_sselect)
{
#ifdef HAS_SELECT
    dVAR; dSP; dTARGET;
    register I32 i;
    register I32 j;
    register char *s;
    register SV *sv;
    NV value;
    I32 maxlen = 0;
    I32 nfound;
    struct timeval timebuf;
    struct timeval *tbuf = &timebuf;
    I32 growsize;
    char *fd_sets[4];
#if BYTEORDER != 0x1234 && BYTEORDER != 0x12345678
	I32 masksize;
	I32 offset;
	I32 k;

#   if BYTEORDER & 0xf0000
#	define ORDERBYTE (0x88888888 - BYTEORDER)
#   else
#	define ORDERBYTE (0x4444 - BYTEORDER)
#   endif

#endif

    SP -= 4;
    for (i = 1; i <= 3; i++) {
	SV * const sv = SP[i];
	if (!SvOK(sv))
	    continue;
	if (SvREADONLY(sv)) {
	    if (SvIsCOW(sv))
		sv_force_normal_flags(sv, 0);
	    if (SvREADONLY(sv) && !(SvPOK(sv) && SvCUR(sv) == 0))
		DIE(aTHX_ PL_no_modify);
	}
	if (!SvPOK(sv)) {
	    if (ckWARN(WARN_MISC))
                Perl_warner(aTHX_ packWARN(WARN_MISC), "Non-string passed as bitmask");
	    SvPV_force_nolen(sv);	/* force string conversion */
	}
	j = SvCUR(sv);
	if (maxlen < j)
	    maxlen = j;
    }

/* little endians can use vecs directly */
#if BYTEORDER != 0x1234 && BYTEORDER != 0x12345678
#  ifdef NFDBITS

#    ifndef NBBY
#     define NBBY 8
#    endif

    masksize = NFDBITS / NBBY;
#  else
    masksize = sizeof(long);	/* documented int, everyone seems to use long */
#  endif
    Zero(&fd_sets[0], 4, char*);
#endif

#  if SELECT_MIN_BITS == 1
    growsize = sizeof(fd_set);
#  else
#   if defined(__GLIBC__) && defined(__FD_SETSIZE)
#      undef SELECT_MIN_BITS
#      define SELECT_MIN_BITS __FD_SETSIZE
#   endif
    /* If SELECT_MIN_BITS is greater than one we most probably will want
     * to align the sizes with SELECT_MIN_BITS/8 because for example
     * in many little-endian (Intel, Alpha) systems (Linux, OS/2, Digital
     * UNIX, Solaris, NeXT, Darwin) the smallest quantum select() operates
     * on (sets/tests/clears bits) is 32 bits.  */
    growsize = maxlen + (SELECT_MIN_BITS/8 - (maxlen % (SELECT_MIN_BITS/8)));
#  endif

    sv = SP[4];
    if (SvOK(sv)) {
	value = SvNV(sv);
	if (value < 0.0)
	    value = 0.0;
	timebuf.tv_sec = (long)value;
	value -= (NV)timebuf.tv_sec;
	timebuf.tv_usec = (long)(value * 1000000.0);
    }
    else
	tbuf = NULL;

    for (i = 1; i <= 3; i++) {
	sv = SP[i];
	if (!SvOK(sv) || SvCUR(sv) == 0) {
	    fd_sets[i] = 0;
	    continue;
	}
	assert(SvPOK(sv));
	j = SvLEN(sv);
	if (j < growsize) {
	    Sv_Grow(sv, growsize);
	}
	j = SvCUR(sv);
	s = SvPVX(sv) + j;
	while (++j <= growsize) {
	    *s++ = '\0';
	}

#if BYTEORDER != 0x1234 && BYTEORDER != 0x12345678
	s = SvPVX(sv);
	Newx(fd_sets[i], growsize, char);
	for (offset = 0; offset < growsize; offset += masksize) {
	    for (j = 0, k=ORDERBYTE; j < masksize; j++, (k >>= 4))
		fd_sets[i][j+offset] = s[(k % masksize) + offset];
	}
#else
	fd_sets[i] = SvPVX(sv);
#endif
    }

#ifdef PERL_IRIX5_SELECT_TIMEVAL_VOID_CAST
    /* Can't make just the (void*) conditional because that would be
     * cpp #if within cpp macro, and not all compilers like that. */
    nfound = PerlSock_select(
	maxlen * 8,
	(Select_fd_set_t) fd_sets[1],
	(Select_fd_set_t) fd_sets[2],
	(Select_fd_set_t) fd_sets[3],
	(void*) tbuf); /* Workaround for compiler bug. */
#else
    nfound = PerlSock_select(
	maxlen * 8,
	(Select_fd_set_t) fd_sets[1],
	(Select_fd_set_t) fd_sets[2],
	(Select_fd_set_t) fd_sets[3],
	tbuf);
#endif
    for (i = 1; i <= 3; i++) {
	if (fd_sets[i]) {
	    sv = SP[i];
#if BYTEORDER != 0x1234 && BYTEORDER != 0x12345678
	    s = SvPVX(sv);
	    for (offset = 0; offset < growsize; offset += masksize) {
		for (j = 0, k=ORDERBYTE; j < masksize; j++, (k >>= 4))
		    s[(k % masksize) + offset] = fd_sets[i][j+offset];
	    }
	    Safefree(fd_sets[i]);
#endif
	    SvSETMAGIC(sv);
	}
    }

    PUSHi(nfound);
    if (GIMME == G_ARRAY && tbuf) {
	value = (NV)(timebuf.tv_sec) +
		(NV)(timebuf.tv_usec) / 1000000.0;
	PUSHs(sv_2mortal(newSVnv(value)));
    }
    RETURN;
#else
    DIE(aTHX_ "select not implemented");
#endif
}

void
Perl_setdefout(pTHX_ GV *gv)
{
    dVAR;
    SvREFCNT_inc_simple_void(gv);
    if (PL_defoutgv)
	SvREFCNT_dec(PL_defoutgv);
    PL_defoutgv = gv;
}

PP(pp_select)
{
    dVAR; dSP; dTARGET;
    HV *hv;
    GV * const newdefout = (PL_op->op_private > 0) ? ((GV *) POPs) : NULL;
    GV * egv = GvEGV(PL_defoutgv);

    if (!egv)
	egv = PL_defoutgv;
    hv = GvSTASH(egv);
    if (! hv)
	XPUSHs(&PL_sv_undef);
    else {
	GV * const * const gvp = (GV**)hv_fetch(hv, GvNAME(egv), GvNAMELEN(egv), FALSE);
	if (gvp && *gvp == egv) {
	    gv_efullname4(TARG, PL_defoutgv, NULL, TRUE);
	    XPUSHTARG;
	}
	else {
	    XPUSHs(sv_2mortal(newRV((SV*)egv)));
	}
    }

    if (newdefout) {
	if (!GvIO(newdefout))
	    gv_IOadd(newdefout);
	setdefout(newdefout);
    }

    RETURN;
}

PP(pp_getc)
{
    dVAR; dSP; dTARGET;
    IO *io = NULL;
    GV * const gv = (MAXARG==0) ? PL_stdingv : (GV*)POPs;

    if (gv && (io = GvIO(gv))) {
	MAGIC * const mg = SvTIED_mg((SV*)io, PERL_MAGIC_tiedscalar);
	if (mg) {
	    const I32 gimme = GIMME_V;
	    PUSHMARK(SP);
	    XPUSHs(SvTIED_obj((SV*)io, mg));
	    PUTBACK;
	    ENTER;
	    call_method("GETC", gimme);
	    LEAVE;
	    SPAGAIN;
	    if (gimme == G_SCALAR)
		SvSetMagicSV_nosteal(TARG, TOPs);
	    RETURN;
	}
    }
    if (!gv || do_eof(gv)) { /* make sure we have fp with something */
	if ((!io || (!IoIFP(io) && IoTYPE(io) != IoTYPE_WRONLY))
	  && ckWARN2(WARN_UNOPENED,WARN_CLOSED))
	    report_evil_fh(gv, io, PL_op->op_type);
	SETERRNO(EBADF,RMS_IFI);
	RETPUSHUNDEF;
    }
    TAINT;
    sv_setpvn(TARG, " ", 1);
    *SvPVX(TARG) = PerlIO_getc(IoIFP(GvIOp(gv))); /* should never be EOF */
    if (PerlIO_isutf8(IoIFP(GvIOp(gv)))) {
	/* Find out how many bytes the char needs */
	Size_t len = UTF8SKIP(SvPVX_const(TARG));
	if (len > 1) {
	    SvGROW(TARG,len+1);
	    len = PerlIO_read(IoIFP(GvIOp(gv)),SvPVX(TARG)+1,len-1);
	    SvCUR_set(TARG,1+len);
	}
	SvUTF8_on(TARG);
    }
    PUSHTARG;
    RETURN;
}

STATIC OP *
S_doform(pTHX_ CV *cv, GV *gv, OP *retop)
{
    dVAR;
    register PERL_CONTEXT *cx;
    const I32 gimme = GIMME_V;

    ENTER;
    SAVETMPS;

    PUSHBLOCK(cx, CXt_FORMAT, PL_stack_sp);
    PUSHFORMAT(cx);
    cx->blk_sub.retop = retop;
    SAVECOMPPAD();
    PAD_SET_CUR_NOSAVE(CvPADLIST(cv), 1);

    setdefout(gv);	    /* locally select filehandle so $% et al work */
    return CvSTART(cv);
}

PP(pp_enterwrite)
{
    dVAR;
    dSP;
    register GV *gv;
    register IO *io;
    GV *fgv;
    CV *cv;
    SV * tmpsv = NULL;

    if (MAXARG == 0)
	gv = PL_defoutgv;
    else {
	gv = (GV*)POPs;
	if (!gv)
	    gv = PL_defoutgv;
    }
    EXTEND(SP, 1);
    io = GvIO(gv);
    if (!io) {
	RETPUSHNO;
    }
    if (IoFMT_GV(io))
	fgv = IoFMT_GV(io);
    else
	fgv = gv;

    if (!fgv)
	goto not_a_format_reference;

    cv = GvFORM(fgv);
    if (!cv) {
	const char *name;
	tmpsv = sv_newmortal();
	gv_efullname4(tmpsv, fgv, NULL, FALSE);
	name = SvPV_nolen_const(tmpsv);
	if (name && *name)
	    DIE(aTHX_ "Undefined format \"%s\" called", name);

	not_a_format_reference:
	DIE(aTHX_ "Not a format reference");
    }
    if (CvCLONE(cv))
	cv = (CV*)sv_2mortal((SV*)cv_clone(cv));

    IoFLAGS(io) &= ~IOf_DIDTOP;
    return doform(cv,gv,PL_op->op_next);
}

PP(pp_leavewrite)
{
    dVAR; dSP;
    GV * const gv = cxstack[cxstack_ix].blk_sub.gv;
    register IO * const io = GvIOp(gv);
    PerlIO *ofp;
    PerlIO *fp;
    SV **newsp;
    I32 gimme;
    register PERL_CONTEXT *cx;

    if (!io || !(ofp = IoOFP(io)))
        goto forget_top;

    DEBUG_f(PerlIO_printf(Perl_debug_log, "left=%ld, todo=%ld\n",
	  (long)IoLINES_LEFT(io), (long)FmLINES(PL_formtarget)));

    if (IoLINES_LEFT(io) < FmLINES(PL_formtarget) &&
	PL_formtarget != PL_toptarget)
    {
	GV *fgv;
	CV *cv;
	if (!IoTOP_GV(io)) {
	    GV *topgv;

	    if (!IoTOP_NAME(io)) {
		SV *topname;
		if (!IoFMT_NAME(io))
		    IoFMT_NAME(io) = savepv(GvNAME(gv));
		topname = sv_2mortal(Perl_newSVpvf(aTHX_ "%s_TOP", GvNAME(gv)));
		topgv = gv_fetchsv(topname, 0, SVt_PVFM);
		if ((topgv && GvFORM(topgv)) ||
		  !gv_fetchpvs("top", GV_NOTQUAL, SVt_PVFM))
		    IoTOP_NAME(io) = savesvpv(topname);
		else
		    IoTOP_NAME(io) = savepvs("top");
	    }
	    topgv = gv_fetchpv(IoTOP_NAME(io), 0, SVt_PVFM);
	    if (!topgv || !GvFORM(topgv)) {
		IoLINES_LEFT(io) = IoPAGE_LEN(io);
		goto forget_top;
	    }
	    IoTOP_GV(io) = topgv;
	}
	if (IoFLAGS(io) & IOf_DIDTOP) {	/* Oh dear.  It still doesn't fit. */
	    I32 lines = IoLINES_LEFT(io);
	    const char *s = SvPVX_const(PL_formtarget);
	    if (lines <= 0)		/* Yow, header didn't even fit!!! */
		goto forget_top;
	    while (lines-- > 0) {
		s = strchr(s, '\n');
		if (!s)
		    break;
		s++;
	    }
	    if (s) {
		const STRLEN save = SvCUR(PL_formtarget);
		SvCUR_set(PL_formtarget, s - SvPVX_const(PL_formtarget));
		do_print(PL_formtarget, ofp);
		SvCUR_set(PL_formtarget, save);
		sv_chop(PL_formtarget, s);
		FmLINES(PL_formtarget) -= IoLINES_LEFT(io);
	    }
	}
	if (IoLINES_LEFT(io) >= 0 && IoPAGE(io) > 0)
	    do_print(PL_formfeed, ofp);
	IoLINES_LEFT(io) = IoPAGE_LEN(io);
	IoPAGE(io)++;
	PL_formtarget = PL_toptarget;
	IoFLAGS(io) |= IOf_DIDTOP;
	fgv = IoTOP_GV(io);
	if (!fgv)
	    DIE(aTHX_ "bad top format reference");
	cv = GvFORM(fgv);
	if (!cv) {
	    SV * const sv = sv_newmortal();
	    const char *name;
	    gv_efullname4(sv, fgv, NULL, FALSE);
	    name = SvPV_nolen_const(sv);
	    if (name && *name)
		DIE(aTHX_ "Undefined top format \"%s\" called", name);
	    else
		DIE(aTHX_ "Undefined top format called");
	}
	if (cv && CvCLONE(cv))
	    cv = (CV*)sv_2mortal((SV*)cv_clone(cv));
	return doform(cv, gv, PL_op);
    }

  forget_top:
    POPBLOCK(cx,PL_curpm);
    POPFORMAT(cx);
    LEAVE;

    fp = IoOFP(io);
    if (!fp) {
	if (ckWARN2(WARN_CLOSED,WARN_IO)) {
	    if (IoIFP(io))
		report_evil_fh(gv, io, OP_phoney_INPUT_ONLY);
	    else if (ckWARN(WARN_CLOSED))
		report_evil_fh(gv, io, PL_op->op_type);
	}
	PUSHs(&PL_sv_no);
    }
    else {
	if ((IoLINES_LEFT(io) -= FmLINES(PL_formtarget)) < 0) {
	    if (ckWARN(WARN_IO))
		Perl_warner(aTHX_ packWARN(WARN_IO), "page overflow");
	}
	if (!do_print(PL_formtarget, fp))
	    PUSHs(&PL_sv_no);
	else {
	    FmLINES(PL_formtarget) = 0;
	    SvCUR_set(PL_formtarget, 0);
	    *SvEND(PL_formtarget) = '\0';
	    if (IoFLAGS(io) & IOf_FLUSH)
		(void)PerlIO_flush(fp);
	    PUSHs(&PL_sv_yes);
	}
    }
    /* bad_ofp: */
    PL_formtarget = PL_bodytarget;
    PUTBACK;
    PERL_UNUSED_VAR(newsp);
    PERL_UNUSED_VAR(gimme);
    return cx->blk_sub.retop;
}

PP(pp_prtf)
{
    dVAR; dSP; dMARK; dORIGMARK;
    IO *io;
    PerlIO *fp;
    SV *sv;

    GV * const gv = (PL_op->op_flags & OPf_STACKED) ? (GV*)*++MARK : PL_defoutgv;

    if (gv && (io = GvIO(gv))) {
	MAGIC * const mg = SvTIED_mg((SV*)io, PERL_MAGIC_tiedscalar);
	if (mg) {
	    if (MARK == ORIGMARK) {
		MEXTEND(SP, 1);
		++MARK;
		Move(MARK, MARK + 1, (SP - MARK) + 1, SV*);
		++SP;
	    }
	    PUSHMARK(MARK - 1);
	    *MARK = SvTIED_obj((SV*)io, mg);
	    PUTBACK;
	    ENTER;
	    call_method("PRINTF", G_SCALAR);
	    LEAVE;
	    SPAGAIN;
	    MARK = ORIGMARK + 1;
	    *MARK = *SP;
	    SP = MARK;
	    RETURN;
	}
    }

    sv = newSV(0);
    if (!(io = GvIO(gv))) {
	if (ckWARN2(WARN_UNOPENED,WARN_CLOSED))
	    report_evil_fh(gv, io, PL_op->op_type);
	SETERRNO(EBADF,RMS_IFI);
	goto just_say_no;
    }
    else if (!(fp = IoOFP(io))) {
	if (ckWARN2(WARN_CLOSED,WARN_IO))  {
	    if (IoIFP(io))
		report_evil_fh(gv, io, OP_phoney_INPUT_ONLY);
	    else if (ckWARN(WARN_CLOSED))
		report_evil_fh(gv, io, PL_op->op_type);
	}
	SETERRNO(EBADF,IoIFP(io)?RMS_FAC:RMS_IFI);
	goto just_say_no;
    }
    else {
	if (SvTAINTED(MARK[1]))
	    TAINT_PROPER("printf");
	do_sprintf(sv, SP - MARK, MARK + 1);
	if (!do_print(sv, fp))
	    goto just_say_no;

	if (IoFLAGS(io) & IOf_FLUSH)
	    if (PerlIO_flush(fp) == EOF)
		goto just_say_no;
    }
    SvREFCNT_dec(sv);
    SP = ORIGMARK;
    PUSHs(&PL_sv_yes);
    RETURN;

  just_say_no:
    SvREFCNT_dec(sv);
    SP = ORIGMARK;
    PUSHs(&PL_sv_undef);
    RETURN;
}

PP(pp_sysopen)
{
    dVAR;
    dSP;
    const int perm = (MAXARG > 3) ? POPi : 0666;
    const int mode = POPi;
    SV * const sv = POPs;
    GV * const gv = (GV *)POPs;
    STRLEN len;

    /* Need TIEHANDLE method ? */
    const char * const tmps = SvPV_const(sv, len);
    /* FIXME? do_open should do const  */
    if (do_open(gv, tmps, len, TRUE, mode, perm, NULL)) {
	IoLINES(GvIOp(gv)) = 0;
	PUSHs(&PL_sv_yes);
    }
    else {
	PUSHs(&PL_sv_undef);
    }
    RETURN;
}

PP(pp_sysread)
{
    dVAR; dSP; dMARK; dORIGMARK; dTARGET;
    int offset;
    IO *io;
    char *buffer;
    SSize_t length;
    SSize_t count;
    Sock_size_t bufsize;
    SV *bufsv;
    STRLEN blen;
    int fp_utf8;
    int buffer_utf8;
    SV *read_target;
    Size_t got = 0;
    Size_t wanted;
    bool charstart = FALSE;
    STRLEN charskip = 0;
    STRLEN skip = 0;

    GV * const gv = (GV*)*++MARK;
    if ((PL_op->op_type == OP_READ || PL_op->op_type == OP_SYSREAD)
	&& gv && (io = GvIO(gv)) )
    {
	const MAGIC * mg = SvTIED_mg((SV*)io, PERL_MAGIC_tiedscalar);
	if (mg) {
	    SV *sv;
	    PUSHMARK(MARK-1);
	    *MARK = SvTIED_obj((SV*)io, mg);
	    ENTER;
	    call_method("READ", G_SCALAR);
	    LEAVE;
	    SPAGAIN;
	    sv = POPs;
	    SP = ORIGMARK;
	    PUSHs(sv);
	    RETURN;
	}
    }

    if (!gv)
	goto say_undef;
    bufsv = *++MARK;
    if (! SvOK(bufsv))
	sv_setpvn(bufsv, "", 0);
    length = SvIVx(*++MARK);
    SETERRNO(0,0);
    if (MARK < SP)
	offset = SvIVx(*++MARK);
    else
	offset = 0;
    io = GvIO(gv);
    if (!io || !IoIFP(io)) {
	if (ckWARN2(WARN_UNOPENED,WARN_CLOSED))
	    report_evil_fh(gv, io, PL_op->op_type);
	SETERRNO(EBADF,RMS_IFI);
	goto say_undef;
    }
    if ((fp_utf8 = PerlIO_isutf8(IoIFP(io))) && !IN_BYTES) {
	buffer = SvPVutf8_force(bufsv, blen);
	/* UTF-8 may not have been set if they are all low bytes */
	SvUTF8_on(bufsv);
	buffer_utf8 = 0;
    }
    else {
	buffer = SvPV_force(bufsv, blen);
	buffer_utf8 = !IN_BYTES && SvUTF8(bufsv);
    }
    if (length < 0)
	DIE(aTHX_ "Negative length");
    wanted = length;

    charstart = TRUE;
    charskip  = 0;
    skip = 0;

#ifdef HAS_SOCKET
    if (PL_op->op_type == OP_RECV) {
	char namebuf[MAXPATHLEN];
#if (defined(VMS_DO_SOCKETS) && defined(DECCRTL_SOCKETS)) || defined(MPE) || defined(__QNXNTO__)
	bufsize = sizeof (struct sockaddr_in);
#else
	bufsize = sizeof namebuf;
#endif
#ifdef OS2	/* At least Warp3+IAK: only the first byte of bufsize set */
	if (bufsize >= 256)
	    bufsize = 255;
#endif
	buffer = SvGROW(bufsv, (STRLEN)(length+1));
	/* 'offset' means 'flags' here */
	count = PerlSock_recvfrom(PerlIO_fileno(IoIFP(io)), buffer, length, offset,
				  (struct sockaddr *)namebuf, &bufsize);
	if (count < 0)
	    RETPUSHUNDEF;
#ifdef EPOC
        /* Bogus return without padding */
	bufsize = sizeof (struct sockaddr_in);
#endif
	SvCUR_set(bufsv, count);
	*SvEND(bufsv) = '\0';
	(void)SvPOK_only(bufsv);
	if (fp_utf8)
	    SvUTF8_on(bufsv);
	SvSETMAGIC(bufsv);
	/* This should not be marked tainted if the fp is marked clean */
	if (!(IoFLAGS(io) & IOf_UNTAINT))
	    SvTAINTED_on(bufsv);
	SP = ORIGMARK;
	sv_setpvn(TARG, namebuf, bufsize);
	PUSHs(TARG);
	RETURN;
    }
#else
    if (PL_op->op_type == OP_RECV)
	DIE(aTHX_ PL_no_sock_func, "recv");
#endif
    if (DO_UTF8(bufsv)) {
	/* offset adjust in characters not bytes */
	blen = sv_len_utf8(bufsv);
    }
    if (offset < 0) {
	if (-offset > (int)blen)
	    DIE(aTHX_ "Offset outside string");
	offset += blen;
    }
    if (DO_UTF8(bufsv)) {
	/* convert offset-as-chars to offset-as-bytes */
	if (offset >= (int)blen)
	    offset += SvCUR(bufsv) - blen;
	else
	    offset = utf8_hop((U8 *)buffer,offset) - (U8 *) buffer;
    }
 more_bytes:
    bufsize = SvCUR(bufsv);
    /* Allocating length + offset + 1 isn't perfect in the case of reading
       bytes from a byte file handle into a UTF8 buffer, but it won't harm us
       unduly.
       (should be 2 * length + offset + 1, or possibly something longer if
       PL_encoding is true) */
    buffer  = SvGROW(bufsv, (STRLEN)(length+offset+1));
    if (offset > 0 && (Sock_size_t)offset > bufsize) { /* Zero any newly allocated space */
    	Zero(buffer+bufsize, offset-bufsize, char);
    }
    buffer = buffer + offset;
    if (!buffer_utf8) {
	read_target = bufsv;
    } else {
	/* Best to read the bytes into a new SV, upgrade that to UTF8, then
	   concatenate it to the current buffer.  */

	/* Truncate the existing buffer to the start of where we will be
	   reading to:  */
	SvCUR_set(bufsv, offset);

	read_target = sv_newmortal();
	SvUPGRADE(read_target, SVt_PV);
	buffer = SvGROW(read_target, (STRLEN)(length + 1));
    }

    if (PL_op->op_type == OP_SYSREAD) {
#ifdef PERL_SOCK_SYSREAD_IS_RECV
	if (IoTYPE(io) == IoTYPE_SOCKET) {
	    count = PerlSock_recv(PerlIO_fileno(IoIFP(io)),
				   buffer, length, 0);
	}
	else
#endif
	{
	    count = PerlLIO_read(PerlIO_fileno(IoIFP(io)),
				  buffer, length);
	}
    }
    else
#ifdef HAS_SOCKET__bad_code_maybe
    if (IoTYPE(io) == IoTYPE_SOCKET) {
	char namebuf[MAXPATHLEN];
#if defined(VMS_DO_SOCKETS) && defined(DECCRTL_SOCKETS)
	bufsize = sizeof (struct sockaddr_in);
#else
	bufsize = sizeof namebuf;
#endif
	count = PerlSock_recvfrom(PerlIO_fileno(IoIFP(io)), buffer, length, 0,
			  (struct sockaddr *)namebuf, &bufsize);
    }
    else
#endif
    {
	count = PerlIO_read(IoIFP(io), buffer, length);
	/* PerlIO_read() - like fread() returns 0 on both error and EOF */
	if (count == 0 && PerlIO_error(IoIFP(io)))
	    count = -1;
    }
    if (count < 0) {
	if ((IoTYPE(io) == IoTYPE_WRONLY) && ckWARN(WARN_IO))
		report_evil_fh(gv, io, OP_phoney_OUTPUT_ONLY);
	goto say_undef;
    }
    SvCUR_set(read_target, count+(buffer - SvPVX_const(read_target)));
    *SvEND(read_target) = '\0';
    (void)SvPOK_only(read_target);
    if (fp_utf8 && !IN_BYTES) {
	/* Look at utf8 we got back and count the characters */
	const char *bend = buffer + count;
	while (buffer < bend) {
	    if (charstart) {
	        skip = UTF8SKIP(buffer);
		charskip = 0;
	    }
	    if (buffer - charskip + skip > bend) {
		/* partial character - try for rest of it */
		length = skip - (bend-buffer);
		offset = bend - SvPVX_const(bufsv);
		charstart = FALSE;
		charskip += count;
		goto more_bytes;
	    }
	    else {
		got++;
		buffer += skip;
		charstart = TRUE;
		charskip  = 0;
	    }
        }
	/* If we have not 'got' the number of _characters_ we 'wanted' get some more
	   provided amount read (count) was what was requested (length)
	 */
	if (got < wanted && count == length) {
	    length = wanted - got;
	    offset = bend - SvPVX_const(bufsv);
	    goto more_bytes;
	}
	/* return value is character count */
	count = got;
	SvUTF8_on(bufsv);
    }
    else if (buffer_utf8) {
	/* Let svcatsv upgrade the bytes we read in to utf8.
	   The buffer is a mortal so will be freed soon.  */
	sv_catsv_nomg(bufsv, read_target);
    }
    SvSETMAGIC(bufsv);
    /* This should not be marked tainted if the fp is marked clean */
    if (!(IoFLAGS(io) & IOf_UNTAINT))
	SvTAINTED_on(bufsv);
    SP = ORIGMARK;
    PUSHi(count);
    RETURN;

  say_undef:
    SP = ORIGMARK;
    RETPUSHUNDEF;
}

PP(pp_send)
{
    dVAR; dSP; dMARK; dORIGMARK; dTARGET;
    IO *io;
    SV *bufsv;
    const char *buffer;
    SSize_t retval;
    STRLEN blen;
    STRLEN orig_blen_bytes;
    const int op_type = PL_op->op_type;
    bool doing_utf8;
    U8 *tmpbuf = NULL;
    
    GV *const gv = (GV*)*++MARK;
    if (PL_op->op_type == OP_SYSWRITE
	&& gv && (io = GvIO(gv))) {
	MAGIC * const mg = SvTIED_mg((SV*)io, PERL_MAGIC_tiedscalar);
	if (mg) {
	    SV *sv;

	    if (MARK == SP - 1) {
		EXTEND(SP, 1000);
		sv = sv_2mortal(newSViv(sv_len(*SP)));
		PUSHs(sv);
		PUTBACK;
	    }

	    PUSHMARK(ORIGMARK);
	    *(ORIGMARK+1) = SvTIED_obj((SV*)io, mg);
	    ENTER;
	    call_method("WRITE", G_SCALAR);
	    LEAVE;
	    SPAGAIN;
	    sv = POPs;
	    SP = ORIGMARK;
	    PUSHs(sv);
	    RETURN;
	}
    }
    if (!gv)
	goto say_undef;

    bufsv = *++MARK;

    SETERRNO(0,0);
    io = GvIO(gv);
    if (!io || !IoIFP(io) || IoTYPE(io) == IoTYPE_RDONLY) {
	retval = -1;
	if (ckWARN2(WARN_UNOPENED,WARN_CLOSED)) {
	    if (io && IoIFP(io))
		report_evil_fh(gv, io, OP_phoney_INPUT_ONLY);
	    else
		report_evil_fh(gv, io, PL_op->op_type);
	}
	SETERRNO(EBADF,RMS_IFI);
	goto say_undef;
    }

    /* Do this first to trigger any overloading.  */
    buffer = SvPV_const(bufsv, blen);
    orig_blen_bytes = blen;
    doing_utf8 = DO_UTF8(bufsv);

    if (PerlIO_isutf8(IoIFP(io))) {
	if (!SvUTF8(bufsv)) {
	    /* We don't modify the original scalar.  */
	    tmpbuf = bytes_to_utf8((const U8*) buffer, &blen);
	    buffer = (char *) tmpbuf;
	    doing_utf8 = TRUE;
	}
    }
    else if (doing_utf8) {
	STRLEN tmplen = blen;
	U8 * const result = bytes_from_utf8((const U8*) buffer, &tmplen, &doing_utf8);
	if (!doing_utf8) {
	    tmpbuf = result;
	    buffer = (char *) tmpbuf;
	    blen = tmplen;
	}
	else {
	    assert((char *)result == buffer);
	    Perl_croak(aTHX_ "Wide character in %s", OP_DESC(PL_op));
	}
    }

    if (op_type == OP_SYSWRITE) {
	Size_t length = 0; /* This length is in characters.  */
	STRLEN blen_chars;
	IV offset;

	if (doing_utf8) {
	    if (tmpbuf) {
		/* The SV is bytes, and we've had to upgrade it.  */
		blen_chars = orig_blen_bytes;
	    } else {
		/* The SV really is UTF-8.  */
		if (SvGMAGICAL(bufsv) || SvAMAGIC(bufsv)) {
		    /* Don't call sv_len_utf8 again because it will call magic
		       or overloading a second time, and we might get back a
		       different result.  */
		    blen_chars = utf8_length((U8*)buffer, (U8*)buffer + blen);
		} else {
		    /* It's safe, and it may well be cached.  */
		    blen_chars = sv_len_utf8(bufsv);
		}
	    }
	} else {
	    blen_chars = blen;
	}

	if (MARK >= SP) {
	    length = blen_chars;
	} else {
#if Size_t_size > IVSIZE
	    length = (Size_t)SvNVx(*++MARK);
#else
	    length = (Size_t)SvIVx(*++MARK);
#endif
	    if ((SSize_t)length < 0) {
		Safefree(tmpbuf);
		DIE(aTHX_ "Negative length");
	    }
	}

	if (MARK < SP) {
	    offset = SvIVx(*++MARK);
	    if (offset < 0) {
		if (-offset > (IV)blen_chars) {
		    Safefree(tmpbuf);
		    DIE(aTHX_ "Offset outside string");
		}
		offset += blen_chars;
	    } else if (offset >= (IV)blen_chars && blen_chars > 0) {
		Safefree(tmpbuf);
		DIE(aTHX_ "Offset outside string");
	    }
	} else
	    offset = 0;
	if (length > blen_chars - offset)
	    length = blen_chars - offset;
	if (doing_utf8) {
	    /* Here we convert length from characters to bytes.  */
	    if (tmpbuf || SvGMAGICAL(bufsv) || SvAMAGIC(bufsv)) {
		/* Either we had to convert the SV, or the SV is magical, or
		   the SV has overloading, in which case we can't or mustn't
		   or mustn't call it again.  */

		buffer = (const char*)utf8_hop((const U8 *)buffer, offset);
		length = utf8_hop((U8 *)buffer, length) - (U8 *)buffer;
	    } else {
		/* It's a real UTF-8 SV, and it's not going to change under
		   us.  Take advantage of any cache.  */
		I32 start = offset;
		I32 len_I32 = length;

		/* Convert the start and end character positions to bytes.
		   Remember that the second argument to sv_pos_u2b is relative
		   to the first.  */
		sv_pos_u2b(bufsv, &start, &len_I32);

		buffer += start;
		length = len_I32;
	    }
	}
	else {
	    buffer = buffer+offset;
	}
#ifdef PERL_SOCK_SYSWRITE_IS_SEND
	if (IoTYPE(io) == IoTYPE_SOCKET) {
	    retval = PerlSock_send(PerlIO_fileno(IoIFP(io)),
				   buffer, length, 0);
	}
	else
#endif
	{
	    /* See the note at doio.c:do_print about filesize limits. --jhi */
	    retval = PerlLIO_write(PerlIO_fileno(IoIFP(io)),
				   buffer, length);
	}
    }
#ifdef HAS_SOCKET
    else {
	const int flags = SvIVx(*++MARK);
	if (SP > MARK) {
	    STRLEN mlen;
	    char * const sockbuf = SvPVx(*++MARK, mlen);
	    retval = PerlSock_sendto(PerlIO_fileno(IoIFP(io)), buffer, blen,
				     flags, (struct sockaddr *)sockbuf, mlen);
	}
	else {
	    retval
		= PerlSock_send(PerlIO_fileno(IoIFP(io)), buffer, blen, flags);
	}
    }
#else
    else
	DIE(aTHX_ PL_no_sock_func, "send");
#endif

    if (retval < 0)
	goto say_undef;
    SP = ORIGMARK;
    if (doing_utf8)
        retval = utf8_length((U8*)buffer, (U8*)buffer + retval);

    Safefree(tmpbuf);
#if Size_t_size > IVSIZE
    PUSHn(retval);
#else
    PUSHi(retval);
#endif
    RETURN;

  say_undef:
    Safefree(tmpbuf);
    SP = ORIGMARK;
    RETPUSHUNDEF;
}

PP(pp_eof)
{
    dVAR; dSP;
    GV *gv;

    if (MAXARG == 0) {
	if (PL_op->op_flags & OPf_SPECIAL) {	/* eof() */
	    IO *io;
	    gv = PL_last_in_gv = GvEGV(PL_argvgv);
	    io = GvIO(gv);
	    if (io && !IoIFP(io)) {
		if ((IoFLAGS(io) & IOf_START) && av_len(GvAVn(gv)) < 0) {
		    IoLINES(io) = 0;
		    IoFLAGS(io) &= ~IOf_START;
		    do_open(gv, "-", 1, FALSE, O_RDONLY, 0, NULL);
		    if ( GvSV(gv) ) {
			sv_setpvn(GvSV(gv), "-", 1);
		    }
		    else {
			GvSV(gv) = newSVpvn("-", 1);
		    }
		    SvSETMAGIC(GvSV(gv));
		}
		else if (!nextargv(gv))
		    RETPUSHYES;
	    }
	}
	else
	    gv = PL_last_in_gv;			/* eof */
    }
    else
	gv = PL_last_in_gv = (GV*)POPs;		/* eof(FH) */

    if (gv) {
	IO * const io = GvIO(gv);
	MAGIC * mg;
	if (io && (mg = SvTIED_mg((SV*)io, PERL_MAGIC_tiedscalar))) {
	    PUSHMARK(SP);
	    XPUSHs(SvTIED_obj((SV*)io, mg));
	    PUTBACK;
	    ENTER;
	    call_method("EOF", G_SCALAR);
	    LEAVE;
	    SPAGAIN;
	    RETURN;
	}
    }

    PUSHs(boolSV(!gv || do_eof(gv)));
    RETURN;
}

PP(pp_tell)
{
    dVAR; dSP; dTARGET;
    GV *gv;
    IO *io;

    if (MAXARG != 0)
	PL_last_in_gv = (GV*)POPs;
    gv = PL_last_in_gv;

    if (gv && (io = GvIO(gv))) {
	MAGIC * const mg = SvTIED_mg((SV*)io, PERL_MAGIC_tiedscalar);
	if (mg) {
	    PUSHMARK(SP);
	    XPUSHs(SvTIED_obj((SV*)io, mg));
	    PUTBACK;
	    ENTER;
	    call_method("TELL", G_SCALAR);
	    LEAVE;
	    SPAGAIN;
	    RETURN;
	}
    }

#if LSEEKSIZE > IVSIZE
    PUSHn( do_tell(gv) );
#else
    PUSHi( do_tell(gv) );
#endif
    RETURN;
}

PP(pp_sysseek)
{
    dVAR; dSP;
    const int whence = POPi;
#if LSEEKSIZE > IVSIZE
    const Off_t offset = (Off_t)SvNVx(POPs);
#else
    const Off_t offset = (Off_t)SvIVx(POPs);
#endif

    GV * const gv = PL_last_in_gv = (GV*)POPs;
    IO *io;

    if (gv && (io = GvIO(gv))) {
	MAGIC * const mg = SvTIED_mg((SV*)io, PERL_MAGIC_tiedscalar);
	if (mg) {
	    PUSHMARK(SP);
	    XPUSHs(SvTIED_obj((SV*)io, mg));
#if LSEEKSIZE > IVSIZE
	    XPUSHs(sv_2mortal(newSVnv((NV) offset)));
#else
	    XPUSHs(sv_2mortal(newSViv(offset)));
#endif
	    XPUSHs(sv_2mortal(newSViv(whence)));
	    PUTBACK;
	    ENTER;
	    call_method("SEEK", G_SCALAR);
	    LEAVE;
	    SPAGAIN;
	    RETURN;
	}
    }

    if (PL_op->op_type == OP_SEEK)
	PUSHs(boolSV(do_seek(gv, offset, whence)));
    else {
	const Off_t sought = do_sysseek(gv, offset, whence);
        if (sought < 0)
            PUSHs(&PL_sv_undef);
        else {
            SV* const sv = sought ?
#if LSEEKSIZE > IVSIZE
                newSVnv((NV)sought)
#else
                newSViv(sought)
#endif
                : newSVpvn(zero_but_true, ZBTLEN);
            PUSHs(sv_2mortal(sv));
        }
    }
    RETURN;
}

PP(pp_truncate)
{
    dVAR;
    dSP;
    /* There seems to be no consensus on the length type of truncate()
     * and ftruncate(), both off_t and size_t have supporters. In
     * general one would think that when using large files, off_t is
     * at least as wide as size_t, so using an off_t should be okay. */
    /* XXX Configure probe for the length type of *truncate() needed XXX */
    Off_t len;

#if Off_t_size > IVSIZE
    len = (Off_t)POPn;
#else
    len = (Off_t)POPi;
#endif
    /* Checking for length < 0 is problematic as the type might or
     * might not be signed: if it is not, clever compilers will moan. */
    /* XXX Configure probe for the signedness of the length type of *truncate() needed? XXX */
    SETERRNO(0,0);
    {
	int result = 1;
	GV *tmpgv;
	IO *io;

	if (PL_op->op_flags & OPf_SPECIAL) {
	    tmpgv = gv_fetchsv(POPs, 0, SVt_PVIO);

	do_ftruncate_gv:
	    if (!GvIO(tmpgv))
		result = 0;
	    else {
		PerlIO *fp;
		io = GvIOp(tmpgv);
	    do_ftruncate_io:
		TAINT_PROPER("truncate");
		if (!(fp = IoIFP(io))) {
		    result = 0;
		}
		else {
		    PerlIO_flush(fp);
#ifdef HAS_TRUNCATE
		    if (ftruncate(PerlIO_fileno(fp), len) < 0)
#else
		    if (my_chsize(PerlIO_fileno(fp), len) < 0)
#endif
			result = 0;
		}
	    }
	}
	else {
	    SV * const sv = POPs;
	    const char *name;

	    if (SvTYPE(sv) == SVt_PVGV) {
	        tmpgv = (GV*)sv;		/* *main::FRED for example */
		goto do_ftruncate_gv;
	    }
	    else if (SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVGV) {
	        tmpgv = (GV*) SvRV(sv);	/* \*main::FRED for example */
		goto do_ftruncate_gv;
	    }
	    else if (SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVIO) {
		io = (IO*) SvRV(sv); /* *main::FRED{IO} for example */
		goto do_ftruncate_io;
	    }

	    name = SvPV_nolen_const(sv);
	    TAINT_PROPER("truncate");
#ifdef HAS_TRUNCATE
	    if (truncate(name, len) < 0)
	        result = 0;
#else
	    {
		const int tmpfd = PerlLIO_open(name, O_RDWR);

		if (tmpfd < 0)
		    result = 0;
		else {
		    if (my_chsize(tmpfd, len) < 0)
		        result = 0;
		    PerlLIO_close(tmpfd);
		}
	    }
#endif
	}

	if (result)
	    RETPUSHYES;
	if (!errno)
	    SETERRNO(EBADF,RMS_IFI);
	RETPUSHUNDEF;
    }
}

PP(pp_ioctl)
{
    dVAR; dSP; dTARGET;
    SV * const argsv = POPs;
    const unsigned int func = POPu;
    const int optype = PL_op->op_type;
    GV * const gv = (GV*)POPs;
    IO * const io = gv ? GvIOn(gv) : NULL;
    char *s;
    IV retval;

    if (!io || !argsv || !IoIFP(io)) {
	if (ckWARN2(WARN_UNOPENED,WARN_CLOSED))
	    report_evil_fh(gv, io, PL_op->op_type);
	SETERRNO(EBADF,RMS_IFI);	/* well, sort of... */
	RETPUSHUNDEF;
    }

    if (SvPOK(argsv) || !SvNIOK(argsv)) {
	STRLEN len;
	STRLEN need;
	s = SvPV_force(argsv, len);
	need = IOCPARM_LEN(func);
	if (len < need) {
	    s = Sv_Grow(argsv, need + 1);
	    SvCUR_set(argsv, need);
	}

	s[SvCUR(argsv)] = 17;	/* a little sanity check here */
    }
    else {
	retval = SvIV(argsv);
	s = INT2PTR(char*,retval);		/* ouch */
    }

    TAINT_PROPER(PL_op_desc[optype]);

    if (optype == OP_IOCTL)
#ifdef HAS_IOCTL
	retval = PerlLIO_ioctl(PerlIO_fileno(IoIFP(io)), func, s);
#else
	DIE(aTHX_ "ioctl is not implemented");
#endif
    else
#ifndef HAS_FCNTL
      DIE(aTHX_ "fcntl is not implemented");
#else
#if defined(OS2) && defined(__EMX__)
	retval = fcntl(PerlIO_fileno(IoIFP(io)), func, (int)s);
#else
	retval = fcntl(PerlIO_fileno(IoIFP(io)), func, s);
#endif
#endif

#if defined(HAS_IOCTL) || defined(HAS_FCNTL)
    if (SvPOK(argsv)) {
	if (s[SvCUR(argsv)] != 17)
	    DIE(aTHX_ "Possible memory corruption: %s overflowed 3rd argument",
		OP_NAME(PL_op));
	s[SvCUR(argsv)] = 0;		/* put our null back */
	SvSETMAGIC(argsv);		/* Assume it has changed */
    }

    if (retval == -1)
	RETPUSHUNDEF;
    if (retval != 0) {
	PUSHi(retval);
    }
    else {
	PUSHp(zero_but_true, ZBTLEN);
    }
#endif
    RETURN;
}

PP(pp_flock)
{
#ifdef FLOCK
    dVAR; dSP; dTARGET;
    I32 value;
    IO *io = NULL;
    PerlIO *fp;
    const int argtype = POPi;
    GV * const gv = (MAXARG == 0) ? PL_last_in_gv : (GV*)POPs;

    if (gv && (io = GvIO(gv)))
	fp = IoIFP(io);
    else {
	fp = NULL;
	io = NULL;
    }
    /* XXX Looks to me like io is always NULL at this point */
    if (fp) {
	(void)PerlIO_flush(fp);
	value = (I32)(PerlLIO_flock(PerlIO_fileno(fp), argtype) >= 0);
    }
    else {
	if (ckWARN2(WARN_UNOPENED,WARN_CLOSED))
	    report_evil_fh(gv, io, PL_op->op_type);
	value = 0;
	SETERRNO(EBADF,RMS_IFI);
    }
    PUSHi(value);
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "flock()");
#endif
}

/* Sockets. */

PP(pp_socket)
{
#ifdef HAS_SOCKET
    dVAR; dSP;
    const int protocol = POPi;
    const int type = POPi;
    const int domain = POPi;
    GV * const gv = (GV*)POPs;
    register IO * const io = gv ? GvIOn(gv) : NULL;
    int fd;

    if (!gv || !io) {
	if (ckWARN2(WARN_UNOPENED,WARN_CLOSED))
	    report_evil_fh(gv, io, PL_op->op_type);
	if (io && IoIFP(io))
	    do_close(gv, FALSE);
	SETERRNO(EBADF,LIB_INVARG);
	RETPUSHUNDEF;
    }

    if (IoIFP(io))
	do_close(gv, FALSE);

    TAINT_PROPER("socket");
    fd = PerlSock_socket(domain, type, protocol);
    if (fd < 0)
	RETPUSHUNDEF;
    IoIFP(io) = PerlIO_fdopen(fd, "r"SOCKET_OPEN_MODE);	/* stdio gets confused about sockets */
    IoOFP(io) = PerlIO_fdopen(fd, "w"SOCKET_OPEN_MODE);
    IoTYPE(io) = IoTYPE_SOCKET;
    if (!IoIFP(io) || !IoOFP(io)) {
	if (IoIFP(io)) PerlIO_close(IoIFP(io));
	if (IoOFP(io)) PerlIO_close(IoOFP(io));
	if (!IoIFP(io) && !IoOFP(io)) PerlLIO_close(fd);
	RETPUSHUNDEF;
    }
#if defined(HAS_FCNTL) && defined(F_SETFD)
    fcntl(fd, F_SETFD, fd > PL_maxsysfd);	/* ensure close-on-exec */
#endif

#ifdef EPOC
    setbuf( IoIFP(io), NULL); /* EPOC gets confused about sockets */
#endif

    RETPUSHYES;
#else
    DIE(aTHX_ PL_no_sock_func, "socket");
#endif
}

PP(pp_sockpair)
{
#if defined (HAS_SOCKETPAIR) || (defined (HAS_SOCKET) && defined(SOCK_DGRAM) && defined(AF_INET) && defined(PF_INET))
    dVAR; dSP;
    const int protocol = POPi;
    const int type = POPi;
    const int domain = POPi;
    GV * const gv2 = (GV*)POPs;
    GV * const gv1 = (GV*)POPs;
    register IO * const io1 = gv1 ? GvIOn(gv1) : NULL;
    register IO * const io2 = gv2 ? GvIOn(gv2) : NULL;
    int fd[2];

    if (!gv1 || !gv2 || !io1 || !io2) {
	if (ckWARN2(WARN_UNOPENED,WARN_CLOSED)) {
	    if (!gv1 || !io1)
		report_evil_fh(gv1, io1, PL_op->op_type);
	    if (!gv2 || !io2)
		report_evil_fh(gv1, io2, PL_op->op_type);
	}
	if (io1 && IoIFP(io1))
	    do_close(gv1, FALSE);
	if (io2 && IoIFP(io2))
	    do_close(gv2, FALSE);
	RETPUSHUNDEF;
    }

    if (IoIFP(io1))
	do_close(gv1, FALSE);
    if (IoIFP(io2))
	do_close(gv2, FALSE);

    TAINT_PROPER("socketpair");
    if (PerlSock_socketpair(domain, type, protocol, fd) < 0)
	RETPUSHUNDEF;
    IoIFP(io1) = PerlIO_fdopen(fd[0], "r"SOCKET_OPEN_MODE);
    IoOFP(io1) = PerlIO_fdopen(fd[0], "w"SOCKET_OPEN_MODE);
    IoTYPE(io1) = IoTYPE_SOCKET;
    IoIFP(io2) = PerlIO_fdopen(fd[1], "r"SOCKET_OPEN_MODE);
    IoOFP(io2) = PerlIO_fdopen(fd[1], "w"SOCKET_OPEN_MODE);
    IoTYPE(io2) = IoTYPE_SOCKET;
    if (!IoIFP(io1) || !IoOFP(io1) || !IoIFP(io2) || !IoOFP(io2)) {
	if (IoIFP(io1)) PerlIO_close(IoIFP(io1));
	if (IoOFP(io1)) PerlIO_close(IoOFP(io1));
	if (!IoIFP(io1) && !IoOFP(io1)) PerlLIO_close(fd[0]);
	if (IoIFP(io2)) PerlIO_close(IoIFP(io2));
	if (IoOFP(io2)) PerlIO_close(IoOFP(io2));
	if (!IoIFP(io2) && !IoOFP(io2)) PerlLIO_close(fd[1]);
	RETPUSHUNDEF;
    }
#if defined(HAS_FCNTL) && defined(F_SETFD)
    fcntl(fd[0],F_SETFD,fd[0] > PL_maxsysfd);	/* ensure close-on-exec */
    fcntl(fd[1],F_SETFD,fd[1] > PL_maxsysfd);	/* ensure close-on-exec */
#endif

    RETPUSHYES;
#else
    DIE(aTHX_ PL_no_sock_func, "socketpair");
#endif
}

PP(pp_bind)
{
#ifdef HAS_SOCKET
    dVAR; dSP;
    SV * const addrsv = POPs;
    /* OK, so on what platform does bind modify addr?  */
    const char *addr;
    GV * const gv = (GV*)POPs;
    register IO * const io = GvIOn(gv);
    STRLEN len;

    if (!io || !IoIFP(io))
	goto nuts;

    addr = SvPV_const(addrsv, len);
    TAINT_PROPER("bind");
    if (PerlSock_bind(PerlIO_fileno(IoIFP(io)), (struct sockaddr *)addr, len) >= 0)
	RETPUSHYES;
    else
	RETPUSHUNDEF;

nuts:
    if (ckWARN(WARN_CLOSED))
	report_evil_fh(gv, io, PL_op->op_type);
    SETERRNO(EBADF,SS_IVCHAN);
    RETPUSHUNDEF;
#else
    DIE(aTHX_ PL_no_sock_func, "bind");
#endif
}

PP(pp_connect)
{
#ifdef HAS_SOCKET
    dVAR; dSP;
    SV * const addrsv = POPs;
    GV * const gv = (GV*)POPs;
    register IO * const io = GvIOn(gv);
    const char *addr;
    STRLEN len;

    if (!io || !IoIFP(io))
	goto nuts;

    addr = SvPV_const(addrsv, len);
    TAINT_PROPER("connect");
    if (PerlSock_connect(PerlIO_fileno(IoIFP(io)), (struct sockaddr *)addr, len) >= 0)
	RETPUSHYES;
    else
	RETPUSHUNDEF;

nuts:
    if (ckWARN(WARN_CLOSED))
	report_evil_fh(gv, io, PL_op->op_type);
    SETERRNO(EBADF,SS_IVCHAN);
    RETPUSHUNDEF;
#else
    DIE(aTHX_ PL_no_sock_func, "connect");
#endif
}

PP(pp_listen)
{
#ifdef HAS_SOCKET
    dVAR; dSP;
    const int backlog = POPi;
    GV * const gv = (GV*)POPs;
    register IO * const io = gv ? GvIOn(gv) : NULL;

    if (!gv || !io || !IoIFP(io))
	goto nuts;

    if (PerlSock_listen(PerlIO_fileno(IoIFP(io)), backlog) >= 0)
	RETPUSHYES;
    else
	RETPUSHUNDEF;

nuts:
    if (ckWARN(WARN_CLOSED))
	report_evil_fh(gv, io, PL_op->op_type);
    SETERRNO(EBADF,SS_IVCHAN);
    RETPUSHUNDEF;
#else
    DIE(aTHX_ PL_no_sock_func, "listen");
#endif
}

PP(pp_accept)
{
#ifdef HAS_SOCKET
    dVAR; dSP; dTARGET;
    register IO *nstio;
    register IO *gstio;
    char namebuf[MAXPATHLEN];
#if (defined(VMS_DO_SOCKETS) && defined(DECCRTL_SOCKETS)) || defined(MPE) || defined(__QNXNTO__)
    Sock_size_t len = sizeof (struct sockaddr_in);
#else
    Sock_size_t len = sizeof namebuf;
#endif
    GV * const ggv = (GV*)POPs;
    GV * const ngv = (GV*)POPs;
    int fd;

    if (!ngv)
	goto badexit;
    if (!ggv)
	goto nuts;

    gstio = GvIO(ggv);
    if (!gstio || !IoIFP(gstio))
	goto nuts;

    nstio = GvIOn(ngv);
    fd = PerlSock_accept(PerlIO_fileno(IoIFP(gstio)), (struct sockaddr *) namebuf, &len);
#if defined(OEMVS)
    if (len == 0) {
	/* Some platforms indicate zero length when an AF_UNIX client is
	 * not bound. Simulate a non-zero-length sockaddr structure in
	 * this case. */
	namebuf[0] = 0;        /* sun_len */
	namebuf[1] = AF_UNIX;  /* sun_family */
	len = 2;
    }
#endif

    if (fd < 0)
	goto badexit;
    if (IoIFP(nstio))
	do_close(ngv, FALSE);
    IoIFP(nstio) = PerlIO_fdopen(fd, "r"SOCKET_OPEN_MODE);
    IoOFP(nstio) = PerlIO_fdopen(fd, "w"SOCKET_OPEN_MODE);
    IoTYPE(nstio) = IoTYPE_SOCKET;
    if (!IoIFP(nstio) || !IoOFP(nstio)) {
	if (IoIFP(nstio)) PerlIO_close(IoIFP(nstio));
	if (IoOFP(nstio)) PerlIO_close(IoOFP(nstio));
	if (!IoIFP(nstio) && !IoOFP(nstio)) PerlLIO_close(fd);
	goto badexit;
    }
#if defined(HAS_FCNTL) && defined(F_SETFD)
    fcntl(fd, F_SETFD, fd > PL_maxsysfd);	/* ensure close-on-exec */
#endif

#ifdef EPOC
    len = sizeof (struct sockaddr_in); /* EPOC somehow truncates info */
    setbuf( IoIFP(nstio), NULL); /* EPOC gets confused about sockets */
#endif
#ifdef __SCO_VERSION__
    len = sizeof (struct sockaddr_in); /* OpenUNIX 8 somehow truncates info */
#endif

    PUSHp(namebuf, len);
    RETURN;

nuts:
    if (ckWARN(WARN_CLOSED))
	report_evil_fh(ggv, ggv ? GvIO(ggv) : 0, PL_op->op_type);
    SETERRNO(EBADF,SS_IVCHAN);

badexit:
    RETPUSHUNDEF;

#else
    DIE(aTHX_ PL_no_sock_func, "accept");
#endif
}

PP(pp_shutdown)
{
#ifdef HAS_SOCKET
    dVAR; dSP; dTARGET;
    const int how = POPi;
    GV * const gv = (GV*)POPs;
    register IO * const io = GvIOn(gv);

    if (!io || !IoIFP(io))
	goto nuts;

    PUSHi( PerlSock_shutdown(PerlIO_fileno(IoIFP(io)), how) >= 0 );
    RETURN;

nuts:
    if (ckWARN(WARN_CLOSED))
	report_evil_fh(gv, io, PL_op->op_type);
    SETERRNO(EBADF,SS_IVCHAN);
    RETPUSHUNDEF;
#else
    DIE(aTHX_ PL_no_sock_func, "shutdown");
#endif
}

PP(pp_ssockopt)
{
#ifdef HAS_SOCKET
    dVAR; dSP;
    const int optype = PL_op->op_type;
    SV * const sv = (optype == OP_GSOCKOPT) ? sv_2mortal(newSV(257)) : POPs;
    const unsigned int optname = (unsigned int) POPi;
    const unsigned int lvl = (unsigned int) POPi;
    GV * const gv = (GV*)POPs;
    register IO * const io = GvIOn(gv);
    int fd;
    Sock_size_t len;

    if (!io || !IoIFP(io))
	goto nuts;

    fd = PerlIO_fileno(IoIFP(io));
    switch (optype) {
    case OP_GSOCKOPT:
	SvGROW(sv, 257);
	(void)SvPOK_only(sv);
	SvCUR_set(sv,256);
	*SvEND(sv) ='\0';
	len = SvCUR(sv);
	if (PerlSock_getsockopt(fd, lvl, optname, SvPVX(sv), &len) < 0)
	    goto nuts2;
	SvCUR_set(sv, len);
	*SvEND(sv) ='\0';
	PUSHs(sv);
	break;
    case OP_SSOCKOPT: {
#if defined(__SYMBIAN32__)
# define SETSOCKOPT_OPTION_VALUE_T void *
#else
# define SETSOCKOPT_OPTION_VALUE_T const char *
#endif
	/* XXX TODO: We need to have a proper type (a Configure probe,
	 * etc.) for what the C headers think of the third argument of
	 * setsockopt(), the option_value read-only buffer: is it
	 * a "char *", or a "void *", const or not.  Some compilers
	 * don't take kindly to e.g. assuming that "char *" implicitly
	 * promotes to a "void *", or to explicitly promoting/demoting
	 * consts to non/vice versa.  The "const void *" is the SUS
	 * definition, but that does not fly everywhere for the above
	 * reasons. */
	    SETSOCKOPT_OPTION_VALUE_T buf;
	    int aint;
	    if (SvPOKp(sv)) {
		STRLEN l;
		buf = (SETSOCKOPT_OPTION_VALUE_T) SvPV_const(sv, l);
		len = l;
	    }
	    else {
		aint = (int)SvIV(sv);
		buf = (SETSOCKOPT_OPTION_VALUE_T) &aint;
		len = sizeof(int);
	    }
	    if (PerlSock_setsockopt(fd, lvl, optname, buf, len) < 0)
		goto nuts2;
	    PUSHs(&PL_sv_yes);
	}
	break;
    }
    RETURN;

nuts:
    if (ckWARN(WARN_CLOSED))
	report_evil_fh(gv, io, optype);
    SETERRNO(EBADF,SS_IVCHAN);
nuts2:
    RETPUSHUNDEF;

#else
    DIE(aTHX_ PL_no_sock_func, PL_op_desc[PL_op->op_type]);
#endif
}

PP(pp_getpeername)
{
#ifdef HAS_SOCKET
    dVAR; dSP;
    const int optype = PL_op->op_type;
    GV * const gv = (GV*)POPs;
    register IO * const io = GvIOn(gv);
    Sock_size_t len;
    SV *sv;
    int fd;

    if (!io || !IoIFP(io))
	goto nuts;

    sv = sv_2mortal(newSV(257));
    (void)SvPOK_only(sv);
    len = 256;
    SvCUR_set(sv, len);
    *SvEND(sv) ='\0';
    fd = PerlIO_fileno(IoIFP(io));
    switch (optype) {
    case OP_GETSOCKNAME:
	if (PerlSock_getsockname(fd, (struct sockaddr *)SvPVX(sv), &len) < 0)
	    goto nuts2;
	break;
    case OP_GETPEERNAME:
	if (PerlSock_getpeername(fd, (struct sockaddr *)SvPVX(sv), &len) < 0)
	    goto nuts2;
#if defined(VMS_DO_SOCKETS) && defined (DECCRTL_SOCKETS)
	{
	    static const char nowhere[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	    /* If the call succeeded, make sure we don't have a zeroed port/addr */
	    if (((struct sockaddr *)SvPVX_const(sv))->sa_family == AF_INET &&
		!memcmp(SvPVX_const(sv) + sizeof(u_short), nowhere,
			sizeof(u_short) + sizeof(struct in_addr))) {
		goto nuts2;	
	    }
	}
#endif
	break;
    }
#ifdef BOGUS_GETNAME_RETURN
    /* Interactive Unix, getpeername() and getsockname()
      does not return valid namelen */
    if (len == BOGUS_GETNAME_RETURN)
	len = sizeof(struct sockaddr);
#endif
    SvCUR_set(sv, len);
    *SvEND(sv) ='\0';
    PUSHs(sv);
    RETURN;

nuts:
    if (ckWARN(WARN_CLOSED))
	report_evil_fh(gv, io, optype);
    SETERRNO(EBADF,SS_IVCHAN);
nuts2:
    RETPUSHUNDEF;

#else
    DIE(aTHX_ PL_no_sock_func, PL_op_desc[PL_op->op_type]);
#endif
}

/* Stat calls. */

PP(pp_stat)
{
    dVAR;
    dSP;
    GV *gv = NULL;
    IO *io;
    I32 gimme;
    I32 max = 13;

    if (PL_op->op_flags & OPf_REF) {
	gv = cGVOP_gv;
	if (PL_op->op_type == OP_LSTAT) {
	    if (gv != PL_defgv) {
	    do_fstat_warning_check:
		if (ckWARN(WARN_IO))
		    Perl_warner(aTHX_ packWARN(WARN_IO),
			"lstat() on filehandle %s", gv ? GvENAME(gv) : "");
	    } else if (PL_laststype != OP_LSTAT)
		Perl_croak(aTHX_ "The stat preceding lstat() wasn't an lstat");
	}

      do_fstat:
	if (gv != PL_defgv) {
	    PL_laststype = OP_STAT;
	    PL_statgv = gv;
	    sv_setpvn(PL_statname, "", 0);
            if(gv) {
                io = GvIO(gv);
                do_fstat_have_io:
                if (io) {
                    if (IoIFP(io)) {
                        PL_laststatval = 
                            PerlLIO_fstat(PerlIO_fileno(IoIFP(io)), &PL_statcache);   
                    } else if (IoDIRP(io)) {
                        PL_laststatval =
                            PerlLIO_fstat(my_dirfd(IoDIRP(io)), &PL_statcache);
                    } else {
                        PL_laststatval = -1;
                    }
	        }
            }
        }

	if (PL_laststatval < 0) {
	    if (ckWARN2(WARN_UNOPENED,WARN_CLOSED))
		report_evil_fh(gv, GvIO(gv), PL_op->op_type);
	    max = 0;
	}
    }
    else {
	SV* const sv = POPs;
	if (SvTYPE(sv) == SVt_PVGV) {
	    gv = (GV*)sv;
	    goto do_fstat;
	} else if(SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVGV) {
            gv = (GV*)SvRV(sv);
            if (PL_op->op_type == OP_LSTAT)
                goto do_fstat_warning_check;
            goto do_fstat;
        } else if (SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVIO) { 
            io = (IO*)SvRV(sv);
            if (PL_op->op_type == OP_LSTAT)
                goto do_fstat_warning_check;
            goto do_fstat_have_io; 
        }
        
	sv_setpv(PL_statname, SvPV_nolen_const(sv));
	PL_statgv = NULL;
	PL_laststype = PL_op->op_type;
	if (PL_op->op_type == OP_LSTAT)
	    PL_laststatval = PerlLIO_lstat(SvPV_nolen_const(PL_statname), &PL_statcache);
	else
	    PL_laststatval = PerlLIO_stat(SvPV_nolen_const(PL_statname), &PL_statcache);
	if (PL_laststatval < 0) {
	    if (ckWARN(WARN_NEWLINE) && strchr(SvPV_nolen_const(PL_statname), '\n'))
		Perl_warner(aTHX_ packWARN(WARN_NEWLINE), PL_warn_nl, "stat");
	    max = 0;
	}
    }

    gimme = GIMME_V;
    if (gimme != G_ARRAY) {
	if (gimme != G_VOID)
	    XPUSHs(boolSV(max));
	RETURN;
    }
    if (max) {
	EXTEND(SP, max);
	EXTEND_MORTAL(max);
	PUSHs(sv_2mortal(newSViv(PL_statcache.st_dev)));
	PUSHs(sv_2mortal(newSViv(PL_statcache.st_ino)));
	PUSHs(sv_2mortal(newSVuv(PL_statcache.st_mode)));
	PUSHs(sv_2mortal(newSVuv(PL_statcache.st_nlink)));
#if Uid_t_size > IVSIZE
	PUSHs(sv_2mortal(newSVnv(PL_statcache.st_uid)));
#else
#   if Uid_t_sign <= 0
	PUSHs(sv_2mortal(newSViv(PL_statcache.st_uid)));
#   else
	PUSHs(sv_2mortal(newSVuv(PL_statcache.st_uid)));
#   endif
#endif
#if Gid_t_size > IVSIZE
	PUSHs(sv_2mortal(newSVnv(PL_statcache.st_gid)));
#else
#   if Gid_t_sign <= 0
	PUSHs(sv_2mortal(newSViv(PL_statcache.st_gid)));
#   else
	PUSHs(sv_2mortal(newSVuv(PL_statcache.st_gid)));
#   endif
#endif
#ifdef USE_STAT_RDEV
	PUSHs(sv_2mortal(newSViv(PL_statcache.st_rdev)));
#else
	PUSHs(sv_2mortal(newSVpvs("")));
#endif
#if Off_t_size > IVSIZE
	PUSHs(sv_2mortal(newSVnv((NV)PL_statcache.st_size)));
#else
	PUSHs(sv_2mortal(newSViv(PL_statcache.st_size)));
#endif
#ifdef BIG_TIME
	PUSHs(sv_2mortal(newSVnv(PL_statcache.st_atime)));
	PUSHs(sv_2mortal(newSVnv(PL_statcache.st_mtime)));
	PUSHs(sv_2mortal(newSVnv(PL_statcache.st_ctime)));
#else
	PUSHs(sv_2mortal(newSViv((IV)PL_statcache.st_atime)));
	PUSHs(sv_2mortal(newSViv((IV)PL_statcache.st_mtime)));
	PUSHs(sv_2mortal(newSViv((IV)PL_statcache.st_ctime)));
#endif
#ifdef USE_STAT_BLOCKS
	PUSHs(sv_2mortal(newSVuv(PL_statcache.st_blksize)));
	PUSHs(sv_2mortal(newSVuv(PL_statcache.st_blocks)));
#else
	PUSHs(sv_2mortal(newSVpvs("")));
	PUSHs(sv_2mortal(newSVpvs("")));
#endif
    }
    RETURN;
}

/* This macro is used by the stacked filetest operators :
 * if the previous filetest failed, short-circuit and pass its value.
 * Else, discard it from the stack and continue. --rgs
 */
#define STACKED_FTEST_CHECK if (PL_op->op_private & OPpFT_STACKED) { \
	if (TOPs == &PL_sv_no || TOPs == &PL_sv_undef) { RETURN; } \
	else { (void)POPs; PUTBACK; } \
    }

PP(pp_ftrread)
{
    dVAR;
    I32 result;
    /* Not const, because things tweak this below. Not bool, because there's
       no guarantee that OPp_FT_ACCESS is <= CHAR_MAX  */
#if defined(HAS_ACCESS) || defined (PERL_EFF_ACCESS)
    I32 use_access = PL_op->op_private & OPpFT_ACCESS;
    /* Giving some sort of initial value silences compilers.  */
#  ifdef R_OK
    int access_mode = R_OK;
#  else
    int access_mode = 0;
#  endif
#else
    /* access_mode is never used, but leaving use_access in makes the
       conditional compiling below much clearer.  */
    I32 use_access = 0;
#endif
    int stat_mode = S_IRUSR;

    bool effective = FALSE;
    dSP;

    STACKED_FTEST_CHECK;

    switch (PL_op->op_type) {
    case OP_FTRREAD:
#if !(defined(HAS_ACCESS) && defined(R_OK))
	use_access = 0;
#endif
	break;

    case OP_FTRWRITE:
#if defined(HAS_ACCESS) && defined(W_OK)
	access_mode = W_OK;
#else
	use_access = 0;
#endif
	stat_mode = S_IWUSR;
	break;

    case OP_FTREXEC:
#if defined(HAS_ACCESS) && defined(X_OK)
	access_mode = X_OK;
#else
	use_access = 0;
#endif
	stat_mode = S_IXUSR;
	break;

    case OP_FTEWRITE:
#ifdef PERL_EFF_ACCESS
	access_mode = W_OK;
#endif
	stat_mode = S_IWUSR;
	/* Fall through  */

    case OP_FTEREAD:
#ifndef PERL_EFF_ACCESS
	use_access = 0;
#endif
	effective = TRUE;
	break;


    case OP_FTEEXEC:
#ifdef PERL_EFF_ACCESS
	access_mode = W_OK;
#else
	use_access = 0;
#endif
	stat_mode = S_IXUSR;
	effective = TRUE;
	break;
    }

    if (use_access) {
#if defined(HAS_ACCESS) || defined (PERL_EFF_ACCESS)
	const char *name = POPpx;
	if (effective) {
#  ifdef PERL_EFF_ACCESS
	    result = PERL_EFF_ACCESS(name, access_mode);
#  else
	    DIE(aTHX_ "panic: attempt to call PERL_EFF_ACCESS in %s",
		OP_NAME(PL_op));
#  endif
	}
	else {
#  ifdef HAS_ACCESS
	    result = access(name, access_mode);
#  else
	    DIE(aTHX_ "panic: attempt to call access() in %s", OP_NAME(PL_op));
#  endif
	}
	if (result == 0)
	    RETPUSHYES;
	if (result < 0)
	    RETPUSHUNDEF;
	RETPUSHNO;
#endif
    }

    result = my_stat();
    SPAGAIN;
    if (result < 0)
	RETPUSHUNDEF;
    if (cando(stat_mode, effective, &PL_statcache))
	RETPUSHYES;
    RETPUSHNO;
}

PP(pp_ftis)
{
    dVAR;
    I32 result;
    const int op_type = PL_op->op_type;
    dSP;
    STACKED_FTEST_CHECK;
    result = my_stat();
    SPAGAIN;
    if (result < 0)
	RETPUSHUNDEF;
    if (op_type == OP_FTIS)
	RETPUSHYES;
    {
	/* You can't dTARGET inside OP_FTIS, because you'll get
	   "panic: pad_sv po" - the op is not flagged to have a target.  */
	dTARGET;
	switch (op_type) {
	case OP_FTSIZE:
#if Off_t_size > IVSIZE
	    PUSHn(PL_statcache.st_size);
#else
	    PUSHi(PL_statcache.st_size);
#endif
	    break;
	case OP_FTMTIME:
	    PUSHn( (((NV)PL_basetime - PL_statcache.st_mtime)) / 86400.0 );
	    break;
	case OP_FTATIME:
	    PUSHn( (((NV)PL_basetime - PL_statcache.st_atime)) / 86400.0 );
	    break;
	case OP_FTCTIME:
	    PUSHn( (((NV)PL_basetime - PL_statcache.st_ctime)) / 86400.0 );
	    break;
	}
    }
    RETURN;
}

PP(pp_ftrowned)
{
    dVAR;
    I32 result;
    dSP;

    /* I believe that all these three are likely to be defined on most every
       system these days.  */
#ifndef S_ISUID
    if(PL_op->op_type == OP_FTSUID)
	RETPUSHNO;
#endif
#ifndef S_ISGID
    if(PL_op->op_type == OP_FTSGID)
	RETPUSHNO;
#endif
#ifndef S_ISVTX
    if(PL_op->op_type == OP_FTSVTX)
	RETPUSHNO;
#endif

    STACKED_FTEST_CHECK;
    result = my_stat();
    SPAGAIN;
    if (result < 0)
	RETPUSHUNDEF;
    switch (PL_op->op_type) {
    case OP_FTROWNED:
	if (PL_statcache.st_uid == PL_uid)
	    RETPUSHYES;
	break;
    case OP_FTEOWNED:
	if (PL_statcache.st_uid == PL_euid)
	    RETPUSHYES;
	break;
    case OP_FTZERO:
	if (PL_statcache.st_size == 0)
	    RETPUSHYES;
	break;
    case OP_FTSOCK:
	if (S_ISSOCK(PL_statcache.st_mode))
	    RETPUSHYES;
	break;
    case OP_FTCHR:
	if (S_ISCHR(PL_statcache.st_mode))
	    RETPUSHYES;
	break;
    case OP_FTBLK:
	if (S_ISBLK(PL_statcache.st_mode))
	    RETPUSHYES;
	break;
    case OP_FTFILE:
	if (S_ISREG(PL_statcache.st_mode))
	    RETPUSHYES;
	break;
    case OP_FTDIR:
	if (S_ISDIR(PL_statcache.st_mode))
	    RETPUSHYES;
	break;
    case OP_FTPIPE:
	if (S_ISFIFO(PL_statcache.st_mode))
	    RETPUSHYES;
	break;
#ifdef S_ISUID
    case OP_FTSUID:
	if (PL_statcache.st_mode & S_ISUID)
	    RETPUSHYES;
	break;
#endif
#ifdef S_ISGID
    case OP_FTSGID:
	if (PL_statcache.st_mode & S_ISGID)
	    RETPUSHYES;
	break;
#endif
#ifdef S_ISVTX
    case OP_FTSVTX:
	if (PL_statcache.st_mode & S_ISVTX)
	    RETPUSHYES;
	break;
#endif
    }
    RETPUSHNO;
}

PP(pp_ftlink)
{
    dVAR;
    I32 result = my_lstat();
    dSP;
    if (result < 0)
	RETPUSHUNDEF;
    if (S_ISLNK(PL_statcache.st_mode))
	RETPUSHYES;
    RETPUSHNO;
}

PP(pp_fttty)
{
    dVAR;
    dSP;
    int fd;
    GV *gv;
    SV *tmpsv = NULL;

    STACKED_FTEST_CHECK;

    if (PL_op->op_flags & OPf_REF)
	gv = cGVOP_gv;
    else if (isGV(TOPs))
	gv = (GV*)POPs;
    else if (SvROK(TOPs) && isGV(SvRV(TOPs)))
	gv = (GV*)SvRV(POPs);
    else
	gv = gv_fetchsv(tmpsv = POPs, 0, SVt_PVIO);

    if (GvIO(gv) && IoIFP(GvIOp(gv)))
	fd = PerlIO_fileno(IoIFP(GvIOp(gv)));
    else if (tmpsv && SvOK(tmpsv)) {
	const char *tmps = SvPV_nolen_const(tmpsv);
	if (isDIGIT(*tmps))
	    fd = atoi(tmps);
	else 
	    RETPUSHUNDEF;
    }
    else
	RETPUSHUNDEF;
    if (PerlLIO_isatty(fd))
	RETPUSHYES;
    RETPUSHNO;
}

#if defined(atarist) /* this will work with atariST. Configure will
			make guesses for other systems. */
# define FILE_base(f) ((f)->_base)
# define FILE_ptr(f) ((f)->_ptr)
# define FILE_cnt(f) ((f)->_cnt)
# define FILE_bufsiz(f) ((f)->_cnt + ((f)->_ptr - (f)->_base))
#endif

PP(pp_fttext)
{
    dVAR;
    dSP;
    I32 i;
    I32 len;
    I32 odd = 0;
    STDCHAR tbuf[512];
    register STDCHAR *s;
    register IO *io;
    register SV *sv;
    GV *gv;
    PerlIO *fp;

    STACKED_FTEST_CHECK;

    if (PL_op->op_flags & OPf_REF)
	gv = cGVOP_gv;
    else if (isGV(TOPs))
	gv = (GV*)POPs;
    else if (SvROK(TOPs) && isGV(SvRV(TOPs)))
	gv = (GV*)SvRV(POPs);
    else
	gv = NULL;

    if (gv) {
	EXTEND(SP, 1);
	if (gv == PL_defgv) {
	    if (PL_statgv)
		io = GvIO(PL_statgv);
	    else {
		sv = PL_statname;
		goto really_filename;
	    }
	}
	else {
	    PL_statgv = gv;
	    PL_laststatval = -1;
	    sv_setpvn(PL_statname, "", 0);
	    io = GvIO(PL_statgv);
	}
	if (io && IoIFP(io)) {
	    if (! PerlIO_has_base(IoIFP(io)))
		DIE(aTHX_ "-T and -B not implemented on filehandles");
	    PL_laststatval = PerlLIO_fstat(PerlIO_fileno(IoIFP(io)), &PL_statcache);
	    if (PL_laststatval < 0)
		RETPUSHUNDEF;
	    if (S_ISDIR(PL_statcache.st_mode)) { /* handle NFS glitch */
		if (PL_op->op_type == OP_FTTEXT)
		    RETPUSHNO;
		else
		    RETPUSHYES;
            }
	    if (PerlIO_get_cnt(IoIFP(io)) <= 0) {
		i = PerlIO_getc(IoIFP(io));
		if (i != EOF)
		    (void)PerlIO_ungetc(IoIFP(io),i);
	    }
	    if (PerlIO_get_cnt(IoIFP(io)) <= 0)	/* null file is anything */
		RETPUSHYES;
	    len = PerlIO_get_bufsiz(IoIFP(io));
	    s = (STDCHAR *) PerlIO_get_base(IoIFP(io));
	    /* sfio can have large buffers - limit to 512 */
	    if (len > 512)
		len = 512;
	}
	else {
	    if (ckWARN2(WARN_UNOPENED,WARN_CLOSED)) {
		gv = cGVOP_gv;
		report_evil_fh(gv, GvIO(gv), PL_op->op_type);
	    }
	    SETERRNO(EBADF,RMS_IFI);
	    RETPUSHUNDEF;
	}
    }
    else {
	sv = POPs;
      really_filename:
	PL_statgv = NULL;
	PL_laststype = OP_STAT;
	sv_setpv(PL_statname, SvPV_nolen_const(sv));
	if (!(fp = PerlIO_open(SvPVX_const(PL_statname), "r"))) {
	    if (ckWARN(WARN_NEWLINE) && strchr(SvPV_nolen_const(PL_statname),
					       '\n'))
		Perl_warner(aTHX_ packWARN(WARN_NEWLINE), PL_warn_nl, "open");
	    RETPUSHUNDEF;
	}
	PL_laststatval = PerlLIO_fstat(PerlIO_fileno(fp), &PL_statcache);
	if (PL_laststatval < 0)	{
	    (void)PerlIO_close(fp);
	    RETPUSHUNDEF;
	}
	PerlIO_binmode(aTHX_ fp, '<', O_BINARY, NULL);
	len = PerlIO_read(fp, tbuf, sizeof(tbuf));
	(void)PerlIO_close(fp);
	if (len <= 0) {
	    if (S_ISDIR(PL_statcache.st_mode) && PL_op->op_type == OP_FTTEXT)
		RETPUSHNO;		/* special case NFS directories */
	    RETPUSHYES;		/* null file is anything */
	}
	s = tbuf;
    }

    /* now scan s to look for textiness */
    /*   XXX ASCII dependent code */

#if defined(DOSISH) || defined(USEMYBINMODE)
    /* ignore trailing ^Z on short files */
    if (len && len < (I32)sizeof(tbuf) && tbuf[len-1] == 26)
	--len;
#endif

    for (i = 0; i < len; i++, s++) {
	if (!*s) {			/* null never allowed in text */
	    odd += len;
	    break;
	}
#ifdef EBCDIC
        else if (!(isPRINT(*s) || isSPACE(*s)))
            odd++;
#else
	else if (*s & 128) {
#ifdef USE_LOCALE
	    if (IN_LOCALE_RUNTIME && isALPHA_LC(*s))
		continue;
#endif
	    /* utf8 characters don't count as odd */
	    if (UTF8_IS_START(*s)) {
		int ulen = UTF8SKIP(s);
		if (ulen < len - i) {
		    int j;
		    for (j = 1; j < ulen; j++) {
			if (!UTF8_IS_CONTINUATION(s[j]))
			    goto not_utf8;
		    }
		    --ulen;	/* loop does extra increment */
		    s += ulen;
		    i += ulen;
		    continue;
		}
	    }
	  not_utf8:
	    odd++;
	}
	else if (*s < 32 &&
	  *s != '\n' && *s != '\r' && *s != '\b' &&
	  *s != '\t' && *s != '\f' && *s != 27)
	    odd++;
#endif
    }

    if ((odd * 3 > len) == (PL_op->op_type == OP_FTTEXT)) /* allow 1/3 odd */
	RETPUSHNO;
    else
	RETPUSHYES;
}

/* File calls. */

PP(pp_chdir)
{
    dVAR; dSP; dTARGET;
    const char *tmps = NULL;
    GV *gv = NULL;

    if( MAXARG == 1 ) {
	SV * const sv = POPs;
	if (PL_op->op_flags & OPf_SPECIAL) {
	    gv = gv_fetchsv(sv, 0, SVt_PVIO);
	}
        else if (SvTYPE(sv) == SVt_PVGV) {
	    gv = (GV*)sv;
        }
	else if (SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVGV) {
            gv = (GV*)SvRV(sv);
        }
        else {
	    tmps = SvPV_nolen_const(sv);
	}
    }

    if( !gv && (!tmps || !*tmps) ) {
	HV * const table = GvHVn(PL_envgv);
	SV **svp;

        if (    (svp = hv_fetchs(table, "HOME", FALSE))
             || (svp = hv_fetchs(table, "LOGDIR", FALSE))
#ifdef VMS
             || (svp = hv_fetchs(table, "SYS$LOGIN", FALSE))
#endif
           )
        {
            if( MAXARG == 1 )
                deprecate("chdir('') or chdir(undef) as chdir()");
            tmps = SvPV_nolen_const(*svp);
        }
        else {
            PUSHi(0);
            TAINT_PROPER("chdir");
            RETURN;
        }
    }

    TAINT_PROPER("chdir");
    if (gv) {
#ifdef HAS_FCHDIR
	IO* const io = GvIO(gv);
	if (io) {
	    if (IoDIRP(io)) {
		PUSHi(fchdir(my_dirfd(IoDIRP(io))) >= 0);
	    } else if (IoIFP(io)) {
                PUSHi(fchdir(PerlIO_fileno(IoIFP(io))) >= 0);
	    }
	    else {
		if (ckWARN2(WARN_UNOPENED,WARN_CLOSED))
		    report_evil_fh(gv, io, PL_op->op_type);
		SETERRNO(EBADF, RMS_IFI);
		PUSHi(0);
	    }
        }
	else {
	    if (ckWARN2(WARN_UNOPENED,WARN_CLOSED))
		report_evil_fh(gv, io, PL_op->op_type);
	    SETERRNO(EBADF,RMS_IFI);
	    PUSHi(0);
	}
#else
	DIE(aTHX_ PL_no_func, "fchdir");
#endif
    }
    else 
        PUSHi( PerlDir_chdir(tmps) >= 0 );
#ifdef VMS
    /* Clear the DEFAULT element of ENV so we'll get the new value
     * in the future. */
    hv_delete(GvHVn(PL_envgv),"DEFAULT",7,G_DISCARD);
#endif
    RETURN;
}

PP(pp_chown)
{
    dVAR; dSP; dMARK; dTARGET;
    const I32 value = (I32)apply(PL_op->op_type, MARK, SP);

    SP = MARK;
    XPUSHi(value);
    RETURN;
}

PP(pp_chroot)
{
#ifdef HAS_CHROOT
    dVAR; dSP; dTARGET;
    char * const tmps = POPpx;
    TAINT_PROPER("chroot");
    PUSHi( chroot(tmps) >= 0 );
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "chroot");
#endif
}

PP(pp_rename)
{
    dVAR; dSP; dTARGET;
    int anum;
    const char * const tmps2 = POPpconstx;
    const char * const tmps = SvPV_nolen_const(TOPs);
    TAINT_PROPER("rename");
#ifdef HAS_RENAME
    anum = PerlLIO_rename(tmps, tmps2);
#else
    if (!(anum = PerlLIO_stat(tmps, &PL_statbuf))) {
	if (same_dirent(tmps2, tmps))	/* can always rename to same name */
	    anum = 1;
	else {
	    if (PL_euid || PerlLIO_stat(tmps2, &PL_statbuf) < 0 || !S_ISDIR(PL_statbuf.st_mode))
		(void)UNLINK(tmps2);
	    if (!(anum = link(tmps, tmps2)))
		anum = UNLINK(tmps);
	}
    }
#endif
    SETi( anum >= 0 );
    RETURN;
}

#if defined(HAS_LINK) || defined(HAS_SYMLINK)
PP(pp_link)
{
    dVAR; dSP; dTARGET;
    const int op_type = PL_op->op_type;
    int result;

#  ifndef HAS_LINK
    if (op_type == OP_LINK)
	DIE(aTHX_ PL_no_func, "link");
#  endif
#  ifndef HAS_SYMLINK
    if (op_type == OP_SYMLINK)
	DIE(aTHX_ PL_no_func, "symlink");
#  endif

    {
	const char * const tmps2 = POPpconstx;
	const char * const tmps = SvPV_nolen_const(TOPs);
	TAINT_PROPER(PL_op_desc[op_type]);
	result =
#  if defined(HAS_LINK)
#    if defined(HAS_SYMLINK)
	    /* Both present - need to choose which.  */
	    (op_type == OP_LINK) ?
	    PerlLIO_link(tmps, tmps2) : symlink(tmps, tmps2);
#    else
    /* Only have link, so calls to pp_symlink will have DIE()d above.  */
	PerlLIO_link(tmps, tmps2);
#    endif
#  else
#    if defined(HAS_SYMLINK)
    /* Only have symlink, so calls to pp_link will have DIE()d above.  */
	symlink(tmps, tmps2);
#    endif
#  endif
    }

    SETi( result >= 0 );
    RETURN;
}
#else
PP(pp_link)
{
    /* Have neither.  */
    DIE(aTHX_ PL_no_func, PL_op_desc[PL_op->op_type]);
}
#endif

PP(pp_readlink)
{
    dVAR;
    dSP;
#ifdef HAS_SYMLINK
    dTARGET;
    const char *tmps;
    char buf[MAXPATHLEN];
    int len;

#ifndef INCOMPLETE_TAINTS
    TAINT;
#endif
    tmps = POPpconstx;
    len = readlink(tmps, buf, sizeof(buf) - 1);
    EXTEND(SP, 1);
    if (len < 0)
	RETPUSHUNDEF;
    PUSHp(buf, len);
    RETURN;
#else
    EXTEND(SP, 1);
    RETSETUNDEF;		/* just pretend it's a normal file */
#endif
}

#if !defined(HAS_MKDIR) || !defined(HAS_RMDIR)
STATIC int
S_dooneliner(pTHX_ const char *cmd, const char *filename)
{
    char * const save_filename = filename;
    char *cmdline;
    char *s;
    PerlIO *myfp;
    int anum = 1;
    Size_t size = strlen(cmd) + (strlen(filename) * 2) + 10;

    Newx(cmdline, size, char);
    my_strlcpy(cmdline, cmd, size);
    my_strlcat(cmdline, " ", size);
    for (s = cmdline + strlen(cmdline); *filename; ) {
	*s++ = '\\';
	*s++ = *filename++;
    }
    if (s - cmdline < size)
	my_strlcpy(s, " 2>&1", size - (s - cmdline));
    myfp = PerlProc_popen(cmdline, "r");
    Safefree(cmdline);

    if (myfp) {
	SV * const tmpsv = sv_newmortal();
	/* Need to save/restore 'PL_rs' ?? */
	s = sv_gets(tmpsv, myfp, 0);
	(void)PerlProc_pclose(myfp);
	if (s != NULL) {
	    int e;
	    for (e = 1;
#ifdef HAS_SYS_ERRLIST
		 e <= sys_nerr
#endif
		 ; e++)
	    {
		/* you don't see this */
		const char * const errmsg =
#ifdef HAS_SYS_ERRLIST
		    sys_errlist[e]
#else
		    strerror(e)
#endif
		    ;
		if (!errmsg)
		    break;
		if (instr(s, errmsg)) {
		    SETERRNO(e,0);
		    return 0;
		}
	    }
	    SETERRNO(0,0);
#ifndef EACCES
#define EACCES EPERM
#endif
	    if (instr(s, "cannot make"))
		SETERRNO(EEXIST,RMS_FEX);
	    else if (instr(s, "existing file"))
		SETERRNO(EEXIST,RMS_FEX);
	    else if (instr(s, "ile exists"))
		SETERRNO(EEXIST,RMS_FEX);
	    else if (instr(s, "non-exist"))
		SETERRNO(ENOENT,RMS_FNF);
	    else if (instr(s, "does not exist"))
		SETERRNO(ENOENT,RMS_FNF);
	    else if (instr(s, "not empty"))
		SETERRNO(EBUSY,SS_DEVOFFLINE);
	    else if (instr(s, "cannot access"))
		SETERRNO(EACCES,RMS_PRV);
	    else
		SETERRNO(EPERM,RMS_PRV);
	    return 0;
	}
	else {	/* some mkdirs return no failure indication */
	    anum = (PerlLIO_stat(save_filename, &PL_statbuf) >= 0);
	    if (PL_op->op_type == OP_RMDIR)
		anum = !anum;
	    if (anum)
		SETERRNO(0,0);
	    else
		SETERRNO(EACCES,RMS_PRV);	/* a guess */
	}
	return anum;
    }
    else
	return 0;
}
#endif

/* This macro removes trailing slashes from a directory name.
 * Different operating and file systems take differently to
 * trailing slashes.  According to POSIX 1003.1 1996 Edition
 * any number of trailing slashes should be allowed.
 * Thusly we snip them away so that even non-conforming
 * systems are happy.
 * We should probably do this "filtering" for all
 * the functions that expect (potentially) directory names:
 * -d, chdir(), chmod(), chown(), chroot(), fcntl()?,
 * (mkdir()), opendir(), rename(), rmdir(), stat(). --jhi */

#define TRIMSLASHES(tmps,len,copy) (tmps) = SvPV_const(TOPs, (len)); \
    if ((len) > 1 && (tmps)[(len)-1] == '/') { \
	do { \
	    (len)--; \
	} while ((len) > 1 && (tmps)[(len)-1] == '/'); \
	(tmps) = savepvn((tmps), (len)); \
	(copy) = TRUE; \
    }

PP(pp_mkdir)
{
    dVAR; dSP; dTARGET;
    STRLEN len;
    const char *tmps;
    bool copy = FALSE;
    const int mode = (MAXARG > 1) ? POPi : 0777;

    TRIMSLASHES(tmps,len,copy);

    TAINT_PROPER("mkdir");
#ifdef HAS_MKDIR
    SETi( PerlDir_mkdir(tmps, mode) >= 0 );
#else
    {
    int oldumask;
    SETi( dooneliner("mkdir", tmps) );
    oldumask = PerlLIO_umask(0);
    PerlLIO_umask(oldumask);
    PerlLIO_chmod(tmps, (mode & ~oldumask) & 0777);
    }
#endif
    if (copy)
	Safefree(tmps);
    RETURN;
}

PP(pp_rmdir)
{
    dVAR; dSP; dTARGET;
    STRLEN len;
    const char *tmps;
    bool copy = FALSE;

    TRIMSLASHES(tmps,len,copy);
    TAINT_PROPER("rmdir");
#ifdef HAS_RMDIR
    SETi( PerlDir_rmdir(tmps) >= 0 );
#else
    SETi( dooneliner("rmdir", tmps) );
#endif
    if (copy)
	Safefree(tmps);
    RETURN;
}

/* Directory calls. */

PP(pp_open_dir)
{
#if defined(Direntry_t) && defined(HAS_READDIR)
    dVAR; dSP;
    const char * const dirname = POPpconstx;
    GV * const gv = (GV*)POPs;
    register IO * const io = GvIOn(gv);

    if (!io)
	goto nope;

    if ((IoIFP(io) || IoOFP(io)) && ckWARN2(WARN_IO, WARN_DEPRECATED))
	Perl_warner(aTHX_ packWARN2(WARN_IO, WARN_DEPRECATED),
		"Opening filehandle %s also as a directory", GvENAME(gv));
    if (IoDIRP(io))
	PerlDir_close(IoDIRP(io));
    if (!(IoDIRP(io) = PerlDir_open(dirname)))
	goto nope;

    RETPUSHYES;
nope:
    if (!errno)
	SETERRNO(EBADF,RMS_DIR);
    RETPUSHUNDEF;
#else
    DIE(aTHX_ PL_no_dir_func, "opendir");
#endif
}

PP(pp_readdir)
{
#if !defined(Direntry_t) || !defined(HAS_READDIR)
    DIE(aTHX_ PL_no_dir_func, "readdir");
#else
#if !defined(I_DIRENT) && !defined(VMS)
    Direntry_t *readdir (DIR *);
#endif
    dVAR;
    dSP;

    SV *sv;
    const I32 gimme = GIMME;
    GV * const gv = (GV *)POPs;
    register const Direntry_t *dp;
    register IO * const io = GvIOn(gv);

    if (!io || !IoDIRP(io)) {
        if(ckWARN(WARN_IO)) {
            Perl_warner(aTHX_ packWARN(WARN_IO),
                "readdir() attempted on invalid dirhandle %s", GvENAME(gv));
        }
        goto nope;
    }

    do {
        dp = (Direntry_t *)PerlDir_read(IoDIRP(io));
        if (!dp)
            break;
#ifdef DIRNAMLEN
        sv = newSVpvn(dp->d_name, dp->d_namlen);
#else
        sv = newSVpv(dp->d_name, 0);
#endif
#ifndef INCOMPLETE_TAINTS
        if (!(IoFLAGS(io) & IOf_UNTAINT))
            SvTAINTED_on(sv);
#endif
        XPUSHs(sv_2mortal(sv));
    } while (gimme == G_ARRAY);

    if (!dp && gimme != G_ARRAY)
        goto nope;

    RETURN;

nope:
    if (!errno)
	SETERRNO(EBADF,RMS_ISI);
    if (GIMME == G_ARRAY)
	RETURN;
    else
	RETPUSHUNDEF;
#endif
}

PP(pp_telldir)
{
#if defined(HAS_TELLDIR) || defined(telldir)
    dVAR; dSP; dTARGET;
 /* XXX does _anyone_ need this? --AD 2/20/1998 */
 /* XXX netbsd still seemed to.
    XXX HAS_TELLDIR_PROTO is new style, NEED_TELLDIR_PROTO is old style.
    --JHI 1999-Feb-02 */
# if !defined(HAS_TELLDIR_PROTO) || defined(NEED_TELLDIR_PROTO)
    long telldir (DIR *);
# endif
    GV * const gv = (GV*)POPs;
    register IO * const io = GvIOn(gv);

    if (!io || !IoDIRP(io)) {
        if(ckWARN(WARN_IO)) {
            Perl_warner(aTHX_ packWARN(WARN_IO),
	        "telldir() attempted on invalid dirhandle %s", GvENAME(gv));
        }
        goto nope;
    }

    PUSHi( PerlDir_tell(IoDIRP(io)) );
    RETURN;
nope:
    if (!errno)
	SETERRNO(EBADF,RMS_ISI);
    RETPUSHUNDEF;
#else
    DIE(aTHX_ PL_no_dir_func, "telldir");
#endif
}

PP(pp_seekdir)
{
#if defined(HAS_SEEKDIR) || defined(seekdir)
    dVAR; dSP;
    const long along = POPl;
    GV * const gv = (GV*)POPs;
    register IO * const io = GvIOn(gv);

    if (!io || !IoDIRP(io)) {
	if(ckWARN(WARN_IO)) {
	    Perl_warner(aTHX_ packWARN(WARN_IO),
                "seekdir() attempted on invalid dirhandle %s", GvENAME(gv));
        }
        goto nope;
    }
    (void)PerlDir_seek(IoDIRP(io), along);

    RETPUSHYES;
nope:
    if (!errno)
	SETERRNO(EBADF,RMS_ISI);
    RETPUSHUNDEF;
#else
    DIE(aTHX_ PL_no_dir_func, "seekdir");
#endif
}

PP(pp_rewinddir)
{
#if defined(HAS_REWINDDIR) || defined(rewinddir)
    dVAR; dSP;
    GV * const gv = (GV*)POPs;
    register IO * const io = GvIOn(gv);

    if (!io || !IoDIRP(io)) {
	if(ckWARN(WARN_IO)) {
	    Perl_warner(aTHX_ packWARN(WARN_IO),
	        "rewinddir() attempted on invalid dirhandle %s", GvENAME(gv));
	}
	goto nope;
    }
    (void)PerlDir_rewind(IoDIRP(io));
    RETPUSHYES;
nope:
    if (!errno)
	SETERRNO(EBADF,RMS_ISI);
    RETPUSHUNDEF;
#else
    DIE(aTHX_ PL_no_dir_func, "rewinddir");
#endif
}

PP(pp_closedir)
{
#if defined(Direntry_t) && defined(HAS_READDIR)
    dVAR; dSP;
    GV * const gv = (GV*)POPs;
    register IO * const io = GvIOn(gv);

    if (!io || !IoDIRP(io)) {
	if(ckWARN(WARN_IO)) {
	    Perl_warner(aTHX_ packWARN(WARN_IO),
                "closedir() attempted on invalid dirhandle %s", GvENAME(gv));
        }
        goto nope;
    }
#ifdef VOID_CLOSEDIR
    PerlDir_close(IoDIRP(io));
#else
    if (PerlDir_close(IoDIRP(io)) < 0) {
	IoDIRP(io) = 0; /* Don't try to close again--coredumps on SysV */
	goto nope;
    }
#endif
    IoDIRP(io) = 0;

    RETPUSHYES;
nope:
    if (!errno)
	SETERRNO(EBADF,RMS_IFI);
    RETPUSHUNDEF;
#else
    DIE(aTHX_ PL_no_dir_func, "closedir");
#endif
}

/* Process control. */

PP(pp_fork)
{
#ifdef HAS_FORK
    dVAR; dSP; dTARGET;
    Pid_t childpid;

    EXTEND(SP, 1);
    PERL_FLUSHALL_FOR_CHILD;
    childpid = PerlProc_fork();
    if (childpid < 0)
	RETSETUNDEF;
    if (!childpid) {
	GV * const tmpgv = gv_fetchpvs("$", GV_ADD|GV_NOTQUAL, SVt_PV);
	if (tmpgv) {
            SvREADONLY_off(GvSV(tmpgv));
	    sv_setiv(GvSV(tmpgv), (IV)PerlProc_getpid());
            SvREADONLY_on(GvSV(tmpgv));
        }
#ifdef THREADS_HAVE_PIDS
	PL_ppid = (IV)getppid();
#endif
#ifdef PERL_USES_PL_PIDSTATUS
	hv_clear(PL_pidstatus);	/* no kids, so don't wait for 'em */
#endif
    }
    PUSHi(childpid);
    RETURN;
#else
#  if defined(USE_ITHREADS) && defined(PERL_IMPLICIT_SYS)
    dSP; dTARGET;
    Pid_t childpid;

    EXTEND(SP, 1);
    PERL_FLUSHALL_FOR_CHILD;
    childpid = PerlProc_fork();
    if (childpid == -1)
	RETSETUNDEF;
    PUSHi(childpid);
    RETURN;
#  else
    DIE(aTHX_ PL_no_func, "fork");
#  endif
#endif
}

PP(pp_wait)
{
#if (!defined(DOSISH) || defined(OS2) || defined(WIN32)) && !defined(MACOS_TRADITIONAL) && !defined(__LIBCATAMOUNT__)
    dVAR; dSP; dTARGET;
    Pid_t childpid;
    int argflags;

    if (PL_signals & PERL_SIGNALS_UNSAFE_FLAG)
        childpid = wait4pid(-1, &argflags, 0);
    else {
        while ((childpid = wait4pid(-1, &argflags, 0)) == -1 &&
	       errno == EINTR) {
	  PERL_ASYNC_CHECK();
	}
    }
#  if defined(USE_ITHREADS) && defined(PERL_IMPLICIT_SYS)
    /* 0 and -1 are both error returns (the former applies to WNOHANG case) */
    STATUS_NATIVE_CHILD_SET((childpid && childpid != -1) ? argflags : -1);
#  else
    STATUS_NATIVE_CHILD_SET((childpid > 0) ? argflags : -1);
#  endif
    XPUSHi(childpid);
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "wait");
#endif
}

PP(pp_waitpid)
{
#if (!defined(DOSISH) || defined(OS2) || defined(WIN32)) && !defined(MACOS_TRADITIONAL) && !defined(__LIBCATAMOUNT__)
    dVAR; dSP; dTARGET;
    const int optype = POPi;
    const Pid_t pid = TOPi;
    Pid_t result;
    int argflags;

    if (PL_signals & PERL_SIGNALS_UNSAFE_FLAG)
        result = wait4pid(pid, &argflags, optype);
    else {
        while ((result = wait4pid(pid, &argflags, optype)) == -1 &&
	       errno == EINTR) {
	  PERL_ASYNC_CHECK();
	}
    }
#  if defined(USE_ITHREADS) && defined(PERL_IMPLICIT_SYS)
    /* 0 and -1 are both error returns (the former applies to WNOHANG case) */
    STATUS_NATIVE_CHILD_SET((result && result != -1) ? argflags : -1);
#  else
    STATUS_NATIVE_CHILD_SET((result > 0) ? argflags : -1);
#  endif
    SETi(result);
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "waitpid");
#endif
}

PP(pp_system)
{
    dVAR; dSP; dMARK; dORIGMARK; dTARGET;
#if defined(__LIBCATAMOUNT__)
    PL_statusvalue = -1;
    SP = ORIGMARK;
    XPUSHi(-1);
#else
    I32 value;
    int result;

    if (PL_tainting) {
	TAINT_ENV();
	while (++MARK <= SP) {
	    (void)SvPV_nolen_const(*MARK);      /* stringify for taint check */
	    if (PL_tainted)
		break;
	}
	MARK = ORIGMARK;
	TAINT_PROPER("system");
    }
    PERL_FLUSHALL_FOR_CHILD;
#if (defined(HAS_FORK) || defined(AMIGAOS)) && !defined(VMS) && !defined(OS2) || defined(PERL_MICRO)
    {
	Pid_t childpid;
	int pp[2];
	I32 did_pipes = 0;

	if (PerlProc_pipe(pp) >= 0)
	    did_pipes = 1;
	while ((childpid = PerlProc_fork()) == -1) {
	    if (errno != EAGAIN) {
		value = -1;
		SP = ORIGMARK;
		XPUSHi(value);
		if (did_pipes) {
		    PerlLIO_close(pp[0]);
		    PerlLIO_close(pp[1]);
		}
		RETURN;
	    }
	    sleep(5);
	}
	if (childpid > 0) {
	    Sigsave_t ihand,qhand; /* place to save signals during system() */
	    int status;

	    if (did_pipes)
		PerlLIO_close(pp[1]);
#ifndef PERL_MICRO
	    rsignal_save(SIGINT,  (Sighandler_t) SIG_IGN, &ihand);
	    rsignal_save(SIGQUIT, (Sighandler_t) SIG_IGN, &qhand);
#endif
	    do {
		result = wait4pid(childpid, &status, 0);
	    } while (result == -1 && errno == EINTR);
#ifndef PERL_MICRO
	    (void)rsignal_restore(SIGINT, &ihand);
	    (void)rsignal_restore(SIGQUIT, &qhand);
#endif
	    STATUS_NATIVE_CHILD_SET(result == -1 ? -1 : status);
	    do_execfree();	/* free any memory child malloced on fork */
	    SP = ORIGMARK;
	    if (did_pipes) {
		int errkid;
		unsigned n = 0;
		SSize_t n1;

		while (n < sizeof(int)) {
		    n1 = PerlLIO_read(pp[0],
				      (void*)(((char*)&errkid)+n),
				      (sizeof(int)) - n);
		    if (n1 <= 0)
			break;
		    n += n1;
		}
		PerlLIO_close(pp[0]);
		if (n) {			/* Error */
		    if (n != sizeof(int))
			DIE(aTHX_ "panic: kid popen errno read");
		    errno = errkid;		/* Propagate errno from kid */
		    STATUS_NATIVE_CHILD_SET(-1);
		}
	    }
	    XPUSHi(STATUS_CURRENT);
	    RETURN;
	}
	if (did_pipes) {
	    PerlLIO_close(pp[0]);
#if defined(HAS_FCNTL) && defined(F_SETFD)
	    fcntl(pp[1], F_SETFD, FD_CLOEXEC);
#endif
	}
	if (PL_op->op_flags & OPf_STACKED) {
	    SV * const really = *++MARK;
	    value = (I32)do_aexec5(really, MARK, SP, pp[1], did_pipes);
	}
	else if (SP - MARK != 1)
	    value = (I32)do_aexec5(NULL, MARK, SP, pp[1], did_pipes);
	else {
	    value = (I32)do_exec3(SvPVx_nolen(sv_mortalcopy(*SP)), pp[1], did_pipes);
	}
	PerlProc__exit(-1);
    }
#else /* ! FORK or VMS or OS/2 */
    PL_statusvalue = 0;
    result = 0;
    if (PL_op->op_flags & OPf_STACKED) {
	SV * const really = *++MARK;
#  if defined(WIN32) || defined(OS2) || defined(__SYMBIAN32__)
	value = (I32)do_aspawn(really, MARK, SP);
#  else
	value = (I32)do_aspawn(really, (void **)MARK, (void **)SP);
#  endif
    }
    else if (SP - MARK != 1) {
#  if defined(WIN32) || defined(OS2) || defined(__SYMBIAN32__)
	value = (I32)do_aspawn(NULL, MARK, SP);
#  else
	value = (I32)do_aspawn(NULL, (void **)MARK, (void **)SP);
#  endif
    }
    else {
	value = (I32)do_spawn(SvPVx_nolen(sv_mortalcopy(*SP)));
    }
    if (PL_statusvalue == -1)	/* hint that value must be returned as is */
	result = 1;
    STATUS_NATIVE_CHILD_SET(value);
    do_execfree();
    SP = ORIGMARK;
    XPUSHi(result ? value : STATUS_CURRENT);
#endif /* !FORK or VMS or OS/2 */
#endif
    RETURN;
}

PP(pp_exec)
{
    dVAR; dSP; dMARK; dORIGMARK; dTARGET;
    I32 value;

    if (PL_tainting) {
	TAINT_ENV();
	while (++MARK <= SP) {
	    (void)SvPV_nolen_const(*MARK);      /* stringify for taint check */
	    if (PL_tainted)
		break;
	}
	MARK = ORIGMARK;
	TAINT_PROPER("exec");
    }
    PERL_FLUSHALL_FOR_CHILD;
    if (PL_op->op_flags & OPf_STACKED) {
	SV * const really = *++MARK;
	value = (I32)do_aexec(really, MARK, SP);
    }
    else if (SP - MARK != 1)
#ifdef VMS
	value = (I32)vms_do_aexec(NULL, MARK, SP);
#else
#  ifdef __OPEN_VM
	{
	   (void ) do_aspawn(NULL, MARK, SP);
	   value = 0;
	}
#  else
	value = (I32)do_aexec(NULL, MARK, SP);
#  endif
#endif
    else {
#ifdef VMS
	value = (I32)vms_do_exec(SvPVx_nolen(sv_mortalcopy(*SP)));
#else
#  ifdef __OPEN_VM
	(void) do_spawn(SvPVx_nolen(sv_mortalcopy(*SP)));
	value = 0;
#  else
	value = (I32)do_exec(SvPVx_nolen(sv_mortalcopy(*SP)));
#  endif
#endif
    }

    SP = ORIGMARK;
    XPUSHi(value);
    RETURN;
}

PP(pp_getppid)
{
#ifdef HAS_GETPPID
    dVAR; dSP; dTARGET;
#   ifdef THREADS_HAVE_PIDS
    if (PL_ppid != 1 && getppid() == 1)
	/* maybe the parent process has died. Refresh ppid cache */
	PL_ppid = 1;
    XPUSHi( PL_ppid );
#   else
    XPUSHi( getppid() );
#   endif
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "getppid");
#endif
}

PP(pp_getpgrp)
{
#ifdef HAS_GETPGRP
    dVAR; dSP; dTARGET;
    Pid_t pgrp;
    const Pid_t pid = (MAXARG < 1) ? 0 : SvIVx(POPs);

#ifdef BSD_GETPGRP
    pgrp = (I32)BSD_GETPGRP(pid);
#else
    if (pid != 0 && pid != PerlProc_getpid())
	DIE(aTHX_ "POSIX getpgrp can't take an argument");
    pgrp = getpgrp();
#endif
    XPUSHi(pgrp);
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "getpgrp()");
#endif
}

PP(pp_setpgrp)
{
#ifdef HAS_SETPGRP
    dVAR; dSP; dTARGET;
    Pid_t pgrp;
    Pid_t pid;
    if (MAXARG < 2) {
	pgrp = 0;
	pid = 0;
    }
    else {
	pgrp = POPi;
	pid = TOPi;
    }

    TAINT_PROPER("setpgrp");
#ifdef BSD_SETPGRP
    SETi( BSD_SETPGRP(pid, pgrp) >= 0 );
#else
    if ((pgrp != 0 && pgrp != PerlProc_getpid())
	|| (pid != 0 && pid != PerlProc_getpid()))
    {
	DIE(aTHX_ "setpgrp can't take arguments");
    }
    SETi( setpgrp() >= 0 );
#endif /* USE_BSDPGRP */
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "setpgrp()");
#endif
}

PP(pp_getpriority)
{
#ifdef HAS_GETPRIORITY
    dVAR; dSP; dTARGET;
    const int who = POPi;
    const int which = TOPi;
    SETi( getpriority(which, who) );
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "getpriority()");
#endif
}

PP(pp_setpriority)
{
#ifdef HAS_SETPRIORITY
    dVAR; dSP; dTARGET;
    const int niceval = POPi;
    const int who = POPi;
    const int which = TOPi;
    TAINT_PROPER("setpriority");
    SETi( setpriority(which, who, niceval) >= 0 );
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "setpriority()");
#endif
}

/* Time calls. */

PP(pp_time)
{
    dVAR; dSP; dTARGET;
#ifdef BIG_TIME
    XPUSHn( time(NULL) );
#else
    XPUSHi( time(NULL) );
#endif
    RETURN;
}

PP(pp_tms)
{
#ifdef HAS_TIMES
    dVAR;
    dSP;
    EXTEND(SP, 4);
#ifndef VMS
    (void)PerlProc_times(&PL_timesbuf);
#else
    (void)PerlProc_times((tbuffer_t *)&PL_timesbuf);  /* time.h uses different name for */
                                                   /* struct tms, though same data   */
                                                   /* is returned.                   */
#endif

    PUSHs(sv_2mortal(newSVnv(((NV)PL_timesbuf.tms_utime)/(NV)PL_clocktick)));
    if (GIMME == G_ARRAY) {
	PUSHs(sv_2mortal(newSVnv(((NV)PL_timesbuf.tms_stime)/(NV)PL_clocktick)));
	PUSHs(sv_2mortal(newSVnv(((NV)PL_timesbuf.tms_cutime)/(NV)PL_clocktick)));
	PUSHs(sv_2mortal(newSVnv(((NV)PL_timesbuf.tms_cstime)/(NV)PL_clocktick)));
    }
    RETURN;
#else
#   ifdef PERL_MICRO
    dSP;
    PUSHs(sv_2mortal(newSVnv((NV)0.0)));
    EXTEND(SP, 4);
    if (GIMME == G_ARRAY) {
	 PUSHs(sv_2mortal(newSVnv((NV)0.0)));
	 PUSHs(sv_2mortal(newSVnv((NV)0.0)));
	 PUSHs(sv_2mortal(newSVnv((NV)0.0)));
    }
    RETURN;
#   else
    DIE(aTHX_ "times not implemented");
#   endif
#endif /* HAS_TIMES */
}

#ifdef LOCALTIME_EDGECASE_BROKEN
static struct tm *S_my_localtime (pTHX_ Time_t *tp)
{
    auto time_t     T;
    auto struct tm *P;

    /* No workarounds in the valid range */
    if (!tp || *tp < 0x7fff573f || *tp >= 0x80000000)
	return (localtime (tp));

    /* This edge case is to workaround the undefined behaviour, where the
     * TIMEZONE makes the time go beyond the defined range.
     * gmtime (0x7fffffff) => 2038-01-19 03:14:07
     * If there is a negative offset in TZ, like MET-1METDST, some broken
     * implementations of localtime () (like AIX 5.2) barf with bogus
     * return values:
     * 0x7fffffff gmtime               2038-01-19 03:14:07
     * 0x7fffffff localtime            1901-12-13 21:45:51
     * 0x7fffffff mylocaltime          2038-01-19 04:14:07
     * 0x3c19137f gmtime               2001-12-13 20:45:51
     * 0x3c19137f localtime            2001-12-13 21:45:51
     * 0x3c19137f mylocaltime          2001-12-13 21:45:51
     * Given that legal timezones are typically between GMT-12 and GMT+12
     * we turn back the clock 23 hours before calling the localtime
     * function, and add those to the return value. This will never cause
     * day wrapping problems, since the edge case is Tue Jan *19*
     */
    T = *tp - 82800; /* 23 hour. allows up to GMT-23 */
    P = localtime (&T);
    P->tm_hour += 23;
    if (P->tm_hour >= 24) {
	P->tm_hour -= 24;
	P->tm_mday++;	/* 18  -> 19  */
	P->tm_wday++;	/* Mon -> Tue */
	P->tm_yday++;	/* 18  -> 19  */
    }
    return (P);
} /* S_my_localtime */
#endif

PP(pp_gmtime)
{
    dVAR;
    dSP;
    Time_t when;
    const struct tm *tmbuf;
    static const char * const dayname[] =
	{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char * const monname[] =
	{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    if (MAXARG < 1)
	(void)time(&when);
    else
#ifdef BIG_TIME
	when = (Time_t)SvNVx(POPs);
#else
	when = (Time_t)SvIVx(POPs);
#endif

    if (PL_op->op_type == OP_LOCALTIME)
#ifdef LOCALTIME_EDGECASE_BROKEN
	tmbuf = S_my_localtime(aTHX_ &when);
#else
	tmbuf = localtime(&when);
#endif
    else
	tmbuf = gmtime(&when);

    if (GIMME != G_ARRAY) {
	SV *tsv;
        EXTEND(SP, 1);
        EXTEND_MORTAL(1);
	if (!tmbuf)
	    RETPUSHUNDEF;
	tsv = Perl_newSVpvf(aTHX_ "%s %s %2d %02d:%02d:%02d %d",
			    dayname[tmbuf->tm_wday],
			    monname[tmbuf->tm_mon],
			    tmbuf->tm_mday,
			    tmbuf->tm_hour,
			    tmbuf->tm_min,
			    tmbuf->tm_sec,
			    tmbuf->tm_year + 1900);
	PUSHs(sv_2mortal(tsv));
    }
    else if (tmbuf) {
        EXTEND(SP, 9);
        EXTEND_MORTAL(9);
        PUSHs(sv_2mortal(newSViv(tmbuf->tm_sec)));
	PUSHs(sv_2mortal(newSViv(tmbuf->tm_min)));
	PUSHs(sv_2mortal(newSViv(tmbuf->tm_hour)));
	PUSHs(sv_2mortal(newSViv(tmbuf->tm_mday)));
	PUSHs(sv_2mortal(newSViv(tmbuf->tm_mon)));
	PUSHs(sv_2mortal(newSViv(tmbuf->tm_year)));
	PUSHs(sv_2mortal(newSViv(tmbuf->tm_wday)));
	PUSHs(sv_2mortal(newSViv(tmbuf->tm_yday)));
	PUSHs(sv_2mortal(newSViv(tmbuf->tm_isdst)));
    }
    RETURN;
}

PP(pp_alarm)
{
#ifdef HAS_ALARM
    dVAR; dSP; dTARGET;
    int anum;
    anum = POPi;
    anum = alarm((unsigned int)anum);
    EXTEND(SP, 1);
    if (anum < 0)
	RETPUSHUNDEF;
    PUSHi(anum);
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "alarm");
#endif
}

PP(pp_sleep)
{
    dVAR; dSP; dTARGET;
    I32 duration;
    Time_t lasttime;
    Time_t when;

    (void)time(&lasttime);
    if (MAXARG < 1)
	PerlProc_pause();
    else {
	duration = POPi;
	PerlProc_sleep((unsigned int)duration);
    }
    (void)time(&when);
    XPUSHi(when - lasttime);
    RETURN;
}

/* Shared memory. */
/* Merged with some message passing. */

PP(pp_shmwrite)
{
#if defined(HAS_MSG) || defined(HAS_SEM) || defined(HAS_SHM)
    dVAR; dSP; dMARK; dTARGET;
    const int op_type = PL_op->op_type;
    I32 value;

    switch (op_type) {
    case OP_MSGSND:
	value = (I32)(do_msgsnd(MARK, SP) >= 0);
	break;
    case OP_MSGRCV:
	value = (I32)(do_msgrcv(MARK, SP) >= 0);
	break;
    case OP_SEMOP:
	value = (I32)(do_semop(MARK, SP) >= 0);
	break;
    default:
	value = (I32)(do_shmio(op_type, MARK, SP) >= 0);
	break;
    }

    SP = MARK;
    PUSHi(value);
    RETURN;
#else
    return pp_semget();
#endif
}

/* Semaphores. */

PP(pp_semget)
{
#if defined(HAS_MSG) || defined(HAS_SEM) || defined(HAS_SHM)
    dVAR; dSP; dMARK; dTARGET;
    const int anum = do_ipcget(PL_op->op_type, MARK, SP);
    SP = MARK;
    if (anum == -1)
	RETPUSHUNDEF;
    PUSHi(anum);
    RETURN;
#else
    DIE(aTHX_ "System V IPC is not implemented on this machine");
#endif
}

PP(pp_semctl)
{
#if defined(HAS_MSG) || defined(HAS_SEM) || defined(HAS_SHM)
    dVAR; dSP; dMARK; dTARGET;
    const int anum = do_ipcctl(PL_op->op_type, MARK, SP);
    SP = MARK;
    if (anum == -1)
	RETSETUNDEF;
    if (anum != 0) {
	PUSHi(anum);
    }
    else {
	PUSHp(zero_but_true, ZBTLEN);
    }
    RETURN;
#else
    return pp_semget();
#endif
}

/* I can't const this further without getting warnings about the types of
   various arrays passed in from structures.  */
static SV *
S_space_join_names_mortal(pTHX_ char *const *array)
{
    SV *target;

    if (array && *array) {
	target = sv_2mortal(newSVpvs(""));
	while (1) {
	    sv_catpv(target, *array);
	    if (!*++array)
		break;
	    sv_catpvs(target, " ");
	}
    } else {
	target = sv_mortalcopy(&PL_sv_no);
    }
    return target;
}

/* Get system info. */

PP(pp_ghostent)
{
#if defined(HAS_GETHOSTBYNAME) || defined(HAS_GETHOSTBYADDR) || defined(HAS_GETHOSTENT)
    dVAR; dSP;
    I32 which = PL_op->op_type;
    register char **elem;
    register SV *sv;
#ifndef HAS_GETHOST_PROTOS /* XXX Do we need individual probes? */
    struct hostent *gethostbyaddr(Netdb_host_t, Netdb_hlen_t, int);
    struct hostent *gethostbyname(Netdb_name_t);
    struct hostent *gethostent(void);
#endif
    struct hostent *hent;
    unsigned long len;

    EXTEND(SP, 10);
    if (which == OP_GHBYNAME) {
#ifdef HAS_GETHOSTBYNAME
	const char* const name = POPpbytex;
	hent = PerlSock_gethostbyname(name);
#else
	DIE(aTHX_ PL_no_sock_func, "gethostbyname");
#endif
    }
    else if (which == OP_GHBYADDR) {
#ifdef HAS_GETHOSTBYADDR
	const int addrtype = POPi;
	SV * const addrsv = POPs;
	STRLEN addrlen;
	const char *addr = (char *)SvPVbyte(addrsv, addrlen);

	hent = PerlSock_gethostbyaddr(addr, (Netdb_hlen_t) addrlen, addrtype);
#else
	DIE(aTHX_ PL_no_sock_func, "gethostbyaddr");
#endif
    }
    else
#ifdef HAS_GETHOSTENT
	hent = PerlSock_gethostent();
#else
	DIE(aTHX_ PL_no_sock_func, "gethostent");
#endif

#ifdef HOST_NOT_FOUND
	if (!hent) {
#ifdef USE_REENTRANT_API
#   ifdef USE_GETHOSTENT_ERRNO
	    h_errno = PL_reentrant_buffer->_gethostent_errno;
#   endif
#endif
	    STATUS_UNIX_SET(h_errno);
	}
#endif

    if (GIMME != G_ARRAY) {
	PUSHs(sv = sv_newmortal());
	if (hent) {
	    if (which == OP_GHBYNAME) {
		if (hent->h_addr)
		    sv_setpvn(sv, hent->h_addr, hent->h_length);
	    }
	    else
		sv_setpv(sv, (char*)hent->h_name);
	}
	RETURN;
    }

    if (hent) {
	PUSHs(sv_2mortal(newSVpv((char*)hent->h_name, 0)));
	PUSHs(space_join_names_mortal(hent->h_aliases));
	PUSHs(sv_2mortal(newSViv((IV)hent->h_addrtype)));
	len = hent->h_length;
	PUSHs(sv_2mortal(newSViv((IV)len)));
#ifdef h_addr
	for (elem = hent->h_addr_list; elem && *elem; elem++) {
	    XPUSHs(sv_2mortal(newSVpvn(*elem, len)));
	}
#else
	if (hent->h_addr)
	    PUSHs(newSVpvn(hent->h_addr, len));
	else
	    PUSHs(sv_mortalcopy(&PL_sv_no));
#endif /* h_addr */
    }
    RETURN;
#else
    DIE(aTHX_ PL_no_sock_func, "gethostent");
#endif
}

PP(pp_gnetent)
{
#if defined(HAS_GETNETBYNAME) || defined(HAS_GETNETBYADDR) || defined(HAS_GETNETENT)
    dVAR; dSP;
    I32 which = PL_op->op_type;
    register SV *sv;
#ifndef HAS_GETNET_PROTOS /* XXX Do we need individual probes? */
    struct netent *getnetbyaddr(Netdb_net_t, int);
    struct netent *getnetbyname(Netdb_name_t);
    struct netent *getnetent(void);
#endif
    struct netent *nent;

    if (which == OP_GNBYNAME){
#ifdef HAS_GETNETBYNAME
	const char * const name = POPpbytex;
	nent = PerlSock_getnetbyname(name);
#else
        DIE(aTHX_ PL_no_sock_func, "getnetbyname");
#endif
    }
    else if (which == OP_GNBYADDR) {
#ifdef HAS_GETNETBYADDR
	const int addrtype = POPi;
	const Netdb_net_t addr = (Netdb_net_t) (U32)POPu;
	nent = PerlSock_getnetbyaddr(addr, addrtype);
#else
	DIE(aTHX_ PL_no_sock_func, "getnetbyaddr");
#endif
    }
    else
#ifdef HAS_GETNETENT
	nent = PerlSock_getnetent();
#else
        DIE(aTHX_ PL_no_sock_func, "getnetent");
#endif

#ifdef HOST_NOT_FOUND
	if (!nent) {
#ifdef USE_REENTRANT_API
#   ifdef USE_GETNETENT_ERRNO
	     h_errno = PL_reentrant_buffer->_getnetent_errno;
#   endif
#endif
	    STATUS_UNIX_SET(h_errno);
	}
#endif

    EXTEND(SP, 4);
    if (GIMME != G_ARRAY) {
	PUSHs(sv = sv_newmortal());
	if (nent) {
	    if (which == OP_GNBYNAME)
		sv_setiv(sv, (IV)nent->n_net);
	    else
		sv_setpv(sv, nent->n_name);
	}
	RETURN;
    }

    if (nent) {
	PUSHs(sv_2mortal(newSVpv(nent->n_name, 0)));
	PUSHs(space_join_names_mortal(nent->n_aliases));
	PUSHs(sv_2mortal(newSViv((IV)nent->n_addrtype)));
	PUSHs(sv_2mortal(newSViv((IV)nent->n_net)));
    }

    RETURN;
#else
    DIE(aTHX_ PL_no_sock_func, "getnetent");
#endif
}

PP(pp_gprotoent)
{
#if defined(HAS_GETPROTOBYNAME) || defined(HAS_GETPROTOBYNUMBER) || defined(HAS_GETPROTOENT)
    dVAR; dSP;
    I32 which = PL_op->op_type;
    register SV *sv;
#ifndef HAS_GETPROTO_PROTOS /* XXX Do we need individual probes? */
    struct protoent *getprotobyname(Netdb_name_t);
    struct protoent *getprotobynumber(int);
    struct protoent *getprotoent(void);
#endif
    struct protoent *pent;

    if (which == OP_GPBYNAME) {
#ifdef HAS_GETPROTOBYNAME
	const char* const name = POPpbytex;
	pent = PerlSock_getprotobyname(name);
#else
	DIE(aTHX_ PL_no_sock_func, "getprotobyname");
#endif
    }
    else if (which == OP_GPBYNUMBER) {
#ifdef HAS_GETPROTOBYNUMBER
	const int number = POPi;
	pent = PerlSock_getprotobynumber(number);
#else
	DIE(aTHX_ PL_no_sock_func, "getprotobynumber");
#endif
    }
    else
#ifdef HAS_GETPROTOENT
	pent = PerlSock_getprotoent();
#else
	DIE(aTHX_ PL_no_sock_func, "getprotoent");
#endif

    EXTEND(SP, 3);
    if (GIMME != G_ARRAY) {
	PUSHs(sv = sv_newmortal());
	if (pent) {
	    if (which == OP_GPBYNAME)
		sv_setiv(sv, (IV)pent->p_proto);
	    else
		sv_setpv(sv, pent->p_name);
	}
	RETURN;
    }

    if (pent) {
	PUSHs(sv_2mortal(newSVpv(pent->p_name, 0)));
	PUSHs(space_join_names_mortal(pent->p_aliases));
	PUSHs(sv_2mortal(newSViv((IV)pent->p_proto)));
    }

    RETURN;
#else
    DIE(aTHX_ PL_no_sock_func, "getprotoent");
#endif
}

PP(pp_gservent)
{
#if defined(HAS_GETSERVBYNAME) || defined(HAS_GETSERVBYPORT) || defined(HAS_GETSERVENT)
    dVAR; dSP;
    I32 which = PL_op->op_type;
    register SV *sv;
#ifndef HAS_GETSERV_PROTOS /* XXX Do we need individual probes? */
    struct servent *getservbyname(Netdb_name_t, Netdb_name_t);
    struct servent *getservbyport(int, Netdb_name_t);
    struct servent *getservent(void);
#endif
    struct servent *sent;

    if (which == OP_GSBYNAME) {
#ifdef HAS_GETSERVBYNAME
	const char * const proto = POPpbytex;
	const char * const name = POPpbytex;
	sent = PerlSock_getservbyname(name, (proto && !*proto) ? NULL : proto);
#else
	DIE(aTHX_ PL_no_sock_func, "getservbyname");
#endif
    }
    else if (which == OP_GSBYPORT) {
#ifdef HAS_GETSERVBYPORT
	const char * const proto = POPpbytex;
	unsigned short port = (unsigned short)POPu;
#ifdef HAS_HTONS
	port = PerlSock_htons(port);
#endif
	sent = PerlSock_getservbyport(port, (proto && !*proto) ? NULL : proto);
#else
	DIE(aTHX_ PL_no_sock_func, "getservbyport");
#endif
    }
    else
#ifdef HAS_GETSERVENT
	sent = PerlSock_getservent();
#else
	DIE(aTHX_ PL_no_sock_func, "getservent");
#endif

    EXTEND(SP, 4);
    if (GIMME != G_ARRAY) {
	PUSHs(sv = sv_newmortal());
	if (sent) {
	    if (which == OP_GSBYNAME) {
#ifdef HAS_NTOHS
		sv_setiv(sv, (IV)PerlSock_ntohs(sent->s_port));
#else
		sv_setiv(sv, (IV)(sent->s_port));
#endif
	    }
	    else
		sv_setpv(sv, sent->s_name);
	}
	RETURN;
    }

    if (sent) {
	PUSHs(sv_2mortal(newSVpv(sent->s_name, 0)));
	PUSHs(space_join_names_mortal(sent->s_aliases));
#ifdef HAS_NTOHS
	PUSHs(sv_2mortal(newSViv((IV)PerlSock_ntohs(sent->s_port))));
#else
	PUSHs(sv_2mortal(newSViv((IV)(sent->s_port))));
#endif
	PUSHs(sv_2mortal(newSVpv(sent->s_proto, 0)));
    }

    RETURN;
#else
    DIE(aTHX_ PL_no_sock_func, "getservent");
#endif
}

PP(pp_shostent)
{
#ifdef HAS_SETHOSTENT
    dVAR; dSP;
    PerlSock_sethostent(TOPi);
    RETSETYES;
#else
    DIE(aTHX_ PL_no_sock_func, "sethostent");
#endif
}

PP(pp_snetent)
{
#ifdef HAS_SETNETENT
    dVAR; dSP;
    PerlSock_setnetent(TOPi);
    RETSETYES;
#else
    DIE(aTHX_ PL_no_sock_func, "setnetent");
#endif
}

PP(pp_sprotoent)
{
#ifdef HAS_SETPROTOENT
    dVAR; dSP;
    PerlSock_setprotoent(TOPi);
    RETSETYES;
#else
    DIE(aTHX_ PL_no_sock_func, "setprotoent");
#endif
}

PP(pp_sservent)
{
#ifdef HAS_SETSERVENT
    dVAR; dSP;
    PerlSock_setservent(TOPi);
    RETSETYES;
#else
    DIE(aTHX_ PL_no_sock_func, "setservent");
#endif
}

PP(pp_ehostent)
{
#ifdef HAS_ENDHOSTENT
    dVAR; dSP;
    PerlSock_endhostent();
    EXTEND(SP,1);
    RETPUSHYES;
#else
    DIE(aTHX_ PL_no_sock_func, "endhostent");
#endif
}

PP(pp_enetent)
{
#ifdef HAS_ENDNETENT
    dVAR; dSP;
    PerlSock_endnetent();
    EXTEND(SP,1);
    RETPUSHYES;
#else
    DIE(aTHX_ PL_no_sock_func, "endnetent");
#endif
}

PP(pp_eprotoent)
{
#ifdef HAS_ENDPROTOENT
    dVAR; dSP;
    PerlSock_endprotoent();
    EXTEND(SP,1);
    RETPUSHYES;
#else
    DIE(aTHX_ PL_no_sock_func, "endprotoent");
#endif
}

PP(pp_eservent)
{
#ifdef HAS_ENDSERVENT
    dVAR; dSP;
    PerlSock_endservent();
    EXTEND(SP,1);
    RETPUSHYES;
#else
    DIE(aTHX_ PL_no_sock_func, "endservent");
#endif
}

PP(pp_gpwent)
{
#ifdef HAS_PASSWD
    dVAR; dSP;
    I32 which = PL_op->op_type;
    register SV *sv;
    struct passwd *pwent  = NULL;
    /*
     * We currently support only the SysV getsp* shadow password interface.
     * The interface is declared in <shadow.h> and often one needs to link
     * with -lsecurity or some such.
     * This interface is used at least by Solaris, HP-UX, IRIX, and Linux.
     * (and SCO?)
     *
     * AIX getpwnam() is clever enough to return the encrypted password
     * only if the caller (euid?) is root.
     *
     * There are at least three other shadow password APIs.  Many platforms
     * seem to contain more than one interface for accessing the shadow
     * password databases, possibly for compatibility reasons.
     * The getsp*() is by far he simplest one, the other two interfaces
     * are much more complicated, but also very similar to each other.
     *
     * <sys/types.h>
     * <sys/security.h>
     * <prot.h>
     * struct pr_passwd *getprpw*();
     * The password is in
     * char getprpw*(...).ufld.fd_encrypt[]
     * Mention HAS_GETPRPWNAM here so that Configure probes for it.
     *
     * <sys/types.h>
     * <sys/security.h>
     * <prot.h>
     * struct es_passwd *getespw*();
     * The password is in
     * char *(getespw*(...).ufld.fd_encrypt)
     * Mention HAS_GETESPWNAM here so that Configure probes for it.
     *
     * <userpw.h> (AIX)
     * struct userpw *getuserpw();
     * The password is in
     * char *(getuserpw(...)).spw_upw_passwd
     * (but the de facto standard getpwnam() should work okay)
     *
     * Mention I_PROT here so that Configure probes for it.
     *
     * In HP-UX for getprpw*() the manual page claims that one should include
     * <hpsecurity.h> instead of <sys/security.h>, but that is not needed
     * if one includes <shadow.h> as that includes <hpsecurity.h>,
     * and pp_sys.c already includes <shadow.h> if there is such.
     *
     * Note that <sys/security.h> is already probed for, but currently
     * it is only included in special cases.
     *
     * In Digital UNIX/Tru64 if using the getespw*() (which seems to be
     * be preferred interface, even though also the getprpw*() interface
     * is available) one needs to link with -lsecurity -ldb -laud -lm.
     * One also needs to call set_auth_parameters() in main() before
     * doing anything else, whether one is using getespw*() or getprpw*().
     *
     * Note that accessing the shadow databases can be magnitudes
     * slower than accessing the standard databases.
     *
     * --jhi
     */

#   if defined(__CYGWIN__) && defined(USE_REENTRANT_API)
    /* Cygwin 1.5.3-1 has buggy getpwnam_r() and getpwuid_r():
     * the pw_comment is left uninitialized. */
    PL_reentrant_buffer->_pwent_struct.pw_comment = NULL;
#   endif

    switch (which) {
    case OP_GPWNAM:
      {
	const char* const name = POPpbytex;
	pwent  = getpwnam(name);
      }
      break;
    case OP_GPWUID:
      {
	Uid_t uid = POPi;
	pwent = getpwuid(uid);
      }
	break;
    case OP_GPWENT:
#   ifdef HAS_GETPWENT
	pwent  = getpwent();
#ifdef POSIX_BC   /* In some cases pw_passwd has invalid addresses */
	if (pwent) pwent = getpwnam(pwent->pw_name);
#endif
#   else
	DIE(aTHX_ PL_no_func, "getpwent");
#   endif
	break;
    }

    EXTEND(SP, 10);
    if (GIMME != G_ARRAY) {
	PUSHs(sv = sv_newmortal());
	if (pwent) {
	    if (which == OP_GPWNAM)
#   if Uid_t_sign <= 0
		sv_setiv(sv, (IV)pwent->pw_uid);
#   else
		sv_setuv(sv, (UV)pwent->pw_uid);
#   endif
	    else
		sv_setpv(sv, pwent->pw_name);
	}
	RETURN;
    }

    if (pwent) {
	PUSHs(sv_2mortal(newSVpv(pwent->pw_name, 0)));

	PUSHs(sv = sv_2mortal(newSViv(0)));
	/* If we have getspnam(), we try to dig up the shadow
	 * password.  If we are underprivileged, the shadow
	 * interface will set the errno to EACCES or similar,
	 * and return a null pointer.  If this happens, we will
	 * use the dummy password (usually "*" or "x") from the
	 * standard password database.
	 *
	 * In theory we could skip the shadow call completely
	 * if euid != 0 but in practice we cannot know which
	 * security measures are guarding the shadow databases
	 * on a random platform.
	 *
	 * Resist the urge to use additional shadow interfaces.
	 * Divert the urge to writing an extension instead.
	 *
	 * --jhi */
	/* Some AIX setups falsely(?) detect some getspnam(), which
	 * has a different API than the Solaris/IRIX one. */
#   if defined(HAS_GETSPNAM) && !defined(_AIX)
	{
	    const int saverrno = errno;
	    const struct spwd * const spwent = getspnam(pwent->pw_name);
			  /* Save and restore errno so that
			   * underprivileged attempts seem
			   * to have never made the unsccessful
			   * attempt to retrieve the shadow password. */
	    errno = saverrno;
	    if (spwent && spwent->sp_pwdp)
		sv_setpv(sv, spwent->sp_pwdp);
	}
#   endif
#   ifdef PWPASSWD
	if (!SvPOK(sv)) /* Use the standard password, then. */
	    sv_setpv(sv, pwent->pw_passwd);
#   endif

#   ifndef INCOMPLETE_TAINTS
	/* passwd is tainted because user himself can diddle with it.
	 * admittedly not much and in a very limited way, but nevertheless. */
	SvTAINTED_on(sv);
#   endif

#   if Uid_t_sign <= 0
	PUSHs(sv_2mortal(newSViv((IV)pwent->pw_uid)));
#   else
	PUSHs(sv_2mortal(newSVuv((UV)pwent->pw_uid)));
#   endif

#   if Uid_t_sign <= 0
	PUSHs(sv_2mortal(newSViv((IV)pwent->pw_gid)));
#   else
	PUSHs(sv_2mortal(newSVuv((UV)pwent->pw_gid)));
#   endif
	/* pw_change, pw_quota, and pw_age are mutually exclusive--
	 * because of the poor interface of the Perl getpw*(),
	 * not because there's some standard/convention saying so.
	 * A better interface would have been to return a hash,
	 * but we are accursed by our history, alas. --jhi.  */
#   ifdef PWCHANGE
	PUSHs(sv_2mortal(newSViv((IV)pwent->pw_change)));
#   else
#       ifdef PWQUOTA
	PUSHs(sv_2mortal(newSViv((IV)pwent->pw_quota)));
#       else
#           ifdef PWAGE
	PUSHs(sv_2mortal(newSVpv(pwent->pw_age, 0)));
#	    else
	/* I think that you can never get this compiled, but just in case.  */
	PUSHs(sv_mortalcopy(&PL_sv_no));
#           endif
#       endif
#   endif

	/* pw_class and pw_comment are mutually exclusive--.
	 * see the above note for pw_change, pw_quota, and pw_age. */
#   ifdef PWCLASS
	PUSHs(sv_2mortal(newSVpv(pwent->pw_class, 0)));
#   else
#       ifdef PWCOMMENT
	PUSHs(sv_2mortal(newSVpv(pwent->pw_comment, 0)));
#	else
	/* I think that you can never get this compiled, but just in case.  */
	PUSHs(sv_mortalcopy(&PL_sv_no));
#       endif
#   endif

#   ifdef PWGECOS
	PUSHs(sv = sv_2mortal(newSVpv(pwent->pw_gecos, 0)));
#   else
	PUSHs(sv = sv_mortalcopy(&PL_sv_no));
#   endif
#   ifndef INCOMPLETE_TAINTS
	/* pw_gecos is tainted because user himself can diddle with it. */
	SvTAINTED_on(sv);
#   endif

	PUSHs(sv_2mortal(newSVpv(pwent->pw_dir, 0)));

	PUSHs(sv = sv_2mortal(newSVpv(pwent->pw_shell, 0)));
#   ifndef INCOMPLETE_TAINTS
	/* pw_shell is tainted because user himself can diddle with it. */
	SvTAINTED_on(sv);
#   endif

#   ifdef PWEXPIRE
	PUSHs(sv_2mortal(newSViv((IV)pwent->pw_expire)));
#   endif
    }
    RETURN;
#else
    DIE(aTHX_ PL_no_func, PL_op_desc[PL_op->op_type]);
#endif
}

PP(pp_spwent)
{
#if defined(HAS_PASSWD) && defined(HAS_SETPWENT)
    dVAR; dSP;
    setpwent();
    RETPUSHYES;
#else
    DIE(aTHX_ PL_no_func, "setpwent");
#endif
}

PP(pp_epwent)
{
#if defined(HAS_PASSWD) && defined(HAS_ENDPWENT)
    dVAR; dSP;
    endpwent();
    RETPUSHYES;
#else
    DIE(aTHX_ PL_no_func, "endpwent");
#endif
}

PP(pp_ggrent)
{
#ifdef HAS_GROUP
    dVAR; dSP;
    const I32 which = PL_op->op_type;
    const struct group *grent;

    if (which == OP_GGRNAM) {
	const char* const name = POPpbytex;
	grent = (const struct group *)getgrnam(name);
    }
    else if (which == OP_GGRGID) {
	const Gid_t gid = POPi;
	grent = (const struct group *)getgrgid(gid);
    }
    else
#ifdef HAS_GETGRENT
	grent = (struct group *)getgrent();
#else
        DIE(aTHX_ PL_no_func, "getgrent");
#endif

    EXTEND(SP, 4);
    if (GIMME != G_ARRAY) {
	SV * const sv = sv_newmortal();

	PUSHs(sv);
	if (grent) {
	    if (which == OP_GGRNAM)
		sv_setiv(sv, (IV)grent->gr_gid);
	    else
		sv_setpv(sv, grent->gr_name);
	}
	RETURN;
    }

    if (grent) {
	PUSHs(sv_2mortal(newSVpv(grent->gr_name, 0)));

#ifdef GRPASSWD
	PUSHs(sv_2mortal(newSVpv(grent->gr_passwd, 0)));
#else
	PUSHs(sv_mortalcopy(&PL_sv_no));
#endif

	PUSHs(sv_2mortal(newSViv((IV)grent->gr_gid)));

#if !(defined(_CRAYMPP) && defined(USE_REENTRANT_API))
	/* In UNICOS/mk (_CRAYMPP) the multithreading
	 * versions (getgrnam_r, getgrgid_r)
	 * seem to return an illegal pointer
	 * as the group members list, gr_mem.
	 * getgrent() doesn't even have a _r version
	 * but the gr_mem is poisonous anyway.
	 * So yes, you cannot get the list of group
	 * members if building multithreaded in UNICOS/mk. */
	PUSHs(space_join_names_mortal(grent->gr_mem));
#endif
    }

    RETURN;
#else
    DIE(aTHX_ PL_no_func, PL_op_desc[PL_op->op_type]);
#endif
}

PP(pp_sgrent)
{
#if defined(HAS_GROUP) && defined(HAS_SETGRENT)
    dVAR; dSP;
    setgrent();
    RETPUSHYES;
#else
    DIE(aTHX_ PL_no_func, "setgrent");
#endif
}

PP(pp_egrent)
{
#if defined(HAS_GROUP) && defined(HAS_ENDGRENT)
    dVAR; dSP;
    endgrent();
    RETPUSHYES;
#else
    DIE(aTHX_ PL_no_func, "endgrent");
#endif
}

PP(pp_getlogin)
{
#ifdef HAS_GETLOGIN
    dVAR; dSP; dTARGET;
    char *tmps;
    EXTEND(SP, 1);
    if (!(tmps = PerlProc_getlogin()))
	RETPUSHUNDEF;
    PUSHp(tmps, strlen(tmps));
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "getlogin");
#endif
}

/* Miscellaneous. */

PP(pp_syscall)
{
#ifdef HAS_SYSCALL
    dVAR; dSP; dMARK; dORIGMARK; dTARGET;
    register I32 items = SP - MARK;
    unsigned long a[20];
    register I32 i = 0;
    I32 retval = -1;

    if (PL_tainting) {
	while (++MARK <= SP) {
	    if (SvTAINTED(*MARK)) {
		TAINT;
		break;
	    }
	}
	MARK = ORIGMARK;
	TAINT_PROPER("syscall");
    }

    /* This probably won't work on machines where sizeof(long) != sizeof(int)
     * or where sizeof(long) != sizeof(char*).  But such machines will
     * not likely have syscall implemented either, so who cares?
     */
    while (++MARK <= SP) {
	if (SvNIOK(*MARK) || !i)
	    a[i++] = SvIV(*MARK);
	else if (*MARK == &PL_sv_undef)
	    a[i++] = 0;
	else
	    a[i++] = (unsigned long)SvPV_force_nolen(*MARK);
	if (i > 15)
	    break;
    }
    switch (items) {
    default:
	DIE(aTHX_ "Too many args to syscall");
    case 0:
	DIE(aTHX_ "Too few args to syscall");
    case 1:
	retval = syscall(a[0]);
	break;
    case 2:
	retval = syscall(a[0],a[1]);
	break;
    case 3:
	retval = syscall(a[0],a[1],a[2]);
	break;
    case 4:
	retval = syscall(a[0],a[1],a[2],a[3]);
	break;
    case 5:
	retval = syscall(a[0],a[1],a[2],a[3],a[4]);
	break;
    case 6:
	retval = syscall(a[0],a[1],a[2],a[3],a[4],a[5]);
	break;
    case 7:
	retval = syscall(a[0],a[1],a[2],a[3],a[4],a[5],a[6]);
	break;
    case 8:
	retval = syscall(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]);
	break;
#ifdef atarist
    case 9:
	retval = syscall(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]);
	break;
    case 10:
	retval = syscall(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9]);
	break;
    case 11:
	retval = syscall(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],
	  a[10]);
	break;
    case 12:
	retval = syscall(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],
	  a[10],a[11]);
	break;
    case 13:
	retval = syscall(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],
	  a[10],a[11],a[12]);
	break;
    case 14:
	retval = syscall(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],
	  a[10],a[11],a[12],a[13]);
	break;
#endif /* atarist */
    }
    SP = ORIGMARK;
    PUSHi(retval);
    RETURN;
#else
    DIE(aTHX_ PL_no_func, "syscall");
#endif
}

#ifdef FCNTL_EMULATE_FLOCK

/*  XXX Emulate flock() with fcntl().
    What's really needed is a good file locking module.
*/

static int
fcntl_emulate_flock(int fd, int operation)
{
    struct flock flock;

    switch (operation & ~LOCK_NB) {
    case LOCK_SH:
	flock.l_type = F_RDLCK;
	break;
    case LOCK_EX:
	flock.l_type = F_WRLCK;
	break;
    case LOCK_UN:
	flock.l_type = F_UNLCK;
	break;
    default:
	errno = EINVAL;
	return -1;
    }
    flock.l_whence = SEEK_SET;
    flock.l_start = flock.l_len = (Off_t)0;

    return fcntl(fd, (operation & LOCK_NB) ? F_SETLK : F_SETLKW, &flock);
}

#endif /* FCNTL_EMULATE_FLOCK */

#ifdef LOCKF_EMULATE_FLOCK

/*  XXX Emulate flock() with lockf().  This is just to increase
    portability of scripts.  The calls are not completely
    interchangeable.  What's really needed is a good file
    locking module.
*/

/*  The lockf() constants might have been defined in <unistd.h>.
    Unfortunately, <unistd.h> causes troubles on some mixed
    (BSD/POSIX) systems, such as SunOS 4.1.3.

   Further, the lockf() constants aren't POSIX, so they might not be
   visible if we're compiling with _POSIX_SOURCE defined.  Thus, we'll
   just stick in the SVID values and be done with it.  Sigh.
*/

# ifndef F_ULOCK
#  define F_ULOCK	0	/* Unlock a previously locked region */
# endif
# ifndef F_LOCK
#  define F_LOCK	1	/* Lock a region for exclusive use */
# endif
# ifndef F_TLOCK
#  define F_TLOCK	2	/* Test and lock a region for exclusive use */
# endif
# ifndef F_TEST
#  define F_TEST	3	/* Test a region for other processes locks */
# endif

static int
lockf_emulate_flock(int fd, int operation)
{
    int i;
    const int save_errno = errno;
    Off_t pos;

    /* flock locks entire file so for lockf we need to do the same	*/
    pos = PerlLIO_lseek(fd, (Off_t)0, SEEK_CUR);    /* get pos to restore later */
    if (pos > 0)	/* is seekable and needs to be repositioned	*/
	if (PerlLIO_lseek(fd, (Off_t)0, SEEK_SET) < 0)
	    pos = -1;	/* seek failed, so don't seek back afterwards	*/
    errno = save_errno;

    switch (operation) {

	/* LOCK_SH - get a shared lock */
	case LOCK_SH:
	/* LOCK_EX - get an exclusive lock */
	case LOCK_EX:
	    i = lockf (fd, F_LOCK, 0);
	    break;

	/* LOCK_SH|LOCK_NB - get a non-blocking shared lock */
	case LOCK_SH|LOCK_NB:
	/* LOCK_EX|LOCK_NB - get a non-blocking exclusive lock */
	case LOCK_EX|LOCK_NB:
	    i = lockf (fd, F_TLOCK, 0);
	    if (i == -1)
		if ((errno == EAGAIN) || (errno == EACCES))
		    errno = EWOULDBLOCK;
	    break;

	/* LOCK_UN - unlock (non-blocking is a no-op) */
	case LOCK_UN:
	case LOCK_UN|LOCK_NB:
	    i = lockf (fd, F_ULOCK, 0);
	    break;

	/* Default - can't decipher operation */
	default:
	    i = -1;
	    errno = EINVAL;
	    break;
    }

    if (pos > 0)      /* need to restore position of the handle	*/
	PerlLIO_lseek(fd, pos, SEEK_SET);	/* ignore error here	*/

    return (i);
}

#endif /* LOCKF_EMULATE_FLOCK */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 *
 * ex: set ts=8 sts=4 sw=4 noet:
 */
