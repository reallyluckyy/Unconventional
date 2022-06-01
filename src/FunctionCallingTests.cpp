#include <cassert>

#include "Unconventional.hpp"

namespace IntegerSubtractionTests
{

	int32_t __cdecl Subtract_ArgumentsStackOnly(int32_t x, int32_t y)
	{
		return x - y;
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

		auto subtract_ArgumentsStackOnly = Function<int32_t, int32_t, int32_t>((uintptr_t)&Subtract_ArgumentsStackOnly, {});
		assert(subtract_ArgumentsStackOnly.Call(5, 3) == 2);

		auto subtract_ArgumentsRegistersOnly = Function<int32_t, int32_t, int32_t>((uintptr_t)&Subtract_ArgumentsRegistersOnly, {
			{
				Location::EAX, Location::EBX
			} });
		assert(subtract_ArgumentsRegistersOnly.Call(5, 3) == 2);

		auto subtract_ArgumentsMixed = Function<int32_t, int32_t, int32_t>((uintptr_t)&Subtract_ArgumentsMixed, {
			{
				Location::EAX, Location::Stack
			} });
		assert(subtract_ArgumentsMixed.Call(5, 3) == 2);

	}

};

namespace ByteSubtractionTests
{

	int8_t __cdecl SubtractBytes_ArgumentsStackOnly(int8_t x, int8_t y)
	{
		return x - y;
	}

	int8_t __declspec(naked) SubtractBytes_ArgumentsLowRegistersOnly(/* int8_t<al> x, int8_t<bl> y*/)
	{
		__asm
		{
			sub al, bl
			ret
		}
	}

	int8_t/*<ah>*/ __declspec(naked) SubtractBytes_ArgumentsHighRegistersOnly(/* int8_t<ah> x, int8_t<bh> y*/)
	{
		__asm
		{
			sub ah, bh
			ret
		}
	}

	int8_t/*<ah>*/ __declspec(naked) SubtractBytes_ArgumentsHighLowRegistersOnly(/* int8_t<ah> x, int8_t<bl> y*/)
	{
		__asm
		{
			sub ah, bl
			ret
		}
	}

	int8_t/*<al>*/ __declspec(naked) SubtractBytes_ArgumentsMixed(/*int8_t<al> x, int8_t y*/)
	{
		__asm
		{
			sub al, [esp + 4]
			ret
		}
	}

	void Run()
	{
		using namespace Unconventional;

		auto subtractBytes_ArgumentsStackOnly = Function<int8_t, int8_t, int8_t>((uintptr_t)&SubtractBytes_ArgumentsStackOnly, {});
		assert(subtractBytes_ArgumentsStackOnly.Call(5, 3) == 2);

		auto subtractBytes_ArgumentsLowRegistersOnly = Function<int8_t, int8_t, int8_t>((uintptr_t)&SubtractBytes_ArgumentsLowRegistersOnly, {
			{
				Location::AL, Location::BL
			} });
		assert(subtractBytes_ArgumentsLowRegistersOnly.Call(5, 3) == 2);

		auto subtractBytes_ArgumentsHighRegistersOnly = Function<int8_t, int8_t, int8_t>((uintptr_t)&SubtractBytes_ArgumentsHighRegistersOnly, {
			Location::AH,
			{
				Location::AH, Location::BH
			} });
		assert(subtractBytes_ArgumentsHighRegistersOnly.Call(5, 3) == 2);

		auto subtractBytes_ArgumentsHighLowRegistersOnly = Function<int8_t, int8_t, int8_t>((uintptr_t)&SubtractBytes_ArgumentsHighLowRegistersOnly, {
			Location::AH,
			{
				Location::AH, Location::BL
			} });
		assert(subtractBytes_ArgumentsHighLowRegistersOnly.Call(5, 3) == 2);

		auto subtractBytes_ArgumentsMixed = Function<int8_t, int8_t, int8_t>((uintptr_t)&SubtractBytes_ArgumentsMixed, {
			Location::AL,
			{
				Location::AL, Location::Stack
			} });
		assert(subtractBytes_ArgumentsMixed.Call(5, 3) == 2);
	}

}

namespace FloatSubtractionTests
{

	float __declspec(naked) FloatSubtract_ArgumentsStackOnly(/*float x, float y*/)
	{
		__asm
		{
			fsubp st(1), st(0)
			ret
		}
	}

	float __declspec(naked) FloatSubtract_ArgumentsRegistersOnly(/*float<st5> x, float<st4> y*/)
	{
		__asm
		{
			fstp st(0)
			fstp st(0)
			fstp st(0)
			fstp st(0)
			fsubp st(1), st(0)
			ret
		}
	}

	float __declspec(naked) FloatSubtract_ArgumentsMixed(/*float<st5> x, float y*/)
	{
		static float temp;
		__asm
		{
			fstp temp
			fstp st(0)
			fstp st(0)
			fstp st(0)
			fstp st(0)
			fld temp
			fsubp st(1), st(0)
			ret

		}
	}

	void Run()
	{
		using namespace Unconventional;

		auto floatSubtract_ArgumentsStackOnly = Function<float, float, float>((uintptr_t)&FloatSubtract_ArgumentsStackOnly, { });
		assert(abs(floatSubtract_ArgumentsStackOnly.Call(5, 3) - 2.0f) < 0.001f);

		auto floatSubtract_ArgumentsRegistersOnly = Function<float, float, float>((uintptr_t)&FloatSubtract_ArgumentsRegistersOnly, {
			{
				Location::ST5, Location::ST4
			} });
		assert(abs(floatSubtract_ArgumentsRegistersOnly.Call(5, 3) - 2.0f) < 0.001f);

		auto floatSubtract_ArgumentsMixed = Function<float, float, float>((uintptr_t)&FloatSubtract_ArgumentsMixed, {
			{
				Location::ST5, Location::Stack
			} });
		assert(abs(floatSubtract_ArgumentsMixed.Call(5, 3) - 2.0f) < 0.001f);
	}
}


void RunErrorTests()
{
	using namespace Unconventional;

	try
	{
		auto function1 = Function<void, int32_t, int32_t>(0x0, {
			{Location::EAX, Location::EAX}
			});

		assert(false);
	}
	catch (std::invalid_argument)
	{
	}

	try
	{
		auto function2 = Function<void, int32_t, int32_t>(0x0, {
			{Location::AL, Location::EAX}
			});

		assert(false);
	}
	catch (std::invalid_argument)
	{
	}
}

void RunFunctionCallingTests()
{
	IntegerSubtractionTests::Run();
	ByteSubtractionTests::Run();
	FloatSubtractionTests::Run();

	RunErrorTests();
}