#as: -march=ms1-16-003
#objdump: -dr
#name: ms1-16-003

.*: +file format .*

Disassembly of section .text:

00000000 <iflush>:
   0:	6a 00 00 00 	iflush
00000004 <mul>:
   4:	08 00 00 00 	mul R0,R0,R0
00000008 <muli>:
   8:	09 00 00 00 	muli R0,R0,#\$0
0000000c <dbnz_>:
   c:	3d 00 00 00 	dbnz R0,c <dbnz_>
[ 	]*c: R_MS1_PC16	dbnz
00000010 <fbcbincs>:
  10:	f0 00 00 00 	fbcbincs #\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
00000014 <mfbcbincs>:
  14:	f4 00 00 00 	mfbcbincs R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
00000018 <fbcbincrs>:
  18:	f8 00 00 00 	fbcbincrs R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
0000001c <mfbcbincrs>:
  1c:	fc 00 00 00 	mfbcbincrs R0,R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
00000020 <wfbinc>:
  20:	e0 00 00 00 	wfbinc #\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
00000024 <mwfbinc>:
  24:	e4 00 00 00 	mwfbinc R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
00000028 <wfbincr>:
  28:	e8 00 00 00 	wfbincr R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
0000002c <mwfbincr>:
  2c:	ec 00 00 00 	mwfbincr R0,R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
