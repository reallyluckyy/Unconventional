#include "../Unconventional.hpp"

#include <iostream>
#include <chrono>

#define MEASURE_START(x) const std::chrono::steady_clock::time_point x = std::chrono::steady_clock::now();
#define MEASURE_END(x, description) std::cout << description << " (sec) = " << std::fixed << std::setprecision(6) << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - x).count() / 1000000.0 << std::endl;


uint32_t Multiply(uint32_t a, uint32_t b)
{
	return a * b;
}

void RunBenchmark()
{
	using namespace Unconventional;

	Function<uint32_t, uint32_t, uint32_t> function((uintptr_t) &Multiply, {});

	MEASURE_START(UnconvFunction);
	for (int i = 0; i < 100000; i++)
	{
		function.Call(i, 2);
	}
	MEASURE_END(UnconvFunction, "Unconventional::Function Call");


	MEASURE_START(StandardFunction);
	for (int i = 0; i < 100000; i++)
	{
		Multiply(i, 2);
	}
	MEASURE_END(StandardFunction, "Direct Function Call");
}