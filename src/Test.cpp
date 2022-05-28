#include <cassert>

#include "Unconventional.hpp"

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
		sub eax, [esp+4]
		ret
	}
}



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

void RunIntegerTests()
{
	auto subtract_ArgumentsStackOnly = Unconventional::Function<int32_t, int32_t, int32_t>((uintptr_t)&Subtract_ArgumentsStackOnly, {});
	assert(subtract_ArgumentsStackOnly.Call(5, 3) == 2);

	auto subtract_ArgumentsRegistersOnly = Unconventional::Function<int32_t, int32_t, int32_t>((uintptr_t)&Subtract_ArgumentsRegistersOnly, {
		{
			Unconventional::Location::EAX, Unconventional::Location::EBX
		} });
	assert(subtract_ArgumentsRegistersOnly.Call(5, 3) == 2);

	auto subtract_ArgumentsMixed = Unconventional::Function<int32_t, int32_t, int32_t>((uintptr_t)&Subtract_ArgumentsMixed, {
		{
			Unconventional::Location::EAX, Unconventional::Location::STACK
		} });
	assert(subtract_ArgumentsMixed.Call(5, 3) == 2);

}

void RunByteTests()
{
	auto subtractBytes_ArgumentsStackOnly = Unconventional::Function<int8_t, int8_t, int8_t>((uintptr_t)&SubtractBytes_ArgumentsStackOnly, {});
	assert(subtractBytes_ArgumentsStackOnly.Call(5, 3) == 2);

	auto subtractBytes_ArgumentsLowRegistersOnly = Unconventional::Function<int8_t, int8_t, int8_t>((uintptr_t)&SubtractBytes_ArgumentsLowRegistersOnly, {
		{
			Unconventional::Location::AL, Unconventional::Location::BL
		} });
	assert(subtractBytes_ArgumentsLowRegistersOnly.Call(5, 3) == 2);

	auto subtractBytes_ArgumentsHighRegistersOnly = Unconventional::Function<int8_t, int8_t, int8_t>((uintptr_t)&SubtractBytes_ArgumentsHighRegistersOnly, {
		Unconventional::Location::AH,
		{
			Unconventional::Location::AH, Unconventional::Location::BH
		} });
	assert(subtractBytes_ArgumentsHighRegistersOnly.Call(5, 3) == 2);

	auto subtractBytes_ArgumentsHighLowRegistersOnly = Unconventional::Function<int8_t, int8_t, int8_t>((uintptr_t)&SubtractBytes_ArgumentsHighLowRegistersOnly, {
		Unconventional::Location::AH,
		{
			Unconventional::Location::AH, Unconventional::Location::BL
		} });
	assert(subtractBytes_ArgumentsHighLowRegistersOnly.Call(5, 3) == 2);

	auto subtractBytes_ArgumentsMixed = Unconventional::Function<int8_t, int8_t, int8_t>((uintptr_t)&SubtractBytes_ArgumentsMixed, {
		Unconventional::Location::AL,
		{
			Unconventional::Location::AL, Unconventional::Location::STACK
		} });
	assert(subtractBytes_ArgumentsMixed.Call(5, 3) == 2);
}

void RunFloatingPointTests()
{
	auto floatSubtract_ArgumentsStackOnly = Unconventional::Function<float, float, float>((uintptr_t)&FloatSubtract_ArgumentsStackOnly, { });
	assert(abs(floatSubtract_ArgumentsStackOnly.Call(5, 3) - 2.0f) < 0.001f);

	auto floatSubtract_ArgumentsRegistersOnly = Unconventional::Function<float, float, float>((uintptr_t)&FloatSubtract_ArgumentsRegistersOnly, {
		{
			Unconventional::Location::ST5, Unconventional::Location::ST4
		} });
	assert(abs(floatSubtract_ArgumentsRegistersOnly.Call(5, 3) - 2.0f) < 0.001f);

	auto floatSubtract_ArgumentsMixed = Unconventional::Function<float, float, float>((uintptr_t)&FloatSubtract_ArgumentsMixed, {
		{
			Unconventional::Location::ST5, Unconventional::Location::STACK
		} });
	assert(abs(floatSubtract_ArgumentsMixed.Call(5, 3) - 2.0f) < 0.001f);
}

int main()
{
	RunIntegerTests();
	RunByteTests();
	RunFloatingPointTests();

	return 0;
}