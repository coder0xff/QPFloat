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
#include "__float128.h"
#include "DoubleDecomposition.h"

#ifdef _MANAGED
#pragma unmanaged
#endif

#define QUAD_CONSTANT(name, dataSource) __float128 Quad##name = __float128::FromData( dataSource );
QUAD_CONSTANT(NaN, snan);
QUAD_CONSTANT(Indefinite, ind);
QUAD_CONSTANT(PositiveInfinity, posInf);
QUAD_CONSTANT(NegativeInfinity, negInf);
QUAD_CONSTANT(E, e);
QUAD_CONSTANT(Log2E, log2e);
QUAD_CONSTANT(Ln2, ln2);
QUAD_CONSTANT(TwoPi, twoPi);
QUAD_CONSTANT(Pi, pi);
QUAD_CONSTANT(HalfPi, halfPi);
QUAD_CONSTANT(QuarterPi, quarterPi);
QUAD_CONSTANT(Zero, zero);
QUAD_CONSTANT(NegZero, negZero);
QUAD_CONSTANT(One, one);
QUAD_CONSTANT(NegOne, negOne);
QUAD_CONSTANT(Half, half);
QUAD_CONSTANT(SinQuarterPi, sinQuarterPi);
#undef QUAD_CONSTANT

#define FPU_EXCEPTION_DEFINITION(x) \
const char* x##ExceptionClass::what()const \
{ \
	return "An " #x " occurred while performing a floating point operation."; \
} \
x##ExceptionClass _##x##Exception;

FPU_EXCEPTION_DEFINITION(Overflow)
FPU_EXCEPTION_DEFINITION(Underflow)
FPU_EXCEPTION_DEFINITION(DivideByZero)
FPU_EXCEPTION_DEFINITION(Invalid)
FPU_EXCEPTION_DEFINITION(Inexact)

void __float128::ReadOutResult( ui32 *buffer, int headScanBackStart, int biasedExponentAtScanStart, bool sign, __float128 &result )
{
	int implicitPosition = FindHeadAndApplyRounding(buffer, headScanBackStart);
	if (implicitPosition == -1)
	{
		//no bits
		result = QuadZero;
		result.SetSign(sign);
		return;
	}
	int currentExponent = biasedExponentAtScanStart + implicitPosition - headScanBackStart;
	if (currentExponent >= QUAD_EXPONENT_MASK)
	{
		if (sign)
			result = QuadNegativeInfinity;
		else
			result = QuadPositiveInfinity;
		Overflow();
		return;
	}
	if (currentExponent < 0) currentExponent = 0;
	int expInc = currentExponent - biasedExponentAtScanStart;
	result.SetBiasedExponent((ui16)currentExponent);
	BitBlockTransfer(buffer, headScanBackStart + expInc - 112 + (currentExponent == 0 ? 1 : 0), &result, 0, 112);
	result.SetSign(sign);
	if (currentExponent == 0) Underflow();
	if (enableInexactException) if (ReverseBitScan((ui32*)buffer, 0, headScanBackStart + expInc - 112 - 1 + (currentExponent == 0 ? 1 : 0)) != -1) Inexact();
}

__float128 __float128::PartialLn( __float128 &v )
{
	if (v == QuadOne) return QuadZero;
	//todo: implement a faster algorithm - though this is only used for 1 <= x < 2 (hence partial), so it's not too bad
	//http://en.wikipedia.org/wiki/Natural_logarithm#High_precision
	if (v.IsNaN()) return v;
	if (v == QuadPositiveInfinity) return v;
	if (v.GetSign()) return QuadIndefinite;
	__float128 temp1, temp2, temp3;
	Abs(v, temp1);
	if (temp1 < QuadOne)
	{
		//return -(1 / this).Ln();
		temp3 = QuadOne;
		Div(temp3, v, temp1);
		temp1 = Ln(temp1);
		Negate(temp1);
		return temp1;
	}
	//__float128 y = (-1 + this) / (1 + this);
	temp3 = QuadNegOne;
	Add(temp3, v, temp1);
	temp3 = QuadOne;
	Add(temp3, v, temp2);
	__float128 y;
	Div(temp1, temp2, y);

	//__float128 ySquared = y * y;
	__float128 ySquared;
	__float128::Mul(y, y, ySquared);

	bool affecting = true;
	__float128 result = QuadZero;
	int seriesValue = 1;
	while (affecting)
	{
		//__float128 increment = y / seriesValue;
		__float128 increment;
		temp3 = seriesValue;
		__float128::Div(y, temp3, increment);

		if (result.GetBase2Exponent() - increment.GetBase2Exponent() <= QUAD_SIGNIFICANT_BITS)
		{
			//result += increment;
			__float128::Add(result, increment, result);

			seriesValue += 2;

			//y *= ySquare;
			__float128::Mul(y, ySquared, y);
		}
		else
		{
			affecting = false;
		}
	}
	temp3 = 2;
	__float128::Mul(result, temp3, result);
	return result;
}

