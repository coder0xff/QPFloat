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

#include "stdafx.h"

#ifdef _MANAGED

#include "ManagedQuadruple.h"
#include "DoubleDecomposition.h"
#include "__float128.h"

using namespace System::Collections::Generic;

namespace System
{
	array<byte>^ Quadruple::mantissa::get()
	{
		array<byte>^ results = gcnew array<byte>(QUAD_MANTISSA_BYTES);
		pin_ptr<byte> buf = &results[0];
		pin_ptr<byte> ptr = storage;
		BitBlockTransfer(ptr, 0, buf, QUAD_MANTISSA_BYTES * 8 - QUAD_SIGNIFICANT_BITS, QUAD_SIGNIFICANT_BITS); //place in H.O. bits
		return results;
	}

	void Quadruple::mantissa::set(array<byte>^ value)
	{
		int bitCount = value->Length * 8;
		pin_ptr<byte> buf = &value[0];
		pin_ptr<byte> ptr = storage;
		if (bitCount < QUAD_SIGNIFICANT_BITS) ClearBlock(ptr, 0, QUAD_SIGNIFICANT_BITS);
		BitWindowTransfer(buf, 0, bitCount, bitCount, ptr, 0, QUAD_SIGNIFICANT_BITS, QUAD_SIGNIFICANT_BITS); //read H.O. bits to H.O. bits
	}

	//reads bits from the buffer and stores the result in result, adjusting the exponent, performing round-to-even, and handling subnormals
	void Quadruple::ReadOutResult(ui32* buffer, int headScanBackStart, int biasedExponentAtScanStart, bool sign, Quadruple %result )
	{
		pin_ptr<byte> rPtr = result.storage;
		int implicitPosition = FindHeadAndApplyRounding(buffer, headScanBackStart);
		if (implicitPosition == -1)
		{
			//no bits
			result = 0;
			result.Sign = sign;
			return;
		}
		int currentExponent = biasedExponentAtScanStart + implicitPosition - headScanBackStart;
		if (currentExponent >= QUAD_EXPONENT_MASK)
		{
			if (sign)
				result = NegativeInfinity;
			else
				result = PositiveInfinity;
			Overflow();
			return;
		}
		if (currentExponent < 0) currentExponent = 0;
		int expInc = currentExponent - biasedExponentAtScanStart;
		result.biasedExponent = (ui16)currentExponent;
		BitBlockTransfer(buffer, headScanBackStart + expInc - 112, rPtr, 0, 112);
		result.Sign = sign;
		if (currentExponent == 0) Underflow();
		if (EnableInexactException) if (ReverseBitScan((ui32*)buffer, 0, headScanBackStart + expInc - 112 - 1) != -1) Inexact();
	}

	void Quadruple::Add( Quadruple %a, Quadruple %b, Quadruple %result )
	{
		pin_ptr<byte> aPtr = a.storage;
		pin_ptr<byte> bPtr = b.storage;
		pin_ptr<byte> rPtr = result.storage;
		__float128::Add(*(__float128*)aPtr, *(__float128*)bPtr, *(__float128*)rPtr);
	}

	void Quadruple::Sub( Quadruple %a, Quadruple %b, Quadruple %result )
	{
		pin_ptr<byte> aPtr = a.storage;
		pin_ptr<byte> bPtr = b.storage;
		pin_ptr<byte> rPtr = result.storage;
		__float128::Sub(*(__float128*)aPtr, *(__float128*)bPtr, *(__float128*)rPtr);
	}

	void Quadruple::Mul( Quadruple %a, Quadruple %b, Quadruple %result )
	{
		pin_ptr<byte> aPtr = a.storage;
		pin_ptr<byte> bPtr = b.storage;
		pin_ptr<byte> rPtr = result.storage;
		__float128::Mul(*(__float128*)aPtr, *(__float128*)bPtr, *(__float128*)rPtr);
	}

	void Quadruple::Div( Quadruple %a, Quadruple %b, Quadruple %result )
	{
		//minimize managed to native transitions by just doing one
		pin_ptr<byte> aPtr = a.storage;
		pin_ptr<byte> bPtr = b.storage;
		pin_ptr<byte> rPtr = result.storage;
		__float128::Div(*(__float128*)aPtr, *(__float128*)bPtr, *(__float128*)rPtr);
	}

	bool Quadruple::operator==( Quadruple %a, Quadruple %b )
	{
		pin_ptr<byte> aPtr = a.storage;
		pin_ptr<byte> bPtr = b.storage;
		return (*(__float128*)aPtr) == (*(__float128*)bPtr);
	}

	bool Quadruple::operator!=( Quadruple %a, Quadruple %b )
	{
		pin_ptr<byte> aPtr = a.storage;
		pin_ptr<byte> bPtr = b.storage;
		return (*(__float128*)aPtr) != (*(__float128*)bPtr);
	}

	bool Quadruple::operator>( Quadruple %a, Quadruple %b )
	{
		pin_ptr<byte> aPtr = a.storage;
		pin_ptr<byte> bPtr = b.storage;
		return (*(__float128*)aPtr) > (*(__float128*)bPtr);
	}

