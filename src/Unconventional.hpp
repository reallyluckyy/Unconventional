#pragma once

#include <stdexcept>
#include <functional>
#include <cstdint>
#include <array>
#include <vector>

#include <Windows.h>

namespace Unconventional
{
	enum class Location
	{
		Stack,
		EAX, EBX, ECX, EDX, ESI, EDI,
		// TODO: AH, AL, BH, BL, CH, CL, DH, DL, SIL, DIL,
		ST0
	};

	enum class CallingConvention
	{
		Cdecl
	};

	namespace CallingConventionUtils
	{
		constexpr bool SpecifiesCallerCleanup(const CallingConvention convention)
		{
			switch (convention)
			{
			case CallingConvention::Cdecl:
				return true;
			default:
				throw std::exception("Not implemented");
			}

			return false;
		}
	}

	namespace Utils
	{
		static uint8_t GetLowByte(uint32_t x)
		{
			return x & 0xFF;
		}

		static uint8_t GetHighByte(uint32_t x)
		{
			return (x >> 8) & 0xFF;
		}
	}

	// TODO: Make these optional
	template<CallingConvention callingConvention, Location returnValueLocation, Location... argumentLocations>
	class FunctionSignature
	{
	public:

		static consteval CallingConvention GetCallingConvention()
		{
			return callingConvention;
		}

		static consteval Location GetReturnValueLocation()
		{
			static_assert(returnValueLocation != Location::Stack, "Return value location can not be stack");

			return returnValueLocation;
		}

		static consteval std::array<Location, sizeof...(argumentLocations)> GetArgumentLocations()
		{
			// TODO: static_assert that argument locations are not overlapping

			return std::array<Location, sizeof...(argumentLocations)>({ argumentLocations... });
		}

		static consteval uint32_t GetStackArgumentCount()
		{
			uint32_t count = 0;
			for (int32_t i = 0; i < (int32_t)sizeof...(argumentLocations); i++)
			{
				constexpr auto argumentLocationsArray = GetArgumentLocations();
				if (argumentLocationsArray[i] == Location::Stack)
					count++;
			}
			return count;
		}

		static consteval std::array<int32_t, GetStackArgumentCount()> GetStackArgumentIndices()
		{
			std::array<int32_t, GetStackArgumentCount()> indices{};
			uint32_t writeIndex = 0;
			for (int32_t i = 0; i < (int32_t)sizeof...(argumentLocations); i++)
			{
				constexpr auto argumentLocationsArray = GetArgumentLocations();
				if (argumentLocationsArray[i] == Location::Stack)
				{
					indices[writeIndex++] = i;
				}
			}
			return indices;
		}

		static consteval int32_t GetArgumentIndexForRegister(Location location)
		{
			if (location == Location::Stack)
				throw std::invalid_argument("Location passed to GetArgumentIndexForRegister can not be Location::Stack");

			
			constexpr auto argumentLocationsArray = GetArgumentLocations();
			for (int32_t i = 0; i < (int32_t)sizeof...(argumentLocations); i++)
			{
				if (argumentLocationsArray[i] == location)
				{
					return i;
				}
			}

			return -1;
		}

		static consteval bool HasArgumentInRegister(Location location)
		{
			if (location == Location::Stack)
				throw std::invalid_argument("Location passed to HasArgumentInRegister can not be Location::Stack");

			return GetArgumentIndexForRegister(location) != -1;
		}
		
	};

	template<typename Signature, typename ReturnType, typename... ArgumentTypes>
	class Function
	{
	public:

		Function(uintptr_t address) : address(address)
		{
			static_assert(sizeof(uint32_t) == 4);
			static_assert(sizeof(uint16_t) == 2);
			static_assert(sizeof(uint8_t) == 1);
		}

		uintptr_t GetAddress() const { return address; }