void __float128::Add( __float128 &a, __float128 &b, __float128 &result )
{
	//can only add values of the same sign. we do this check first so we don't repeat the other checks
	bool aSign = a.GetSign();
	bool bSign = b.GetSign();
	if (aSign && !bSign) { __float128 temp; Negate(a, temp); Sub(b, temp, result); return; }
	if (!aSign && bSign) { __float128 temp; Negate(b, temp); Sub(a, temp, result); return; }
	//if either value is NaN, then result is NaN
	if (a.IsNaN()) { result = a; return; }
	if (b.IsNaN()) { result = b; return; }
	//check for the special case of zero
	if (a.IsZero())	{ result = b; return; }
	if (b.IsZero())	{ result = a; return; }
	//cache the exponents
	int aExponent = a.GetBiasedExponent();
	int bExponent = b.GetBiasedExponent();
	//if the exponent is ExponentMask then it's either NaN or +-Infinity, but we already checked for NaN
	//if both are infinite, return either NaN or +-Infinity. Infinity-Infinity=NaN, Infinity-(-Infinity)=Infinity
	if (aExponent == QUAD_EXPONENT_MASK && bExponent == QUAD_EXPONENT_MASK) { result = a; return; } //already know signs are the same
	//if only one is infinite, then the other doesn't effect it
	if (aExponent == QUAD_EXPONENT_MASK) { result = a; return; }
	if (bExponent == QUAD_EXPONENT_MASK) { result = b; return; }
	//see if the values can even affect each other
	int exponentDistance = aExponent - bExponent;
	if (exponentDistance > QUAD_SIGNIFICANT_BITS + 2) { result = a; return; }
	if (exponentDistance < -QUAD_SIGNIFICANT_BITS - 2) { result = b;	return; }

	//pin the data
	byte* aData = a.storage;
	byte* bData = b.storage;

	//storage for performing the operations
	//[0] and [16] are guard bits since we offset everything by 8
	//[15](bit 1) is used for overflowing into the next byte 
	byte buffer[32];
	memset(buffer, 0, 32);
	byte* b1 = buffer;
	byte* b2 = buffer + 16;

	int resultExponent;
	bool aSubNormal, bSubNormal;
	if (aExponent > bExponent) //keep the larger value in b1 since the result reads are based on its alignment
	{
		resultExponent = aExponent;
		aSubNormal = a.IsSubNormal();
		bSubNormal = b.IsSubNormal();
		BitBlockTransfer(aData, 0, b1, 0 + 8 + (aSubNormal ? 1 : 0), 112);
		BitWindowTransfer(bData, 0, 112, exponentDistance, b2, 0, 128, 0 + 8 + (bSubNormal ? 1 : 0));
	}
	else //if (bExponent >= aExponent)
	{
		resultExponent = bExponent;
		aSubNormal = b.IsSubNormal();
		bSubNormal = a.IsSubNormal();
		BitBlockTransfer(bData, 0, b1, 0 + 8 + (bSubNormal ? 1 : 0), 112);
		BitWindowTransfer(aData, 0, 112, -exponentDistance, b2, 0, 128, 0 + 8 + (aSubNormal ? 1 : 0));
	}
	if (!aSubNormal) SetBit(b1, 112 + 8); //set the implicit bit
	if (!bSubNormal) SetBit(b2, 112 + 8 - abs(exponentDistance)); //set the implicit bit
	IntBlockAdd((ui64*)b1, (ui64*)b2, 2);

	ReadOutResult((ui32*)b1, 112 + 8 + 1, resultExponent + 1, aSign, result);
}

void __float128::Sub( __float128 &a, __float128 &b, __float128 &result )
{
	//see Add for notes on all these special cases, as they are nearly identical
	bool aSign = a.GetSign();
	if (aSign != b.GetSign()) { __float128 temp; Negate(b, temp); Add(result, a, temp); return; }
	if (a.IsNaN()) { result = a; return; }
	if (b.IsNaN()) { result = b; return; }
	if (b.IsZero())	{ result = a; return; }
	if (a.IsZero())	{ Negate(b, result); return; }
	int aExponent = a.GetBiasedExponent();
	int bExponent = b.GetBiasedExponent();
	if (aExponent == QUAD_EXPONENT_MASK && bExponent == QUAD_EXPONENT_MASK) { result = QuadIndefinite; Invalid(); return; } //already know signs are the same
	if (aExponent == QUAD_EXPONENT_MASK) { result = a; return; }
	if (bExponent == QUAD_EXPONENT_MASK) { Negate(b, result); return; }
	int exponentDistance = aExponent - bExponent;
	//exponentDistance early rejection doesn't work for subtraction (L.O. subs bubble up to H.O. bits)

	//pin the data
	byte* aData = a.storage;
	byte* bData = b.storage;

	//storage for the operations
	byte buffer[64];
	memset(buffer, 0, 64);
	byte* b1 = buffer;
	byte* b2 = buffer + 32;

	int resultExponent;
	bool negatedForValue; 
	//we always subtract the lesser (absolute) value because it's simpler
	//however, this means we need to negate the result if we do b-a
	bool aSubNormal, bSubNormal;
	if ((a > b) ^ aSign)
	{
		negatedForValue = false;
		resultExponent = aExponent;
		aSubNormal = a.IsSubNormal();
		bSubNormal = b.IsSubNormal();
		BitBlockTransfer(aData, 0, b1, 0 + 128 + (aSubNormal ? 1 : 0), 112);
		BitWindowTransfer(bData, 0, 112, exponentDistance, b2, 0, 256, 0 + 128 + (bSubNormal ? 1 : 0));
	}
	else //if (bExponent >= aExponent)
	{
		negatedForValue = true;
		resultExponent = bExponent;
		aSubNormal = b.IsSubNormal();
		bSubNormal = a.IsSubNormal();
		BitBlockTransfer(bData, 0, b1, 0 + 128 + (bSubNormal ? 1 : 0), 112);
		BitWindowTransfer(aData, 0, 112, -exponentDistance, b2, 0, 256, 0 + 128 + (aSubNormal ? 1 : 0));
	}
	if (!aSubNormal) SetBit(b1, 112 + 128);

	int bImpliedBitPosition = 112 + 128 - abs(exponentDistance);
	if (bImpliedBitPosition < 0)
	{
		bSubNormal = false; //doesn't matter in this case. We are subtracting one bit regardless
		bImpliedBitPosition = 0; //this is the treatment for exponents that are far off from each other
	}
	//it works on the fact that subtracting any value far far below is the same as subtracting any one guard bit

	if (!bSubNormal) SetBit(b2,bImpliedBitPosition);

	IntBlockSub((ui64*)b1, (ui64*)b2, 4);

	ReadOutResult((ui32*)b1, 112 + 128, resultExponent, aSign ^ negatedForValue, result);
}

