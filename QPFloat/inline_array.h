#pragma once

template <typename T, int size>
ref class InlineArrayDebugViewer
{
private:
	Object^ obj;
	int count;
public:
	InlineArrayDebugViewer( Object^ inline_array_ref );

	[System::Diagnostics::DebuggerBrowsableAttribute(System::Diagnostics::DebuggerBrowsableState::RootHidden)]
	array<T>^ Values;
};

template<typename T, int size>
[System::Runtime::CompilerServices::UnsafeValueType]
[System::Runtime::InteropServices::StructLayout
	(
	System::Runtime::InteropServices::LayoutKind::Explicit,
	Size=(sizeof(T)*size)
	)
]
[System::Diagnostics::DebuggerTypeProxyAttribute(InlineArrayDebugViewer<T, size>::typeid)]
[System::Diagnostics::DebuggerDisplayAttribute("GetAddressString() Count = {Length()}")]
public value struct inline_array {
private:
	[System::Runtime::InteropServices::FieldOffset(0)]
	T dummy_item;
public:
	T% operator[](int index) {
		return *((&dummy_item)+index);
	}

	static operator interior_ptr<T>(inline_array<T,size>% ia) {
		return &ia.dummy_item;
	}

	int Length();

	System::String^ GetAddressString();
};
