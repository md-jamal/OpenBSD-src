#as: -nosched
#objdump: -dr
#name: msys 

.*: +file format .*

Disassembly of section .text:

00000000 <.text>:
   0:	80 00 00 00 	ldctxt R0,R0,#\$0,#\$0,#\$0
   4:	84 00 00 00 	ldfb R0,R0,#\$0
   8:	88 00 00 00 	stfb R0,R0,#\$0
   c:	8c 00 00 00 	fbcb R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  10:	90 00 00 00 	mfbcb R0,#\$0,R0,#\$0,#\$0,#\$0,#\$0,#\$0
  14:	94 00 00 00 	fbcci R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  18:	98 00 00 00 	fbrci R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  1c:	9c 00 00 00 	fbcri R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  20:	a0 00 00 00 	fbrri R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  24:	a4 00 00 00 	mfbcci R0,#\$0,R0,#\$0,#\$0,#\$0,#\$0
  28:	a8 00 00 00 	mfbrci R0,#\$0,R0,#\$0,#\$0,#\$0,#\$0
  2c:	ac 00 00 00 	mfbcri R0,#\$0,R0,#\$0,#\$0,#\$0,#\$0
  30:	b0 00 00 00 	mfbrri R0,#\$0,R0,#\$0,#\$0,#\$0,#\$0
  34:	b4 00 00 00 	fbcbdr R0,#\$0,R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  38:	b8 00 00 00 	rcfbcb #\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  3c:	bc 00 00 00 	mrcfbcb R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  40:	c0 00 00 00 	cbcast #\$0,#\$0,#\$0
  44:	c4 00 00 00 	dupcbcast #\$0,#\$0,#\$0,#\$0
  48:	c8 00 00 00 	wfbi #\$0,#\$0,#\$0,#\$0,#\$0
  4c:	cc 00 00 00 	wfb R0,R0,#\$0,#\$0,#\$0
  50:	d0 00 00 00 	rcrisc R0,#\$0,R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  54:	d4 00 00 00 	fbcbinc R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  58:	d8 00 00 00 	rcxmode R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  5c:	64 00 e0 00 	si R14
  60:	b4 00 00 40 	fbcbdr R0,#\$0,R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$1,#\$0
  64:	b4 00 00 00 	fbcbdr R0,#\$0,R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  68:	b4 00 00 40 	fbcbdr R0,#\$0,R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$1,#\$0
  6c:	b4 00 00 00 	fbcbdr R0,#\$0,R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  70:	64 00 e0 00 	si R14
  74:	b8 08 00 00 	rcfbcb #\$0,#\$0,#\$1,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  78:	b8 00 00 00 	rcfbcb #\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  7c:	b8 08 00 00 	rcfbcb #\$0,#\$0,#\$1,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  80:	b8 00 00 00 	rcfbcb #\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  84:	64 00 e0 00 	si R14
  88:	bc 20 00 00 	mrcfbcb R0,#\$0,#\$2,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  8c:	bc 10 00 00 	mrcfbcb R0,#\$0,#\$1,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  90:	bc 00 00 00 	mrcfbcb R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  94:	bc 20 00 00 	mrcfbcb R0,#\$0,#\$2,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  98:	bc 10 00 00 	mrcfbcb R0,#\$0,#\$1,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  9c:	bc 00 00 00 	mrcfbcb R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  a0:	64 00 e0 00 	si R14
  a4:	d8 80 00 00 	rcxmode R0,#\$0,#\$0,#\$1,#\$0,#\$0,#\$0,#\$0,#\$0
  a8:	d8 00 00 00 	rcxmode R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  ac:	d8 80 00 00 	rcxmode R0,#\$0,#\$0,#\$1,#\$0,#\$0,#\$0,#\$0,#\$0
  b0:	d8 00 00 00 	rcxmode R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  b4:	64 00 e0 00 	si R14
  b8:	80 00 80 00 	ldctxt R0,R0,#\$1,#\$0,#\$0
  bc:	80 00 00 00 	ldctxt R0,R0,#\$0,#\$0,#\$0
  c0:	80 00 80 00 	ldctxt R0,R0,#\$1,#\$0,#\$0
  c4:	80 00 00 00 	ldctxt R0,R0,#\$0,#\$0,#\$0
  c8:	8c 00 08 00 	fbcb R0,#\$0,#\$0,#\$0,#\$1,#\$0,#\$0,#\$0,#\$0
  cc:	8c 00 00 00 	fbcb R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  d0:	c0 00 00 40 	cbcast #\$0,#\$1,#\$0
  d4:	c0 00 00 00 	cbcast #\$0,#\$0,#\$0
  d8:	64 00 e0 00 	si R14
  dc:	8c 00 04 00 	fbcb R0,#\$0,#\$0,#\$0,#\$0,#\$1,#\$0,#\$0,#\$0
  e0:	8c 00 00 00 	fbcb R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  e4:	8c 00 04 00 	fbcb R0,#\$0,#\$0,#\$0,#\$0,#\$1,#\$0,#\$0,#\$0
  e8:	8c 00 00 00 	fbcb R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  ec:	64 00 e0 00 	si R14
  f0:	8f 00 00 00 	fbcb R0,#\$3,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  f4:	8e 00 00 00 	fbcb R0,#\$2,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  f8:	8d 00 00 00 	fbcb R0,#\$1,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
  fc:	8c 00 00 00 	fbcb R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
 100:	8f 00 00 00 	fbcb R0,#\$3,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
 104:	8e 00 00 00 	fbcb R0,#\$2,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
 108:	8d 00 00 00 	fbcb R0,#\$1,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
 10c:	8c 00 00 00 	fbcb R0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0,#\$0
 110:	dc 00 00 00 	intlvr R0,#\$0,R0,#\$0,#\$0
