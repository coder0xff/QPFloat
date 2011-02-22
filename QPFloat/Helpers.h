/*
Copyright 2010 Brent W. Lewis
coder0xff on hotmail

This file is part of QPFloat.

	QPFloat is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QPFloat is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QPFloat.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <intrin.h>
#include <memory>

typedef unsigned long long ui64;
typedef long long i64;
typedef unsigned int ui32;
typedef int i32;
typedef unsigned short ui16;
typedef short i16;
typedef unsigned char byte;
static_assert(sizeof(ui64) == 8, "ui64 is not 8 bytes");
static_assert(sizeof(i64) == 8, "i64 is not 8 bytes");
static_assert(sizeof(ui32) == 4, "ui64 is not 4 bytes");
static_assert(sizeof(i32) == 4, "i64 is not 4 bytes");
static_assert(sizeof(ui16) == 2, "ui64 is not 2 bytes");
static_assert(sizeof(i16) == 2, "i64 is not 2 bytes");
static_assert(sizeof(byte) == 1, "byte is not one byte");

//cd = constant data
//these are defined in quad_shared.cpp
#define cd(name) extern byte name [16]
cd(snan);
cd(qnan);
cd(ind);
cd(posInf);
cd(negInf);
cd(e);
cd(log2e);
cd(ln2);
cd(twoPi);
cd(pi);
cd(halfPi);
cd(quarterPi);
cd(zero);
cd(negZero);
cd(one);
cd(negOne);
cd(half);
cd(sinQuarterPi);
#undef cd

 #define MAX_FACTORIAL 1754
struct __float128;

extern __float128 factorials[MAX_FACTORIAL];
extern __float128 factorialReciprocals[MAX_FACTORIAL];

#define QUAD_SIGNIFICANT_BITS 112
#define QUAD_EXPONENT_BITS 15
#define QUAD_EXPONENT_BIAS 0x3FFF

#define QUAD_MANTISSA_BYTES ((QUAD_SIGNIFICANT_BITS + 7) / 8)
#define QUAD_EXPONENT_MASK ((1 << QUAD_EXPONENT_BITS) - 1)
#define QUAD_EXPONENT_MAX (QUAD_EXPONENT_MASK - QUAD_EXPONENT_BIAS)
#define QUAD_EXPONENT_MIN (-QUAD_EXPONENT_BIAS)

extern bool enableOverflowException;
extern bool enableUnderflowException;
extern bool enableDivideByZeroException;
extern bool enableInvalidException;
extern bool enableInexactException;

#ifdef _MANAGED
#pragma unmanaged
#endif

bool inline ReadBit(void const *buffer, long bitOffset)
{
	return _bittest((const long*)buffer, bitOffset) != 0;
}

void inline SetBit(void *buffer, long bitOffset)
{
	_bittestandset((long*)buffer, bitOffset);
}

void inline ClearBit(void *buffer, long bitOffset)
{
	_bittestandreset((long*)buffer, bitOffset);
}

inline void IntBlockCompare( i32* result, ui32 *a, ui32 *b, ui32 intCount )
{
#if defined _M_X64 || !defined _MSC_VER
	a += (intCount-1);
	b += (intCount-1);
	for (; intCount > 0; intCount--)
	{
		if (*a < *b)
		{
			*result = -1;
			return;
		}
		if (*a > *b)
		{
			*result = 1;
			return;
		}
		a--;
		b--;
	}
	*result = 0;
#else
	__asm
	{
		mov esi, [a]
		mov edi, [b]
		mov ecx, [intCount]
		mov edx, ecx
			dec edx
			shl edx, 2
			add esi, edx
			add edi, edx
			xor edx,edx
			std
			repe cmpsd
			je end //returns zero
			jb returnLessThan
			inc edx
			jmp end
returnLessThan:
		dec edx
			jmp end
end:
		mov ecx, [result]
		mov [ecx], edx
			cld //.net crashes if we don't do this! :O
	}
#endif
}


#ifdef _MANAGED
#pragma managed
#endif

ui64 computeMask(i32 bitOffset, byte bitCount);

void ClearBlock(void* destination, int destinationBitOffset, int bitCount);

void BitBlockTransfer(void* source, int sourceBitOffset, void* destination, int destinationBitOffset, int bitCount);

void BitWindowTransfer(void* source, int sourceWindowBitOffset, int sourceWindowBitSize, int sourceWindowAlignBit, void* destination, int destinationWindowBitOffset, int destinationWindowBitSize, int destinationWindowAlignBit);

void IntBlockAdd(ui64* a, ui64* b, byte longCount);

void IntBlockTwosCompliment(ui64* a, byte longCount);

bool IntBlockSub(ui64* a, ui64* b, byte longCount);

void IntBlockMul(ui64* _result, ui64* _a, ui64* _b, byte longCount);

void IntBlockMul(ui64* _result, ui64* _a, byte longCount, ui64* _b, byte bLongCount);

i32 ReverseBitScan(ui32* a, byte intCount, i32 startingBit);

bool IntBlockDiv(ui64* _result, ui64* _a, ui64* _b, byte longCount, int desiredBits);

int FindHeadAndApplyRounding( ui32* buffer, int headScanBackStart );