void __float128::Mul( __float128 &a, __float128 &b, __float128 &result )
{
	if (a.IsNaN()) { result = a; return; }
	if (b.IsNaN()) { result = b; return; }
	bool aZero = a.IsZero();
	bool bZero = b.IsZero();
	bool aInf = a.IsInfinite();
	bool bInf = b.IsInfinite();
	if (aZero && bInf) { result = QuadNaN; Invalid(); return; }
	if (bZero && aInf) { result = QuadNaN; Invalid(); return; }
	bool aSign = a.GetSign();
	bool bSign = b.GetSign();
	if (aInf || bInf) { result = (aSign ^ bSign) ? QuadNegativeInfinity : QuadPositiveInfinity; return; }
	if (aZero || bZero) { result = 0; return; }
	byte buffer[64];
	memset(buffer, 0, 64);
	int resultExponent = a.GetBiasedExponent() + b.GetBiasedExponent() - QUAD_EXPONENT_BIAS;
	if (resultExponent >= QUAD_EXPONENT_MASK) //wont get any smaller (not even with a sub normal)
	{
		result = (aSign ^ bSign) ? QuadNegativeInfinity : QuadPositiveInfinity;
		Overflow();
		return;
	}
	byte* b1 = buffer;
	byte* b2 = buffer + 16;
	byte* res = buffer + 32;
	byte* aPtr = a.storage;
	byte* bPtr = b.storage;
	bool aSubNormal = a.IsSubNormal();
	bool bSubNormal = b.IsSubNormal();
	BitBlockTransfer(aPtr, 0, b1, aSubNormal ? 1 : 0, 112);
	BitBlockTransfer(bPtr, 0, b2, bSubNormal ? 1 : 0, 112);
	if (!aSubNormal) SetBit(b1, 112);
	if (!bSubNormal) SetBit(b2, 112);
	IntBlockMul((ui64*)res, (ui64*)b1, (ui64*)b2, 2);

	ReadOutResult((ui32*)res, 112 * 2 + 2, resultExponent + 2,  aSign ^ bSign, result);
}

void __float128::Div( __float128 &a, __float128 &b, __float128 &result )
{
	if (a.IsNaN()) { result = a; return; }
	if (b.IsNaN()) { result = b; return; }
	if (b.IsZero()) { result = QuadIndefinite; DivideByZero(); return; }
	if (a.IsZero()) { result = a; return; }
	int aExponent = a.GetBiasedExponent();
	int bExponent = b.GetBiasedExponent();

	if (aExponent == QUAD_EXPONENT_MASK)
	{
		if (bExponent == QUAD_EXPONENT_MASK) { result = QuadIndefinite; Invalid(); return; }
		else { result = a; return; }
	}
	bool aSign = a.GetSign();
	bool bSign = b.GetSign();
	if (bExponent == QUAD_EXPONENT_MASK)
	{
		result = (aSign ^ bSign) ? QuadZero : QuadNegZero;
		return;
	}
	int resultExponent = aExponent - bExponent + QUAD_EXPONENT_BIAS;
	if (resultExponent >= QUAD_EXPONENT_MASK)
	{
		result = (aSign ^ bSign) ? QuadNegativeInfinity : QuadPositiveInfinity;
		return;
	}		
	byte buffer[16 + 32 + 32];
	memset(buffer, 0, 16 + 32 + 32);

	byte* b2 = buffer;
	byte* b1 = buffer + 16;
	byte* res = buffer + 48;
	byte* aPtr = a.storage;
	byte* bPtr = b.storage;
	bool aIsSubNormal = a.IsSubNormal();
	bool bIsSubNormal = b.IsSubNormal();
	BitBlockTransfer(aPtr, 0, b1 + 16, aIsSubNormal ? 1 : 0, 112);
	BitBlockTransfer(bPtr, 0, b2, bIsSubNormal ? 1 : 0, 112);
	if (!aIsSubNormal) SetBit(b1 + 16, 112);
	if (!bIsSubNormal) SetBit(b2, 112);
	IntBlockDiv((ui64*)res, (ui64*)b1, (ui64*)b2, 2, 114);

	if (aIsSubNormal || bIsSubNormal)
		//normally, the implicit head bit will be in position 127 or 128
		//this is because the operand head bits are placed in 128 + 112, and 112
		//and the division shift in the head therefore places the head at (128 + 112) - 112
		//HOWEVER! This cannot be assumed to be true with subnormals, so we start way out at 128+112
		//because those high bits can still be set in the worst case scenario
		ReadOutResult((ui32*)res, 128 + 112, resultExponent + 112, aSign ^ bSign, result);
	else
		ReadOutResult((ui32*)res, 128, resultExponent, aSign ^ bSign, result);
}