		ReturnType Call(ArgumentTypes... arguments)
		{
			constexpr uint32_t argumentCount = sizeof...(arguments);
			static_assert(Signature::GetArgumentLocations().size() == argumentCount, "Amount of argument locations does not match number of function arguments");
			
			uint32_t integerArguments[argumentCount] { *(std::uint32_t*)&arguments... };

			uintptr_t functionAddress = address;

			// TODO: Prepare FPU Stack Arguments if needed. For now, floating point arguments are passed on the (regular) stack.

			// Prepare Stack Arguments
			
			constexpr uint32_t stackArgumentCount = Signature::GetStackArgumentCount();
			constexpr std::array<int32_t, stackArgumentCount> stackArgumentIndices = Signature::GetStackArgumentIndices();
			constexpr auto byteSizeOfStackArguments = stackArgumentCount * 4;
			
			__asm pushad

			// TODO: Turn this into assembly
			for (uint32_t i = stackArgumentCount; i > 0; --i)
			{
				uint32_t stackArgument = integerArguments[stackArgumentIndices[i - 1]];
				__asm
				{
					push stackArgument
				}
			}

			// Prepare Register Values

			static_assert(!Signature::HasArgumentInRegister(Location::ST0), "Arguments in FPU registers are currently not supported");

			if constexpr (Signature::HasArgumentInRegister(Location::EAX))
			{
				constexpr auto argumentOffset = Signature::GetArgumentIndexForRegister(Location::EAX) * sizeof(uint32_t);
				__asm
				{
					lea eax, integerArguments
					add eax, argumentOffset
					mov eax, [eax]
				}
			}

			if constexpr (Signature::HasArgumentInRegister(Location::EBX))
			{
				constexpr auto argumentOffset = Signature::GetArgumentIndexForRegister(Location::EBX) * sizeof(uint32_t);
				__asm
				{
					lea ebx, integerArguments
					add ebx, argumentOffset
					mov ebx, [ebx]
				}
			}

			if constexpr (Signature::HasArgumentInRegister(Location::ECX))
			{
				constexpr auto argumentOffset = Signature::GetArgumentIndexForRegister(Location::ECX) * sizeof(uint32_t);
				__asm
				{
					lea ecx, integerArguments
					add ecx, argumentOffset
					mov ecx, [ecx]
				}
			}

			if constexpr (Signature::HasArgumentInRegister(Location::EDX))
			{
				constexpr auto argumentOffset = Signature::GetArgumentIndexForRegister(Location::EDX) * sizeof(uint32_t);
				__asm
				{
					lea edx, integerArguments
					add edx, argumentOffset
					mov edx, [edx]
				}
			}

			if constexpr (Signature::HasArgumentInRegister(Location::ESI))
			{
				constexpr auto argumentOffset = Signature::GetArgumentIndexForRegister(Location::ESI) * sizeof(uint32_t);
				__asm
				{
					lea esi, integerArguments
					add esi, argumentOffset
					mov esi, [esi]
				}
			}

			if constexpr (Signature::HasArgumentInRegister(Location::EDI))
			{
				constexpr auto argumentOffset = Signature::GetArgumentIndexForRegister(Location::EDI) * sizeof(uint32_t);
				__asm
				{
					lea edi, integerArguments
					add edi, argumentOffset
					mov edi, [edi]
				}
			}

			__asm
			{
				call functionAddress
			}

			if constexpr (CallingConventionUtils::SpecifiesCallerCleanup(Signature::GetCallingConvention()) && byteSizeOfStackArguments > 0)
			{
				__asm add esp, byteSizeOfStackArguments
			}

			uint32_t returnValue;
			if constexpr (Signature::GetReturnValueLocation() == Location::EAX)
			{
				__asm mov returnValue, eax
			}

			if constexpr (Signature::GetReturnValueLocation() == Location::ST0)
			{
				__asm fstp returnValue
			}

			__asm popad

			return *(ReturnType*)&returnValue;
		}

	private:
		uintptr_t address;
	};

	
	template<typename Signature, typename ReturnType, typename... ArgumentTypes>
	class Hook
	{
	public:
		void Install()
		{
			if (!isInitialized)
			{
				throw std::logic_error("Hook was not initialized");
			}

			if (!isInstalled) 
			{
				WriteJump(originalFunction.GetAddress(), hookWrapperAddress);
				isInstalled = true;
			}
		}

		void Uninstall()
		{
			if (!isInitialized)
			{
				throw std::logic_error("Hook was not initialized");
			}

			if (isInstalled) 
			{
				std::memcpy((void*)originalFunction.GetAddress(), (void*)trampolineAddress, opCodeSize);
				isInstalled = false;
			}
		}

		ReturnType CallOriginalFunction(ArgumentTypes... arguments)
		{
			Function<Signature, ReturnType, ArgumentTypes...> trampolineFunction(trampolineAddress);
			return trampolineFunction.Call(arguments...);
		}

		Hook() : isInitialized(false), isInstalled(false), opCodeSize(0), originalFunction(0), userHookFunctionAddress(0),
		         trampolineAddress(0),
		         hookWrapperAddress(0),
		         tempStorage{}
		{
		}

		Hook(Function<Signature, ReturnType, ArgumentTypes...> originalFunction, uintptr_t hookFunctionAddress, const uint8_t opCodeSize)
			: isInstalled(false), opCodeSize(opCodeSize), originalFunction(originalFunction), userHookFunctionAddress(hookFunctionAddress), trampolineAddress(0), hookWrapperAddress(0)
		{
			if (opCodeSize < 5)
			{
				throw std::invalid_argument("At least 5 bytes are required for hooking");
			}

			SetupTrampoline();
			SetupHookWrapper();

			isInitialized = true;
		}

