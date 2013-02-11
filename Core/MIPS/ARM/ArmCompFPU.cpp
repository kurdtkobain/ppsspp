// Copyright (c) 2012- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.
#include "../MIPS.h"

#include "ArmJit.h"
#include "ArmRegCache.h"

#define _RS ((op>>21) & 0x1F)
#define _RT ((op>>16) & 0x1F)
#define _RD ((op>>11) & 0x1F)
#define _FS ((op>>11) & 0x1F)
#define _FT ((op>>16) & 0x1F)
#define _FD ((op>>6 ) & 0x1F)
#define _POS	((op>>6 ) & 0x1F)
#define _SIZE ((op>>11 ) & 0x1F)

#define DISABLE Comp_Generic(op); return;
#define CONDITIONAL_DISABLE ; 

namespace MIPSComp
{

void Jit::Comp_FPU3op(u32 op)
{ 
	// DISABLE

	int ft = _FT;
	int fs = _FS;
	int fd = _FD;
	switch (op & 0x3f) 
	{
	case 0: 
		fpr.MapDirtyInIn(fd, fs, ft);
		VADD(fpr.R(fd), fpr.R(fs), fpr.R(ft)); break; //F(fd) = F(fs) + F(ft); //add
	case 1:
  	fpr.MapDirtyInIn(fd, fs, ft);
		VSUB(fpr.R(fd), fpr.R(fs), fpr.R(ft)); break; //F(fd) = F(fs) - F(ft); //sub
	//case 2:
	//	fpr.MapDirtyInIn(fd, fs, ft);
	//	VMUL(fpr.R(fd), fpr.R(fs), fpr.R(ft)); break; //F(fd) = F(fs) * F(ft); //mul
	//case 3: VDIV(fpr.R(fd), fpr.R(fs), fpr.R(fd)); break; //F(fd) = F(fs) / F(ft); //div
	default:
		DISABLE;
		return;
	}
}

extern int logBlocks;

void Jit::Comp_FPULS(u32 op)
{
	DISABLE

	s32 offset = (s16)(op & 0xFFFF);
	int ft = _FT;
	int rs = _RS;
	// u32 addr = R(rs) + offset;
	logBlocks = 1;
	switch(op >> 26)
	{
	case 49: //FI(ft) = Memory::Read_U32(addr); break; //lwc1
		gpr.MapReg(rs);
		fpr.MapReg(ft, MAP_NOINIT | MAP_DIRTY);
		ERROR_LOG(HLE, "lwc1 rs=%i offset=%i   armr=%i", rs, offset, fpr.R(ft) - S0);
		SetR0ToEffectiveAddress(rs, offset);
		VLDR(fpr.R(ft), R0, 0);
		break;

	case 57: //Memory::Write_U32(FI(ft), addr); break; //swc1
		DISABLE;
		fpr.MapReg(ft, 0);
		gpr.MapReg(rs);
		SetR0ToEffectiveAddress(rs, offset);
		VSTR(fpr.R(ft), R0, 0);
		break;

	default:
		Comp_Generic(op);
		return;
	}
}

void Jit::Comp_FPUComp(u32 op) {
	DISABLE;
}

void Jit::Comp_FPU2op(u32 op)
{
	DISABLE

	int fs = _FS;
	int fd = _FD;

	switch (op & 0x3f) 
	{
		/*	
	case 5:	//F(fd)	= fabsf(F(fs)); break; //abs
		fpr.Lock(fd, fs);
		fpr.BindToRegister(fd, fd == fs, true);
		MOVSS(fpr.R(fd), fpr.R(fs));
		PAND(fpr.R(fd), M((void *)ssNoSignMask));
		fpr.UnlockAll();
		break;
		*/

	case 4:	//F(fd)	= sqrtf(F(fs)); break; //sqrt
		fpr.MapDirtyIn(fd, fs);
		VSQRT(fpr.R(fd), fpr.R(fd));
		return;


	case 6:	//F(fd)	= F(fs);				break; //mov
		fpr.MapDirtyIn(fd, fs);
		VMOV(fpr.R(fd), fpr.R(fd));
		break;

		/*
	case 7:	//F(fd)	= -F(fs);			 break; //neg
		fpr.Lock(fd, fs);
		fpr.BindToRegister(fd, fd == fs, true);
		MOVSS(fpr.R(fd), fpr.R(fs));
		PXOR(fpr.R(fd), M((void *)ssSignBits2));
		fpr.UnlockAll();
		break;

	case 12: //FsI(fd) = (int)floorf(F(fs)+0.5f); break; //round.w.s

	case 13: //FsI(fd) = F(fs)>=0 ? (int)floorf(F(fs)) : (int)ceilf(F(fs)); break;//trunc.w.s
		fpr.Lock(fs, fd);
		fpr.StoreFromRegister(fd);
		CVTTSS2SI(EAX, fpr.R(fs));
		MOV(32, fpr.R(fd), R(EAX));
		fpr.UnlockAll();
		break;

	case 14: //FsI(fd) = (int)ceilf (F(fs)); break; //ceil.w.s
	case 15: //FsI(fd) = (int)floorf(F(fs)); break; //floor.w.s
	case 32: //F(fd)	= (float)FsI(fs);			break; //cvt.s.w
	case 36: //FsI(fd) = (int)	F(fs);			 break; //cvt.w.s
	*/
	default:
		Comp_Generic(op);
		return;
	}
}

void Jit::Comp_mxc1(u32 op)
{
	DISABLE
	int fs = _FS;
	int rt = _RT;

	switch((op >> 21) & 0x1f) 
	{
		/*
	case 0: // R(rt) = FI(fs); break; //mfc1
		// Cross move! slightly tricky
		fpr.StoreFromRegister(fs);
		gpr.Lock(rt);
		gpr.BindToRegister(rt, false, true);
		MOV(32, gpr.R(rt), fpr.R(fs));
		gpr.UnlockAll();
		return;

	case 2: // R(rt) = currentMIPS->ReadFCR(fs); break; //cfc1
		Comp_Generic(op);
		return;

	case 4: //FI(fs) = R(rt);	break; //mtc1
		// Cross move! slightly tricky
		gpr.StoreFromRegister(rt);
		fpr.Lock(fs);
		fpr.BindToRegister(fs, false, true);
		MOVSS(fpr.R(fs), gpr.R(rt));
		fpr.UnlockAll();
		return;
		*/
	case 6: //currentMIPS->WriteFCR(fs, R(rt)); break; //ctc1
		Comp_Generic(op);
		return;
	}
}

}	// namespace MIPSComp