bool __float128::operator==( __float128 &b )
{
	if (this->IsNaN() || b.IsNaN()) return false; //NaN =/= NaN
	if (this->IsZero() && b.IsZero()) return true; //-0 = 0
	ui64* aPtr = (ui64*)this->storage;
	ui64* bPtr = (ui64*)b.storage;
	if (*(aPtr++) != *(bPtr++)) return false;
	if (*(aPtr) != *(bPtr)) return false;
	return true;
}

bool __float128::operator!=( __float128 &b )
{
	if (this->IsNaN() || b.IsNaN()) return true; //NaN =/= NaN
	if (this->IsZero() && b.IsZero()) return false;
	ui64* aPtr = (ui64*)this->storage;
	ui64* bPtr = (ui64*)b.storage;
	if (*(aPtr++) != *(bPtr++)) return true;
	if (*(aPtr) != *(bPtr)) return true;
	return false;
}

bool __float128::operator>( __float128 &b )
{
	if (this->IsNaN() || b.IsNaN()) return false;
	bool aSign = this->GetSign();
	if (this->IsZero()) return b.GetSign();
	if (b.IsZero()) return !aSign;
	if (aSign != b.GetSign()) return !aSign;
	ui16 aExponent = this->GetBiasedExponent();
	ui16 bExponent = b.GetBiasedExponent();
	if (aExponent < bExponent)
		return aSign;
	else if (aExponent > bExponent)
		return !aSign;
	else
	{
		byte* aPtr = this->storage;
		byte* bPtr = b.storage;
		byte temp[32];
		memset(temp, 0, 32);
		BitBlockTransfer(aPtr, 0, temp, 0, 112);
		BitBlockTransfer(bPtr, 0, temp + 16, 0, 112);
		i32 cmp;
		IntBlockCompare(&cmp, (ui32*)temp, (ui32*)(temp+16), 4);
		return cmp == (aSign ? -1 : 1);
	}
}

bool __float128::operator<( __float128 &b )
{
	if (this->IsNaN() || b.IsNaN()) return false;
	bool aSign = this->GetSign();
	if (this->IsZero()) return !b.GetSign();
	if (b.IsZero()) return aSign;
	if (aSign != b.GetSign()) return aSign;
	ui16 aExponent = this->GetBiasedExponent();
	ui16 bExponent = b.GetBiasedExponent();
	if (aExponent < bExponent)
		return !aSign;
	else if (aExponent > bExponent)
		return aSign;
	else
	{
		byte* aPtr = this->storage;
		byte* bPtr = b.storage;
		byte temp[32];
		memset(temp, 0, 32);
		BitBlockTransfer(aPtr, 0, temp, 0, 112);
		BitBlockTransfer(bPtr, 0, temp + 16, 0, 112);
		i32 cmp;
		IntBlockCompare(&cmp, (ui32*)temp, (ui32*)(temp+16), 4);
		return cmp == (aSign ? 1 : -1);
	}
}

__float128::__float128(double v)
{
	DoubleDecomposition d;
	d.value = v;
	int checkExponent = d.GetBiasedExponent();
	if (checkExponent == 0x7FF)
	{
		*this = d.GetSign() ? QuadNegativeInfinity : QuadPositiveInfinity;
		return;
	}
	memset(storage, 0, 16);
	SetSign(d.GetSign());
	d.GetMantissa(storage, 14);
	if (checkExponent == 0)
		//it's a non-normalized value
		SetBiasedExponent(0);
	else
		SetBase2Exponent(d.GetUnbiasedExponent());
}

void __float128::ToDouble(__float128 &v, double &result)
{
	DoubleDecomposition _result;
	_result.value = 0;
	_result.SetSign(v.GetSign());
	_result.SetUnbiasedExponent(v.GetBase2Exponent());
	_result.SetMantissa(v.storage, 14); //setting the mantissa will check for rounding and update exponent if necessary
	result = _result.value;
}

void __float128::ToInt64(__float128 &v, i64 &result)
{
	bool sign = v.GetSign();
	int exp = v.GetBase2Exponent();
	if (exp <= -1)
	{
		result = sign ? -1 : 0;
		return;
	}
	else if (exp > 62)
	{
		result = sign ? (i64)0x8000000000000000 : (i64)0x7fffffffffffffff;
		return;
	}
	else
	{
		result = 0;
		byte* ptr = v.storage;
		BitWindowTransfer(ptr, 0, 112, 112 - exp, &result, 0, 64, 0);
		SetBit(&result, exp);
		if (sign)
		{
			result = -result;
		}
		else
		{
		}
		return;
	}
}

