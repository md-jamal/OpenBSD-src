/* Language lexer for the GNU compiler for the Java(TM) language.
   Copyright (C) 1997, 1998, 1999 Free Software Foundation, Inc.
   Contributed by Alexandre Petit-Bianco (apbianco@cygnus.com)

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
Boston, MA 02111-1307, USA. 

Java and all Java-based marks are trademarks or registered trademarks
of Sun Microsystems, Inc. in the United States and other countries.
The Free Software Foundation is independent of Sun Microsystems, Inc.  */

/* It defines java_lex (yylex) that reads a Java ASCII source file
possibly containing Unicode escape sequence or utf8 encoded characters
and returns a token for everything found but comments, white spaces
and line terminators. When necessary, it also fills the java_lval
(yylval) union. It's implemented to be called by a re-entrant parser
generated by Bison.

The lexical analysis conforms to the Java grammar described in "The
Java(TM) Language Specification. J. Gosling, B. Joy, G. Steele.
Addison Wesley 1996" (http://java.sun.com/docs/books/jls/html/3.doc.html)  */

#include "keyword.h"

#ifndef JC1_LITE
extern struct obstack *expression_obstack;
#endif

/* Function declaration  */
static int java_lineterminator PROTO ((unicode_t));
static char *java_sprint_unicode PROTO ((struct java_line *, int));
static void java_unicode_2_utf8 PROTO ((unicode_t));
static void java_lex_error PROTO ((char *, int));
#ifndef JC1_LITE
static int java_is_eol PROTO ((FILE *, int));
static tree build_wfl_node PROTO ((tree));
#endif
static void java_store_unicode PROTO ((struct java_line *, unicode_t, int));
static unicode_t java_parse_escape_sequence PROTO ((void));
static int java_letter_or_digit_p PROTO ((unicode_t));
static int java_parse_doc_section PROTO ((unicode_t));
static void java_parse_end_comment PROTO ((unicode_t));
static unicode_t java_get_unicode PROTO (());
static unicode_t java_read_unicode PROTO ((int, int *));
static void java_store_unicode PROTO ((struct java_line *, unicode_t, int));
static unicode_t java_read_char PROTO (());
static void java_allocate_new_line PROTO (());
static void java_unget_unicode PROTO (());
static unicode_t java_sneak_unicode PROTO (());

void
java_init_lex ()
{
#ifndef JC1_LITE
  int java_lang_imported = 0;

  if (!java_lang_id)
    java_lang_id = get_identifier ("java.lang");
  if (!java_lang_cloneable)
    java_lang_cloneable = get_identifier ("java.lang.Cloneable");

  if (!java_lang_imported)
    {
      tree node = build_tree_list 
	(build_expr_wfl (java_lang_id, NULL, 0, 0), NULL_TREE);
      read_import_dir (TREE_PURPOSE (node));
      TREE_CHAIN (node) = ctxp->import_demand_list;
      ctxp->import_demand_list = node;
      java_lang_imported = 1;
    }

  if (!wfl_operator)
    wfl_operator = build_expr_wfl (NULL_TREE, ctxp->filename, 0, 0);
  if (!label_id)
    label_id = get_identifier ("$L");
  if (!wfl_append) 
    wfl_append = build_expr_wfl (get_identifier ("append"), NULL, 0, 0);
  if (!wfl_string_buffer)
    wfl_string_buffer = 
      build_expr_wfl (get_identifier ("java.lang.StringBuffer"), NULL, 0, 0);
  if (!wfl_to_string)
    wfl_to_string = build_expr_wfl (get_identifier ("toString"), NULL, 0, 0);

  ctxp->static_initialized = ctxp->non_static_initialized = 
    ctxp->incomplete_class = NULL_TREE;
  
  bzero ((PTR) ctxp->modifier_ctx, 11*sizeof (ctxp->modifier_ctx[0]));
  bzero ((PTR) current_jcf, sizeof (JCF));
  ctxp->current_parsed_class = NULL;
  ctxp->package = NULL_TREE;
#endif

  ctxp->filename = input_filename;
  ctxp->lineno = lineno = 0;
  ctxp->p_line = NULL;
  ctxp->c_line = NULL;
  ctxp->unget_utf8_value = 0;
  ctxp->minus_seen = 0;
  ctxp->java_error_flag = 0;
}

static char *
java_sprint_unicode (line, i)
    struct java_line *line;
    int i;
{
  static char buffer [10];
  if (line->unicode_escape_p [i] || line->line [i] > 128)
    sprintf (buffer, "\\u%04x", line->line [i]);
  else
    {
      buffer [0] = line->line [i];
      buffer [1] = '\0';
    }
  return buffer;
}

static unicode_t
java_sneak_unicode ()
{
  return (ctxp->c_line->line [ctxp->c_line->current]);
}

static void
java_unget_unicode ()
{
  if (!ctxp->c_line->current)
    fatal ("can't unget unicode - java_unget_unicode");
  ctxp->c_line->current--;
  ctxp->c_line->char_col -= JAVA_COLUMN_DELTA (0);
}

