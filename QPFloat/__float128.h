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

#include "Helpers.h"
#include <exception>

#pragma warning(disable: 4949)
#pragma unmanaged
#pragma warning(default: 4949)

#pragma region Exceptions
#define FPU_EXCEPTION_DECLARATION(x) \
class x##ExceptionClass : public std::exception \
{ \
 virtual const char* what() const throw(); \
} extern _##x##Exception; \
inline void x##() { if (enable##x##Exception) throw _##x##Exception; }

FPU_EXCEPTION_DECLARATION(Overflow)
FPU_EXCEPTION_DECLARATION(Underflow)
FPU_EXCEPTION_DECLARATION(DivideByZero)
FPU_EXCEPTION_DECLARATION(Invalid)
FPU_EXCEPTION_DECLARATION(Inexact);
#undef FPU_EXCEPTION_DECLARATION
#pragma endregion

struct __float128;
#define QUAD_CONSTANT(name, dataSource) extern __float128 Quad##name ;
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

struct __float128
{
private:
	byte storage[16];
	inline void SetBiasedExponent(ui16 value)
	{
		value &= QUAD_EXPONENT_MASK;
		ui16* ptr = (ui16*)this + 7;
		*ptr &= ~QUAD_EXPONENT_MASK;
		*ptr |= value;
	}
	inline ui16 GetBiasedExponent()
	{
		return *((ui16*)this + 7) & QUAD_EXPONENT_MASK;
	}
	static void ReadOutResult(ui32 *buffer, int headScanBackStart, int biasedExponentAtScanStart, bool sign, __float128 &result);
	static __float128 PartialLn(__float128 &v);
public:
	__float128();
	static inline __float128& FromData(byte *data)
	{
		return *((__float128*)data);
	}
	inline bool GetSign()
	{
		byte* ptr = storage;
		return (*(ptr + 15) & 0x80) != 0;
	}
	inline void SetSign(bool value)
	{
		byte* ptr = storage;
		if (value)
			*(ptr + 15) |= 0x80;
		else
			*(ptr + 15) &= 0x7f;
	}
	inline i32 GetBase2Exponent()
	{
		return (i32)GetBiasedExponent() - QUAD_EXPONENT_BIAS;
	}
	inline void SetBase2Exponent(i32 value)
	{
		if (value >= QUAD_EXPONENT_MAX)
		{
			if (GetSign())
				*this = QuadNegativeInfinity;
			else
				*this = QuadPositiveInfinity;
			return;
		}
		if (value < QUAD_EXPONENT_MIN) value = QUAD_EXPONENT_MIN;
		SetBiasedExponent((ui16)(value + QUAD_EXPONENT_BIAS));
	}
	inline bool IsZero()
	{
		ui32* ptr = (ui32*)storage;
		if (*(ptr + 3) != 0)
			if (*(ptr + 3) != 0x80000000) return false;
		if (*ptr != 0) return false;
		ptr++;
		if (*ptr != 0) return false;
		ptr++;
		if (*ptr != 0) return false;
		return true;
	}
	inline bool IsNaN()
	{
		if (GetBiasedExponent() != 0x7FFF) return false;
		i32* ptr = (i32*)storage;
		if (*ptr != 0) return true;
		ptr++;
		if (*ptr != 0) return true;
		ptr++;
		if (*ptr != 0) return true;
		ptr++;
		if (*(ui16*)ptr != 0) return true;
		return false;
	}
	inline bool IsInfinite()
	{
		ui32* ptr = (ui32*)storage;
		if (*ptr != 0) return false;
		ptr++;
		if (*ptr != 0) return false;
		ptr++;
		if (*ptr != 0) return false;
		ptr++;
		if ((*ptr & 0x7FFFFFFF) != 0x7FFF0000) return false;
		return true;
	}
	inline bool IsSubNormal()
	{
		return GetBiasedExponent() == 0 && !IsZero();
	}
	static inline void Negate( __float128 &a)
	{
		*(a.storage + 15) ^= 0x80;
	}
	static inline void Negate( __float128 &a, __float128 &result )
	{
		result = a;
		Negate(result);
	}
	__float128 operator-()
	{
		__float128 copy = *this;
		Negate(copy);
		return copy;
	}
	static void Add( __float128 &a, __float128 &b, __float128 &result );
	static void Sub( __float128 &a, __float128 &b, __float128 &result );
	static void Mul( __float128 &a, __float128 &b, __float128 &result );
	static void Div( __float128 &a, __float128 &b, __float128 &result );
	static inline void Increment( __float128 &v )
	{
		Add(v, QuadOne, v);
	}
	static inline void Decrement( __float128 &v )
	{
		__float128 temp = QuadOne;
		Sub(v, temp, v);
	}
	__float128 inline operator+(__float128 &b)
	{
		__float128 result;
		Add(*this, b, result);
		return result;
	}
	__float128 inline operator-(__float128 &b)
	{
		__float128 result;
		Sub(*this, b, result);
		return result;
	}
	__float128 inline operator*(__float128 &b)
	{
		__float128 result;
		Mul(*this, b, result);
		return result;
	}
	__float128 inline operator/(__float128 &b)
	{
		__float128 result;
		Div(*this, b, result);
		return result;
	}
	bool operator ==(__float128 &b);
	bool operator !=(__float128 &b);
	bool operator >(__float128 &b);
	bool operator <(__float128 &b);
	bool inline operator >=(__float128 &b)
	{
		if (*this == b) return true; //ensure that equals test happens first because it's much faster
		else return *this > b;
	}
	bool inline operator <=(__float128 &b)
	{
		if (*this == b) return true; //ensure that equals test happens first because it's much faster
		else return *this < b;
	}
	__float128 inline operator++()
	{
		__float128 temp = QuadOne;
		Add(temp, *this, *this);
		return *this;
	}
	__float128 inline operator--()
	{
		__float128 temp = QuadNegOne;
		Add(temp, *this, *this);
		return *this;
	}
	__float128 inline operator<<(i32 shift)
	{
		__float128 temp = *this;
		temp.SetBase2Exponent(temp.GetBase2Exponent() + shift);
		return temp;
	}
	__float128 inline operator>>(i32 shift)
	{
		__float128 temp = *this;
		temp.SetBase2Exponent(temp.GetBase2Exponent() - shift);
		return temp;
	}
	__float128(double v);
	static void ToDouble(__float128 &v, double &result);
	__float128(i64 v);
	static void ToInt64(__float128 &v, i64 &result);
	__float128(i32 v);
	static void ToInt32(__float128 &v, i32 &result);
	static void Abs(__float128 &v, __float128 &result);
	static inline __float128 Abs(__float128 &v)
	{
		__float128 result;
		Abs(v, result);
		return result;
	}
	static __float128 Ln(__float128 &v);
	static __float128 Exp(__float128 &v);
	static __float128 Pow(__float128 &base, __float128 &exponent);
	__float128 inline operator^(__float128 &b)
	{
		return Pow(*this, b);
	}
	static __float128 Log(__float128 &v, __float128 &base);
	static __float128 Log2(__float128 &v);
	static void Ceiling( __float128 &v, __float128 &result );
	static inline __float128 Ceiling(__float128 &v)
	{
		__float128 result;
		Ceiling(v, result);
		return result;
	}
	static void Floor( __float128 &v, __float128 &result );
	static inline __float128 Floor(__float128 &v)
	{
		__float128 result;
		Floor(v, result);
		return result;
	}
	static void Round( __float128 &v, __float128 &result );
	static inline __float128 Round(__float128 &v)
	{
		__float128 result;
		Round(v, result);
		return result;
	}
	//result = Floor(Abs(v))
	static void Truncate( __float128 &v, ui64 &result );
	static inline ui64 Truncate( __float128 &v)
	{
		ui64 result;
		Truncate(v, result);
		return result;
	}
	//result = Abs(v) - Floor(Abs(v))
	static void Fraction( __float128 &v, __float128 &result );
	static inline __float128 Fraction(__float128 &v)
	{
		__float128 result;
		Fraction(v, result);
		return result;
	}
	static __float128 Sin(__float128 &v);
	static __float128 Cos(__float128 &v);
	static __float128 Tan(__float128 &v);
	static __float128 ASin(__float128 &v);
	static __float128 ACos(__float128 &v);
	static __float128 ATan(__float128 &v);
	static __float128 ATan2(__float128 &y, __float128 &x);
};

#pragma warning(disable: 4949)
#pragma managed
#pragma warning(default: 4949)