void __float128::ToInt32(__float128 &v, i32 &result)
{
	bool sign = v.GetSign();
	int exp = v.GetBase2Exponent();
	if (exp <= -1)
	{
		result = sign ? -1 : 0;
		return;
	}
	else if (exp > 30)
	{
		result = sign ? (i32)0x80000000 : (i32)0x7fffffff;
		return;
	}
	else
	{
		result = 0;
		byte* ptr = v.storage;
		BitWindowTransfer(ptr, 0, 112, 112 - exp, &result, 0, 32, 0);
		SetBit(&result, exp);
		if (sign)
		{
			result = -result;
		}
		else
		{
		}
		return;
	}
}

__float128::__float128(i32 v)
{
	byte* ptr = storage;
	memset(ptr, 0, 16);
	if (v == 0)
	{
		return;
	}
	bool sign = v < 0;
	if (sign) v = -v;
	SetSign(sign);
	i32 firstBit = ReverseBitScan((ui32*)&v, 1, 30);
	SetBase2Exponent(firstBit);
	BitWindowTransfer(&v, 0, 32, firstBit, ptr, 0, 112, 112);
}

__float128::__float128( i64 v )
{
	byte* ptr = storage;
	memset(ptr, 0, 16);
	if (v == 0)
	{
		return;
	}
	bool sign = v < 0;
	if (sign) v = -v;
	SetSign(sign);
	i32 firstBit = ReverseBitScan((ui32*)&v, 2, 62);
	SetBase2Exponent(firstBit);
	BitWindowTransfer(&v, 0, 64, firstBit, ptr, 0, 112, 112);
}

__float128::__float128()
{
	*this = QuadZero;
}

__float128 __float128::Ln( __float128 &v )
{
	return Log2(v) * QuadLn2;
}

__float128 __float128::Log2( __float128 &v )
{
	if (v.IsZero()) { Invalid(); return QuadNaN; }
	if (v.IsNaN()) return v;
	__float128 baseTwoMantissa = v;
	baseTwoMantissa.SetBase2Exponent(0);
	__float128 baseTwoExponent;
	baseTwoExponent = v.GetBase2Exponent();
	__float128 base2LogOfMantissa = PartialLn(baseTwoMantissa) * QuadLog2E;
	return baseTwoExponent + base2LogOfMantissa;
}

__float128 __float128::PartialExp( __float128 &v )
{
	double checkV;
	__float128::ToDouble(v, checkV);
	if (v.IsNaN()) return v;
	if (v.IsZero()) return 1;
	if (v == QuadPositiveInfinity) return v;
	if (v == QuadNegativeInfinity) return 0;
	__float128 result = 1;
	__float128 factorial = 1;
	int iteration = 1;
	__float128 x = v;
	__float128 temp1;
	bool affecting = true;
	while (affecting)
	{
		__float128 increment;
		Div(x, factorial, increment);
		if (!increment.IsZero() && result.GetBase2Exponent() - increment.GetBase2Exponent() <= QUAD_SIGNIFICANT_BITS)
		{
			Add(result, increment, result);
			iteration++;
			__float128 temp1 = iteration;
			Mul(factorial, temp1, factorial);
			Mul(x, v, x);
		}
		else
		{
			affecting = false;
		}
	}
	return result;
}

__float128 __float128::Base2Exp(i32 exponent)
{
	if (exponent >= QUAD_EXPONENT_MAX) return QuadPositiveInfinity;
	if (exponent > QUAD_EXPONENT_MIN)
	{
		__float128 result = QuadOne;
		result.SetBase2Exponent(exponent);
		return result;
	}
	else //is subnormal or zero
	{
		i32 bitOffset = 111 - (QUAD_EXPONENT_MIN - exponent);
		if (bitOffset < 0) return QuadZero;
		__float128 result = QuadZero;
		SetBit(result.storage, bitOffset);
		return result;
	}
}

__float128 __float128::Base2Exp (__float128 &v)
{
	i32 integerPortion;
	__float128 fractionalPortion;
	if (v > QuadZero)
	{
		__float128 temp;
		__float128::Floor(v, temp);
		ToInt32(temp, integerPortion);
	}
	else
	{
		__float128 temp;
		__float128::Ceiling(v, temp);
		ToInt32(temp, integerPortion);
	}
	__float128 temp = (__float128)integerPortion;
	fractionalPortion = v - temp;
	__float128 fromIntegerPortion = Base2Exp(integerPortion); //this may result in Infinity, subnormal, or zero
	if (fromIntegerPortion.IsInfinite() || fromIntegerPortion.IsZero()) return fromIntegerPortion;
	Mul(fractionalPortion, QuadLn2, fractionalPortion);
	__float128 fromFractionPortion = PartialExp(fractionalPortion);
	return fromIntegerPortion * fromFractionPortion;
}