	bool Quadruple::operator<( Quadruple %a, Quadruple %b )
	{
		pin_ptr<byte> aPtr = a.storage;
		pin_ptr<byte> bPtr = b.storage;
		return (*(__float128*)aPtr) < (*(__float128*)bPtr);
	}

	Quadruple::operator Quadruple( Double v )
	{
		__float128 temp = v;
		return *(Quadruple*)&temp;
	}

	Quadruple::operator Double(Quadruple v )
	{
		pin_ptr<byte> vPtr = v.storage;
		double result;
		__float128::ToDouble(*(__float128*)vPtr, result);
		return result;
	}

	Quadruple::operator float(Quadruple v)
	{
		return (float)((Double)v);
	}

	Quadruple::operator Quadruple( i64 v )
	{
		__float128 temp = v;
		return *(Quadruple*)&temp;
	}

	Quadruple::operator i64(Quadruple v)
	{
		pin_ptr<byte> vPtr = v.storage;
		i64 result;
		__float128::ToInt64(*(__float128*)vPtr, result);
		return result;
	}

	Quadruple::operator Quadruple( i32 v )
	{
		__float128 temp = v;
		return *(Quadruple*)&temp;
	}

	Quadruple::operator i32(Quadruple v)
	{
		pin_ptr<byte> vPtr = v.storage;
		i32 result;
		__float128::ToInt32(*(__float128*)vPtr, result);
		return result;
	}

	void Quadruple::Abs( Quadruple %v, Quadruple %result )
	{
		result = v;
		result.Sign = false;
	}

	Quadruple Quadruple::Ln( Quadruple v )
	{
		pin_ptr<byte> vPtr = v.storage;
		__float128 result = __float128::Ln(*(__float128*)vPtr);
		return *(Quadruple*)&result;
	}

	Quadruple Quadruple::Exp( Quadruple v )
	{
		pin_ptr<byte> vPtr = v.storage;
		__float128 result = __float128::Exp(*(__float128*)vPtr);
		return *(Quadruple*)&result;
	}

	Quadruple Quadruple::Pow( Quadruple base, Quadruple exponent )
	{
		pin_ptr<byte> basePtr = base.storage;
		pin_ptr<byte> expPtr = exponent.storage;
		__float128 result = __float128::Pow(*(__float128*)basePtr, *(__float128*)expPtr);
		return *(Quadruple*)&result;
	}

	Quadruple Quadruple::Log( Quadruple v, Quadruple base )
	{
		pin_ptr<byte> vPtr = v.storage;
		pin_ptr<byte> basePtr = base.storage;
		__float128 result = __float128::Log(*(__float128*)vPtr, *(__float128*)basePtr);
		return *(Quadruple*)&result;	
	}

	Quadruple Quadruple::Log2( Quadruple v )
	{
		pin_ptr<byte> vPtr = v.storage;
		__float128 result = __float128::Log2(*(__float128*)vPtr);
		return *(Quadruple*)&result;
	}

	void Quadruple::Ceiling( Quadruple %v, Quadruple %result )
	{
		pin_ptr<byte> vPtr = v.storage;
		pin_ptr<byte> resultPtr = result.storage;
		__float128::Ceiling(*(__float128*)vPtr, *(__float128*)resultPtr);
	}

	void Quadruple::Floor( Quadruple %v, Quadruple %result )
	{
		pin_ptr<byte> vPtr = v.storage;
		pin_ptr<byte> resultPtr = result.storage;
		__float128::Floor(*(__float128*)vPtr, *(__float128*)resultPtr);
	}

	void Quadruple::Truncate( Quadruple %v, ui64 %result )
	{
		pin_ptr<byte> vPtr = v.storage;
		result = __float128::Truncate(*(__float128*)vPtr);
	}

	void Quadruple::Round( Quadruple %v, Quadruple %result )
	{
		pin_ptr<byte> vPtr = v.storage;
		pin_ptr<byte> resultPtr = result.storage;
		__float128::Round(*(__float128*)vPtr, *(__float128*)resultPtr);
	}

	void Quadruple::Fraction( Quadruple %v, Quadruple %result )
	{
		pin_ptr<byte> vPtr = v.storage;
		pin_ptr<byte> rPtr = result.storage;
		__float128::Fraction(*(__float128*)vPtr, *(__float128*)rPtr);
	}

	System::Quadruple Quadruple::Sin( Quadruple v )
	{
		pin_ptr<byte> vPtr = v.storage;
		__float128 result = __float128::Sin(*(__float128*)vPtr);
		return *(Quadruple*)&result;
	}

	System::Quadruple Quadruple::Cos( Quadruple v )
	{
		pin_ptr<byte> vPtr = v.storage;
		__float128 result = __float128::Cos(*(__float128*)vPtr);
		return *(Quadruple*)&result;
	}

