/*	$OpenBSD$	*/
/*
 * Copyright (C) 2009-2013  Advanced Micro Devices, Inc. All rights reserved.
 * 
 * REDISTRIBUTION: Permission is hereby granted, free of any license fees,
 * to any person obtaining a copy of this microcode (the "Software"), to
 * install, reproduce, copy and distribute copies, in binary form only, of
 * the Software and to permit persons to whom the Software is provided to
 * do the same, provided that the following conditions are met:
 * 
 * No reverse engineering, decompilation, or disassembly of this Software
 * is permitted.
 * 
 * Redistributions must reproduce the above copyright notice, this
 * permission notice, and the following disclaimers and notices in the
 * Software documentation and/or other materials provided with the
 * Software.
 * 
 * DISCLAIMER: THE USE OF THE SOFTWARE IS AT YOUR SOLE RISK.  THE SOFTWARE
 * IS PROVIDED "AS IS" AND WITHOUT WARRANTY OF ANY KIND AND COPYRIGHT
 * HOLDER AND ITS LICENSORS EXPRESSLY DISCLAIM ALL WARRANTIES, EXPRESS AND
 * IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * COPYRIGHT HOLDER AND ITS LICENSORS DO NOT WARRANT THAT THE SOFTWARE WILL
 * MEET YOUR REQUIREMENTS, OR THAT THE OPERATION OF THE SOFTWARE WILL BE
 * UNINTERRUPTED OR ERROR-FREE.  THE ENTIRE RISK ASSOCIATED WITH THE USE OF
 * THE SOFTWARE IS ASSUMED BY YOU.  FURTHERMORE, COPYRIGHT HOLDER AND ITS
 * LICENSORS DO NOT WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE
 * OR THE RESULTS OF THE USE OF THE SOFTWARE IN TERMS OF ITS CORRECTNESS,
 * ACCURACY, RELIABILITY, CURRENTNESS, OR OTHERWISE.
 * 
 * DISCLAIMER: UNDER NO CIRCUMSTANCES INCLUDING NEGLIGENCE, SHALL COPYRIGHT
 * HOLDER AND ITS LICENSORS OR ITS DIRECTORS, OFFICERS, EMPLOYEES OR AGENTS
 * ("AUTHORIZED REPRESENTATIVES") BE LIABLE FOR ANY INCIDENTAL, INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES (INCLUDING DAMAGES FOR LOSS OF BUSINESS
 * PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, AND THE
 * LIKE) ARISING OUT OF THE USE, MISUSE OR INABILITY TO USE THE SOFTWARE,
 * BREACH OR DEFAULT, INCLUDING THOSE ARISING FROM INFRINGEMENT OR ALLEGED
 * INFRINGEMENT OF ANY PATENT, TRADEMARK, COPYRIGHT OR OTHER INTELLECTUAL
 * PROPERTY RIGHT EVEN IF COPYRIGHT HOLDER AND ITS AUTHORIZED
 * REPRESENTATIVES HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  IN
 * NO EVENT SHALL COPYRIGHT HOLDER OR ITS AUTHORIZED REPRESENTATIVES TOTAL
 * LIABILITY FOR ALL DAMAGES, LOSSES, AND CAUSES OF ACTION (WHETHER IN
 * CONTRACT, TORT (INCLUDING NEGLIGENCE) OR OTHERWISE) EXCEED THE AMOUNT OF
 * US$10.
 * 
 * Notice:  The Software is subject to United States export laws and
 * regulations.  You agree to comply with all domestic and international
 * export laws and regulations that apply to the Software, including but
 * not limited to the Export Administration Regulations administered by the
 * U.S. Department of Commerce and International Traffic in Arm Regulations
 * administered by the U.S. Department of State.  These laws include
 * restrictions on destinations, end users and end use.
 */