__float128 __float128::Exp(__float128 &v)
{
	__float128 temp;
	Mul(v, QuadLog2E, temp);
	return Base2Exp(temp);
}
__float128 __float128::Pow( __float128 &base, __float128 &exponent )
{
	if (base.IsNaN()) return base;
	if (exponent.IsNaN()) return exponent;
	if (base.IsZero())
	{
		if (exponent.IsZero()) return QuadNaN;
		return QuadZero;
	}
	if (exponent.IsZero())
	{
		if (base.IsInfinite()) return QuadNaN;
		return QuadOne;
	}
	if (base.IsInfinite())
	{
		if (exponent.IsInfinite())
		{
			if (exponent.GetSign()) return QuadZero;
			return base;
		}
		if ((((__float128)-1) ^ exponent).GetSign()) return QuadNegativeInfinity; else return QuadPositiveInfinity;
	}
	if (exponent.IsInfinite())
	{
		if (exponent.GetSign()) return QuadZero;
		if (base.GetSign()) return QuadNegativeInfinity; else return QuadPositiveInfinity;
	}
	if (base == QuadOne) return QuadOne;
	__float128 temp = Log2(base);
	Mul(temp, exponent, temp);
	return Base2Exp(temp);
}
void __float128::Abs( __float128 &v, __float128 &result )
{
	result = v;
	result.SetSign(false);
}

__float128 __float128::Sin( __float128 &v )
{
	if (v.IsNaN()) return v;
	//first wrap to -2Pi < x < 2Pi
	__float128 temp = v + QuadHalfPi;
	Div(temp, QuadTwoPi, temp);
	temp = Fraction(temp);
	if (temp.GetSign()) Add(temp, QuadOne, temp);
	bool negate = false;
	if (temp > QuadHalf)
	{
		negate = true;
		Sub(temp, QuadHalf, temp);
	}		
	Mul(temp, QuadTwoPi, temp);
	Sub(temp, QuadHalfPi, temp);
	if (temp.IsZero()) return v;

	__float128 currentX = temp;
	__float128 negVSquared;
	Mul(temp, temp, negVSquared);
	Negate(negVSquared);

	int iIteration = 1;
	__float128 factorial = QuadOne;
	__float128 result = QuadZero;
	__float128 one = QuadOne;
	bool affecting = true;
	while (affecting)
	{
		__float128 increment;
		__float128 factorialReciprocal = __float128::FactorialReciprocal(iIteration);
		iIteration += 2;
		Mul(currentX, factorialReciprocal, increment);
		if (!increment.IsZero() && result.GetBase2Exponent() - increment.GetBase2Exponent() <= QUAD_SIGNIFICANT_BITS)
		{
			Add(result, increment, result);
			Mul(currentX, negVSquared, currentX);
		}
		else
		{
			affecting = false;
		}
	}
	if (negate) Negate(result);
	return result;
}

__float128 __float128::Cos( __float128 &v )
{
	__float128 temp = v + QuadHalfPi;
	return Sin(temp);
}

void __float128::SinCos( __float128 &v, __float128 &resultSin, __float128 &resultCos )
{
	if (v.IsNaN())
	{
		resultCos = resultSin = QuadNaN;
	}
	//first wrap to -2Pi < x < 2Pi
	__float128 tempSin = v + QuadHalfPi;
	__float128 tempCos = v + QuadPi;
	Div(tempSin, QuadTwoPi, tempSin);
	Div(tempCos, QuadTwoPi, tempCos);
	tempSin = Fraction(tempSin);
	tempCos = Fraction(tempCos);
	if (tempSin.GetSign()) Add(tempSin, QuadOne, tempSin);
	if (tempCos.GetSign()) Add(tempCos, QuadOne, tempCos);
	bool negateSin = false, negateCos = false;
	if (tempSin > QuadHalf)
	{
		negateSin = true;
		Sub(tempSin, QuadHalf, tempSin);
	}
	if (tempCos > QuadHalf)
	{
		negateCos = true;
		Sub(tempCos, QuadHalf, tempCos);
	}
	Mul(tempSin, QuadTwoPi, tempSin);
	Sub(tempSin, QuadHalfPi, tempSin);
	if (tempSin.IsZero())
	{
		resultSin = QuadZero;
		resultCos = QuadOne;
		return;
	}
	Mul(tempCos, QuadTwoPi, tempCos);
	Sub(tempCos, QuadHalfPi, tempCos);

	__float128 currentXSin = tempSin;
	__float128 currentXCos = tempCos;
	__float128 negVSquaredSin;
	Mul(tempSin, tempSin, negVSquaredSin);
	Negate(negVSquaredSin);
	
	__float128 negVSquaredCos;
	Mul(tempCos, tempCos, negVSquaredCos);
	Negate(negVSquaredCos);

	int iIteration = 1;
	resultSin = QuadZero;
	resultCos = QuadZero;
	__float128 one = QuadOne;
	bool affecting = true;
	while (affecting)
	{
		__float128 factorialReciprocal = FactorialReciprocal(iIteration);
		iIteration += 2;
		__float128 incrementSin;
		Mul(currentXSin, factorialReciprocal, incrementSin);
		__float128 incrementCos;
		Mul(currentXCos, factorialReciprocal, incrementCos);
		if ((!incrementSin.IsZero()
			&& resultSin.GetBase2Exponent() - incrementSin.GetBase2Exponent() <= QUAD_SIGNIFICANT_BITS) ||
			(!incrementCos.IsZero()
			&& resultCos.GetBase2Exponent() - incrementCos.GetBase2Exponent() <= QUAD_SIGNIFICANT_BITS))
		{
			Add(resultSin, incrementSin, resultSin);
			Add(resultCos, incrementCos, resultCos);

			Mul(currentXSin, negVSquaredSin, currentXSin);
			Mul(currentXCos, negVSquaredCos, currentXCos);
		}
		else
		{
			affecting = false;
		}
	}
	if (negateSin) Negate(resultSin);
	if (negateCos) Negate(resultCos);
}

