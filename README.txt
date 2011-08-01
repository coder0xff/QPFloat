# QPFloat (GPL 3.0) #

For high-precision mathematics, the Quadruple-Precision Floating Point library (QPFloat) emulates the IEEE 754 2008 binary128 on x86, and x64 (and probably any other little-endian platform) using integer arithmetic and bit manipulation. It contains a native C++ and assembler implementation, and a .Net C++/CLI implementation.

## Features ##

Much effort has been put into supporting a full feature set, including optimized transcendental functions to full precision.

### Standard operations ###
* addition, subtraction, multiplication, division
* Min, Max, Abs, Ceiling, Floor, Round, Truncate, Fraction
* ToString and FromString
* Cast operators to and from native numeric data types

### Numeric information ###
* IsZero
* IsNaN
* IsInfinite
* IsSigned
* IsSubNormal

### Transcendental functions ###
* natural logarithm (Ln), arbitrary-base logarithm (Log)
* exponentiation (Exp)
* power function (Pow)
* Sin, Cos, Tan
* ASin, ACos, ATan, ATan2

### Miscellaneous features ###
* Fast! Optimized low level bit manipulation
* Hard coded constant Pi and E to full quadruple precision
* Implemented on little-endian architecture, may or may not work on big-endian???
* Strictly follows IEEE specifications to the extent availabile on Wikipedia.
* Multiple guard bits
* Arithmetic on sub-normals is fully supported.
* Inf, -Inf, and NaN are fully supported.
* round-to-even
* emulates FPU exceptions with enable/disable flags (default: all disabled)
       
## Using ##

### VB.Net, C# users, and C++/CLI ###
1. Add a reference to Release/QPFloat.dll (x64/Release/QPFloat.dll for x64)
2. Create new variables using System.Quadruple
    
### C++ users ###
1. Add a reference to QPFloat.dll (x64/Release/QPFloat.dll for x64)
2. include "__float128.h"
3. create new variables using __float128

## Compiling ##

### Microsoft Visual C++ ###
* This code base can be compiled with or without /clr (Managed C++) to the compiler.
* If /clr IS used, then this library can be used by C#, VB.Net, and C++/CLI via the type System.Quadruple.
* If /clr IS NOT used, #ifdef _MANAGED has been used to automatically exclude the managed implementation.
* Since extension methods can't be used to add static functions to System.Math, Operations like Abs, Sin, etc are static methods of Quadruple.
* __float128 is the unmanaged (faster) implementation, which is still present even when /clr is used.
    
### Other compilers ###
* This code uses #ifdef to remove Microsoft specific functionality automatically (#ifdef _MANAGED)
* Though I've not tried, it should be relatively easy to build using other compilers.
* following existing conventions, __float128 is the type proffered by this library.