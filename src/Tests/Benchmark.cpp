#include "Test.hpp"
#include "../Unconventional.hpp"

#define MEASURE_START(x) const std::chrono::steady_clock::time_point x = std::chrono::steady_clock::now()
#define MEASURE_END(x) std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - x).count() / 1000000.0

using namespace Unconventional;

uint32_t __declspec(naked) Multiply()
{
	__asm
	{
		mov eax, [esp + 4]
		mul[esp + 8]
		ret
	}
}

Hook<FunctionSignature <CallingConvention::Cdecl, Location::EAX, Location::Stack, Location::Stack>, uint32_t, uint32_t, uint32_t> hook;

uint32_t testHook(uint32_t a, uint32_t b)
{
	return hook.CallOriginalFunction(a, b);
}

void RunBenchmark()
{

	constexpr int ITERATIONS = 1000;

	Function<FunctionSignature <CallingConvention::Cdecl, Location::EAX, Location::Stack, Location::Stack>, uint32_t, uint32_t, uint32_t> function((uintptr_t) & Multiply);

	MEASURE_START(UnconvFunction);
	for (int i = 0; i < ITERATIONS; i++)
	{
		function.Call(i, 2);
	}
	auto unconvTime = MEASURE_END(UnconvFunction);
	

	MEASURE_START(StandardFunction);
	for (int i = 0; i < ITERATIONS; i++)
	{
		__asm
		{
			push i
			push 2
			call Multiply
			add esp, 8
		}
	}
	auto standardTime = MEASURE_END(StandardFunction);

	std::cout << std::fixed << std::setprecision(6) << "Unconventional::Function Call: " << unconvTime << std::endl;
	std::cout << std::fixed << std::setprecision(6) << "Standard Function Call: " << standardTime << std::endl;
	std::cout << std::fixed << std::setprecision(6) << (unconvTime / standardTime) << "x" << std::endl;

	hook = Hook(function, (uintptr_t)&testHook, 8);
	hook.Install();
	MEASURE_START(hookedFunction);
	for (int i = 0; i < ITERATIONS; i++)
	{
		function.Call(i, 2);
	}
	auto hookedTime = MEASURE_END(hookedFunction);

	hook.Uninstall();

	MEASURE_START(unhookedFunction);
	for (int i = 0; i < ITERATIONS; i++)
	{
		__asm
		{
			push i
			push 2
			call Multiply
			add esp, 8
		}
	}
	auto unhookedTime = MEASURE_END(unhookedFunction);

	std::cout << std::fixed << std::setprecision(6) << "Hooked Call: " << hookedTime << std::endl;
	std::cout << std::fixed << std::setprecision(6) << "Unhooked Call: " << unhookedTime << std::endl;
	std::cout << std::fixed << std::setprecision(6) << (hookedTime / unhookedTime) << "x" << std::endl;
}