__float128 __float128::Tan( __float128 &v )
{
	__float128 sin, cos, tan;
	SinCos(v, sin, cos);
	Div(sin, cos, tan);
	return tan;
}

__float128 __float128::ASin( __float128 &v )
{
	if (v.IsNaN()) return v;
	if (v > QuadOne) return QuadNaN;
	if (v < QuadNegOne) return QuadNaN;
	if (Abs(v) > QuadSinQuarterPi)
	{
		__float128 temp;
		Mul(v, v, temp);
		//double dTemp = temp.ToDouble();
		bool negate = v.GetSign();
		Sub(QuadOne, temp, temp);
		//dTemp = temp.ToDouble();
		temp = Pow(temp, QuadHalf);
		//dTemp = temp.ToDouble();
		temp = ACos(temp);
		if (negate) Negate(temp);
		return temp;
	}
	__float128 result = QuadZero;
	__float128 Two = 2;
	__float128 Half = QuadOne / Two;
	__float128 Four = 4;
	__float128 i = 0;
	int iIteration = 0;
	__float128 i2 = 0;
	__float128 i4 = 0;
	__float128 fourPowITimesIFactorialSquared = 1;
	__float128 i2Factorial = 1;
	__float128 i2PlusOne = 1;
	__float128 vPowI2PlusOne = v;
	__float128 vSquared;
	Mul(v, v, vSquared);

	bool affecting = true;
	while (affecting)
	{
		__float128 increment;
		Mul(fourPowITimesIFactorialSquared, i2PlusOne, increment);
		Div(i2Factorial, increment, increment);
		Mul(increment, vPowI2PlusOne, increment);		
		//double dIncrement = increment.ToDouble();

		if (!increment.IsZero() && result.GetBase2Exponent() - increment.GetBase2Exponent() <= QUAD_SIGNIFICANT_BITS)
		{
			//dResult += dIncrement;
			Add(result, increment, result);

			//i++;
			Add(i, QuadOne, i);

			iIteration++;

			//i2 = 2*i (start with 0, and add two each iteration)
			//i2Factorial = (2i)! (start with one and multiply by i2-1 and i2 each iteration
			Add(i2, QuadOne, i2);
			Mul(i2Factorial, i2, i2Factorial);
			Add(i2, QuadOne, i2);
			Mul(i2Factorial, i2, i2Factorial);


			Mul(vPowI2PlusOne, vSquared, vPowI2PlusOne);
			//i2PlusOne = 2*i + 1 (start with one, and add two each iteration)
			Add(i2PlusOne, Two, i2PlusOne);

			//fourPowITimesIFactorialSquare = 4^i * (i!) ^ 2
			//start with one, and multiply by 4i * i each iteration
			__float128 temp;
			Add(i4, Four, i4);
			Mul(i4, i, temp);
			Mul(fourPowITimesIFactorialSquared, temp, fourPowITimesIFactorialSquared);
			//double dResult = result.ToDouble();
		}
		else
		{
			affecting = false;
		}
		if (iIteration > 14 && result.IsZero()) return result; //the increment magnitude check doesn't work with zero
	}
	return result;
}

__float128 __float128::ACos( __float128 &v )
{
	if (v.IsNaN()) return v;
	if (v > QuadOne) return QuadNaN;
	if (v < QuadNegOne) return QuadNaN;
	if (v.GetSign())
	{
		__float128 temp1;
		__float128 temp2;
		Negate(QuadHalfPi, temp1);
		temp2 = ASin(v);
		Sub(temp1, temp2, temp1);
		return temp1;
		//return (-QuadHalfPi) - ASin(v);
	}
	else
	{
		__float128 temp;
		temp = ASin(v);
		Sub(QuadHalfPi, temp, temp);
		return temp;
		//return QuadHalfPi - ASin(v);
	}
}

__float128 __float128::ATan( __float128 &v )
{
	if (v.IsNaN()) return v;
	if (v.IsInfinite())
	{
		if (v.GetSign())
			return -QuadHalfPi;
		else
			return QuadHalfPi;
	}
	if (v.IsZero()) return QuadZero;
	if (v.GetSign())
	{
		__float128 temp;
		Negate(v, temp);
		temp = ATan(temp);
		Negate(temp);
		return temp;
	}
	if (v > QuadOne)
	{
		__float128 temp;
		Div(QuadOne, v, temp);
		temp = ATan(temp);
		Sub(QuadHalfPi, temp, temp);
		return temp;
	}
	if (v > QuadHalf)
	{
		//convergence is very slow for values near one, so we are going to compute from arcsin
		//sin = tan / sqrt((1 + tan^2))
		__float128 temp;
		Mul(v, v, temp);
		Add(temp, QuadOne, temp);
		temp = Pow(temp, QuadHalf);
		__float128 sin;
		Div(v, temp, sin);
		return ASin(sin);
	}
	__float128 result = QuadZero;
	__float128 Two = 2;
	int iIteration = 0;
	__float128 i2PlusOne = 1;
	__float128 vPowI2PlusOne = v;
	__float128 vSquared;
	bool signFlip = false;
	Mul(v, v, vSquared);

	bool affecting = true;
	while (affecting)
	{
		__float128 increment;
		Div(vPowI2PlusOne, i2PlusOne, increment);

		if (!increment.IsZero() && result.GetBase2Exponent() - increment.GetBase2Exponent() <= QUAD_SIGNIFICANT_BITS)
		{
			//dResult += dIncrement;
			if (signFlip)
				Sub(result, increment, result);
			else
				Add(result, increment, result);

			signFlip = !signFlip;
			iIteration++;

			Mul(vPowI2PlusOne, vSquared, vPowI2PlusOne);
			//i2PlusOne = 2*i + 1 (start with one, and add two each iteration)
			Add(i2PlusOne, Two, i2PlusOne);

			//fourPowITimesIFactorialSquare = 4^i * (i!) ^ 2
			//start with one, and multiply by 4i * i each iteration
			//double dResult = result.ToDouble();
		}
		else
		{
			affecting = false;
		}
		if (iIteration > 14 && result.IsZero()) return result; //the increment magnitude check doesn't work with zero
	}
	return result;
}

