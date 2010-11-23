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

#include "Stdafx.h"
#include "DoubleDecomposition.h"
#include "Helpers.h"

#define DBL_SIGNIFICANT_BITS 52
#define DBL_EXPONENT_BITS 11
#define DBL_EXPONENT_BIAS 0x3FF

#define DBL_MANTISSA_BYTES (DBL_SIGNIFICANT_BITS + 7) / 8
#define DBL_EXPONENT_MASK ((1 << DBL_EXPONENT_BITS) - 1)
#define DBL_EXPONENT_MAX (DBL_EXPONENT_MASK - DBL_EXPONENT_BIAS)
#define DBL_EXPONENT_MIN (-DBL_EXPONENT_BIAS)

#ifdef _MANAGED
#pragma unmanaged
#endif

bool DoubleDecomposition::GetSign()
{
	byte* ptr = (byte*)&value + 7;
	return (*ptr & (byte)0x80) != 0;
}

void DoubleDecomposition::SetSign(bool value)
{
	byte* ptr = (byte*)&this->value + 7;
	if (value)
		*ptr |= 0x80;
	else
		*ptr &= 0x7F;
}

ui16 DoubleDecomposition::GetBiasedExponent()
{
	ui16* ptr = ((ui16*)&this->value) + 3;
	return (ui16)((*ptr >> 4) & 0x7FF);
}

void DoubleDecomposition::SetBiasedExponent(ui16 value)
{
	const int exponentTruncate = (1 << DBL_EXPONENT_BITS) - 1;
	ui16* ptr = ((ui16*)&this->value) + 3;
	*ptr = (ui16)(*ptr & 0x800F | (value & exponentTruncate) << 4);
}

int DoubleDecomposition::GetUnbiasedExponent()
{
	return (int)GetBiasedExponent() - DBL_EXPONENT_BIAS;
}

void DoubleDecomposition::SetUnbiasedExponent(int value)
{
	if (value >= DBL_EXPONENT_MAX)
	{
		bool s = GetSign();
		value = 0;
		SetSign(s);
		SetBiasedExponent((1 << DBL_EXPONENT_BITS) - 1); //infinity
		return;
	}
	if (value < DBL_EXPONENT_MIN) value = DBL_EXPONENT_MIN; //-1023 results in a subnormal
	SetBiasedExponent((ui16)(value + DBL_EXPONENT_BIAS));
}

void DoubleDecomposition::GetMantissa(byte* data, int byteCount)
{
	if (byteCount < DBL_MANTISSA_BYTES) throw -1;
	BitBlockTransfer(&value, 0, data, byteCount * 8 - DBL_SIGNIFICANT_BITS, DBL_SIGNIFICANT_BITS); //place in H.O.
}

void DoubleDecomposition::SetMantissa(byte* data, int byteCount)
{
	if (GetBiasedExponent() == DBL_EXPONENT_MASK)
	{
		//cannot set the mantissa while exponent represents infinity
		//since it'd create an invalid value
		return;
	}
	int bitCount = byteCount * 8;
	double* dptr = &value;
	if (bitCount < DBL_SIGNIFICANT_BITS) ClearBlock((byte*)dptr, 0, DBL_SIGNIFICANT_BITS);
	bool carry = false;
	if (bitCount > DBL_SIGNIFICANT_BITS) carry = ReadBit(data, bitCount - DBL_SIGNIFICANT_BITS - 1);
	//load highest order bits into highest order bits
	if (!carry)
	{
		BitWindowTransfer(data, 0, bitCount, bitCount, (byte*)dptr, 0, DBL_SIGNIFICANT_BITS, DBL_SIGNIFICANT_BITS);
	}
	if (carry)
	{
		ui64 mantissaMask = ((ui64)1 << DBL_SIGNIFICANT_BITS) - 1;
		ui64* ui64Ptr = (ui64*)dptr;
		if ((*ui64Ptr & mantissaMask) == mantissaMask)
		{
			//we cannot simply increment because it'd overflow
			//instead we clear all the bits and increment the exponent
			*ui64Ptr &= ~mantissaMask;
			SetBiasedExponent(GetBiasedExponent()+1); //we know it's not ExponentMax already because we checked at the beginning of the func
		}
		else
		{
			//we can increment the whole ui64 without it overflowing to the exponent part
			//but first we need to copy it in ^^
			BitWindowTransfer(data, 0, bitCount, bitCount, (byte*)dptr, 0, DBL_SIGNIFICANT_BITS, DBL_SIGNIFICANT_BITS);
			(*ui64Ptr)++;
		}
	}
}

#ifdef _MANAGED
#pragma unmanaged
#endif