static void
java_allocate_new_line ()
{
  unicode_t ahead = (ctxp->c_line ? ctxp->c_line->ahead[0] : '\0');
  char ahead_escape_p = (ctxp->c_line ? 
			 ctxp->c_line->unicode_escape_ahead_p : 0);

  if (ctxp->c_line && !ctxp->c_line->white_space_only)
    {
      if (ctxp->p_line)
	{
	  free (ctxp->p_line->unicode_escape_p);
	  free (ctxp->p_line->line);
	  free (ctxp->p_line);
	}
      ctxp->p_line = ctxp->c_line;
      ctxp->c_line = NULL;		/* Reallocated */
    }

  if (!ctxp->c_line)
    {
      ctxp->c_line = (struct java_line *)xmalloc (sizeof (struct java_line));
      ctxp->c_line->max = JAVA_LINE_MAX;
      ctxp->c_line->line = (unicode_t *)xmalloc 
	(sizeof (unicode_t)*ctxp->c_line->max);
      ctxp->c_line->unicode_escape_p = 
	  (char *)xmalloc (sizeof (char)*ctxp->c_line->max);
      ctxp->c_line->white_space_only = 0;
    }

  ctxp->c_line->line [0] = ctxp->c_line->size = 0;
  ctxp->c_line->char_col = ctxp->c_line->current = 0;
  if (ahead)
    {
      ctxp->c_line->line [ctxp->c_line->size] = ahead;
      ctxp->c_line->unicode_escape_p [ctxp->c_line->size] = ahead_escape_p;
      ctxp->c_line->size++;
    }
  ctxp->c_line->ahead [0] = 0;
  ctxp->c_line->unicode_escape_ahead_p = 0;
  ctxp->c_line->lineno = ++lineno;
  ctxp->c_line->white_space_only = 1;
}

#define BAD_UTF8_VALUE 0xFFFE

static unicode_t
java_read_char ()
{
  int c;
  int c1, c2;

  if (ctxp->unget_utf8_value)
    {
      int to_return = ctxp->unget_utf8_value;
      ctxp->unget_utf8_value = 0;
      return (to_return);
    }

  c = GETC ();

  if (c < 128)
    return (unicode_t)c;
  if (c == EOF)
    return UEOF;
  else
    {
      if ((c & 0xe0) == 0xc0)
        {
          c1 = GETC ();
	  if ((c1 & 0xc0) == 0x80)
	    return (unicode_t)(((c &0x1f) << 6) + (c1 & 0x3f));
	  c = c1;
	}
      else if ((c & 0xf0) == 0xe0)
        {
          c1 = GETC ();
	  if ((c1 & 0xc0) == 0x80)
	    {
	      c2 = GETC ();
	      if ((c2 & 0xc0) == 0x80)
	        return (unicode_t)(((c & 0xf) << 12) + 
				   (( c1 & 0x3f) << 6) + (c2 & 0x3f));
	      else
		c = c2;
	    }
	  else
	    c = c1;
	}
      /* We looked for a UTF8 multi-byte sequence (since we saw an initial
	 byte with the high bit set), but found invalid bytes instead.
	 If the most recent byte was Ascii (and not EOF), we should
	 unget it, in case it was a comment terminator or other delimitor. */
      if ((c & 0x80) == 0)
	UNGETC (c);
      return BAD_UTF8_VALUE;
    }
}

static void
java_store_unicode (l, c, unicode_escape_p)
    struct java_line *l;
    unicode_t c;
    int unicode_escape_p;
{
  if (l->size == l->max)
    {
      l->max += JAVA_LINE_MAX;
      l->line = (unicode_t *)realloc (l->line, sizeof (unicode_t)*l->max);
      l->unicode_escape_p = (char *)realloc (l->unicode_escape_p, 
					     sizeof (char)*l->max);
    }
  l->line [l->size] = c;
  l->unicode_escape_p [l->size++] = unicode_escape_p;
}

static unicode_t
java_read_unicode (term_context, unicode_escape_p)
    int term_context;
    int *unicode_escape_p;
{
  unicode_t c;
  long i, base;

  c = java_read_char ();
  *unicode_escape_p = 0;

  if (c != '\\')
    return ((term_context ? c : 
	     java_lineterminator (c) ? '\n' : (unicode_t)c));

  /* Count the number of preceeding '\' */
  for (base = ftell (finput), i = base-2; c == '\\';)
    { 
      fseek (finput, i--, SEEK_SET);
      c = java_read_char ();	/* Will fail if reading utf8 stream. FIXME */
    }
  fseek (finput, base, SEEK_SET);
  if ((base-i-3)%2 == 0)	/* If odd number of \ seen */
    {
      c = java_read_char ();
      if (c == 'u')
        {
	  unsigned short unicode = 0;
	  int shift = 12;
	  /* Next should be 4 hex digits, otherwise it's an error.
	     The hex value is converted into the unicode, pushed into
	     the Unicode stream.  */
	  for (shift = 12; shift >= 0; shift -= 4)
	    {
	      if ((c = java_read_char ()) == UEOF)
	        return UEOF;
	      if (c >= '0' && c <= '9')
		unicode |= (unicode_t)((c-'0') << shift);
	      else if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
	        unicode |= (unicode_t)((10+(c | 0x20)-'a') << shift);
	      else
		  java_lex_error 
		    ("Non hex digit in Unicode escape sequence", 0);
	    }
	  *unicode_escape_p = 1;
	  return (term_context ? unicode :
		  (java_lineterminator (c) ? '\n' : unicode));
	}
      ctxp->unget_utf8_value = c;
    }
  return (unicode_t)'\\';
}

static unicode_t
java_get_unicode ()
{
  /* It's time to read a line when... */
  if (!ctxp->c_line || ctxp->c_line->current == ctxp->c_line->size)
    {
      unicode_t c;
      java_allocate_new_line ();
      if (ctxp->c_line->line[0] != '\n')
	for (;;)
	  {
	    int unicode_escape_p;
	    c = java_read_unicode (0, &unicode_escape_p);
	    java_store_unicode (ctxp->c_line, c, unicode_escape_p);
	    if (ctxp->c_line->white_space_only 
		&& !JAVA_WHITE_SPACE_P (c) && c!='\n')
	      ctxp->c_line->white_space_only = 0;
	    if ((c == '\n') || (c == UEOF))
	      break;
	  }
    }
  ctxp->c_line->char_col += JAVA_COLUMN_DELTA (0);
  JAVA_LEX_CHAR (ctxp->c_line->line [ctxp->c_line->current]);
  return ctxp->c_line->line [ctxp->c_line->current++];
}