		~Hook()
		{
			if (isInitialized) 
			{
				Uninstall();

				VirtualFree((void*)trampolineAddress, trampolineSize, MEM_RELEASE);
				VirtualFree((void*)hookWrapperAddress, MAX_HOOK_WRAPPER_CODE_SIZE, MEM_RELEASE);
			}
		}

	private:
		bool isInitialized;
		bool isInstalled;

		uint8_t opCodeSize;
		Function<Signature, ReturnType, ArgumentTypes...> originalFunction;
		uintptr_t userHookFunctionAddress;

		uintptr_t trampolineAddress;
		uint32_t trampolineSize;

		uintptr_t hookWrapperAddress;
		std::uint32_t tempStorage[128];

		static constexpr uint32_t MAX_HOOK_WRAPPER_CODE_SIZE = 512;
		static constexpr uint8_t SIZE_OF_JUMP = 5;

		static void WriteJump(const std::uintptr_t address, const std::uintptr_t target)
		{
			DWORD oldProtection;
			VirtualProtect((void*)address, SIZE_OF_JUMP, PAGE_EXECUTE_READWRITE, &oldProtection);

			const auto relativeJumpOffset = target - address - SIZE_OF_JUMP;

			*(uint8_t*)address = 0xE9;
			*(uint32_t*)(address + 1) = relativeJumpOffset;
		}

