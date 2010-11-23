QPFloat 0.3 beta
Release under the GPL 3.0 License. See COPYING.txt

***** FEATURES *****

This library emulates the quadruple precision floating-point (IEEE-754-2008 binary128). It includes:
	Primitive operations such as addition, subtraction, multiplication, and division
	Higher operations such as natural logarithm, arbitrary base logarithm, exp, and pow
	Cieling, Floor, Round, Truncate, and Fraction (Fraction returns just the fractional portion)
	Sin, Cos, Tan, ASin, ACos, ATan, ATan2 implemented using Maclaurin series
	ToString function
	Hard coded constants such as Pi and e to full quadruple precision
	Implemented on little-endian, may or may not work on big-endian
	Follows the IEEE in-memory format on little-endian machines (transferable to and from hardware)
	arithmetic on sub-normals
	round-to-even
	correct propogation Inf, -Inf, and NaN
	enablable exception mechanisms
	
	This library contains both an unmanaged implementation and a managed implementation (atop the unmanaged).
	
***** USING *****

VB.Net and C# users:
	Add a reference to Release/QPFloat.dll (x64/Release/QPFloat.dll for x64)
	create new variables using the System.Quadruple type (C# can do "using System;" so that you can just type Quadruple)
	
VC++/CLI users:
	Add a reference to Release/QPFloat.dll (x64/Release/QPFloat.dll for x64)
	create new variables using System::Quadruple
	
C++ users:
	Add a reference to Release/QPFloat.dll (x64/Release/QPFloat.dll for x64)
	#include "__float128.h"
	create new variables using __float128

***** COMPILING *****

Microsoft Visual C++ users:
	This code base can be compiled with or without /clr (Managed C++) to the compiler.
	If /clr IS used, then this library can be used by C# and VB.Net via the type Quadruple, the same way as Double
	If /clr IS NOT used, #ifdef _MANAGED has been used to automatically exclude the managed implementation.
	Since extension methods can't be used to add static functions to System.Math, Operations like Abs, Sin, etc are static methods of Quadruple
	__float128 is the unmanaged (faster) implementation, which is still present even when /clr is used.
	
Other compiler users:
	This code uses #ifdef to remove Microsoft specific functionality automatically (#ifdef _MANAGED)
	Though I've not tried, it should be relatively easy to build using other compilers.
	following existing conventions, __float128 is the type proffered by this library.