#include "Test.hpp"
#include "../Unconventional.hpp"


namespace IntegerSubtractionTests
{

	int32_t __declspec(naked) Subtract_ArgumentsStackOnly()
	{
		__asm
		{
			mov eax, [esp + 4]
			sub eax, [esp + 8]
			ret
		}
	}

	int32_t __declspec(naked) Subtract_ArgumentsRegistersOnly(/* int32_t<eax> x, int32_t<ebx> y*/)
	{
		__asm
		{
			sub eax, ebx
			ret
		}
	}

	int32_t __declspec(naked) Subtract_ArgumentsMixed(/*int32_t<eax> x, int32_t y*/)
	{
		__asm
		{
			sub eax, [esp + 4]
			ret
		}
	}

	void Run()
	{
		using namespace Unconventional;

		{
			Function<FunctionSignature<CallingConvention::Cdecl, Location::EAX, Location::Stack, Location::Stack>, int32_t, int32_t, int32_t> function((uintptr_t)&Subtract_ArgumentsStackOnly);
			auto result = function.Call(5, 3);
			assert(result == 2);
		}

		{
			Function<FunctionSignature<CallingConvention::Cdecl, Location::EAX, Location::EAX, Location::EBX>, int32_t, int32_t, int32_t> function((uintptr_t)&Subtract_ArgumentsRegistersOnly);
			auto result = function.Call(5, 3);
			assert(result == 2);
		}

		{
			Function<FunctionSignature<CallingConvention::Cdecl, Location::EAX, Location::EAX, Location::Stack>, int32_t, int32_t, int32_t> function((uintptr_t)&Subtract_ArgumentsMixed);
			assert(function.Call(5, 3) == 2);
		}

	}

};

namespace FloatSubtractionTests
{

	void __declspec(naked) FloatSubtract()
	{
		__asm
		{
			fld [esp + 4]
			fld [esp + 8]
			fsub
			ret
		}
	}

	void Run()
	{
		using namespace Unconventional;

		{
			Function<FunctionSignature<CallingConvention::Cdecl, Location::ST0, Location::Stack, Location::Stack>, float, float, float> function((uintptr_t)&FloatSubtract);
			assert(abs(function.Call(5, 3) - 2.0f) < 0.001f);
		}
	}
}

void RunFunctionCallingTests()
{
	IntegerSubtractionTests::Run();
	FloatSubtractionTests::Run();
}