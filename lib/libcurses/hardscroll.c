/*	$OpenBSD: hardscroll.c,v 1.6 1997/12/03 05:21:08 millert Exp $	*/


/***************************************************************************
*                            COPYRIGHT NOTICE                              *
****************************************************************************
*                ncurses is copyright (C) 1992-1995                        *
*                          Zeyd M. Ben-Halim                               *
*                          zmbenhal@netcom.com                             *
*                          Eric S. Raymond                                 *
*                          esr@snark.thyrsus.com                           *
*                                                                          *
*        Permission is hereby granted to reproduce and distribute ncurses  *
*        by any means and for any fee, whether alone or as part of a       *
*        larger distribution, in source or in binary form, PROVIDED        *
*        this notice is included with any such distribution, and is not    *
*        removed from any of its header files. Mention of ncurses in any   *
*        applications linked with it is highly appreciated.                *
*                                                                          *
*        ncurses comes AS IS with no warranty, implied or expressed.       *
*                                                                          *
***************************************************************************/


/******************************************************************************

NAME
   hardscroll.c -- hardware-scrolling optimization for ncurses

SYNOPSIS
   void _nc_scroll_optimize(void)

DESCRIPTION
			OVERVIEW

This algorithm for computes optimum hardware scrolling to transform an
old screen (curscr) into a new screen (newscr) via vertical line moves.

Because the screen has a `grain' (there are insert/delete/scroll line
operations but no insert/delete/scroll column operations), it is efficient
break the update algorithm into two pieces: a first stage that does only line
moves, optimizing the end product of user-invoked insertions, deletions, and
scrolls; and a second phase (corresponding to the present doupdate code in
ncurses) that does only line transformations.

The common case we want hardware scrolling for is to handle line insertions
and deletions in screen-oriented text-editors.  This two-stage approach will
accomplish that at a low computation and code-size cost.

			LINE-MOVE COMPUTATION

Now, to a discussion of the line-move computation.

For expository purposes, consider the screen lines to be represented by
integers 0..23 (with the understanding that the value of 23 may vary).
Let a new line introduced by insertion, scrolling, or at the bottom of
the screen following a line delete be given the index -1.

Assume that the real screen starts with lines 0..23.  Now, we have
the following possible line-oriented operations on the screen:

Insertion: inserts a line at a given screen row, forcing all lines below
to scroll forward.  The last screen line is lost.  For example, an insertion
at line 5 would produce: 0..4 -1 5..23.

Deletion: deletes a line at a given screen row, forcing all lines below
to scroll forward.  The last screen line is made new.  For example, a deletion
at line 7 would produce: 0..6 8..23 -1.

Scroll up: move a range of lines up 1.  The bottom line of the range
becomes new.  For example, scrolling up the region from 9 to 14 will
produce 0..8 10..14 -1 15..23.

Scroll down: move a range of lines down 1.  The top line of the range
becomes new.  For example, scrolling down the region from 12 to 16 will produce
0..11 -1 12..15 17..23.

Now, an obvious property of all these operations is that they preserve the
order of old lines, though not their position in the sequence.

The key trick of this algorithm is that the original line indices described
above are actually maintained as _line[].oldindex fields in the window
structure, and stick to each line through scroll and insert/delete operations.

Thus, it is possible at update time to look at the oldnum fields and compute
an optimal set of il/dl/scroll operations that will take the real screen
lines to the virtual screen lines.  Once these vertical moves have been done,
we can hand off to the second stage of the update algorithm, which does line
transformations.

Note that the move computation does not need to have the full generality
of a diff algorithm (which it superficially resembles) because lines cannot
be moved out of order.

			THE ALGORITHM

The scrolling is done in two passes. The first pass is from top to bottom
scroling hunks UP. The second one is from bottom to top scrolling hunks DOWN.
Obviously enough, no lines to be scrolled will be destroyed. (lav)

HOW TO TEST THIS:

Use the following production:

hardscroll: hardscroll.c
	$(CC) -g -DSCROLLDEBUG hardscroll.c -o hardscroll

Then just type scramble vectors and watch.  The following test loads are
a representative sample of cases:

-----------------------------  CUT HERE ------------------------------------
# No lines moved
 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
#
# A scroll up
 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 -1
#
# A scroll down
-1  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22
#
# An insertion (after line 12)
 0  1  2  3  4  5  6  7  8  9 10 11 12 -1 13 14 15 16 17 18 19 20 21 22
#
# A simple deletion (line 10)
 0  1  2  3  4  5  6  7  8  9  11 12 13 14 15 16 17 18 19 20 21 22 23 -1
#
# A more complex case
-1 -1 -1 -1 -1  3  4  5  6  7  -1 -1  8  9 10 11 12 13 14 15 16 17 -1 -1
-----------------------------  CUT HERE ------------------------------------

AUTHOR
    Eric S. Raymond <esr@snark.thyrsus.com>, November 1994
    New algorithm by Alexander V. Lukyanov <lav@yars.free.net>, Aug 1997

*****************************************************************************/

