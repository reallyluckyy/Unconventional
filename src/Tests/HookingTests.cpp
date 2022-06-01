#include <cassert>

#include "../Unconventional.hpp"

// We have to use naked functions here as we need to be sure what this compiles to,
// as we need to specify the opcode size later for hooking

void __declspec(naked) Subtract_ArgumentsStackOnly(/*int32_t a, int32_t b*/)
{
	__asm
	{
		mov eax, [esp + 4]
		sub eax, [esp + 8]
		ret
	}
}

void __declspec(naked) Subtract_ArgumentsRegistersOnly(/*int32_t<eax> a, int32_t<ebx> b*/)
{
	__asm
	{
		sub eax, ebx
		nop
		nop
		nop
		ret
	}
}

void __declspec(naked) Subtract_ArgumentsMixed(/*int32_t<eax> a, int32_t b*/)
{
	__asm
	{
		sub eax, [esp + 4]
		nop
		ret
	}
}



namespace BasicRedirectionTests
{

	int32_t Subtract_Hook(int32_t a, int32_t b)
	{
		return b - a;
	}

	void Run()
	{
		using namespace Unconventional;

		int eaxValue = 0;

		constexpr int parameter1 = 10;
		constexpr int parameter2 = 8;
		int expectedHookResult = Subtract_Hook(parameter1, parameter2);
		int expectedOriginalResult = parameter1 - parameter2;

		{
			Function<int32_t, int32_t, int32_t> function((uintptr_t)&Subtract_ArgumentsStackOnly, {});
			Hook hook(function, (uintptr_t)&Subtract_Hook, 8);
			hook.Install();
			
			__asm
			{
				push parameter2
				push parameter1
				call Subtract_ArgumentsStackOnly
				add esp, 8
				mov eaxValue, eax
			}
			assert(eaxValue == expectedHookResult);
			
			hook.Uninstall();

			__asm
			{
				push parameter2
				push parameter1
				call Subtract_ArgumentsStackOnly
				add esp, 8
				mov eaxValue, eax
			}
			assert(eaxValue == expectedOriginalResult);
		}

		{
			Function<int32_t, int32_t, int32_t> function((uintptr_t)&Subtract_ArgumentsRegistersOnly, { {Location::EAX, Location::EBX} });
			Hook hook(function, (uintptr_t)&Subtract_Hook, 5);
			hook.Install();

			__asm
			{
				mov ebx, parameter2
				mov eax, parameter1
				call Subtract_ArgumentsRegistersOnly
				mov eaxValue, eax
			}
			assert(eaxValue == expectedHookResult);

			hook.Uninstall();

			__asm
			{
				mov ebx, parameter2
				mov eax, parameter1
				call Subtract_ArgumentsRegistersOnly
				mov eaxValue, eax
			}
			assert(eaxValue == expectedOriginalResult);
		}

		{
			Function<int32_t, int32_t, int32_t> function((uintptr_t)&Subtract_ArgumentsMixed, { {Location::EAX, Location::Stack} });
			Hook hook(function, (uintptr_t)&Subtract_Hook, 5);
			hook.Install();

			__asm
			{
				push parameter2
				mov eax, parameter1
				call Subtract_ArgumentsMixed
				add esp, 4
				mov eaxValue, eax
			}
			assert(eaxValue == expectedHookResult);

			hook.Uninstall();

			__asm
			{
				push parameter2
				mov eax, parameter1
				call Subtract_ArgumentsMixed
				add esp, 4
				mov eaxValue, eax
			}
			assert(eaxValue == expectedOriginalResult);
		}
	}
}

namespace TrampolineTests
{
	Unconventional::Hook<int32_t, int32_t, int32_t> hook;

	int32_t Subtract_Hook(int32_t a, int32_t b)
	{
		return hook.CallOriginalFunction(a, b) + 100;
	}

	void Run()
	{
		using namespace Unconventional;

		int eaxValue = 0;

		constexpr int parameter1 = 10;
		constexpr int parameter2 = 8;
		constexpr int expectedOriginalResult = parameter1 - parameter2;
		constexpr int expectedHookResult = expectedOriginalResult + 100;

		{
			Function<int32_t, int32_t, int32_t> function((uintptr_t)&Subtract_ArgumentsStackOnly, {});
			hook = Hook(function, (uintptr_t)&Subtract_Hook, 8);
			hook.Install();
			
			__asm
			{
				push parameter2
				push parameter1
				call Subtract_ArgumentsStackOnly
				add esp, 8
				mov eaxValue, eax
			}
			assert(eaxValue == expectedHookResult);
			
			hook.Uninstall();

			__asm
			{
				push parameter2
				push parameter1
				call Subtract_ArgumentsStackOnly
				add esp, 8
				mov eaxValue, eax
			}
			assert(eaxValue == expectedOriginalResult);
		}

		{
			Function<int32_t, int32_t, int32_t> function((uintptr_t)&Subtract_ArgumentsRegistersOnly, { {Location::EAX, Location::EBX} });
			hook = Hook(function, (uintptr_t)&Subtract_Hook, 5);
			hook.Install();

			__asm
			{
				mov ebx, parameter2
				mov eax, parameter1
				call Subtract_ArgumentsRegistersOnly
				mov eaxValue, eax
			}
			assert(eaxValue == expectedHookResult);

			hook.Uninstall();

			__asm
			{
				mov ebx, parameter2
				mov eax, parameter1
				call Subtract_ArgumentsRegistersOnly
				mov eaxValue, eax
			}
			assert(eaxValue == expectedOriginalResult);
		}

		{
			Function<int32_t, int32_t, int32_t> function((uintptr_t)&Subtract_ArgumentsMixed, { {Location::EAX, Location::Stack} });
			hook = Hook(function, (uintptr_t)&Subtract_Hook, 5);
			hook.Install();

			__asm
			{
				push parameter2
				mov eax, parameter1
				call Subtract_ArgumentsMixed
				add esp, 4
				mov eaxValue, eax
			}
			assert(eaxValue == expectedHookResult);

			hook.Uninstall();

			__asm
			{
				push parameter2
				mov eax, parameter1
				call Subtract_ArgumentsMixed
				add esp, 4
				mov eaxValue, eax
			}
			assert(eaxValue == expectedOriginalResult);
		}
	}
}


void RunHookingTests()
{
	BasicRedirectionTests::Run();
	TrampolineTests::Run();
}