static int
java_lineterminator (c)
     unicode_t c;
{
  int unicode_escape_p;
  if (c == '\n')		/* CR */
    {
      if ((c = java_read_unicode (1, &unicode_escape_p)) != '\r')
	{
	  ctxp->c_line->ahead [0] = c;
	  ctxp->c_line->unicode_escape_ahead_p = unicode_escape_p;
	}
      return 1;
    }
  else if (c == '\r')		/* LF */
    {
      if ((c = java_read_unicode (1, &unicode_escape_p)) != '\n')
	{
	  ctxp->c_line->ahead [0] = c;
	  ctxp->c_line->unicode_escape_ahead_p = unicode_escape_p;
	}
      return 1;
    }
  else 
    return 0;
}

/* Parse the end of a C style comment.
 * C is the first character following the '/' and '*'. */
static void
java_parse_end_comment (c)
     unicode_t c;
{

  for ( ;; c = java_get_unicode ())
    {
      switch (c)
	{
	case UEOF:
	  java_lex_error ("Comment not terminated at end of input", 0);
	case '*':
	  switch (c = java_get_unicode ())
	    {
	    case UEOF:
	      java_lex_error ("Comment not terminated at end of input", 0);
	    case '/':
	      return;
	    case '*':	/* reparse only '*' */
	      java_unget_unicode ();
	    }
	}
    }
}

/* Parse the documentation section. Keywords must be at the beginning
   of a documentation comment line (ignoring white space and any `*'
   character). Parsed keyword(s): @DEPRECATED.  */

static int
java_parse_doc_section (c)
     unicode_t c;
{
  int valid_tag = 0, seen_star = 0;

  while (JAVA_WHITE_SPACE_P (c) || (c == '*') || c == '\n')
    {
      switch (c)
	{
	case '*':
	  seen_star = 1;
	  break;
	case '\n': /* ULT */
	  valid_tag = 1;
	default:
	  seen_star = 0;
	}
      c = java_get_unicode();
    }
  
  if (c == UEOF)
    java_lex_error ("Comment not terminated at end of input", 0);
  
  if (seen_star && (c == '/'))
    return 1;			/* Goto step1 in caller */

  /* We're parsing @deprecated */
  if (valid_tag && (c == '@'))
    {
      char tag [10];
      int  tag_index = 0;

      while (tag_index < 10 && c != UEOF && c != ' ' && c != '\n')
	{
	  c = java_get_unicode ();
	  tag [tag_index++] = c;
	}
      
      if (c == UEOF)
	java_lex_error ("Comment not terminated at end of input", 0);
      
      java_unget_unicode ();
      tag [tag_index] = '\0';

      if (!strcmp (tag, "deprecated"))
	ctxp->deprecated = 1;
    }
  return 0;
}

/* This function to be used only by JAVA_ID_CHAR_P (), otherwise it
   will return a wrong result.  */
static int
java_letter_or_digit_p (c)
     unicode_t c;
{
  return _JAVA_LETTER_OR_DIGIT_P (c);
}

static unicode_t
java_parse_escape_sequence ()
{
  unicode_t char_lit;
  unicode_t c;

  switch (c = java_get_unicode ())
    {
    case 'b':
      return (unicode_t)0x8;
    case 't':
      return (unicode_t)0x9;
    case 'n':
      return (unicode_t)0xa;
    case 'f':
      return (unicode_t)0xc;
    case 'r':
      return (unicode_t)0xd;
    case '"':
      return (unicode_t)0x22;
    case '\'':
      return (unicode_t)0x27;
    case '\\':
      return (unicode_t)0x5c;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      {
	int octal_escape[3];
	int octal_escape_index = 0;
	
	for (; octal_escape_index < 3 && RANGE (c, '0', '9');
	     c = java_get_unicode ())
	  octal_escape [octal_escape_index++] = c;

	java_unget_unicode ();

	if ((octal_escape_index == 3) && (octal_escape [0] > '3'))
	  {
	    java_lex_error ("Literal octal escape out of range", 0);
	    return JAVA_CHAR_ERROR;
	  }
	else
	  {
	    int i, shift;
	    for (char_lit=0, i = 0, shift = 3*(octal_escape_index-1);
		 i < octal_escape_index; i++, shift -= 3)
	      char_lit |= (octal_escape [i] - '0') << shift;

	    return (char_lit);
	  }
	break;
      }
    case '\n':
      return '\n';		/* ULT, caught latter as a specific error */
    default:
      java_lex_error ("Illegal character in escape sequence", 0);
      return JAVA_CHAR_ERROR;
    }
}

