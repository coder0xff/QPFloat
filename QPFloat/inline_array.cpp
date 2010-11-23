#include "Stdafx.h"

#ifdef _MANAGED

#include "inline_array.h"

template <typename T, int size>
InlineArrayDebugViewer<T, size>::InlineArrayDebugViewer( Object^ inline_array_ref )
{
	Values = gcnew array<T>(size);
	pin_ptr<T> outPtr = &results[0];
	pin_ptr<T> inPtr = (inline_array<T,size>)inline_array_ref;
	memcpy(outPtr, inPtr, count * sizeof(T));
}

template<typename T, int size>
int inline_array<T, size>::Length()
{
	return size;
}

template<typename T, int size>
System::String^ inline_array<T, size>::GetAddressString()
{
	pin_ptr<T> ptr = &dummy_item;		
	return IntPtr::IntPtr(ptr).ToString();
}

#endif