# Unconventional

A work-in-progress header-only x86 function hooking library with support for unconventional function signatures.

## Usage:
Simply include Unconventional.hpp into your project; there are no other dependencies or requirements.

Here is an example for how the library can be used to call a native function:
```C

int main() {

	// Assuming we have a function at address 0xDEADBEEF, that returns an int32_t and takes two int32_t parameters, the first in EAX and the second one passed on the stack
	auto function = Unconventional::Function<int32_t, int32_t, int32_t>(0xDEADBEEF, {
		{
			Unconventional::Location::EAX, Unconventional::Location::STACK
		} });
	
	// Call the function
	int32_t result = function.Call(5, 3);

	return 0;

}

```