int
#ifdef JC1_LITE
yylex (java_lval)
#else
java_lex (java_lval)
#endif
     YYSTYPE *java_lval;
{
  unicode_t c, first_unicode;
  int ascii_index, all_ascii;
  char *string;

  /* Translation of the Unicode escape in the raw stream of Unicode
     characters. Takes care of line terminator.  */
 step1:
  /* Skip white spaces: SP, TAB and FF or ULT */ 
  for (c = java_get_unicode ();
       c == '\n' || JAVA_WHITE_SPACE_P (c); c = java_get_unicode ())
    if (c == '\n')
      {
	ctxp->elc.line = ctxp->c_line->lineno;
	ctxp->elc.col  = ctxp->c_line->char_col-2;
      }

  ctxp->elc.col = (ctxp->elc.col < 0 ? 0 : ctxp->elc.col);

  if (c == 0x1a)		/* CTRL-Z */
    {
      if ((c = java_get_unicode ()) == UEOF)
	return 0;		/* Ok here */
      else
	java_unget_unicode ();	/* Caught latter at the end the function */
    }
  /* Handle EOF here */
  if (c == UEOF)	/* Should probably do something here... */
    return 0;

  /* Take care of eventual comments.  */
  if (c == '/')
    {
      switch (c = java_get_unicode ())
	{
	case '/':
	  for (;;)
	    {
	      c = java_get_unicode ();
	      if (c == UEOF)
		java_lex_error ("Comment not terminated at end of input", 0);
	      if (c == '\n')	/* ULT */
		goto step1;
	    }
	  break;

	case '*':
	  if ((c = java_get_unicode ()) == '*')
	    {
	      if ((c = java_get_unicode ()) == '/')
		goto step1;	/* Empy documentation comment  */
	      else if (java_parse_doc_section (c))
		goto step1;
	    }

	  java_parse_end_comment (c);
	  goto step1;
	  break;
	default:
	  java_unget_unicode ();
	  c = '/';
	  break;
	}
    }

  ctxp->elc.line = ctxp->c_line->lineno;
  ctxp->elc.prev_col = ctxp->elc.col;
  ctxp->elc.col = ctxp->c_line->char_col - JAVA_COLUMN_DELTA (-1);
  if (ctxp->elc.col < 0)
    fatal ("ctxp->elc.col < 0 - java_lex");

  /* Numeric literals */
  if (JAVA_ASCII_DIGIT (c) || (c == '.'))
    {
      /* This section of code is borrowed from gcc/c-lex.c  */
#define TOTAL_PARTS ((HOST_BITS_PER_WIDE_INT / HOST_BITS_PER_CHAR) * 2 + 2)
      int parts[TOTAL_PARTS];
      HOST_WIDE_INT high, low;
      /* End borrowed section  */
      char literal_token [256];
      int  literal_index = 0, radix = 10, long_suffix = 0, overflow = 0, bytes;
      int  i;
#ifndef JC1_LITE
      int  number_beginning = ctxp->c_line->current;
#endif
      
      /* We might have a . separator instead of a FP like .[0-9]* */
      if (c == '.')
	{
	  unicode_t peep = java_sneak_unicode ();

	  if (!JAVA_ASCII_DIGIT (peep))
	    {
	      JAVA_LEX_SEP('.');
	      BUILD_OPERATOR (DOT_TK);
	    }
	}

      for (i = 0; i < TOTAL_PARTS; i++)
	parts [i] = 0;

      if (c == '0')
	{
	  c = java_get_unicode ();
	  if (c == 'x' || c == 'X')
	    {
	      radix = 16;
	      c = java_get_unicode ();
	    }
	  else if (JAVA_ASCII_DIGIT (c))
	    radix = 8;
	  else if (c == '.')
	    {
	      /* Push the '.' back and prepare for a FP parsing... */
	      java_unget_unicode ();
	      c = '0';
	    }
	  else
	    {
	      /* We have a zero literal: 0, 0{f,F}, 0{d,D} */
	      JAVA_LEX_LIT ("0", 10);
              switch (c)
		{		
		case 'L': case 'l':
		  SET_LVAL_NODE (long_zero_node);
		  return (INT_LIT_TK);
		case 'f': case 'F':
		  SET_LVAL_NODE (float_zero_node);
		  return (FP_LIT_TK);
		case 'd': case 'D':
		  SET_LVAL_NODE (double_zero_node);
		  return (FP_LIT_TK);
		default:
		  java_unget_unicode ();
		  SET_LVAL_NODE (integer_zero_node);
		  return (INT_LIT_TK);
		}
	    }
	}
      /* Parse the first part of the literal, until we find something
	 which is not a number.  */
      while ((radix == 10 && JAVA_ASCII_DIGIT (c)) ||
	     (radix == 16 && JAVA_ASCII_HEXDIGIT (c)) ||
	     (radix == 8  && JAVA_ASCII_OCTDIGIT (c)))
	{
	  /* We store in a string (in case it turns out to be a FP) and in
	     PARTS if we have to process a integer literal.  */
	  int numeric = (RANGE (c, '0', '9') ? c-'0' : 10 +(c|0x20)-'a');
	  int count;

	  literal_token [literal_index++] = c;
	  /* This section of code if borrowed from gcc/c-lex.c  */
	  for (count = 0; count < TOTAL_PARTS; count++)
	    {
	      parts[count] *= radix;
	      if (count)
		{
		  parts[count]   += (parts[count-1] >> HOST_BITS_PER_CHAR);
		  parts[count-1] &= (1 << HOST_BITS_PER_CHAR) - 1;
		}
	      else
		parts[0] += numeric;
	    }
	  if (parts [TOTAL_PARTS-1] != 0)
	    overflow = 1;
	  /* End borrowed section.  */
	  c = java_get_unicode ();
	}

      /* If we have something from the FP char set but not a digit, parse
	 a FP literal.  */
      if (JAVA_ASCII_FPCHAR (c) && !JAVA_ASCII_DIGIT (c))
	{
	  int stage = 0;
	  int seen_digit = (literal_index ? 1 : 0);
	  int seen_exponent = 0;
	  int fflag = 0;	/* 1 for {f,F}, 0 for {d,D}. FP literal are
				   double unless specified. */
	  if (radix != 10)
	    java_lex_error ("Can't express non-decimal FP literal", 0);

	  for (;;)
	    {
	      if (c == '.')
		{
		  if (stage < 1)
		    {
		      stage = 1;
		      literal_token [literal_index++ ] = c;
		      c = java_get_unicode ();
		    }
		  else
		    java_lex_error ("Invalid character in FP literal", 0);
		}

	      if (c == 'e' || c == 'E')
		{
		  if (stage < 2)
		    {
		      /* {E,e} must have seen at list a digit */
		      if (!seen_digit)
			java_lex_error ("Invalid FP literal", 0);
		      seen_digit = 0;
		      seen_exponent = 1;
		      stage = 2;
		      literal_token [literal_index++] = c;
		      c = java_get_unicode ();
		    }
		  else
		    java_lex_error ("Invalid character in FP literal", 0);
		}
	      if ( c == 'f' || c == 'F' || c == 'd' || c == 'D')
		{
		  fflag = ((c == 'd') || (c == 'D')) ? 0 : 1;
		  stage = 4;	/* So we fall through */
		}

	      if ((c=='-' || c =='+') && stage < 3)
		{
		  stage = 3;
		  literal_token [literal_index++] = c;
		  c = java_get_unicode ();
		}

	      if ((stage == 0 && JAVA_ASCII_FPCHAR (c)) ||
		  (stage == 1 && JAVA_ASCII_FPCHAR (c) && !(c == '.')) ||
		  (stage == 2 && (JAVA_ASCII_DIGIT (c) || JAVA_FP_PM (c))) ||
		  (stage == 3 && JAVA_ASCII_DIGIT (c)))
		{
		  if (JAVA_ASCII_DIGIT (c))
		    seen_digit = 1;
		  literal_token [literal_index++ ] = c;
		  c = java_get_unicode ();
		}
	      else
		{
		  jmp_buf handler;
		  REAL_VALUE_TYPE value;
#ifndef JC1_LITE
		  tree type = (fflag ? FLOAT_TYPE_NODE : DOUBLE_TYPE_NODE);
#endif

		  if (stage != 4) /* Don't push back fF/dD */
		    java_unget_unicode ();
		  
		  /* An exponent (if any) must have seen a digit.  */
		  if (seen_exponent && !seen_digit)
		    java_lex_error ("Invalid FP literal", 0);

		  literal_token [literal_index] = '\0';
		  JAVA_LEX_LIT (literal_token, radix);

		  if (setjmp (handler))
		    {
		      JAVA_FLOAT_RANGE_ERROR ((fflag ? "float" : "double"));
		      value = DCONST0;
		    }
		  else
		    {
		      SET_FLOAT_HANDLER (handler);
		      SET_REAL_VALUE_ATOF 
		        (value, REAL_VALUE_ATOF (literal_token, 
						 TYPE_MODE (type)));

		      if (REAL_VALUE_ISINF (value))
			JAVA_FLOAT_RANGE_ERROR ((fflag ? "float" : "double"));

		      if (REAL_VALUE_ISNAN (value))
			JAVA_FLOAT_RANGE_ERROR ((fflag ? "float" : "double"));

		      SET_LVAL_NODE_TYPE (build_real (type, value), type);
		      SET_FLOAT_HANDLER (NULL_PTR);
		      return FP_LIT_TK;
		    }
		}
	    }
	} /* JAVA_ASCCI_FPCHAR (c) */

      /* Here we get back to converting the integral literal.  */
      if (c == 'L' || c == 'l')
	long_suffix = 1;
      else if (radix == 16 && JAVA_ASCII_LETTER (c))
	java_lex_error ("Digit out of range in hexadecimal literal", 0);
      else if (radix == 8  && JAVA_ASCII_DIGIT (c))
	java_lex_error ("Digit out of range in octal literal", 0);
      else if (radix == 16 && !literal_index)
	java_lex_error ("No digit specified for hexadecimal literal", 0);
      else
	java_unget_unicode ();

#ifdef JAVA_LEX_DEBUG
      literal_token [literal_index] = '\0'; /* So JAVA_LEX_LIT is safe. */
      JAVA_LEX_LIT (literal_token, radix);
#endif
      /* This section of code is borrowed from gcc/c-lex.c  */
      if (!overflow)
	{
	  bytes = GET_TYPE_PRECISION (long_type_node);
	  for (i = bytes; i < TOTAL_PARTS; i++)
	    if (parts [i])
	      {
	        overflow = 1;
		break;
	      }
	}
      high = low = 0;
      for (i = 0; i < HOST_BITS_PER_WIDE_INT / HOST_BITS_PER_CHAR; i++)
	{
	  high |= ((HOST_WIDE_INT) parts[i + (HOST_BITS_PER_WIDE_INT
					      / HOST_BITS_PER_CHAR)]
		   << (i * HOST_BITS_PER_CHAR));
	  low |= (HOST_WIDE_INT) parts[i] << (i * HOST_BITS_PER_CHAR);
	}
      /* End borrowed section.  */

      /* Range checking */
      if (long_suffix)
	{
	  /* 9223372036854775808L is valid if operand of a '-'. Otherwise
	     9223372036854775807L is the biggest `long' literal that can be
	     expressed using a 10 radix. For other radixes, everything that
	     fits withing 64 bits is OK. */
	  int hb = (high >> 31);
	  if (overflow || (hb && low && radix == 10) ||  
	      (hb && high & 0x7fffffff && radix == 10) ||
	      (hb && !(high & 0x7fffffff) && !ctxp->minus_seen && radix == 10))
	    JAVA_INTEGRAL_RANGE_ERROR ("Numeric overflow for `long' literal");
	}
      else
	{
	  /* 2147483648 is valid if operand of a '-'. Otherwise,
	     2147483647 is the biggest `int' literal that can be
	     expressed using a 10 radix. For other radixes, everything
	     that fits within 32 bits is OK.  As all literals are
	     signed, we sign extend here. */
	  int hb = (low >> 31) & 0x1;
	  if (overflow || high || (hb && low & 0x7fffffff && radix == 10) ||
	      (hb && !(low & 0x7fffffff) && !ctxp->minus_seen && radix == 10))
	    JAVA_INTEGRAL_RANGE_ERROR ("Numeric overflow for `int' literal");
	  high = -hb;
	}
      ctxp->minus_seen = 0;
      SET_LVAL_NODE_TYPE (build_int_2 (low, high),
			  (long_suffix ? long_type_node : int_type_node));
      return INT_LIT_TK;
    }

  ctxp->minus_seen = 0;
  /* Character literals */
  if (c == '\'')
    {
      unicode_t char_lit;
      if ((c = java_get_unicode ()) == '\\')
	char_lit = java_parse_escape_sequence ();
      else
	char_lit = c;

      c = java_get_unicode ();
      
      if ((c == '\n') || (c == UEOF))
	java_lex_error ("Character literal not terminated at end of line", 0);
      if (c != '\'')
	java_lex_error ("Syntax error in character literal", 0);

      if (c == JAVA_CHAR_ERROR)
        char_lit = 0;		/* We silently convert it to zero */

      JAVA_LEX_CHAR_LIT (char_lit);
      SET_LVAL_NODE_TYPE (build_int_2 (char_lit, 0), char_type_node);
      return CHAR_LIT_TK;
    }

  /* String literals */
  if (c == '"')
    {
      int no_error;
      char *string;

      for (no_error = 1, c = java_get_unicode (); 
	   c != '"' && c != '\n'; c = java_get_unicode ())
	{
	  if (c == '\\')
	    c = java_parse_escape_sequence ();
	  no_error &= (c != JAVA_CHAR_ERROR ? 1 : 0);
	  java_unicode_2_utf8 (c);
	}
      if (c == '\n' || c == UEOF) /* ULT */
	{
	  lineno--;		/* Refer to the line the terminator was seen */
	  java_lex_error ("String not terminated at end of line.", 0);
	  lineno++;
	}

      obstack_1grow (&temporary_obstack, '\0');
      string = obstack_finish (&temporary_obstack);
#ifndef JC1_LITE
      if (!no_error || (c != '"'))
	java_lval->node = error_mark_node; /* Requires futher testing FIXME */
      else
	{
	  tree s = make_node (STRING_CST);
	  TREE_STRING_LENGTH (s) = strlen (string);
	  TREE_STRING_POINTER (s) = 
	    obstack_alloc (expression_obstack, TREE_STRING_LENGTH (s)+1);
	  strcpy (TREE_STRING_POINTER (s), string);
	  java_lval->node = s;
	}
#endif
      return STRING_LIT_TK;
    }

  /* Separator */
  switch (c)
    {
    case '(':
      JAVA_LEX_SEP (c);
      BUILD_OPERATOR (OP_TK);
    case ')':
      JAVA_LEX_SEP (c);
      return CP_TK;
    case '{':
      JAVA_LEX_SEP (c);
      if (ctxp->ccb_indent == 1)
	ctxp->first_ccb_indent1 = lineno;
      ctxp->ccb_indent++;
      BUILD_OPERATOR (OCB_TK);
    case '}':
      JAVA_LEX_SEP (c);
      ctxp->ccb_indent--;
      if (ctxp->ccb_indent == 1)
        ctxp->last_ccb_indent1 = lineno;
      BUILD_OPERATOR (CCB_TK);
    case '[':
      JAVA_LEX_SEP (c);
      BUILD_OPERATOR (OSB_TK);
    case ']':
      JAVA_LEX_SEP (c);
      return CSB_TK;
    case ';':
      JAVA_LEX_SEP (c);
      return SC_TK;
    case ',':
      JAVA_LEX_SEP (c);
      return C_TK;
    case '.':
      JAVA_LEX_SEP (c);
      BUILD_OPERATOR (DOT_TK);
      /*      return DOT_TK; */
    }

  /* Operators */
  switch (c)
    {
    case '=':
      if ((c = java_get_unicode ()) == '=')
	{
	  BUILD_OPERATOR (EQ_TK);
	}
      else
	{
	  /* Equals is used in two different locations. In the 
	     variable_declarator: rule, it has to be seen as '=' as opposed
	     to being seen as an ordinary assignment operator in
	     assignment_operators: rule.  */
	  java_unget_unicode ();
	  BUILD_OPERATOR (ASSIGN_TK);
	}
      
    case '>':
      switch ((c = java_get_unicode ()))
	{
	case '=':
	  BUILD_OPERATOR (GTE_TK);
	case '>':
	  switch ((c = java_get_unicode ()))
	    {
	    case '>':
	      if ((c = java_get_unicode ()) == '=')
		{
		  BUILD_OPERATOR2 (ZRS_ASSIGN_TK);
		}
	      else
		{
		  java_unget_unicode ();
		  BUILD_OPERATOR (ZRS_TK);
		}
	    case '=':
	      BUILD_OPERATOR2 (SRS_ASSIGN_TK);
	    default:
	      java_unget_unicode ();
	      BUILD_OPERATOR (SRS_TK);
	    }
	default:
	  java_unget_unicode ();
	  BUILD_OPERATOR (GT_TK);
	}
	
    case '<':
      switch ((c = java_get_unicode ()))
	{
	case '=':
	  BUILD_OPERATOR (LTE_TK);
	case '<':
	  if ((c = java_get_unicode ()) == '=')
	    {
	      BUILD_OPERATOR2 (LS_ASSIGN_TK);
	    }
	  else
	    {
	      java_unget_unicode ();
	      BUILD_OPERATOR (LS_TK);
	    }
	default:
	  java_unget_unicode ();
	  BUILD_OPERATOR (LT_TK);
	}

    case '&':
      switch ((c = java_get_unicode ()))
	{
	case '&':
	  BUILD_OPERATOR (BOOL_AND_TK);
	case '=':
	  BUILD_OPERATOR2 (AND_ASSIGN_TK);
	default:
	  java_unget_unicode ();
	  BUILD_OPERATOR (AND_TK);
	}

    case '|':
      switch ((c = java_get_unicode ()))
	{
	case '|':
	  BUILD_OPERATOR (BOOL_OR_TK);
	case '=':
	  BUILD_OPERATOR2 (OR_ASSIGN_TK);
	default:
	  java_unget_unicode ();
	  BUILD_OPERATOR (OR_TK);
	}

    case '+':
      switch ((c = java_get_unicode ()))
	{
	case '+':
	  BUILD_OPERATOR (INCR_TK);
	case '=':
	  BUILD_OPERATOR2 (PLUS_ASSIGN_TK);
	default:
	  java_unget_unicode ();
	  BUILD_OPERATOR (PLUS_TK);
	}

    case '-':
      switch ((c = java_get_unicode ()))
	{
	case '-':
	  BUILD_OPERATOR (DECR_TK);
	case '=':
	  BUILD_OPERATOR2 (MINUS_ASSIGN_TK);
	default:
	  java_unget_unicode ();
	  ctxp->minus_seen = 1;
	  BUILD_OPERATOR (MINUS_TK);
	}

    case '*':
      if ((c = java_get_unicode ()) == '=')
	{
	  BUILD_OPERATOR2 (MULT_ASSIGN_TK);
	}
      else
	{
	  java_unget_unicode ();
	  BUILD_OPERATOR (MULT_TK);
	}

    case '/':
      if ((c = java_get_unicode ()) == '=')
	{
	  BUILD_OPERATOR2 (DIV_ASSIGN_TK);
	}
      else
	{
	  java_unget_unicode ();
	  BUILD_OPERATOR (DIV_TK);
	}

    case '^':
      if ((c = java_get_unicode ()) == '=')
	{
	  BUILD_OPERATOR2 (XOR_ASSIGN_TK);
	}
      else
	{
	  java_unget_unicode ();
	  BUILD_OPERATOR (XOR_TK);
	}

    case '%':
      if ((c = java_get_unicode ()) == '=')
	{
	  BUILD_OPERATOR2 (REM_ASSIGN_TK);
	}
      else
	{
	  java_unget_unicode ();
	  BUILD_OPERATOR (REM_TK);
	}

    case '!':
      if ((c = java_get_unicode()) == '=')
	{
	  BUILD_OPERATOR (NEQ_TK);
	}
      else
	{
	  java_unget_unicode ();
	  BUILD_OPERATOR (NEG_TK);
	}
	  
    case '?':
      JAVA_LEX_OP ("?");
      BUILD_OPERATOR (REL_QM_TK);
    case ':':
      JAVA_LEX_OP (":");
      BUILD_OPERATOR (REL_CL_TK);
    case '~':
      BUILD_OPERATOR (NOT_TK);
    }
  
  /* Keyword, boolean literal or null literal */
  for (first_unicode = c, all_ascii = 1, ascii_index = 0; 
       JAVA_ID_CHAR_P (c); c = java_get_unicode ())
    {
      java_unicode_2_utf8 (c);
      if (all_ascii && c >= 128)
        all_ascii = 0;
      ascii_index++;
    }

  obstack_1grow (&temporary_obstack, '\0');
  string = obstack_finish (&temporary_obstack);
  java_unget_unicode ();

  /* If we have something all ascii, we consider a keyword, a boolean
     literal, a null literal or an all ASCII identifier.  Otherwise,
     this is an identifier (possibly not respecting formation rule).  */
  if (all_ascii)
    {
      struct java_keyword *kw;
      if ((kw=java_keyword (string, ascii_index)))
	{
	  JAVA_LEX_KW (string);
	  switch (kw->token)
	    {
	    case PUBLIC_TK:       case PROTECTED_TK: case STATIC_TK:
	    case ABSTRACT_TK:     case FINAL_TK:     case NATIVE_TK:
	    case SYNCHRONIZED_TK: case TRANSIENT_TK: case VOLATILE_TK:
	    case PRIVATE_TK:
	      SET_MODIFIER_CTX (kw->token);
	      return MODIFIER_TK;
	    case FLOAT_TK:
	      SET_LVAL_NODE (float_type_node);
	      return FP_TK;
	    case DOUBLE_TK:
	      SET_LVAL_NODE (double_type_node);
	      return FP_TK;
	    case BOOLEAN_TK:
	      SET_LVAL_NODE (boolean_type_node);
	      return BOOLEAN_TK;
	    case BYTE_TK:
	      SET_LVAL_NODE (byte_type_node);
	      return INTEGRAL_TK;
	    case SHORT_TK:
	      SET_LVAL_NODE (short_type_node);
	      return INTEGRAL_TK;
	    case INT_TK:
	      SET_LVAL_NODE (int_type_node);
	      return INTEGRAL_TK;
	    case LONG_TK:
	      SET_LVAL_NODE (long_type_node);
	      return INTEGRAL_TK;
	    case CHAR_TK:
	      SET_LVAL_NODE (char_type_node);
	      return INTEGRAL_TK;

	      /* Keyword based literals */
	    case TRUE_TK:
	    case FALSE_TK:
	      SET_LVAL_NODE ((kw->token == TRUE_TK ? 
			      boolean_true_node : boolean_false_node));
	      return BOOL_LIT_TK;
	    case NULL_TK:
	      SET_LVAL_NODE (null_pointer_node);
	      return NULL_TK;

	      /* Some keyword we want to retain information on the location
		 they where found */
	    case CASE_TK:
	    case DEFAULT_TK:
	    case SUPER_TK:
	    case THIS_TK:
	    case RETURN_TK:
	    case BREAK_TK:
	    case CONTINUE_TK:
	    case TRY_TK:
	    case CATCH_TK:
	    case THROW_TK:
	    case INSTANCEOF_TK:
	      BUILD_OPERATOR (kw->token);

	    default:
	      return kw->token;
	    }
	}
    }
  
  /* We may have and ID here */
  if (JAVA_ID_CHAR_P(first_unicode) && !JAVA_DIGIT_P (first_unicode))
    {
      JAVA_LEX_ID (string);
      java_lval->node = BUILD_ID_WFL (GET_IDENTIFIER (string));
      return ID_TK;
    }

  /* Everything else is an invalid character in the input */
  {
    char lex_error_buffer [128];
    sprintf (lex_error_buffer, "Invalid character '%s' in input", 
	     java_sprint_unicode (ctxp->c_line, ctxp->c_line->current));
    java_lex_error (lex_error_buffer, 1);
  }
  return 0;
}