#include <curses.priv.h>

MODULE_ID("Id: hardscroll.c,v 1.28 1997/10/26 21:56:41 tom Exp $")

#if defined(SCROLLDEBUG) || defined(HASHDEBUG)
int oldnums[MAXLINES];
#define OLDNUM(n)	oldnums[n]
#undef T
#define T(x)		(void) printf x ; (void) putchar('\n');
#else
#include <curses.h>
#define OLDNUM(n)	newscr->_line[n].oldindex
#ifndef _NEWINDEX
#define _NEWINDEX	-1
#endif /* _NEWINDEX */
#endif /* defined(SCROLLDEBUG) || defined(HASHDEBUG) */


void _nc_scroll_optimize(void)
/* scroll optimization to transform curscr to newscr */
{
    int i;
    int start, end, shift;

    TR(TRACE_ICALLS, ("_nc_scroll_optimize() begins"));

#ifdef TRACE
    if (_nc_tracing & (TRACE_UPDATE | TRACE_MOVE))
	_nc_linedump();
#endif /* TRACE */

    /* pass 1 - from top to bottom scrolling up */
    for (i = 0; i < screen_lines; )
    {
	while (i < screen_lines && (OLDNUM(i) == _NEWINDEX || OLDNUM(i) <= i))
	    i++;
	if (i >= screen_lines)
	    break;
	    
	shift = OLDNUM(i) - i; /* shift > 0 */
	start = i;
	
	i++;
	while (i < screen_lines && OLDNUM(i) != _NEWINDEX && OLDNUM(i) - i == shift)
	    i++;
	end = i-1 + shift;

	TR(TRACE_UPDATE | TRACE_MOVE, ("scroll [%d, %d] by %d", start, end, shift));
#if !defined(SCROLLDEBUG) && !defined(HASHDEBUG)
	if (_nc_scrolln(shift, start, end, screen_lines - 1) == ERR)
	{
	    TR(TRACE_UPDATE | TRACE_MOVE, ("unable to scroll"));
	    continue;
	}
#endif /* !defined(SCROLLDEBUG) && !defined(HASHDEBUG) */
    }

    /* pass 2 - from bottom to top scrolling down */
    for (i = screen_lines-1; i >= 0; )
    {
	while (i >= 0 && (OLDNUM(i) == _NEWINDEX || OLDNUM(i) >= i))
	    i--;
	if (i < 0)
	    break;
	    
	shift = OLDNUM(i) - i; /* shift < 0 */
	end = i;
	
	i--;
	while (i >= 0 && OLDNUM(i) != _NEWINDEX && OLDNUM(i) - i == shift)
	    i--;
	start = i+1 - (-shift);

	TR(TRACE_UPDATE | TRACE_MOVE, ("scroll [%d, %d] by %d", start, end, shift));
#if !defined(SCROLLDEBUG) && !defined(HASHDEBUG)
	if (_nc_scrolln(shift, start, end, screen_lines - 1) == ERR)
	{
	    TR(TRACE_UPDATE | TRACE_MOVE, ("unable to scroll"));
	    continue;
	}
#endif /* !defined(SCROLLDEBUG) && !defined(HASHDEBUG) */
    }
}

#if defined(TRACE) || defined(SCROLLDEBUG)
void _nc_linedump(void)
/* dump the state of the real and virtual oldnum fields */
{
    static size_t have;
    static char *buf;

    int	n;
    size_t	want = (screen_lines + 1) * 4;

    if (have < want)
	buf = malloc(have = want);

    (void) strcpy(buf, "virt");
    for (n = 0; n < screen_lines; n++)
	(void) sprintf(buf + strlen(buf), " %02d", OLDNUM(n));
    TR(TRACE_UPDATE | TRACE_MOVE, (buf));
#if NO_LEAKS
    free(buf);
    have = 0;
#endif
}
#endif /* defined(TRACE) || defined(SCROLLDEBUG) */

#ifdef SCROLLDEBUG

int
main(int argc GCC_UNUSED, char *argv[] GCC_UNUSED)
{
    char	line[BUFSIZ], *st;

#ifdef TRACE
    _nc_tracing = TRACE_MOVE;
#endif
    for (;;)
    {
	int	n;

	for (n = 0; n < screen_lines; n++)
	    oldnums[n] = _NEWINDEX;

	/* grab the test vector */
	if (fgets(line, sizeof(line), stdin) == (char *)NULL)
	    exit(EXIT_SUCCESS);

	/* parse it */
	n = 0;
	if (line[0] == '#')
	{
	    (void) fputs(line, stderr);
	    continue;
	}
	st = strtok(line, " ");
	do {
	    oldnums[n++] = atoi(st);
	} while
	    ((st = strtok((char *)NULL, " ")) != 0);

	/* display it */
	(void) fputs("Initial input:\n", stderr);
	_nc_linedump();

	_nc_scroll_optimize();
    }
}

#endif /* SCROLLDEBUG */

/* hardscroll.c ends here */
