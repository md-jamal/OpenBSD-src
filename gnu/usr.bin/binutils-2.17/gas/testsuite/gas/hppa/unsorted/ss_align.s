	.SPACE $PRIVATE$
	.SUBSPA $DATA$,QUAD=1,ALIGN=64,ACCESS=31
	.SPACE $TEXT$
	.SUBSPA $LIT$,QUAD=0,ALIGN=8,ACCESS=44
	.SUBSPA $CODE$,QUAD=0,ALIGN=8,ACCESS=44,CODE_ONLY
	.IMPORT $global$,DATA
	.IMPORT $$dyncall,MILLICODE
; gcc_compiled.:
	.SPACE $PRIVATE$
	.SUBSPA $DATA$
sym1:	.WORD 2