static void
java_unicode_2_utf8 (unicode)
    unicode_t unicode;
{
  if (RANGE (unicode, 0x01, 0x7f))
    obstack_1grow (&temporary_obstack, (char)unicode);
  else if (RANGE (unicode, 0x80, 0x7ff) || unicode == 0)
    {
      obstack_1grow (&temporary_obstack,
		     (unsigned char)(0xc0 | ((0x7c0 & unicode) >> 6)));
      obstack_1grow (&temporary_obstack,
		     (unsigned char)(0x80 | (unicode & 0x3f)));
    }
  else				/* Range 0x800-0xffff */
    {
      obstack_1grow (&temporary_obstack,
		     (unsigned char)(0xe0 | (unicode & 0xf000) >> 12));
      obstack_1grow (&temporary_obstack,
		     (unsigned char)(0x80 | (unicode & 0x0fc0) >> 6));
      obstack_1grow (&temporary_obstack,
		     (unsigned char)(0x80 | (unicode & 0x003f)));
    }
}

#ifndef JC1_LITE
static tree
build_wfl_node (node)
     tree node;
{
  return build_expr_wfl (node, ctxp->filename, ctxp->elc.line, ctxp->elc.col);
}
#endif

static void
java_lex_error (msg, forward)
     char *msg ATTRIBUTE_UNUSED;
     int forward ATTRIBUTE_UNUSED;
{
#ifndef JC1_LITE
  ctxp->elc.line = ctxp->c_line->lineno;
  ctxp->elc.col = ctxp->c_line->char_col-1+forward;

  /* Might be caught in the middle of some error report */
  ctxp->java_error_flag = 0;
  java_error (NULL);
  java_error (msg);
#endif
}

