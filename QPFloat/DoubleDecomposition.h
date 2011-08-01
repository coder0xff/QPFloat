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

#ifdef _MANAGED
#pragma unmanaged
#endif 

struct DoubleDecomposition
{
public:
	double value;
	bool GetSign() const;
	void SetSign(bool value);
	ui16 GetBiasedExponent() const;
	void SetBiasedExponent(ui16 value);
	int GetUnbiasedExponent() const;
	void SetUnbiasedExponent(int value);
	void GetMantissa(byte* data, int byteCount) const;
	void SetMantissa(const byte* data, int byteCount);
};

#ifdef _MANAGED
#pragma managed
#endif