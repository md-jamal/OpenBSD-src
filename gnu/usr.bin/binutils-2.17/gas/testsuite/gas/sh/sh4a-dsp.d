#as: -dsp
#objdump: -fdr --prefix-addresses --show-raw-insn
#name: SH4al-dsp constructs shared with sh4a (and sh4)

.*:     file format elf.*sh.*
architecture: sh4a-nofpu, flags 0x00000010:
HAS_SYMS
start address 0x00000000

Disassembly of section \.text:
0x00000000 01 63       	movli\.l	@r1,r0
0x00000002 00 73       	movco\.l	r0,@r0
0x00000004 06 63       	movli\.l	@r6,r0
0x00000006 03 73       	movco\.l	r0,@r3
0x00000008 0a 63       	movli\.l	@r10,r0
0x0000000a 0c 73       	movco\.l	r0,@r12
0x0000000c 40 a9       	movua\.l	@r0,r0
0x0000000e 4d a9       	movua\.l	@r13,r0
0x00000010 47 a9       	movua\.l	@r7,r0
0x00000012 45 e9       	movua\.l	@r5\+,r0
0x00000014 42 e9       	movua\.l	@r2\+,r0
0x00000016 4b e9       	movua\.l	@r11\+,r0
0x00000018 04 e3       	icbi	@r4
0x0000001a 0f e3       	icbi	@r15
0x0000001c 02 e3       	icbi	@r2
0x0000001e 05 d3       	prefi	@r5
0x00000020 0a d3       	prefi	@r10
0x00000022 00 ab       	synco	
0x00000024 45 fa       	ldc	r5,dbr
0x00000026 4a f6       	ldc.l	@r10\+,dbr
0x00000028 0b 3a       	stc	sgr,r11
0x0000002a 49 32       	stc.l	sgr,@-r9
0x0000002c 02 fa       	stc	dbr,r2
0x0000002e 46 f2       	stc.l	dbr,@-r6
0x00000030 03 c3       	movca.l	r0,@r3
0x00000032 0c 93       	ocbi	@r12
0x00000034 07 a3       	ocbp	@r7
0x00000036 0d b3       	ocbwb	@r13
0x00000038 0e 83       	pref	@r14
0x0000003a 00 09       	nop	
