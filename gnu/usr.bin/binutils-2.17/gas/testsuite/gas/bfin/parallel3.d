#objdump: -d
#name: parallel3
.*: +file format .*

Disassembly of section .text:

00000000 <.text>:
   0:	0c cc 0d 08 	R4.H=R4.L=SIGN\(R1.H\)\*R5.H\+SIGN\(R1.L\)\*R5.L\) \|\| \[P0\]=P0 \|\| NOP;
   4:	40 93 00 00 
   8:	09 ce 15 8e 	R7=VIT_MAX\(R5,R2\)\(ASL\) \|\| \[P0\+\+\]=P0 \|\| NOP;
   c:	40 92 00 00 
  10:	09 ce 30 c0 	R0=VIT_MAX\(R0,R6\)\(ASR\) \|\| \[P0--\]=P0 \|\| NOP;
  14:	c0 92 00 00 
  18:	09 ce 03 0a 	R5.L=VIT_MAX \(R3\) \(ASL\) \|\| \[P0\+0x4\]=P0 \|\| NOP;
  1c:	40 bc 00 00 
  20:	09 ce 02 44 	R2.L=VIT_MAX \(R2\) \(ASR\) \|\| \[P0\+0x8\]=P0 \|\| NOP;
  24:	80 bc 00 00 
  28:	06 cc 28 8a 	R5= ABS R5\(V\) \|\| \[P0\+0x3c\]=P0 \|\| NOP;
  2c:	c0 bf 00 00 
  30:	06 cc 00 84 	R2= ABS R0\(V\) \|\| \[P0\+0x38\]=P0 \|\| NOP;
  34:	80 bf 00 00 
  38:	00 cc 1a 0a 	R5=R3\+\|\+R2  \|\| \[P0\+0x34\]=P0 \|\| NOP;
  3c:	40 bf 00 00 
  40:	00 cc 1a 3a 	R5=R3\+\|\+R2 \(SCO\) \|\| \[P1\]=P0 \|\| NOP;
  44:	48 93 00 00 
  48:	00 cc 06 8e 	R7=R0-\|\+R6  \|\| \[P1\+\+\]=P0 \|\| NOP;
  4c:	48 92 00 00 
  50:	00 cc 0b a4 	R2=R1-\|\+R3 \(S\) \|\| \[P1--\]=P0 \|\| NOP;
  54:	c8 92 00 00 
  58:	00 cc 02 48 	R4=R0\+\|-R2  \|\| \[P1\+0x30\]=P0 \|\| NOP;
  5c:	08 bf 00 00 
  60:	00 cc 0a 5a 	R5=R1\+\|-R2 \(CO\) \|\| \[P1\+0x2c\]=P0 \|\| NOP;
  64:	c8 be 00 00 
  68:	00 cc 1c cc 	R6=R3-\|-R4  \|\| \[P1\+0x28\]=P0 \|\| NOP;
  6c:	88 be 00 00 
  70:	00 cc 2e de 	R7=R5-\|-R6 \(CO\) \|\| \[P2\]=P0 \|\| NOP;
  74:	50 93 00 00 
  78:	01 cc 63 bf 	R5=R4\+\|\+R3,R7=R4-\|-R3\(SCO,ASR\) \|\| \[P2\+\+\]=P0 \|\| NOP;
  7c:	50 92 00 00 
  80:	01 cc 1e c2 	R0=R3\+\|\+R6,R1=R3-\|-R6\(ASL\) \|\| \[P2--\]=P0 \|\| NOP;
  84:	d0 92 00 00 
  88:	21 cc ca 2d 	R7=R1\+\|-R2,R6=R1-\|\+R2\(S\) \|\| \[P2\+0x24\]=P0 \|\| NOP;
  8c:	50 be 00 00 
  90:	21 cc 53 0a 	R1=R2\+\|-R3,R5=R2-\|\+R3 \|\| \[P2\+0x20\]=P0 \|\| NOP;
  94:	10 be 00 00 
  98:	04 cc 41 8d 	R5=R0\+R1,R6=R0-R1 \(NS\) \|\| \[P3\]=P0 \|\| NOP;
  9c:	58 93 00 00 
  a0:	04 cc 39 a6 	R0=R7\+R1,R3=R7-R1 \(S\) \|\| \[P3\+\+\]=P0 \|\| NOP;
  a4:	58 92 00 00 
  a8:	11 cc c0 0b 	R7=A1\+A0,R5=A1-A0 \(NS\) \|\| \[P3--\]=P0 \|\| NOP;
  ac:	d8 92 00 00 
  b0:	11 cc c0 6c 	R3=A0\+A1,R6=A0-A1 \(S\) \|\| \[P3\+0x1c\]=P0 \|\| NOP;
  b4:	d8 bd 00 00 
  b8:	81 ce 8b 03 	R1=R3>>>0xf \(V\) \|\| \[P3\+0x18\]=P0 \|\| NOP;
  bc:	98 bd 00 00 
  c0:	81 ce e0 09 	R4=R0>>>0x4 \(V\) \|\| \[P4\]=P0 \|\| NOP;
  c4:	60 93 00 00 
  c8:	81 ce 00 4a 	R5=R0<<0x0 \(V, S\) \|\| \[P4\+\+\]=P0 \|\| NOP;
  cc:	60 92 00 00 
  d0:	81 ce 62 44 	R2=R2<<0xc \(V, S\) \|\| \[P4--\]=P0 \|\| NOP;
  d4:	e0 92 00 00 
  d8:	01 ce 15 0e 	R7= ASHIFT R5 BY R2.L\(V\) \|\| \[P4\+0x18\]=P0 \|\| NOP;
  dc:	a0 bd 00 00 
  e0:	01 ce 02 40 	R0= ASHIFT R2 BY R0.L\(V,S\) \|\| \[P4\+0x14\]=P0 \|\| NOP;
  e4:	60 bd 00 00 
  e8:	81 ce 8a 8b 	R5=R2 >> 0xf \(V\) \|\| \[P4\+0x10\]=P0 \|\| NOP;
  ec:	20 bd 00 00 
  f0:	81 ce 11 80 	R0=R1<<0x2 \(V\) \|\| \[P4\+0xc\]=P0 \|\| NOP;
  f4:	e0 bc 00 00 
  f8:	01 ce 11 88 	R4=SHIFT R1 BY R2.L\(V\) \|\| \[P5\]=P0 \|\| NOP;
  fc:	68 93 00 00 
 100:	06 cc 01 0c 	R6=MAX\(R0,R1\)\(V\) \|\| \[P5\+\+\]=P0 \|\| NOP;
 104:	68 92 00 00 
 108:	06 cc 17 40 	R0=MIN\(R2,R7\)\(V\) \|\| \[P5--\]=P0 \|\| NOP;
 10c:	e8 92 00 00 
 110:	04 ca be 66 	R2.H = R7.L \* R6.H, R2 = R7.H \* R6.H \|\| \[P5\+0x8\]=P0 \|\| NOP;
 114:	a8 bc 00 00 
 118:	04 ca 08 e1 	R4.H = R1.H \* R0.H, R4 = R1.L \* R0.L \|\| \[P5\+0x4\]=P0 \|\| NOP;
 11c:	68 bc 00 00 
 120:	04 ca 1a a0 	R0.H = R3.H \* R2.L, R0 = R3.L \* R2.L \|\| \[P5\]=P0 \|\| NOP;
 124:	68 93 00 00 
 128:	94 ca 5a e1 	R5.H = R3.H \* R2.H \(M\), R5 = R3.L \* R2.L \(FU\) \|\| \[SP\]=P0 \|\| NOP;
 12c:	70 93 00 00 
 130:	2c ca 27 e0 	R1 = R4.H \* R7.H, R0 = R4.L \* R7.L \(S2RND\) \|\| \[SP\+\+\]=P0 \|\| NOP;
 134:	70 92 00 00 
 138:	0c ca 95 27 	R7 = R2.L \* R5.L, R6 = R2.H \* R5.H \|\| \[SP--\]=P0 \|\| NOP;
 13c:	f0 92 00 00 
 140:	24 cb 3e e0 	R0.H = R7.H \* R6.H, R0 = R7.L \* R6.L \(ISS2\) \|\| \[SP\+0x3c\]=P0 \|\| NOP;
 144:	f0 bf 00 00 
 148:	04 cb c1 e0 	R3.H = R0.H \* R1.H, R3 = R0.L \* R1.L \(IS\) \|\| \[FP\]=P0 \|\| NOP;
 14c:	78 93 00 00 
 150:	00 c8 13 46 	a1 = R2.L \* R3.H, a0 = R2.H \* R3.H \|\| \[FP\+\+\]=P0 \|\| NOP;
 154:	78 92 00 00 
 158:	01 c8 08 c0 	a1 \+= R1.H \* R0.H, a0 = R1.L \* R0.L \|\| \[FP--\]=P0 \|\| NOP;
 15c:	f8 92 00 00 
 160:	60 c8 2f c8 	a1 = R5.H \* R7.H, a0 \+= R5.L \* R7.L \(W32\) \|\| \[FP\+0x0\]=P0 \|\| NOP;
 164:	38 bc 00 00 
 168:	01 c9 01 c0 	a1 \+= R0.H \* R1.H, a0 = R0.L \* R1.L \(IS\) \|\| \[FP\+0x3c\]=P0 \|\| NOP;
 16c:	f8 bf 00 00 
 170:	90 c8 1c c8 	a1 = R3.H \* R4.H \(M\), a0 \+= R3.L \* R4.L \(FU\) \|\| \[P0\]=P1 \|\| NOP;
 174:	41 93 00 00 
 178:	01 c8 24 96 	a1 \+= R4.H \* R4.L, a0 -= R4.H \* R4.H \|\| \[P0\]=P2 \|\| NOP;
 17c:	42 93 00 00 
 180:	25 c9 3e e8 	R0.H = \(a1 \+= R7.H \* R6.H\), R0.L = \(a0 \+= R7.L \* R6.L\) \(ISS2\) \|\| \[P0\]=P3 \|\| NOP;
 184:	43 93 00 00 
 188:	27 c8 81 28 	R2.H = A1, R2.L = \(a0 \+= R0.L \* R1.L\) \(S2RND\) \|\| \[P0\]=P4 \|\| NOP;
 18c:	44 93 00 00 
 190:	04 c8 d1 c9 	R7.H = \(a1 = R2.H \* R1.H\), a0 \+= R2.L \* R1.L \|\| \[P0\]=P5 \|\| NOP;
 194:	45 93 00 00 
 198:	04 c8 be 66 	R2.H = \(a1 = R7.L \* R6.H\), R2.L = \(a0 = R7.H \* R6.H\) \|\| \[P0\]=FP \|\| NOP;
 19c:	47 93 00 00 
 1a0:	05 c8 9a e1 	R6.H = \(a1 \+= R3.H \* R2.H\), R6.L = \(a0 = R3.L \* R2.L\) \|\| \[P0\]=SP \|\| NOP;
 1a4:	46 93 00 00 
 1a8:	05 c8 f5 a7 	R7.H = \(a1 \+= R6.H \* R5.L\), R7.L = \(a0 = R6.H \* R5.H\) \|\| \[P0\]=R1 \|\| NOP;
 1ac:	01 93 00 00 
 1b0:	14 c8 3c a8 	R0.H = \(a1 = R7.H \* R4.L\) \(M\), R0.L = \(a0 \+= R7.L \* R4.L\) \|\| \[P0\+\+\]=R2 \|\| NOP;
 1b4:	02 92 00 00 
 1b8:	94 c8 5a e9 	R5.H = \(a1 = R3.H \* R2.H\) \(M\), R5.L = \(a0 \+= R3.L \* R2.L\) \(FU\) \|\| \[P1--\]=R3 \|\| NOP;
 1bc:	8b 92 00 00 
 1c0:	05 c9 1a e0 	R0.H = \(a1 \+= R3.H \* R2.H\), R0.L = \(a0 = R3.L \* R2.L\) \(IS\) \|\| \[I0\]=R0 \|\| NOP;
 1c4:	00 9f 00 00 
 1c8:	1c c8 b7 d0 	R3 = \(a1 = R6.H \* R7.H\) \(M\), a0 -= R6.L \* R7.L \|\| \[I0\+\+\]=R1 \|\| NOP;
 1cc:	01 9e 00 00 
 1d0:	1c c8 3c 2e 	R1 = \(a1 = R7.L \* R4.L\) \(M\), R0 = \(a0 \+= R7.H \* R4.H\) \|\| \[I0--\]=R2 \|\| NOP;
 1d4:	82 9e 00 00 
 1d8:	2d c9 3e e8 	R1 = \(a1 \+= R7.H \* R6.H\), R0 = \(a0 \+= R7.L \* R6.L\) \(ISS2\) \|\| \[I1\]=R3 \|\| NOP;
 1dc:	0b 9f 00 00 
 1e0:	0d c8 37 e1 	R5 = \(a1 \+= R6.H \* R7.H\), R4 = \(a0 = R6.L \* R7.L\) \|\| \[I1\+\+\]=R3 \|\| NOP;
 1e4:	0b 9e 00 00 
 1e8:	0d c8 9d f1 	R7 = \(a1 \+= R3.H \* R5.H\), R6 = \(a0 -= R3.L \* R5.L\) \|\| \[I1--\]=R3 \|\| NOP;
 1ec:	8b 9e 00 00 
 1f0:	0e c8 37 c9 	R5 = \(a1 -= R6.H \* R7.H\), a0 \+= R6.L \* R7.L \|\| \[I2\]=R0 \|\| NOP;
 1f4:	10 9f 00 00 
 1f8:	0c c8 b7 e0 	R3 = \(a1 = R6.H \* R7.H\), R2 = \(a0 = R6.L \* R7.L\) \|\| \[I2\+\+\]=R0 \|\| NOP;
 1fc:	10 9e 00 00 
 200:	9c c8 1f e9 	R5 = \(a1 = R3.H \* R7.H\) \(M\), R4 = \(a0 \+= R3.L \* R7.L\) \(FU\) \|\| \[I2--\]=R0 \|\| NOP;
 204:	90 9e 00 00 
 208:	2f c8 81 28 	R3 = A1, R2 = \(a0 \+= R0.L \* R1.L\) \(S2RND\) \|\| \[I3\]=R7 \|\| NOP;
 20c:	1f 9f 00 00 
 210:	0d c9 1a e0 	R1 = \(a1 \+= R3.H \* R2.H\), R0 = \(a0 = R3.L \* R2.L\) \(IS\) \|\| \[I3\+\+\]=R7 \|\| NOP;
 214:	1f 9e 00 00 
 218:	0f cc 08 c0 	R0=-R1\(V\) \|\| \[I3--\]=R6 \|\| NOP;
 21c:	9e 9e 00 00 
 220:	0f cc 10 ce 	R7=-R2\(V\) \|\| \[P0\+\+P1\]=R0 \|\| NOP;
 224:	08 88 00 00 
 228:	04 ce 08 8e 	R7=PACK\(R0.H,R1.L\) \|\| \[P0\+\+P1\]=R3 \|\| NOP;
 22c:	c8 88 00 00 
 230:	04 ce 31 cc 	R6=PACK\(R1.H,R6.H\) \|\| \[P0\+\+P2\]=R0 \|\| NOP;
 234:	10 88 00 00 
 238:	04 ce 12 4a 	R5=PACK\(R2.L,R2.H\) \|\| \[P0\+\+P3\]=R4 \|\| NOP;
 23c:	18 89 00 00 
 240:	0d cc 10 82 	\(R0,R1\) = SEARCH R2\(LT\) \|\| R2=\[P0\+0x4\] \|\| NOP;
 244:	42 a0 00 00 
 248:	0d cc 80 cf 	\(R6,R7\) = SEARCH R0\(LE\) \|\| R5=\[P0--\] \|\| NOP;
 24c:	85 90 00 00 
 250:	0d cc c8 0c 	\(R3,R6\) = SEARCH R1\(GT\) \|\| R0=\[P0\+0x14\] \|\| NOP;
 254:	40 a1 00 00 
 258:	0d cc 18 4b 	\(R4,R5\) = SEARCH R3\(GE\) \|\| R1=\[P0\+\+\] \|\| NOP;
 25c:	01 90 00 00 
