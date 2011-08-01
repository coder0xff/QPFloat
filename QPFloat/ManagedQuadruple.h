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
#include "Helpers.h"
#include "inline_array.h"

using namespace System;
using namespace System::Runtime::InteropServices;

#define QUAD_CONSTANT(constName, dataSource) \
	static property Quadruple constName \
	{ \
		inline Quadruple get() \
		{ \
			return FromData( dataSource ); \
		} \
	}


#define FPU_EXCEPTION_DECLARATION(x) \
	public: \
	static property bool Enable##x##Exception \
	{ \
		bool get() \
		{ \
			return enable##x##Exception; \
		} \
		void set(bool value) \
		{ \
			enable##x##Exception = value; \
		} \
	} \
	private: \
	static inline void x () { if (Enable##x##Exception) throw gcnew x##Exception(); }

namespace System
{
	[Serializable]
	public ref class UnderflowException : ArithmeticException {};
	[Serializable]
	public ref class InexactException : ArithmeticException {};
	[Serializable]
	public ref class InvalidException : ArithmeticException {};

	[System::Diagnostics::DebuggerDisplayAttribute("{ToString()}")]
	[System::Runtime::InteropServices::StructLayout(System::Runtime::InteropServices::LayoutKind::Explicit)]
	[System::Serializable]
	public value struct Quadruple
	{
	private:
		[System::Runtime::InteropServices::FieldOffset(0)]
		inline_array<byte, 16> storage;
		property ui16 biasedExponent
		{
			inline ui16 get()
			{
				pin_ptr<byte> ptr = storage;
				return (*(ui16*)(ptr+14)) & QUAD_EXPONENT_MASK;
			}

			inline void set( ui16 value )
			{
				value &= QUAD_EXPONENT_MASK;
				pin_ptr<byte> ptr = storage;
				ui16* sptr = (ui16*)(ptr + 14);
				*sptr &= ~QUAD_EXPONENT_MASK;
				*sptr |= value;
			}
		}
		property array<byte>^ mantissa
		{
			array<byte>^ get();
			void set( array<byte>^ value );
		}
#define useDebugView
#ifdef useDebugView
/* add a forward slash at the beginning of this line to toggle the DebugView type
		property String^ DebugView
		{
			inline String^ get()
			{
				return ToString();
			}
		}
/*/
		property Double DebugView
		{
			inline Double get()
			{
				return (Double)*this;
			}
		}
//*/
#endif
		static void ReadOutResult(ui32* buffer, int headScanBackStart, int biasedExponentAtScanStart, bool sign, Quadruple %result);
		static inline Quadruple FromData(byte* data)
		{
			Quadruple result;
			pin_ptr<byte> ptr = result.storage;
			memcpy(ptr, data, 16);
			return result;
		}
		FPU_EXCEPTION_DECLARATION(Overflow);
		FPU_EXCEPTION_DECLARATION(Underflow);
		FPU_EXCEPTION_DECLARATION(DivideByZero);
		FPU_EXCEPTION_DECLARATION(Invalid);
		FPU_EXCEPTION_DECLARATION(Inexact);
	public:
		property bool IsSigned
		{
			inline bool get()
			{
				pin_ptr<byte> ptr = storage;
				return (*(ptr + 15) & 0x80) != 0;
			}
			inline void set( bool value )
			{
				pin_ptr<byte> ptr = storage;
				if (value)
					*(ptr + 15) |= 0x80;
				else
					*(ptr + 15) &= 0x7f;
			}
		}
		property i32 Base2Exponent
		{
			inline i32 get()
			{
				return (i32)biasedExponent - QUAD_EXPONENT_BIAS;
			}

			inline void set( i32 value )
			{
				if (value >= QUAD_EXPONENT_MAX)
				{
					if (IsSigned)
						*this = NegativeInfinity;
					else
						*this = PositiveInfinity;
					return;
				}
				if (value < QUAD_EXPONENT_MIN) value = QUAD_EXPONENT_MIN;
				biasedExponent = (ui16)(value + QUAD_EXPONENT_BIAS);
			}
		}
		property bool IsZero
		{
			inline bool get()
			{
				pin_ptr<byte> bPtr = storage;
				ui32* ptr = (ui32*)bPtr;
				if (*(ptr + 3) != 0)
					if (*(ptr + 3) != 0x80000000) return false;
				if (*ptr != 0) return false;
				ptr++;
				if (*ptr != 0) return false;
				ptr++;
				if (*ptr != 0) return false;
				return true;
			}
		}
		property bool IsNaN
		{
			inline bool get()
			{
				if (biasedExponent != 0x7FFF) return false;
				pin_ptr<byte> ptr = storage;
				if ((i32*)ptr != 0) return true;
				if ((i32*)ptr + 1 != 0) return true;
				if ((i32*)ptr + 2 != 0) return true;
				if ((i16*)ptr + 6 != 0) return true;
				return false;
			}
		}
		property bool IsInfinite
		{
			inline bool get()
			{
				pin_ptr<byte> bPtr = storage;
				ui32* ptr = (ui32*)bPtr;
				if (*ptr != 0) return false;
				ptr++;
				if (*ptr != 0) return false;
				ptr++;
				if (*ptr != 0) return false;
				ptr++;
				if ((*ptr & 0x7FFFFFFF) != 0x7FFF0000) return false;
				return true;
			}
		}
		property bool IsSubNormal
		{
			inline bool get()
			{
				return biasedExponent == 0 && !IsZero;
			}
		}

		static inline void Negate( Quadruple %a)
		{
			pin_ptr<byte> ptr = a.storage;
			*(ptr + 15) ^= 0x80;
		}
		static inline void Negate( Quadruple %a, [Out] Quadruple %result )
		{
			result = a;
			pin_ptr<byte> ptr = result.storage;
			*(ptr + 15) ^= 0x80;
		}
		static inline Quadruple operator-(Quadruple a)
		{
			Negate(a);
			return a;
		}
		static inline Quadruple operator+(Quadruple a)
		{
			return a;
		}
		static void Add( Quadruple %a, Quadruple %b, [Out] Quadruple %result );
		static void Sub( Quadruple %a, Quadruple %b, [Out] Quadruple %result );
		static void Mul( Quadruple %a, Quadruple %b, [Out] Quadruple %result );
		static void Div( Quadruple %a, Quadruple %b, [Out] Quadruple %result );
		static inline void Increment( Quadruple %v )
		{
			Add(v, One, v);
		}
		static inline void Decrement( Quadruple %v )
		{
			Quadruple temp = One;
			Sub(v, temp, v);
		}

		static Quadruple inline operator+(Quadruple a, Quadruple b)
		{
			Quadruple result;
			Add(a, b, result);
			return result;
		}
		static Quadruple inline operator-(Quadruple a, Quadruple b)
		{
			Quadruple result;
			Sub(a, b, result);
			return result;
		}
		static Quadruple inline operator*(Quadruple a, Quadruple b)
		{
			Quadruple result;
			Mul(a, b, result);
			return result;
		}
		static Quadruple inline operator/(Quadruple a, Quadruple b)
		{
			Quadruple result;
			Div(a, b, result);
			return result;
		}
		static bool operator ==(Quadruple a, Quadruple b);
		static bool operator !=(Quadruple a, Quadruple b);
		static bool operator >(Quadruple a, Quadruple b);
		static bool operator <(Quadruple a, Quadruple b);
		static bool inline operator >=(Quadruple a, Quadruple b)
		{
			if (a == b) return true; //ensure that equals test happens first because it's much faster
			else return a > b;
		}
		static bool inline operator <=(Quadruple a, Quadruple b)
		{
			if (a == b) return true; //ensure that equals test happens first because it's much faster
			else return a < b;
		}

#pragma warning(disable: 4460)
		static Quadruple inline operator++(Quadruple a)
		{
			Quadruple temp = One;
			Add(a, temp, temp);
			return temp;
		}
		static Quadruple inline operator--(Quadruple a)
		{
			Quadruple temp = One;
			Sub(a, temp, temp);
			return temp;
		}
#pragma warning(default: 4460)
		static Quadruple inline operator<<(Quadruple a, i32 shift)
		{
			Quadruple result = a;
			result.Base2Exponent+=shift;
			return result;
		}
		static Quadruple inline operator>>(Quadruple a, i32 shift)
		{
			Quadruple result = a;
			result.Base2Exponent-=shift;
			return result;
		}
		static operator Quadruple( Double v );
		static explicit operator Double( Quadruple v);
		static explicit operator float( Quadruple v);
		static operator Quadruple( i64 v );
		static explicit operator i64 (Quadruple v);
		static operator Quadruple( i32 v );
		static explicit operator i32 (Quadruple v);
		static explicit operator __float128 (Quadruple v);
		static explicit operator Quadruple (__float128 v);

		static void Abs(Quadruple %v, [Out] Quadruple %result);
		static inline Quadruple Abs(Quadruple v)
		{
			Quadruple result;
			Abs(v, result);
			return result;
		}
		static void Max(Quadruple %a, Quadruple %b, [Out] Quadruple %result);
		static inline Quadruple Max(Quadruple a, Quadruple b)
		{
			Quadruple result;
			Max(a, b, result);
			return result;
		}
		static void Min(Quadruple %a, Quadruple %b, [Out] Quadruple %result);
		static inline Quadruple Min(Quadruple a, Quadruple b)
		{
			Quadruple result;
			Min(a, b, result);
			return result;
		}
		static Quadruple Ln(Quadruple v);
		static Quadruple Base2Exp(Quadruple v);
		static Quadruple Exp(Quadruple v);
		static Quadruple Pow(Quadruple base, Quadruple exponent);
		static inline Quadruple Sqrt(Quadruple value)
		{
			return Pow(value, Quadruple::Half);
		}
		static Quadruple inline operator^(Quadruple a, Quadruple b)
		{
			return Pow(a, b);
		}
		static Quadruple Log(Quadruple v, Quadruple base);
		static Quadruple Log2(Quadruple v);
		static void Ceiling( Quadruple %v, [Out] Quadruple %result );
		static inline Quadruple Ceiling(Quadruple v)
		{
			Quadruple result;
			Ceiling(v, result);
			return result;
		}
		static void Floor( Quadruple %v, [Out] Quadruple %result );
		static inline Quadruple Floor(Quadruple v)
		{
			Quadruple result;
			Floor(v, result);
			return result;
		}
		static void Round( Quadruple %v, [Out] Quadruple %result );
		static inline Quadruple Round(Quadruple v)
		{
			Quadruple result;
			Round(v, result);
			return result;
		}
		static void Truncate( Quadruple %v, [Out] ui64 %result );
		static inline ui64 Truncate( Quadruple v)
		{
			ui64 result;
			Truncate(v, result);
			return result;
		}
		static void Fraction( Quadruple %v, [Out] Quadruple %result );
		static inline Quadruple Fraction(Quadruple v)
		{
			Quadruple result;
			Fraction(v, result);
			return result;
		}
		static Quadruple Sin(Quadruple v);
		static Quadruple Cos(Quadruple v);
		static Quadruple Tan(Quadruple v);
		static Quadruple ASin(Quadruple v);
		static Quadruple ACos(Quadruple v);
		static Quadruple ATan(Quadruple v);
		static Quadruple ATan2(Quadruple y, Quadruple x);

		virtual String^ ToString() override;
		String^ ToString(String^ format);
		String^ ToString(IFormatProvider^ provider);
		String^ ToString(String^ format, IFormatProvider^ provider);
		static Quadruple FromString(String^ s);

		static inline int Sign(Quadruple v)
		{
			if (v.IsZero) return 0;
			return v.IsSigned ? -1 : 1;
		}
#include "constants.h"

	};
}

#undef QUAD_CONSTANT
#undef FPU_EXCEPTION_DECLARATION