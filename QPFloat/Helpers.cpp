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

#include "StdAfx.h"
#include "Helpers.h"

bool enableOverflowException = false;
bool enableUnderflowException = false;
bool enableDivideByZeroException = false;
bool enableInvalidException = false;
bool enableInexactException = false;

byte snan[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,64,255,127};
byte qnan[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,127,255,127};
byte ind[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,127,255,255};
byte posInf[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 127};
byte negInf[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255};
byte e[16] = {122,78,64,172,184,95,53,149,118,69,177,168,240,91,0,64};
byte log2e[16] = {59,210,160,253,15,125,119,225,47,184,82,118,84,113,255,63};
byte ln2[16] = {229,7,48,103,199,147,87,243,158,163,239,47,228,98,254,63};
byte twoPi[16] = {184,1,23,197,140,137,105,132,209,66,68,181,31,146,1,64};
byte pi[16] = {184,1,23,197,140,137,105,132,209,66,68,181,31,146,0,64};
byte halfPi[16] = {184,1,23,197,140,137,105,132,209,66,68,181,31,146,255,63};
byte quarterPi[16] = {184,1,23,197,140,137,105,132,209,66,68,181,31,146,254,63};
byte zero[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
byte negZero[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128};
byte one[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,63};
byte negOne[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,191};
byte half[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,63};
byte sinQuarterPi[16] = {147,234,102,19,251,178,8,201,188,243,103,230,9,106,254,63};

#ifdef _MANAGED
#pragma unmanaged
#endif

i32 ReverseBitScan( ui32* a, byte longCount, i32 startingBit )
{
	if (startingBit == -1) startingBit = longCount * 64 - 1;
	i32 startingShift = (startingBit + 1) & 31;
	i32 intIndex = startingBit / 8 / 4;
	if (startingShift != 0)
	{
		ui32 firstUInt = *(a + intIndex) & (((ui64)1 << startingShift) - 1);
		ui32 result;
		if (_BitScanReverse((unsigned long*)&result, firstUInt))
		{
			return result + intIndex * 32;
		}
		intIndex--;
	}
	for (; intIndex >=0; intIndex--)
	{
		ui32 v = *(a + intIndex);
		ui32 result;
		if (_BitScanReverse((unsigned long*)&result, v))
		{
			return result + intIndex * 32;
		}
	}
	return -1;
}

ui64 computeMask( i32 bitOffset, byte bitCount )
{
	if (bitCount > 64) bitCount = 64;
	if (bitCount == 64)
	{
		if (bitOffset < 0)
		{
			return ((ui64)1 << (64 + bitOffset)) - 1;
		}
		else if (bitOffset >= 0)
		{
			return 0xFFFFFFFFFFFFFFFF << bitOffset;
		}
	}
	if (bitOffset < 0)
	{
		int totalBits = bitCount - bitOffset;
		if (totalBits <= 0)
			return 0;
		else
			return ((ui64)1 << totalBits) - 1;
	}
	else
	{
		return (((ui64)1 << bitCount) - 1) << bitOffset;
	}
}

void BitBlockTransfer( void* _source, int sourceBitOffset, void* _destination, int destinationBitOffset, int bitCount )
{
	if (bitCount == 0) return; //algorithm 0
	byte* source = (byte*)_source;
	byte* destination = (byte*)_destination;

	int dstShift = destinationBitOffset & 7;
	destination += (destinationBitOffset - dstShift) / 8;

	int srcShift = sourceBitOffset & 7;
	source += (sourceBitOffset - srcShift) / 8;

	//algorithm 1 handles shift-free copies
	//separately handle any partial bytes
	//and use a memcpy to do the rest
	if (srcShift == dstShift)
	{
		if (dstShift > 0)
		{
			i32 leadingBits = (8 - dstShift) & 7;
			i32 mask = (1 << dstShift) - 1;
			*destination &= mask;
			*destination |= *source & ~mask;
			destination++;
			source++;
			bitCount -= leadingBits;
		}

		int blockCopyCount = bitCount >> 3;
		memcpy(destination, source, blockCopyCount);
		destination += blockCopyCount;
		source+= blockCopyCount;

		int trailingBits = bitCount & 7;
		if (trailingBits > 0)
		{
			i32 mask = (1 << trailingBits) - 1;
			*destination &= ~mask;
			*destination |= *source & mask;
		}
	}
	else
	{
		//if it can't be handled by a single long
		if ((bitCount + 7 + srcShift) > 64 || (bitCount + 7 + dstShift) > 64)
		{
			//algorithm 2 handles copies that need shifting of multiple longs
			//it works by reading an entire (byte aligned) int from the source without masking
			//shifting it by the difference between the source and destination alignments
			//and writing out an entire (byte aligned) int to the destination without masking
			//bits that reside on the boundaries that would normally be lost from shifting are preserved in a 5th byte
			ui32* iSrc = (ui32*)source;
			ui32* iDst = (ui32*)destination;

			ui64 shiftBuffer = 0; //only 5 bytes of this will be used
			ui32* read = (ui32*)&shiftBuffer;
			ui32* write = (ui32*)((byte*)read + 1);

			//prime the shift buffer
			//SrcToDstRightShift (below) must not be less than zero (negative shifts are not allowed)
			//so we offset srcShift and iSrc to compensate
			bool compensated = false;
			if (srcShift < dstShift)
			{
				compensated = true;
				//by increasing the srcShift by 8
				//and moving the source pointer back by a byte
				//we compensate
				srcShift += 8;
				//but it also means that the first byte of shiftBuffer will be empty for the moment
				memcpy(write,source, 4); //we are writing to the read spot because this is just preparation
				//iSrc = (ui32*)((byte*)iSrc - 1);
				//this is commented out because we later add 1 to iSrc, since our write spot is also offset by one
				//instead we just comment them both out and add a ++ to the else block
			}
			else
			{
				//this commented out code cannot be reached, because the above runs if srcShift < dstShift
				//which means dstShift must also be zero (since it cant be negative)
				//which means that the earlier block (srcShift==dstShift) handles the transfer
// 				if (srcShift == 0)
// 					*read = *iSrc; //copy 4 bytes
// 				if (srcShift > 0)

					//need 5 bytes since the fifth byte contains the last bits of the first int
					memcpy(read, iSrc, 5); //copying 5 bytes is boundary safe because passing a bitOffset > 0 indicates that there's an extra byte there

					//see the comments after this block
					iSrc = (ui32*)((byte*)iSrc + 1);
			}
			//commented out because we were doing a -1 in srcShift<dstShift and then +1 here
			//instead we just do this in the above block
			//needed because write is incremented (see byte* write above)
			//iSrc = (ui32*)((byte*)iSrc + 1);

			int SrcToDstRightShift = srcShift - dstShift; //shift needed to convert a source aligned bit to a destination aligned bit
			int DstToSrcRightShift = 32 - SrcToDstRightShift; //shift needed to undo SrcToDstRightShift while moving data in the fifth byte (low bits) of shiftBuffer into the first byte (high bits) of shiftBuffer

			shiftBuffer >>= SrcToDstRightShift; //convert data from source to destination
			ui64 mask = ((1 << dstShift) - 1);
			shiftBuffer &= ~mask; //only needed if shiftBuffers lowest byte was written to (the 5 byte memcpy), but cheaper to just do it regardless
			shiftBuffer |= *destination & mask; //preserve the bits of destination before our target area by loading it into the shift buffer
			//and write it out
			*iDst = *read;

			bitCount -= 32 - dstShift; //we have only completed as many bits as have been written into destination

			iSrc++;
			iDst++;

			//this is the fast part that we have been preparing for
			//it works by realizing that 5 bytes (in shiftBuffer) can shift
			//an int by up to 8 bits without destroying data
			//and by continuously rotating data (right) through that 5 bytes
			//all masking is eliminated resulting in a performance gain
			while (bitCount >= 32)
			{
				shiftBuffer >>= DstToSrcRightShift;
				*write = *iSrc;
				shiftBuffer >>= SrcToDstRightShift;
				*iDst = *read;
				iSrc++;
				iDst++;
				bitCount -= 32;
			}
 			if (bitCount > 0)
 			{
				//got a last little bit to transfer
				//it will be handled by algorithm 3
				BitBlockTransfer((byte*)iSrc - 1, SrcToDstRightShift, iDst, 0, bitCount);
			}
		}
		else //it can be handled by a single ui64
		{
			//algorithm 3

			//we use a workingBuffer because it allows us to do ui64 masking and writing without
			//possibly going into into invalid memory - the number of surely valid bytes is
			//(bitCount + 7 + shift) / 8

			//read bytes from destination into a working buffer to preserve the original bits
			ui64 workingBuffer = 0;
			int outBytes = (bitCount + 7 + dstShift) / 8;
			memcpy(&workingBuffer, destination, outBytes);

			//use a mask to clear the bits in the working buffer that we will overwrite
			ui64 mask = computeMask(dstShift, (byte)bitCount);
			workingBuffer &= ~mask;

			//read bytes from source into a value buffer
			ui64 valueBuffer;
			memcpy(&valueBuffer, source, (bitCount + 7 + srcShift) / 8);

			//shift the value by the difference between the source and destination shift
			int rShift = srcShift - dstShift;
			if (rShift > 0)
				valueBuffer >>= rShift;
			else
				valueBuffer <<= -rShift;

			//use the mask to clear the bits in the value buffer that aren't to be applied
			valueBuffer &= mask;

			//or the value into the working buffer
			workingBuffer |= valueBuffer;
			//write out the working buffer to the destination
			memcpy(destination, &workingBuffer, outBytes);
		}
	}
}

void BitWindowTransfer(void* source, int sourceWindowBitOffset, int sourceWindowBitSize, int sourceWindowAlignBit, void* destination, int destinationWindowBitOffset, int destinationWindowBitSize, int destinationWindowAlignBit )
{
	int srcOffToAln = sourceWindowAlignBit - sourceWindowBitOffset;
	int dstOffToAln = destinationWindowAlignBit - destinationWindowBitOffset;
	int preAlignBits;
	if (dstOffToAln < srcOffToAln) preAlignBits = dstOffToAln; else preAlignBits = srcOffToAln;
	//may be negative, in which case the first bit to copy is after the align

	int srcAlnToEnd = sourceWindowBitSize - srcOffToAln;
	int dstAlnToEnd = destinationWindowBitSize - dstOffToAln;
	int postAlignBits;
	if (dstAlnToEnd < srcAlnToEnd) postAlignBits = dstAlnToEnd; else postAlignBits = srcAlnToEnd;
	//may be negative

	int copySize = postAlignBits + preAlignBits;
	if (copySize < 1) return;
	BitBlockTransfer(source, sourceWindowAlignBit - preAlignBits, destination, destinationWindowAlignBit - preAlignBits, copySize);
}

void ClearBlock(void* destination, int destinationBitOffset, int bitCount)
{
	int dstShift = destinationBitOffset & 7;
	byte* _dst = ((byte*)destination) + destinationBitOffset / 8;

	if (dstShift + bitCount < 8)
	{
		//only need to manipulate one byte
		*_dst &= ~(((1 << bitCount) - 1) << dstShift);
	}
	else
	{
		//do any leading H.O. bits
		if (dstShift > 0)
		{
			*_dst &= ((1 << dstShift) - 1);
			_dst++;
			bitCount -= 8 - dstShift;
		}
		//do whole bytes
		if (bitCount >= 8)
		{
			memset(_dst, 0, bitCount / 8);
			_dst += bitCount / 8;
			bitCount &= 7;
		}
		//do trailing L.O. bits
		if (bitCount > 0)
		{
			*_dst &= ~((1 << bitCount) - 1);
		}
	}
}

void IntBlockAdd(ui64* a, ui64* b, byte longCount)
{
	bool carry = false;
	for (i32 i = 0; i < longCount; i++)
	{
		ui64 result = *a + *b;
		if (carry)
		{
			result++;
			carry = result <= *a;
		}
		else
		{
			carry = result < *a;
		}
		*a = result;
		a++;
		b++;
	}
}

void IntBlockTwosCompliment(ui64* a, byte longCount)
{
	ui64* temp = a;
	i32 i;
	for (i = 0; i < longCount; i++)
	{
		*temp = ~*temp;
		temp++;
	}
	temp = a;
	for (i = 0; i < longCount; i++)
	{
		if (++(*temp) > 0) break;
		temp++;
	}
}

bool IntBlockSub(ui64* a, ui64* b, byte longCount)
{
	bool carry = false;
	for (int i = 0; i < longCount; i++)
	{
		ui64 result = *a - *b;
		if (carry)
		{
			result--;
			carry = result >= *a;
		}
		else
		{
			carry = result > *a;
		}

		*a = result;
		a++;
		b++;
	}
	return carry;
}

/// <summary>
/// _result must be at minimum twice as long as longCount
/// </summary>
/// <param name="_result"></param>
/// <param name="_a"></param>
/// <param name="_b"></param>
/// <param name="longCount"></param>
void IntBlockMul(ui64* _result, ui64* _a, ui64* _b, byte longCount)
{
	//works by multiplying each uint by each uint
	//and adding the resulting ulong to _result;
	ui32* a = (ui32*)_a;
	ui32* b = (ui32*)_b;
	ui32* r = (ui32*)_result;
	int intCount = longCount * 2;
	memset(_result, 0, longCount * 2 * sizeof(ui64));
	for (int i = 0; i < intCount; i++)
	{
		for (int j = 0; j < intCount; j++)
		{
			ui64 partialResult = (ui64)(*(a + i)) * (ui64)(*(b + j));
			int intShift = i + j;
			ui64 currentValue = *(ui64*)(r + intShift);
			ui64 newValue = partialResult + currentValue;
			*(ui64*)(r + intShift) = newValue;
			bool carry = newValue < currentValue;
			intShift += 2;
			while (carry)
			{
				(*(r + intShift))++;
				carry = (*(r + intShift) == 0);
				intShift++;
			}
		}
	}
}

/// <summary>
/// longCount is for _result and _a, and bLongCount must be less than or equal to longCount
/// </summary>
/// <param name="_result"></param>
/// <param name="_a"></param>
/// <param name="longCount"></param>
/// <param name="_b"></param>
/// <param name="bLongCount"></param>
void IntBlockMul(ui64* _result, ui64* _a, byte longCount, ui64* _b, byte bLongCount)
{
	//works by multiplying each uint by each uint
	//and adding the resulting ulong to _result;
	ui64* a = (ui64*)_a;
	ui64* b = (ui64*)_b;
	ui64* r = (ui64*)_result;
	int intCount = longCount * 2;
	int bIntCount = bLongCount * 2;
	memset((byte*)_result, 0, longCount * sizeof(ui64));
	for (int i = 0; i < intCount; i++)
	{
		for (int j = 0; j < bIntCount; j++)
		{
			ui64 partialResult = (ui64)(*(a + i)) * (ui64)(*(b + j));
			int intShift = i + j;
			ui64 currentValue = *(ui64*)(r + intShift);
			ui64 newValue = partialResult + currentValue;
			*(ui64*)(r + intShift) = newValue;
			bool carry = newValue < currentValue;
			intShift += 2;
			while (carry)
			{
				(*(r + intShift))++;
				carry = (*(r + intShift) == 0);
				intShift++;
			}
		}
	}
}

/// <summary>
/// result and _a must be at minimum twice as long as longCount
/// the data for _a must be stored in the upper half and the value in _a will be destroyed
/// the last longCount longs of _result contain the integer part of the quotient
/// the first longCount longs of _result contain the fractional of the quotient
/// essential, a binary point (like decimal point) resides at the center of _result
/// </summary>
/// <param name="_result"></param>
/// <param name="_a"></param>
/// <param name="_b"></param>
/// <param name="longCount"></param>
// returns true on success, false on divide by zero
bool IntBlockDiv(ui64* _result, ui64* _a, ui64* _b, byte longCount, int desiredBits)
{
	byte longCount2 = (byte)(2 * longCount);
	ui32 longCount3 = longCount * 3;
	ui32 longCount4 = longCount * 4;
	int HOBitIndexB = ReverseBitScan((ui32*)_b, longCount, -1);
	if (HOBitIndexB == -1) return false;
	int HOBitIndexA = ReverseBitScan((ui32*)_a, (byte)(longCount2), -1);
	memset((byte*)_result, 0, longCount * 2 * sizeof(ui64));
	if (HOBitIndexA == -1) return true; //return zero
	//we are gonna save every shift of _b, one for each 8 bit alignment
	//ui64* testingBuf = (ui64*)malloc(longCount3 * sizeof(ui64));
	int storageSize = longCount3 * sizeof(ui64) * 8; //we use longCount3 because we need overhead bytes as well for shifting down
	ui64* buf = (ui64*)malloc(storageSize);
	memset(buf, 0, storageSize);
	for (int prepare = 0; prepare < 8; prepare++)
	{
		ui64* location = buf + longCount3 * prepare + longCount;
		BitBlockTransfer(_b, 0, location, prepare, 112);
		SetBit(buf + longCount3 * prepare + longCount, 112 + prepare);
	}
	int LastLOOffset = -1;
	while (HOBitIndexA != -1)
	{
		int LOOffset = HOBitIndexA - HOBitIndexB; //HOBitIndexA will usually be 112 + 128 because it's in the upper half, and it'll place the cache in the middle longCount span in buf
		if (LOOffset < LastLOOffset) break;
		int bitOffset = LOOffset & 0x7;
		int byteOffset = (LOOffset - bitOffset) / 8;
		byte* translatedB = ((byte*)(buf + longCount3 * bitOffset + longCount)) - byteOffset;
		i32 cmp = 0;
		IntBlockCompare(&cmp, (ui32*)_a, (ui32*)translatedB, longCount4);
		if (cmp < 0)
		{
			HOBitIndexA--;
			continue;
		}
		if (LastLOOffset == -1)
		{
			LastLOOffset = LOOffset - desiredBits; //we'll do enough to have a couple guard bits - don't wast time doing more
			if (LastLOOffset < 0) LastLOOffset = 0; //subnormal dividend can cause this
		}
		SetBit((byte*)_result, LOOffset);
		IntBlockSub(_a, (ui64*)translatedB, longCount2);
		HOBitIndexA = ReverseBitScan((ui32*)_a, longCount2, HOBitIndexA);
	}
	free(buf);
	return true;
}

int FindHeadAndApplyRounding( ui32* buffer, int headScanBackStart )
{
	int headBitIndex = ReverseBitScan((ui32*)buffer, 0 /*irrelevant because we have a start index*/, headScanBackStart);
	int roundCheckPosition = headBitIndex - 112 - 1;
	if (roundCheckPosition < 0)
	{
		//cannot apply rounding
		//this means there was no guard bit!!!
	}
	else
	{
		if (ReadBit((byte*)buffer, roundCheckPosition)) //do we need to round?
		{
			if (ReadBit((byte*)buffer, roundCheckPosition + 1)) //the LSB is 1, so round up
			{
				byte numberOfLongsNeeded = (byte)((headBitIndex + 63) / 64);
				byte* temp = (byte*)malloc(numberOfLongsNeeded * 8);
				memset(temp, 0, numberOfLongsNeeded * 8);
				SetBit(temp, headBitIndex - 112);
				IntBlockAdd((ui64*)buffer, (ui64*)temp, (byte)numberOfLongsNeeded);
				headBitIndex = ReverseBitScan((ui32*)buffer, 4, headBitIndex + 1);
				free(temp);
			}
		}
	}
	return headBitIndex;
}

#ifdef _MANAGED
#pragma managed
#endif

//TEST is defined when the build is set to Test, which generates an exe instead of a dll
#ifdef TEST

#include "__float128.h"
#include "ManagedQuadruple.h"
#include <stdio.h>

int main(void)
{
	__float128 x = 1;
	__float128 y = 10;
	__float128 b;
	//math tests
	__float128::Div(x, y, b);
	for (int i = 0; i < 10000; i++)
		b = __float128::ATan2(y, x);
	//conversion test
	double c;
	__float128::ToDouble(b, c);
	//string tests
	Quadruple fromStringTest = Quadruple::FromString("1.567000000000001e+50");
	String^ toStringTest = fromStringTest.ToString();
}

#endif