static const uint8_t r600_rlc[] = {
	0x00, 0x28, 0x04, 0x10, 0x00, 0x28, 0x44, 0x00,
	0x00, 0xd4, 0x40, 0x11, 0x00, 0xd4, 0x00, 0x79,
	0x00, 0x04, 0x18, 0x01, 0x00, 0xd5, 0x80, 0x7a,
	0x00, 0xd4, 0x00, 0x74, 0x00, 0xd4, 0x00, 0x73,
	0x00, 0xd4, 0x00, 0x0b, 0x00, 0x84, 0x02, 0xb2,
	0x00, 0x31, 0x84, 0x1f, 0x00, 0x84, 0x02, 0x3f,
	0x00, 0x28, 0x04, 0x01, 0x00, 0xd4, 0x00, 0x0e,
	0x00, 0x28, 0x20, 0x04, 0x00, 0xc8, 0x10, 0x74,
	0x00, 0xc8, 0x04, 0x36, 0x00, 0x98, 0x42, 0xd0,
	0x00, 0x28, 0x29, 0xde, 0x00, 0xc8, 0x04, 0x79,
	0x00, 0x98, 0x40, 0x08, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x28, 0x0a, 0x00, 0x96, 0x80, 0x05,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x29, 0xdf,
	0x00, 0x80, 0x00, 0x88, 0x00, 0xd5, 0x80, 0x0e,
	0x00, 0xc8, 0x28, 0x00, 0x00, 0x96, 0x80, 0x29,
	0x00, 0x28, 0x08, 0x01, 0x00, 0xd4, 0x80, 0x00,
	0x00, 0x0e, 0x89, 0xbb, 0x00, 0x94, 0x80, 0x09,
	0x00, 0xca, 0xc4, 0x01, 0x00, 0x95, 0x00, 0xfd,
	0x00, 0x30, 0x44, 0x0c, 0x00, 0xd0, 0x48, 0x04,
	0x00, 0x94, 0x80, 0xf1, 0x00, 0x28, 0x20, 0x01,
	0x00, 0x80, 0x01, 0x2a, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0e, 0x89, 0xbc, 0x00, 0x98, 0x80, 0xdc,
	0x00, 0x28, 0x20, 0x06, 0x00, 0x95, 0x01, 0xfb,
	0x00, 0x0e, 0x89, 0xbe, 0x00, 0x94, 0x80, 0x04,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xd5, 0x80, 0x79,
	0x00, 0x80, 0x00, 0x0d, 0x00, 0x0e, 0x89, 0xbd,
	0x00, 0x94, 0x80, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xd4, 0x00, 0x79, 0x00, 0x80, 0x00, 0x0d,
	0x00, 0x0e, 0x89, 0xb4, 0x00, 0x98, 0x80, 0xf1,
	0x00, 0x28, 0x20, 0x04, 0x00, 0x16, 0x89, 0xb7,
	0x00, 0x98, 0x81, 0xec, 0x00, 0x1e, 0x89, 0xba,
	0x00, 0x98, 0x81, 0xea, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0e, 0x89, 0xba, 0x00, 0x94, 0x80, 0xe9,
	0x00, 0x28, 0x20, 0x04, 0x00, 0x28, 0x20, 0x02,
	0x00, 0x80, 0x01, 0x2a, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x28, 0x02, 0x00, 0x96, 0x80, 0x17,
	0x00, 0x28, 0x08, 0x01, 0x00, 0xd4, 0x80, 0x02,
	0x00, 0x95, 0x01, 0xde, 0x00, 0x26, 0x84, 0x01,
	0x00, 0x98, 0x40, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x28, 0x20, 0x00, 0x00, 0x22, 0x89, 0xc8,
	0x00, 0x1a, 0x8d, 0xdd, 0x00, 0x7c, 0x8c, 0x8b,
	0x00, 0x98, 0x80, 0xd8, 0x00, 0x22, 0x89, 0x98,
	0x00, 0x1a, 0x8d, 0x9b, 0x00, 0x7c, 0x8c, 0x8b,
	0x00, 0x98, 0x80, 0xd4, 0x00, 0x22, 0x89, 0xa8,
	0x00, 0x1a, 0x8d, 0xab, 0x00, 0x7c, 0x8c, 0x8b,
	0x00, 0x98, 0x80, 0xd0, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x02, 0x28, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x28, 0x01, 0x00, 0x96, 0x80, 0x0b,
	0x00, 0x28, 0x08, 0x01, 0x00, 0xd4, 0x80, 0x01,
	0x00, 0x95, 0x01, 0xc6, 0x00, 0x0e, 0x89, 0xe8,
	0x00, 0x98, 0x80, 0xc6, 0x00, 0x0e, 0x89, 0xea,
	0x00, 0x98, 0x80, 0xc4, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x02, 0x28, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x28, 0x03, 0x00, 0x96, 0x80, 0x07,
	0x00, 0x0e, 0x8d, 0xe5, 0x00, 0x98, 0xc0, 0xfd,
	0x00, 0x28, 0x08, 0x01, 0x00, 0xd4, 0x80, 0x03,
	0x00, 0x80, 0x02, 0x28, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x28, 0x04, 0x00, 0x96, 0x80, 0x09,
	0x00, 0x28, 0x08, 0x01, 0x00, 0xd4, 0x80, 0x04,
	0x00, 0x95, 0x01, 0xb2, 0x00, 0x0e, 0x89, 0xe2,
	0x00, 0x98, 0x80, 0xb2, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x02, 0x28, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x28, 0x05, 0x00, 0x96, 0x83, 0x93,
	0x00, 0x28, 0x08, 0x01, 0x00, 0xd4, 0x80, 0x05,
	0x00, 0x95, 0x01, 0xa8, 0x00, 0x0e, 0x89, 0x92,
	0x00, 0x98, 0x80, 0xa8, 0x00, 0x0e, 0x89, 0x94,
	0x00, 0x94, 0x81, 0xa4, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x01, 0x2a, 0x00, 0x28, 0x20, 0x02,
	0x00, 0x84, 0x00, 0xbd, 0x00, 0x28, 0x20, 0x03,
	0x00, 0xca, 0xc4, 0x00, 0x00, 0x24, 0x44, 0x02,
	0x00, 0x94, 0x40, 0x06, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x04, 0x78, 0x00, 0xd4, 0x40, 0x77,
	0x00, 0x80, 0x01, 0x2e, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x84, 0x02, 0xb2, 0x00, 0x31, 0x84, 0x1f,
	0x00, 0xca, 0xc4, 0x00, 0x00, 0x24, 0x44, 0x01,
	0x00, 0x98, 0x40, 0x3f, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x04, 0x7a, 0x00, 0xd4, 0x00, 0x7a,
	0x00, 0xc8, 0x08, 0x7d, 0x00, 0xca, 0xcc, 0x0b,
	0x00, 0x7c, 0x8c, 0x82, 0x00, 0x94, 0x40, 0x06,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x94, 0x80, 0x0b,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xa7,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x94, 0x80, 0x8b,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x95,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0xb2,
	0x00, 0x31, 0x84, 0x1f, 0x00, 0x80, 0x02, 0x95,
	0x00, 0xc8, 0x04, 0x7d, 0x00, 0xc8, 0x08, 0x7f,
	0x00, 0x7c, 0x48, 0x42, 0x00, 0x98, 0x40, 0x04,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0xb5,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xca, 0xc4, 0x01,
	0x00, 0x30, 0x44, 0x0c, 0x00, 0xd0, 0x48, 0x00,
	0x00, 0x24, 0x88, 0x01, 0x00, 0x98, 0x81, 0x05,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x04,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0xc8,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x95,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x04, 0x21,
	0x00, 0x30, 0x44, 0x08, 0x00, 0x28, 0x08, 0x40,
	0x00, 0xc8, 0x10, 0x22, 0x00, 0x95, 0x03, 0x4c,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xd5, 0x00, 0x78,
	0x00, 0xd4, 0x00, 0x77, 0x00, 0x7c, 0x00, 0xc0,
	0x00, 0x04, 0xcc, 0x0c, 0x00, 0x09, 0x10, 0x01,
	0x00, 0x99, 0x03, 0xfe, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xd0, 0x50, 0x00, 0x00, 0xd5, 0x08, 0x00,
	0x00, 0x08, 0xcc, 0x01, 0x00, 0x94, 0xc0, 0x06,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x44, 0x04,
	0x00, 0x04, 0x88, 0x01, 0x00, 0x80, 0x00, 0xca,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00,
	0x00, 0x28, 0x2c, 0x40, 0x00, 0xd5, 0x80, 0x74,
	0x00, 0xca, 0xe4, 0x0b, 0x00, 0xca, 0xdc, 0x02,
	0x00, 0xd6, 0x40, 0x7f, 0x00, 0xd6, 0x40, 0x7d,
	0x00, 0x36, 0x64, 0x04, 0x00, 0xd6, 0x40, 0x0b,
	0x00, 0xd6, 0x40, 0x2d, 0x00, 0xd5, 0xc0, 0x7e,
	0x00, 0xd5, 0xc0, 0x7c, 0x00, 0xc8, 0x04, 0x24,
	0x00, 0x30, 0x44, 0x08, 0x00, 0xd4, 0x40, 0x7b,
	0x00, 0xca, 0xc4, 0x00, 0x00, 0x24, 0x44, 0x10,
	0x00, 0x98, 0x43, 0x29, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x84, 0x02, 0x37, 0x00, 0x28, 0x04, 0x01,
	0x00, 0x0c, 0x44, 0x02, 0x00, 0x98, 0x41, 0x98,
	0x00, 0x28, 0x05, 0xf1, 0x00, 0x7d, 0xc0, 0x4c,
	0x00, 0x7d, 0xc0, 0x8c, 0x00, 0x84, 0x02, 0x4f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x37,
	0x00, 0x28, 0x04, 0x03, 0x00, 0x0c, 0x44, 0x02,
	0x00, 0x98, 0x41, 0x8f, 0x00, 0x28, 0x05, 0xf0,
	0x00, 0x84, 0x02, 0x37, 0x00, 0x28, 0x04, 0x02,
	0x00, 0x0c, 0x44, 0x02, 0x00, 0x98, 0x41, 0x8a,
	0x00, 0x28, 0x05, 0xf0, 0x00, 0x84, 0x02, 0x37,
	0x00, 0x28, 0x04, 0x04, 0x00, 0x0c, 0x44, 0x02,
	0x00, 0x98, 0x41, 0x85, 0x00, 0x28, 0x05, 0xf2,
	0x00, 0x84, 0x02, 0x39, 0x00, 0x28, 0x04, 0x02,
	0x00, 0x84, 0x02, 0x4a, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc1, 0x01, 0xf0, 0x00, 0x28, 0x04, 0x00,
	0x00, 0xd8, 0x70, 0x00, 0x00, 0x80, 0x00, 0x0d,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x04, 0x7a,
	0x00, 0x98, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xd4, 0x00, 0x7a, 0x00, 0xc8, 0x04, 0x77,
	0x00, 0x08, 0x44, 0x01, 0x00, 0xd4, 0x40, 0x77,
	0x00, 0x84, 0x02, 0xb2, 0x00, 0x31, 0x84, 0x1f,
	0x00, 0x80, 0x02, 0x9e, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xca, 0xc4, 0x01, 0x00, 0x30, 0x44, 0x0c,
	0x00, 0xdc, 0x04, 0x00, 0x00, 0xc8, 0x04, 0x77,
	0x00, 0x04, 0x44, 0x01, 0x00, 0xd4, 0x40, 0x77,
	0x00, 0x06, 0xec, 0x0c, 0x00, 0xc8, 0x08, 0x78,
	0x00, 0x7c, 0x48, 0x82, 0x00, 0x94, 0x83, 0x92,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xec, 0x0c,
	0x00, 0xd5, 0x80, 0x7a, 0x00, 0x84, 0x02, 0xb2,
	0x00, 0x28, 0x04, 0x00, 0x00, 0x95, 0x01, 0x7b,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x95,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xca, 0xc4, 0x01,
	0x00, 0x30, 0x44, 0x0c, 0x00, 0xdd, 0x84, 0x00,
	0x00, 0xc8, 0x04, 0x77, 0x00, 0x04, 0x44, 0x01,
	0x00, 0xd4, 0x40, 0x77, 0x00, 0x06, 0xec, 0x0c,
	0x00, 0x84, 0x02, 0x8e, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x84, 0x02, 0x3d, 0x00, 0x28, 0x04, 0x00,
	0x00, 0x84, 0x02, 0x37, 0x00, 0x28, 0x04, 0x01,
	0x00, 0x0c, 0x44, 0x02, 0x00, 0x94, 0x40, 0x05,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20, 0x04,
	0x00, 0x28, 0x29, 0xf1, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x04, 0x77, 0x00, 0xc8, 0x08, 0x78,
	0x00, 0x7c, 0x48, 0x82, 0x00, 0x94, 0x80, 0x63,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x60,
	0x00, 0x28, 0x20, 0x05, 0x00, 0x84, 0x02, 0x2c,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x28, 0x0a,
	0x00, 0x96, 0x83, 0xff, 0x00, 0x28, 0x20, 0x03,
	0x00, 0x28, 0x29, 0xdf, 0x00, 0x84, 0x00, 0xbd,
	0x00, 0x28, 0x20, 0x03, 0x00, 0xca, 0xc4, 0x0b,
	0x00, 0xc8, 0x08, 0x7d, 0x00, 0x7c, 0x48, 0x82,
	0x00, 0xd4, 0x80, 0x73, 0x00, 0x94, 0x80, 0x53,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x37,
	0x00, 0x28, 0x04, 0x04, 0x00, 0x0c, 0x44, 0x02,
	0x00, 0x98, 0x41, 0x2d, 0x00, 0x28, 0x05, 0xf2,
	0x00, 0xc8, 0x04, 0x73, 0x00, 0x98, 0x40, 0x0a,
	0x00, 0xd4, 0x00, 0x73, 0x00, 0xca, 0xc4, 0x01,
	0x00, 0x30, 0x44, 0x0c, 0x00, 0xd0, 0x48, 0x00,
	0x00, 0x24, 0x88, 0x01, 0x00, 0x98, 0x80, 0x04,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0xc8,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x3d,
	0x00, 0x28, 0x04, 0x01, 0x00, 0xc8, 0x04, 0x20,
	0x00, 0x24, 0x44, 0xfd, 0x00, 0xd4, 0x40, 0x20,
	0x00, 0x84, 0x02, 0x39, 0x00, 0x28, 0x04, 0x02,
	0x00, 0x80, 0x02, 0x95, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x04, 0x7a, 0x00, 0x94, 0x40, 0x06,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x3f,
	0x00, 0x28, 0x04, 0x00, 0x00, 0x98, 0x40, 0x09,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0xb2,
	0x00, 0x31, 0x84, 0x1f, 0x00, 0xc1, 0x01, 0xf0,
	0x00, 0xcf, 0x04, 0x00, 0x00, 0x28, 0x04, 0x00,
	0x00, 0x80, 0x00, 0x0d, 0x00, 0xd4, 0x40, 0x03,
	0x00, 0x28, 0x04, 0x01, 0x00, 0xd4, 0x40, 0x03,
	0x00, 0xc1, 0x01, 0xf1, 0x00, 0xcf, 0x04, 0x00,
	0x00, 0x34, 0x44, 0x04, 0x00, 0x98, 0x43, 0xfe,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc4, 0x28, 0x0a,
	0x00, 0x9a, 0x80, 0x19, 0x00, 0xc8, 0x28, 0x00,
	0x00, 0x0e, 0x85, 0xbc, 0x00, 0x98, 0x40, 0x13,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc1, 0x20, 0x04,
	0x00, 0xcf, 0x04, 0x00, 0x00, 0x34, 0x44, 0x1f,
	0x00, 0x98, 0x40, 0x11, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x28, 0x05, 0x00, 0x96, 0x80, 0x05,
	0x00, 0x28, 0x08, 0x01, 0x00, 0xd4, 0x80, 0x05,
	0x00, 0x84, 0x02, 0x2c, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x28, 0x03, 0x00, 0x0e, 0x85, 0xe5,
	0x00, 0x94, 0x43, 0xed, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x01, 0x99, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xd4, 0x00, 0x7a, 0x00, 0x84, 0x02, 0xb2,
	0x00, 0x31, 0x84, 0x1f, 0x00, 0x84, 0x02, 0x3b,
	0x00, 0x28, 0x04, 0x02, 0x00, 0xd8, 0x30, 0x00,
	0x00, 0x84, 0x02, 0x3f, 0x00, 0x28, 0x04, 0x01,
	0x00, 0x80, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xd4, 0x00, 0x72, 0x00, 0xca, 0xc4, 0x01,
	0x00, 0x30, 0x44, 0x0c, 0x00, 0xd0, 0x48, 0x00,
	0x00, 0x24, 0x88, 0x01, 0x00, 0x98, 0x80, 0x05,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x08, 0x7e,
	0x00, 0x80, 0x01, 0xab, 0x00, 0xd5, 0x80, 0x72,
	0x00, 0xca, 0xc8, 0x02, 0x00, 0xc8, 0x04, 0x7c,
	0x00, 0x84, 0x02, 0x4f, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x84, 0x02, 0x37, 0x00, 0x28, 0x04, 0x03,
	0x00, 0x0c, 0x44, 0x02, 0x00, 0x98, 0x40, 0x04,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x4f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x04, 0x72,
	0x00, 0x94, 0x40, 0x1f, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x02, 0x81, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x84, 0x02, 0x8e, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x84, 0x02, 0x3d, 0x00, 0x28, 0x04, 0x00,
	0x00, 0x84, 0x02, 0x37, 0x00, 0x28, 0x04, 0x01,
	0x00, 0x0c, 0x44, 0x02, 0x00, 0x94, 0x40, 0x05,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x20, 0x04,
	0x00, 0x28, 0x29, 0xf1, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xd4, 0x00, 0x72, 0x00, 0xca, 0xc8, 0x02,
	0x00, 0x7c, 0x00, 0x4c, 0x00, 0x84, 0x02, 0x4f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x37,
	0x00, 0x28, 0x04, 0x02, 0x00, 0x0c, 0x44, 0x02,
	0x00, 0x94, 0x43, 0x81, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x04, 0x72, 0x00, 0x94, 0x40, 0x04,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x81,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xca, 0xc4, 0x0b,
	0x00, 0xd4, 0x40, 0x76, 0x00, 0x84, 0x02, 0x60,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xd6, 0x80, 0x0c,
	0x00, 0x28, 0x20, 0x04, 0x00, 0x28, 0x29, 0xf0,
	0x00, 0xc8, 0x08, 0x76, 0x00, 0xd4, 0x80, 0x7d,
	0x00, 0x34, 0x88, 0x04, 0x00, 0xd4, 0x80, 0x0b,
	0x00, 0xc8, 0x04, 0x77, 0x00, 0x04, 0x44, 0x01,
	0x00, 0xd4, 0x40, 0x77, 0x00, 0x06, 0xec, 0x0c,
	0x00, 0xc8, 0x08, 0x78, 0x00, 0x7c, 0x48, 0x82,
	0x00, 0x94, 0x80, 0x0f, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x14, 0x0a, 0x00, 0x95, 0x43, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x00, 0xbd,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x04, 0x7d,
	0x00, 0xca, 0xc8, 0x0b, 0x00, 0x7c, 0x48, 0x42,
	0x00, 0x98, 0x43, 0xe5, 0x00, 0xc8, 0x04, 0x76,
	0x00, 0x7c, 0x48, 0x42, 0x00, 0x98, 0x43, 0xe2,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xca, 0xc4, 0x01,
	0x00, 0x30, 0x44, 0x0c, 0x00, 0xd0, 0x48, 0x00,
	0x00, 0x24, 0x88, 0x01, 0x00, 0x98, 0x83, 0xcd,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xd5, 0x80, 0x72,
	0x00, 0xc8, 0x08, 0x7e, 0x00, 0x7c, 0x00, 0x4c,
	0x00, 0x84, 0x02, 0x4f, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x01, 0xcb, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x84, 0x02, 0x43, 0x00, 0x28, 0x04, 0x06,
	0x00, 0xc1, 0x05, 0x16, 0x00, 0xca, 0xc4, 0x0a,
	0x00, 0xd8, 0x70, 0x00, 0x00, 0xc1, 0x05, 0x38,
	0x00, 0xca, 0xc4, 0x08, 0x00, 0xd8, 0x70, 0x00,
	0x00, 0xc1, 0x05, 0x39, 0x00, 0xca, 0xc4, 0x09,
	0x00, 0xd8, 0x70, 0x00, 0x00, 0xc1, 0x05, 0x5f,
	0x00, 0xca, 0xc4, 0x03, 0x00, 0xd8, 0x70, 0x00,
	0x00, 0xc1, 0x05, 0x26, 0x00, 0xca, 0xc4, 0x06,
	0x00, 0xd8, 0x70, 0x00, 0x00, 0xc1, 0x05, 0x2e,
	0x00, 0xca, 0xc4, 0x07, 0x00, 0xd8, 0x70, 0x00,
	0x00, 0xc1, 0x05, 0x67, 0x00, 0xca, 0xc4, 0x04,
	0x00, 0xd8, 0x70, 0x00, 0x00, 0xc1, 0x05, 0x6f,
	0x00, 0xca, 0xc4, 0x05, 0x00, 0xd8, 0x70, 0x00,
	0x00, 0xcf, 0x04, 0x00, 0x00, 0xca, 0xc4, 0x00,
	0x00, 0x24, 0x44, 0x04, 0x00, 0x98, 0x40, 0x04,
	0x00, 0x28, 0x04, 0x01, 0x00, 0x80, 0x02, 0x24,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x04, 0x02,
	0x00, 0x84, 0x02, 0x43, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x84, 0x02, 0x2c, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x26, 0xa8, 0xff, 0x00, 0xd6, 0x80, 0x0c,
	0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xd8, 0x70, 0x00, 0x00, 0xcf, 0x04, 0x00,
	0x00, 0x34, 0x44, 0x04, 0x00, 0x94, 0x43, 0xfe,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x30,
	0x00, 0xc1, 0x0e, 0xfb, 0x00, 0x80, 0x02, 0x30,
	0x00, 0xc1, 0x0f, 0x91, 0x00, 0x80, 0x02, 0x30,
	0x00, 0xc1, 0x01, 0xf1, 0x00, 0x80, 0x02, 0x30,
	0x00, 0xc1, 0x03, 0x91, 0x00, 0xc1, 0x20, 0x03,
	0x00, 0xd8, 0x70, 0x00, 0x00, 0x88, 0x00, 0x00,
	0x00, 0xcf, 0x04, 0x00, 0x00, 0x80, 0x02, 0x30,
	0x00, 0xc1, 0x05, 0x1e, 0x00, 0xc8, 0x30, 0x20,
	0x00, 0x2b, 0x30, 0x02, 0x00, 0xd7, 0x00, 0x20,
	0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x30, 0x20, 0x00, 0x27, 0x33, 0xfd,
	0x00, 0xd7, 0x00, 0x20, 0x00, 0x88, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x14, 0x1f,
	0x00, 0x31, 0x54, 0x1b, 0x00, 0x7d, 0x44, 0x4c,
	0x00, 0x28, 0x14, 0x08, 0x00, 0x31, 0x54, 0x1b,
	0x00, 0x7d, 0x48, 0x8c, 0x00, 0xc1, 0x0f, 0x00,
	0x00, 0x28, 0x0c, 0x08, 0x00, 0xd8, 0xb0, 0x00,
	0x00, 0xd8, 0x70, 0x01, 0x00, 0x08, 0xcc, 0x01,
	0x00, 0x94, 0xc0, 0x04, 0x00, 0x07, 0x30, 0x10,
	0x00, 0x80, 0x02, 0x57, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xd4, 0x00, 0x30, 0x00, 0xc8, 0x08, 0x7b,
	0x00, 0xc8, 0x0c, 0x7d, 0x00, 0x7c, 0xe0, 0xcc,
	0x00, 0xdc, 0xc8, 0x00, 0x00, 0xc8, 0x0c, 0x2e,
	0x00, 0xdc, 0xc8, 0x04, 0x00, 0xc8, 0x0c, 0x2f,
	0x00, 0xdc, 0xc8, 0x08, 0x00, 0xc8, 0x14, 0x25,
	0x00, 0xc8, 0x0c, 0x27, 0x00, 0xc8, 0x10, 0x23,
	0x00, 0x04, 0xcc, 0x01, 0x00, 0x7c, 0xd1, 0x07,
	0x00, 0x99, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x04, 0x88, 0x0c, 0x00, 0xd4, 0x80, 0x7b,
	0x00, 0xd4, 0xc0, 0x27, 0x00, 0xdc, 0xd4, 0x00,
	0x00, 0x80, 0x02, 0x7b, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x08, 0x24, 0x00, 0x30, 0x88, 0x08,
	0x00, 0xd4, 0x80, 0x7b, 0x00, 0xd4, 0x00, 0x27,
	0x00, 0x7c, 0x00, 0xcc, 0x00, 0xc8, 0x08, 0x25,
	0x00, 0xdc, 0xc8, 0x00, 0x00, 0x84, 0x02, 0x88,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x42, 0x8c,
	0x00, 0x84, 0x02, 0x60, 0x00, 0x28, 0x20, 0x04,
	0x00, 0x84, 0x02, 0x2c, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x02, 0x86, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x04, 0x0d, 0x00, 0x08, 0x44, 0x03,
	0x00, 0x98, 0x43, 0xfe, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x04, 0x20, 0x00, 0x28, 0x44, 0x02,
	0x00, 0xd4, 0x40, 0x20, 0x00, 0x84, 0x02, 0x39,
	0x00, 0x28, 0x04, 0x01, 0x00, 0x88, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x60,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc1, 0x3c, 0x81,
	0x00, 0xcf, 0x04, 0x00, 0x00, 0xd8, 0x70, 0x00,
	0x00, 0xc1, 0x34, 0x01, 0x00, 0xcf, 0x04, 0x00,
	0x00, 0xd8, 0x70, 0x00, 0x00, 0x84, 0x02, 0x2c,
	0x00, 0xc8, 0x10, 0x74, 0x00, 0x95, 0x01, 0x6e,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x04, 0x7a,
	0x00, 0x94, 0x40, 0x04, 0x00, 0xc8, 0x24, 0x7f,
	0x00, 0x80, 0x02, 0xa7, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xca, 0xe4, 0x0b, 0x00, 0xd6, 0x40, 0x7d,
	0x00, 0x36, 0x64, 0x04, 0x00, 0xd6, 0x40, 0x0b,
	0x00, 0xd6, 0x40, 0x2d, 0x00, 0xca, 0xcc, 0x02,
	0x00, 0xd4, 0xc0, 0x7c, 0x00, 0xca, 0xcc, 0x01,
	0x00, 0x30, 0xcc, 0x0c, 0x00, 0xd4, 0xc0, 0x75,
	0x00, 0x80, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc1, 0x01, 0xf0, 0x00, 0x88, 0x00, 0x00,
	0x00, 0xd8, 0x70, 0x00, 0x00, 0xc8, 0x04, 0x75,
	0x00, 0xc1, 0xff, 0xff, 0x00, 0xd0, 0x48, 0x0c,
	0x00, 0x04, 0x44, 0x04, 0x00, 0x7c, 0xb0, 0xcb,
	0x00, 0x94, 0xc0, 0x0c, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xcc, 0xd0, 0x00, 0x00, 0xdd, 0x04, 0x0c,
	0x00, 0x04, 0x44, 0x04, 0x00, 0x34, 0x88, 0x10,
	0x00, 0x94, 0x80, 0x06, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xcc, 0x90, 0x00, 0x00, 0xdd, 0x04, 0x0c,
	0x00, 0x80, 0x02, 0xb7, 0x00, 0x04, 0x44, 0x04,
	0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc1, 0x30, 0x41, 0x00, 0x31, 0x84, 0x1f,
	0x00, 0xd8, 0x70, 0x00, 0x00, 0xc1, 0x3c, 0x82,
	0x00, 0xd8, 0x70, 0x00, 0x00, 0xc1, 0x34, 0x00,
	0x00, 0xd8, 0x30, 0x00, 0x00, 0xca, 0xc4, 0x01,
	0x00, 0x30, 0x44, 0x0c, 0x00, 0xc1, 0xff, 0xff,
	0x00, 0xd0, 0x48, 0x0c, 0x00, 0x04, 0x44, 0x04,
	0x00, 0x7c, 0xb0, 0xcb, 0x00, 0x94, 0xc0, 0x0a,
	0x00, 0xd0, 0x50, 0x0c, 0x00, 0xd9, 0x0c, 0x00,
	0x00, 0x04, 0x44, 0x04, 0x00, 0x34, 0x88, 0x10,
	0x00, 0x94, 0x80, 0x05, 0x00, 0xd0, 0x50, 0x0c,
	0x00, 0xd9, 0x08, 0x00, 0x00, 0x80, 0x02, 0xd2,
	0x00, 0x04, 0x44, 0x04, 0x00, 0x88, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x04, 0x0a,
	0x00, 0xd4, 0x00, 0x79, 0x00, 0x84, 0x02, 0x37,
	0x00, 0x28, 0x04, 0x01, 0x00, 0xc8, 0x08, 0x7e,
	0x00, 0xc8, 0x04, 0x7c, 0x00, 0x84, 0x02, 0x4f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0x37,
	0x00, 0x28, 0x04, 0x03, 0x00, 0x0c, 0x44, 0x02,
	0x00, 0x98, 0x43, 0x95, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x84, 0x02, 0x37, 0x00, 0x28, 0x04, 0x04,
	0x00, 0x0c, 0x44, 0x02, 0x00, 0x98, 0x43, 0x90,
	0x00, 0x28, 0x05, 0xf2, 0x00, 0x28, 0x20, 0x04,
	0x00, 0x84, 0x02, 0x60, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x84, 0x02, 0x2c, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc8, 0x24, 0x7f, 0x00, 0xd6, 0x40, 0x7d,
	0x00, 0x36, 0x64, 0x04, 0x00, 0xd6, 0x40, 0x0b,
	0x00, 0xd6, 0x40, 0x2d, 0x00, 0x80, 0x00, 0x0d,
	0x00, 0xd4, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00,
};