#ifndef JC1_LITE
static int
java_is_eol (fp, c)
  FILE *fp;
  int c;
{
  int next;
  switch (c)
    {
    case '\r':
      next = getc (fp);
      if (next != '\n' && next != EOF)
	ungetc (next, fp);
      return 1;
    case '\n':
      return 1;
    default:
      return 0;
    }  
}
#endif

char *
java_get_line_col (filename, line, col)
     char *filename ATTRIBUTE_UNUSED;
     int line ATTRIBUTE_UNUSED, col ATTRIBUTE_UNUSED;
{
#ifdef JC1_LITE
  return 0;
#else
  /* Dumb implementation. Doesn't try to cache or optimize things. */
  /* First line of the file is line 1, first column is 1 */

  /* COL == -1 means, at the CR/LF in LINE */
  /* COL == -2 means, at the first non space char in LINE */

  FILE *fp;
  int c, ccol, cline = 1;
  int current_line_col = 0;
  int first_non_space = 0;
  char *base;

  if (!(fp = fopen (filename, "r")))
    fatal ("Can't open file - java_display_line_col");

  while (cline != line)
    {
      c = getc (fp);
      if (c < 0)
	{
	  static char msg[] = "<<file too short - unexpected EOF>>";
	  obstack_grow (&temporary_obstack, msg, sizeof(msg)-1);
	  goto have_line;
	}
      if (java_is_eol (fp, c))
	cline++;
    }

  /* Gather the chars of the current line in a buffer */
  for (;;)
    {
      c = getc (fp);
      if (c < 0 || java_is_eol (fp, c))
	break;
      if (!first_non_space && !JAVA_WHITE_SPACE_P (c))
	first_non_space = current_line_col;
      obstack_1grow (&temporary_obstack, c);
      current_line_col++;
    }
 have_line:

  obstack_1grow (&temporary_obstack, '\n');

  if (col == -1)
    {
      col = current_line_col;
      first_non_space = 0;
    }
  else if (col == -2)
    col = first_non_space;
  else
    first_non_space = 0;

  /* Place the '^' a the right position */
  base = obstack_base (&temporary_obstack);
  for (ccol = 1; ccol <= col; ccol++)
    {
      /* Compute \t when reaching first_non_space */
      char c = (first_non_space ?
		(base [ccol-1] == '\t' ? '\t' : ' ') : ' ');
      obstack_1grow (&temporary_obstack, c);
    }
  obstack_grow0 (&temporary_obstack, "^", 1);

  fclose (fp);
  return obstack_finish (&temporary_obstack);
#endif
}