__float128 __float128::ATan2( __float128 &y, __float128 &x )
{
	__float128 tan = y / x;
	__float128 partial = ATan(tan);
	if (x.GetSign())
	{
		if (y.GetSign())
			return partial - QuadPi;
		else
			return partial + QuadPi;
	}
	else
		return partial;
}

void __float128::Fraction( __float128 &v, __float128 &result )
{
	int unbiasedExponent = v.GetBase2Exponent();
	if (unbiasedExponent < 0) 
		result = v;
	else
	{
		result = v;
		byte* data = result.storage;
		byte buffer[14];
		memset(buffer, 0, 14);
		BitBlockTransfer(data, 0, buffer, 0, 112);
		if (unbiasedExponent > 0) ClearBlock(buffer, 112 - unbiasedExponent, unbiasedExponent); //clear the integer portion
		int HOBit = ReverseBitScan((ui32*)buffer, 4, 111);
		if (HOBit == -1) { result = QuadZero; return; }
		unbiasedExponent = unbiasedExponent + HOBit - 112;
		ClearBlock(data, 0, 112);
		BitWindowTransfer(buffer, 0, 112, HOBit, data, 0, 112, 112); //HO bit is not copied
		result.SetBase2Exponent(unbiasedExponent);
	}
}

void __float128::Ceiling( __float128 &v, __float128 &result )
{
	int unbiasedExponent = v.GetBase2Exponent();
	if (unbiasedExponent == QUAD_EXPONENT_MAX) result = v; //NaN, +inf, -inf
	else if (unbiasedExponent < 0)
	{
		if (v.GetSign())
			result = FromData(zero);
		else
			result = FromData(one);
	}
	else
	{
		if (unbiasedExponent >= 112) { result = v; return; }
		result = v;
		int bitsToErase = 112 - unbiasedExponent;
		ClearBlock(result.storage, 0, bitsToErase);
		if (result != v)
		{
			if (!result.GetSign()) ++result;
		}
	}
}

__float128 __float128::Log( __float128 &v, __float128 &base )
{
	__float128 temp1, temp2;
	temp1 = Log2(v);
	temp2 = Log2(base);
	Div(temp1, temp2, temp1);
	return temp1;
	//return Log2(v) / Log2(base);
}

void __float128::Round( __float128 &v, __float128 &result )
{
	int unbiasedExponent = v.GetBase2Exponent();
	if (unbiasedExponent == QUAD_EXPONENT_MAX) result = v; //NaN, +inf, -inf
	else if (unbiasedExponent < -1) result = QuadZero;
	else if (unbiasedExponent == -1) result = v.GetSign() ? QuadNegOne : QuadOne;
	else if (unbiasedExponent >= 112) result = v;
	else
	{
		result = v;
		bool roundUp = ReadBit(result.storage, 111 - unbiasedExponent);
		int bitsToErase = 112 - unbiasedExponent;
		ClearBlock(result.storage, 0, bitsToErase);
		if (roundUp) ++result;
	}
}

void __float128::Truncate( __float128 &v, ui64 &result )
{
	int exp = v.GetBase2Exponent();
	if (exp < 0) { result = 0; }
	else if (exp > 63) result = (ui64)0xffffffffffffffff;
	else
	{
		result = 0;
		BitWindowTransfer(v.storage, 0, 112, 112 - exp, &result, 0, 64, 0);
		SetBit(&result, exp);
	}
}

void __float128::Floor( __float128 &v, __float128 &result )
{
	int unbiasedExponent = v.GetBase2Exponent();
	if (unbiasedExponent == QUAD_EXPONENT_MAX) result = v; //NaN, +inf, -inf
	else if (unbiasedExponent < 0)
	{
		if (v.GetSign())
		{
			result = FromData(one); Negate(result);
		}
		else
		{
			result = FromData(zero);
		}
	}
	else
	{
		if (unbiasedExponent >= 112) { result = v; return; }
		result = v;
		int bitsToErase = 112 - unbiasedExponent;
		ClearBlock(result.storage, 0, bitsToErase);
		if (result != v)
		{
			if (result.GetSign()) --result;
		}
	}
}

#ifdef _MANAGED
#pragma managed
#endif