	System::Quadruple Quadruple::Tan( Quadruple v )
	{
		pin_ptr<byte> vPtr = v.storage;
		__float128 result = __float128::Tan(*(__float128*)vPtr);
		return *(Quadruple*)&result;
	}

	System::Quadruple Quadruple::ASin( Quadruple v )
	{
		pin_ptr<byte> vPtr = v.storage;
		__float128 result = __float128::ASin(*(__float128*)vPtr);
		return *(Quadruple*)&result;
	}

	System::Quadruple Quadruple::ACos( Quadruple v )
	{
		pin_ptr<byte> vPtr = v.storage;
		__float128 result = __float128::ACos(*(__float128*)vPtr);
		return *(Quadruple*)&result;
	}

	System::Quadruple Quadruple::ATan( Quadruple v )
	{
		pin_ptr<byte> vPtr = v.storage;
		__float128 result = __float128::ATan(*(__float128*)vPtr);
		return *(Quadruple*)&result;
	}

	System::Quadruple Quadruple::ATan2( Quadruple y, Quadruple x )
	{
		pin_ptr<byte> yPtr = x.storage;
		pin_ptr<byte> xPtr = y.storage;
		__float128 result = __float128::ATan2(*(__float128*)yPtr, *(__float128*)xPtr);
		return *(Quadruple*)&result;
	}

	String^ Quadruple::ToString()
	{
		if (IsNaN) return "NaN";
		if (*this == PositiveInfinity) return "Infinity";
		if (*this == NegativeInfinity) return "-Infinity";
		if (IsZero) return Sign ? "-0" : "0";
		System::Text::StringBuilder^ result = gcnew System::Text::StringBuilder();
		//if we are just appending zeros after the decimal, then put them in a temporary buffer
		System::Text::StringBuilder^ nonZeroWaitCache = gcnew System::Text::StringBuilder();
		bool sign = this->Sign;
		if (sign) result->Append("-");

		Quadruple currentValue = *this; //(Quadruple)System::Runtime::InteropServices::Marshal::PtrToStructure((IntPtr)GAHH, Quadruple::typeid);

		Quadruple::Abs(currentValue, currentValue);
		Quadruple ten = 10;
		Quadruple temp = Log(currentValue, ten);
		int decimalDigitPlace = (int)Floor(temp);
		int scientificExponent = decimalDigitPlace;
		Quadruple digitMultiplier = ten ^ decimalDigitPlace; //use a round to make sure it's accurate
		if (currentValue / digitMultiplier >= ten)
		{
			//an error occurred calculating the logarithm - specifically numeric rounding error
			decimalDigitPlace++;
			scientificExponent++;
			digitMultiplier = ten ^ decimalDigitPlace;
		}
		if (decimalDigitPlace < 0) decimalDigitPlace; //start from the ones place at minimum
		currentValue = currentValue / digitMultiplier;
		int displayedDigitPlaces = 0;
		if (scientificExponent < 40 && scientificExponent > -10)
		{
			//it's not excessively large or small, so we'll display it without scientific notation
			while ((currentValue != Quadruple::Zero || decimalDigitPlace >= 0) && displayedDigitPlaces < 34)
			{
				int digitValue = (int)currentValue;
				if (decimalDigitPlace == -1) nonZeroWaitCache->Append(".");
				nonZeroWaitCache->Append(digitValue);
				if (decimalDigitPlace >= 0 || digitValue != 0)
				{
					result->Append(nonZeroWaitCache);
					nonZeroWaitCache->Clear();
				}
				currentValue = Quadruple::Fraction(currentValue);
				Mul(currentValue, ten, currentValue);
				decimalDigitPlace--;
				displayedDigitPlaces++;
			}
		}
		else
		{
			result->Append(currentValue.ToString());
			result->Append("e");
			if (scientificExponent > 0) result->Append("+");
			result->Append(scientificExponent);
		}
		return result->ToString();
	}

	System::Quadruple Quadruple::FromString( String^ str )
	{
		str = str->Trim();
		System::Text::StringBuilder^ s = gcnew System::Text::StringBuilder(str);
		Quadruple result = 0;
		if (s->default[0] == '-')
		{
			result.Sign = true;
			s->Remove(0, 1);
		}
		Quadruple ten = 10;
		int postDecimalDigits = 0;
		bool postDecimal = false;
		while (s->Length > 0)
		{
			Char digit = s->default[0];
			s->Remove(0, 1);
			if (digit >= '0' && digit <= '9')
			{
				int digitValue = (int)Char::GetNumericValue(digit);
				result *= ten;
				result += digitValue;
				if (postDecimal) postDecimalDigits++;
			}
			else if (digit == '.')
			{
				postDecimal = true;
			}
			else if (digit == 'e' || digit == 'E')
			{
				if (s->default[0] == '+') s->Remove(0, 1);
				postDecimalDigits -= System::Convert::ToInt32(s->ToString());
				break;
			}
			else
			{
				throw gcnew FormatException();
			}
		}
		Quadruple temp = Quadruple::Pow(ten, -postDecimalDigits);
		Quadruple::Mul(result, temp, result);
		return result;
	}
}

#endif