#include "Test.hpp"

void RunFunctionCallingTests();
void RunHookingTests();

void RunBenchmark();

int main()
{
	RunFunctionCallingTests();
	RunHookingTests();

	RunBenchmark();

	return 0;
}