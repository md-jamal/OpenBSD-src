/* Configuration for an OpenBSD i386 target.
   Copyright (C) 1999, 2000, 2002 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#define TARGET_VERSION fprintf (stderr, " (OpenBSD/i386)");

/* This goes away when the math-emulator is fixed */
#undef TARGET_SUBTARGET_DEFAULT
#define TARGET_SUBTARGET_DEFAULT \
  (MASK_80387 | MASK_IEEE_FP | MASK_FLOAT_RETURNS | MASK_NO_FANCY_MATH_387)

#define TARGET_OS_CPP_BUILTINS()		\
  do						\
    {						\
    	OPENBSD_OS_CPP_BUILTINS_COMMON();	\
	builtin_define ("__unix__");		\
	builtin_define ("__OpenBSD__");		\
	builtin_assert ("system=unix");		\
	builtin_assert ("system=bsd");		\
	builtin_assert ("system=OpenBSD");	\
    }						\
  while (0)

/* Layout of source language data types.  */

/* This must agree with <machine/ansi.h> */
#undef SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

/* Assembler format: overall framework.  */

#undef ASM_APP_ON
#define ASM_APP_ON "#APP\n"

#undef ASM_APP_OFF
#define ASM_APP_OFF "#NO_APP\n"

/* Stack & calling: aggregate returns.  */

/* Don't default to pcc-struct-return, because gcc is the only compiler, and
   we want to retain compatibility with older gcc versions.  */
#define DEFAULT_PCC_STRUCT_RETURN 0

/* Assembler format: alignment output.  */

/* Kludgy test: when gas is upgraded, it will have p2align, and no problems
   with nops.  */
#ifndef HAVE_GAS_MAX_SKIP_P2ALIGN
/* i386 OpenBSD still uses an older gas that doesn't insert nops by default
   when the .align directive demands to insert extra space in the text
   segment.  */
#undef ASM_OUTPUT_ALIGN
#define ASM_OUTPUT_ALIGN(FILE,LOG) \
  if ((LOG)!=0) fprintf ((FILE), "\t.align %d,0x90\n", (LOG))
#endif

/* Stack & calling: profiling.  */

/* OpenBSD's profiler recovers all information from the stack pointer.
   The icky part is not here, but in machine/profile.h.  */
#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO)  \
  fputs (flag_pic ? "\tcall mcount@PLT\n": "\tcall mcount\n", FILE);

/* Assembler format: exception region output.  */

/* All configurations that don't use elf must be explicit about not using
   dwarf unwind information. egcs doesn't try too hard to check internal
   configuration files...  */
#define DWARF2_UNWIND_INFO 0

#undef ASM_PREFERRED_EH_DATA_FORMAT

#undef ASM_COMMENT_START
#define ASM_COMMENT_START ";#"

/* OpenBSD gas currently does not support quad, so do not use it.  */
#undef ASM_QUAD

#define TRANSFER_FROM_TRAMPOLINE					\
extern void __enable_execute_stack (void *);				\
void									\
__enable_execute_stack (addr)						\
     void *addr;							\
{									\
  long size = getpagesize ();						\
  long mask = ~(size-1);						\
  char *page = (char *) (((long) addr) & mask); 			\
  char *end  = (char *) ((((long) (addr + TRAMPOLINE_SIZE)) & mask) + size); \
								      \
  /* 7 is PROT_READ | PROT_WRITE | PROT_EXEC */ 			\
  if (mprotect (page, end - page, 7) < 0)				\
    perror ("mprotect of trampoline code");				\
}