		void SetupTrampoline()
		{
			trampolineSize = opCodeSize + SIZE_OF_JUMP;

			trampolineAddress = (uintptr_t)VirtualAlloc(nullptr, trampolineSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			std::memcpy((void*)trampolineAddress, (void*)originalFunction.GetAddress(), opCodeSize);

			WriteJump(trampolineAddress + opCodeSize, originalFunction.GetAddress() + opCodeSize);
		}

		void SetupHookWrapper()
		{
			hookWrapperAddress = (uintptr_t)VirtualAlloc(nullptr, MAX_HOOK_WRAPPER_CODE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);


			std::vector<uint8_t> hookWrapperBytes;

			// Pop the return address
			{
				const auto address = (uintptr_t)&tempStorage[0];
				hookWrapperBytes.push_back(0x8F);
				hookWrapperBytes.push_back(0x05);
				hookWrapperBytes.push_back(Utils::GetLowByte(address));
				hookWrapperBytes.push_back(Utils::GetHighByte(address));
				hookWrapperBytes.push_back(Utils::GetLowByte(address >> 16));
				hookWrapperBytes.push_back(Utils::GetHighByte(address >> 16));
			}

			const uint32_t stackArgumentCount = Signature::GetStackArgumentIndices().size();
			for (uint32_t i = 0; i < stackArgumentCount; i++)
			{
				// Write pop to local storage location
				const auto address = (uintptr_t)&tempStorage[i + 1];
				hookWrapperBytes.push_back(0x8F);
				hookWrapperBytes.push_back(0x05);
				hookWrapperBytes.push_back(Utils::GetLowByte(address));
				hookWrapperBytes.push_back(Utils::GetHighByte(address));
				hookWrapperBytes.push_back(Utils::GetLowByte(address >> 16));
				hookWrapperBytes.push_back(Utils::GetHighByte(address >> 16));
			}


			// Push all registers
			hookWrapperBytes.push_back(0x60);

			// Write a push for each argument
			auto argumentLocations = Signature::GetArgumentLocations();
			std::reverse(argumentLocations.begin(), argumentLocations.end());
			uint32_t stackWriteIndex = 0;
			for (const Location location : argumentLocations)
			{
				switch (location)
				{
				case Location::Stack:
				{
					const auto address = (uintptr_t)&tempStorage[stackArgumentCount - stackWriteIndex++];
					hookWrapperBytes.push_back(0xFF);
					hookWrapperBytes.push_back(0x35);
					hookWrapperBytes.push_back(Utils::GetLowByte(address));
					hookWrapperBytes.push_back(Utils::GetHighByte(address));
					hookWrapperBytes.push_back(Utils::GetLowByte(address >> 16));
					hookWrapperBytes.push_back(Utils::GetHighByte(address >> 16));
				}
					break;
				case Location::EAX:
					hookWrapperBytes.push_back(0x50);
					break;
				case Location::EBX:
					hookWrapperBytes.push_back(0x53);
					break;
				case Location::ECX:
					hookWrapperBytes.push_back(0x51);
					break;
				case Location::EDX:
					hookWrapperBytes.push_back(0x52);
					break;
				case Location::ESI:
					hookWrapperBytes.push_back(0x56);
					break;
				case Location::EDI:
					hookWrapperBytes.push_back(0x57);
					break;
				default:
					throw std::exception("Not yet implemented");
				}
			}

			// Write call
			hookWrapperBytes.push_back(0xE8);
			const auto relativeCallOffset = userHookFunctionAddress - (hookWrapperAddress + hookWrapperBytes.size() - 1) - 5;
			hookWrapperBytes.push_back(Utils::GetLowByte(relativeCallOffset));
			hookWrapperBytes.push_back(Utils::GetHighByte(relativeCallOffset));
			hookWrapperBytes.push_back(Utils::GetLowByte(relativeCallOffset >> 16));
			hookWrapperBytes.push_back(Utils::GetHighByte(relativeCallOffset >> 16));

			// add esp, X
			hookWrapperBytes.push_back(0x83);
			hookWrapperBytes.push_back(0xC4);
			hookWrapperBytes.push_back((uint8_t)argumentLocations.size() * 4);

			// Save EAX to tempStorage
			const auto eaxStorageAddress = (uintptr_t)&tempStorage[1];
			hookWrapperBytes.push_back(0xA3);
			hookWrapperBytes.push_back(Utils::GetLowByte(eaxStorageAddress));
			hookWrapperBytes.push_back(Utils::GetHighByte(eaxStorageAddress));
			hookWrapperBytes.push_back(Utils::GetLowByte(eaxStorageAddress >> 16));
			hookWrapperBytes.push_back(Utils::GetHighByte(eaxStorageAddress >> 16));
			

			// Pop all registers
			hookWrapperBytes.push_back(0x61);

			// Put return value where it needs to go
			// We sort of assume the user's hook function itself to be CDECL
			auto returnValueLocation = std::is_floating_point<ReturnType>() ? Location::ST0 : Location::EAX;
			if (returnValueLocation == Location::EAX)
			{
				switch (Signature::GetReturnValueLocation())
				{
				case Location::EAX:
					hookWrapperBytes.push_back(0xA1);
					break;
				case Location::EBX:
					hookWrapperBytes.push_back(0x8B);
					hookWrapperBytes.push_back(0x1D);
					break;
				case Location::ECX:
					hookWrapperBytes.push_back(0x8B);
					hookWrapperBytes.push_back(0x0D);
					break;
				case Location::EDX:
					hookWrapperBytes.push_back(0x8B);
					hookWrapperBytes.push_back(0x15);
					break;
				case Location::ESI:
					hookWrapperBytes.push_back(0x8B);
					hookWrapperBytes.push_back(0x35);
					break;
				case Location::EDI:
					hookWrapperBytes.push_back(0x8B);
					hookWrapperBytes.push_back(0x3D);
					break;
				// TODO: Handle 8-bit registers
				default:
					throw std::exception("Return value location not implemented");
				}
				hookWrapperBytes.push_back(Utils::GetLowByte(eaxStorageAddress));
				hookWrapperBytes.push_back(Utils::GetHighByte(eaxStorageAddress));
				hookWrapperBytes.push_back(Utils::GetLowByte(eaxStorageAddress >> 16));
				hookWrapperBytes.push_back(Utils::GetHighByte(eaxStorageAddress >> 16));
			}
			else if (returnValueLocation == Location::ST0)
			{
				// Pop all registers
				hookWrapperBytes.push_back(0x61);

				switch (Signature::GetReturnValueLocation())
				{
				case Location::ST0:
					break;
				// TODO: Implement these on-demand
				default:
					throw std::exception("Floating-point return value location not implemented");
				}
			}
			else
			{
				throw std::logic_error("Return value location for CDECL function was neither EAX nor ST0");
			}

			// sub esp, X
			hookWrapperBytes.push_back(0x83);
			hookWrapperBytes.push_back(0xEC);
			hookWrapperBytes.push_back((uint8_t)stackArgumentCount * 4);

			// Push return address to top of stack again
			{
				const auto address = (uintptr_t)&tempStorage[0];
				hookWrapperBytes.push_back(0xFF);
				hookWrapperBytes.push_back(0x35);
				hookWrapperBytes.push_back(Utils::GetLowByte(address));
				hookWrapperBytes.push_back(Utils::GetHighByte(address));
				hookWrapperBytes.push_back(Utils::GetLowByte(address >> 16));
				hookWrapperBytes.push_back(Utils::GetHighByte(address >> 16));
			}

			// Write Return
			hookWrapperBytes.push_back(0xC3);

			// Write hook wrapper to memory
			if (hookWrapperBytes.size() > MAX_HOOK_WRAPPER_CODE_SIZE)
				throw std::logic_error("Hook Wrapper Function byte size was larger than MAX_HOOK_WRAPPER_CODE_SIZE");

			std::memcpy((void*)hookWrapperAddress, hookWrapperBytes.data(), hookWrapperBytes.size());
		}
		
	};
	
}