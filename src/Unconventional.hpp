#pragma once

#include <stdexcept>
#include <functional>
#include <cstdint>
#include <optional>
#include <array>
#include <vector>
#include <any>

#include <Windows.h>


namespace Unconventional
{
	enum class Location
	{
		Stack,
		EAX, EBX, ECX, EDX, ESI, EDI,
		AH, AL, BH, BL, CH, CL, DH, DL, SIL, DIL,
		ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7

		// TODO: Support 16-bit registers
		// TODO: Support XMM registers
	};

	class CallingConvention
	{
	public:
		enum Convention
		{
			Cdecl
		};

		CallingConvention(Convention value = Cdecl) : value(value)
		{
		}

		Convention GetValue() const { return value; }

		Location GetReturnValueLocation(const bool isFloatingPointValue) const
		{
			switch (value)
			{
			case Cdecl:
				if (isFloatingPointValue)
					return Location::ST0;
				else
					return Location::EAX;
			default:
				throw std::exception("Not implemented");
			}
		}

		bool SpecifiesCallerCleanup() const
		{
			switch (value)
			{
			case Cdecl:
				return true;
			default:
				throw std::exception("Not implemented");
			}

			return false;
		}

	private:

		Convention value;
	};

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

	template<typename ReturnType, typename... ArgumentTypes>
	class FunctionSignature
	{
	public:
		FunctionSignature(const CallingConvention callingConvention = CallingConvention::Cdecl) :
			callingConvention(callingConvention), returnValueLocation(), argumentLocations(std::array<Location, sizeof...(ArgumentTypes)>())
		{
		}

		FunctionSignature(Location returnValueLocation, const CallingConvention callingConvention = CallingConvention::Cdecl)
			: callingConvention(callingConvention), returnValueLocation(returnValueLocation), argumentLocations(std::array<Location, sizeof...(ArgumentTypes)>())
		{
		}

		FunctionSignature(Location returnValueLocation, std::array<Location, sizeof...(ArgumentTypes)> argumentLocations, const CallingConvention callingConvention = CallingConvention::Cdecl)
			: callingConvention(callingConvention), returnValueLocation(returnValueLocation), argumentLocations(argumentLocations)
		{
			VerifyReturnValueLocation();
			VerifyArgumentLocations();
		}

		FunctionSignature(std::array<Location, sizeof...(ArgumentTypes)> argumentLocations, const CallingConvention callingConvention = CallingConvention::Cdecl)
			: callingConvention(callingConvention), returnValueLocation(), argumentLocations(argumentLocations)
		{
			VerifyReturnValueLocation();
			VerifyArgumentLocations();
		}

		CallingConvention GetCallingConvention() const
		{
			return callingConvention;
		}

		Location GetReturnValueLocation() const
		{
			if (returnValueLocation.has_value())
				return returnValueLocation.value();
			return callingConvention.GetReturnValueLocation(std::is_floating_point<ReturnType>());
		}

		std::array<Location, sizeof...(ArgumentTypes)> GetArgumentLocations()
		{
			return argumentLocations;
		}

		std::vector<int32_t> GetStackArgumentIndices()
		{
			std::vector<int32_t> indices;
			for (int32_t i = 0; i < (int32_t)sizeof...(ArgumentTypes); i++)
			{
				if (argumentLocations[i] == Location::Stack)
				{
					indices.push_back(i);
				}
			}
			return indices;
		}

		int32_t GetArgumentIndexForRegister(Location location)
		{
			if (location == Location::Stack)
				throw std::invalid_argument("Location passed to GetArgumentIndexForRegister was Location::Stack");

			for (int32_t i = 0; i < (int32_t)sizeof...(ArgumentTypes); i++)
			{
				if (argumentLocations[i] == location)
				{
					return i;
				}
			}

			return -1;
		}

		bool HasArgumentInRegister(Location location)
		{
			if (location == Location::Stack)
				throw std::invalid_argument("Location passed to HasArgumentInRegister was Location::Stack");

			return this->GetArgumentIndexForRegister(location) != -1;
		}

	private:
		CallingConvention callingConvention;
		std::optional<Location> returnValueLocation;
		std::array<Location, sizeof...(ArgumentTypes)> argumentLocations;

		void VerifyReturnValueLocation()
		{
			if (returnValueLocation.has_value() && returnValueLocation == Location::Stack)
				throw std::invalid_argument("Return value location can not be stack");
		}

		void VerifyArgumentLocations()
		{
			auto AreArgumentLocationsIncompatible = [](Location x, Location y)
			{
				switch (x)
				{
				case Location::EAX:
					return y == Location::AH || y == Location::AL;
				case Location::EBX:
					return y == Location::BH || y == Location::BL;
				case Location::ECX:
					return y == Location::CH || y == Location::CL;
				case Location::EDX:
					return y == Location::DH || y == Location::DL;
				case Location::ESI:
					return y == Location::SIL;
				case Location::EDI:
					return y == Location::DIL;
				default: 
					return false;
				}
			};

			for (uint32_t i = 0; i < argumentLocations.size(); i++)
			{
				for (uint32_t j = 0; j < argumentLocations.size(); j++)
				{
					if (argumentLocations[i] == argumentLocations[j] && i != j)
						throw std::invalid_argument("An argument location was specified more than once");

					if (AreArgumentLocationsIncompatible(argumentLocations[i], argumentLocations[j]))
						throw std::invalid_argument("Invalid combination of argument locations used");
				}
			}
		}
	};

	template<typename ReturnType, typename... ArgumentTypes>
	class Function
	{
	public:
		Function() : address(0x0), signature(FunctionSignature<ReturnType, ArgumentTypes...>())
		{
		}

		Function(const uintptr_t address, const FunctionSignature<ReturnType, ArgumentTypes...> signature) : address(address), signature(signature) {}

		uintptr_t GetAddress() const { return address; }

		FunctionSignature<ReturnType, ArgumentTypes...> GetSignature() { return signature; };

		// TODO: This does not support doubles currently
		ReturnType Call(ArgumentTypes... arguments)
		{
			constexpr uint32_t argumentCount = sizeof...(arguments);
			std::array<std::uint32_t, argumentCount> integerArguments = { *(std::uint32_t*)&arguments... };
			std::array<std::any, argumentCount> anyArguments = { arguments... };


			std::vector<int32_t> stackArgumentIndices = signature.GetStackArgumentIndices();

			// Prepare FPU Stack Arguments

			std::vector<int32_t> fpuStackArgumentIndices;
			std::uint32_t fpuStackArgumentsReadIndex = 0;
			auto iterator = stackArgumentIndices.begin();
			while (iterator != stackArgumentIndices.end())
			{
				if (anyArguments[*iterator].type() == typeid(float))
				{
					fpuStackArgumentIndices.insert(fpuStackArgumentIndices.begin(), *iterator);
					iterator = stackArgumentIndices.erase(iterator);
				}
				else
				{
					++iterator;
				}
			}

			auto GetValueForFPURegister = [&](Location location) -> float
			{
				if (signature.HasArgumentInRegister(location))
					return *(float*)&integerArguments[signature.GetArgumentIndexForRegister(location)];
				else
					return fpuStackArgumentsReadIndex < fpuStackArgumentIndices.size() ? * (float*)&integerArguments[fpuStackArgumentIndices[fpuStackArgumentsReadIndex++]] : 0;
			};

			// Prepare Stack Arguments

			uint32_t stackArgumentCount = stackArgumentIndices.size();
			auto byteSizeOfStackArguments = stackArgumentCount * 4;

			auto argumentCleanupByteSize = signature.GetCallingConvention().SpecifiesCallerCleanup() ? byteSizeOfStackArguments : 0;

			auto stackArguments = (uint8_t*)malloc(byteSizeOfStackArguments);
			uint32_t stackArgumentsWriteIndex = 0;
			for (auto index : stackArgumentIndices)
			{
				*(uint32_t*)(stackArguments + stackArgumentsWriteIndex) = integerArguments[index];
				stackArgumentsWriteIndex += 4;
			}

			// Prepare Register Values

			auto GetValueForRegister = [&](Location location) -> uint32_t
			{
				if (signature.HasArgumentInRegister(location))
					return integerArguments[signature.GetArgumentIndexForRegister(location)];
				else
					return 0;
			};

			uint32_t eaxValue = GetValueForRegister(Location::EAX) | GetValueForRegister(Location::AL) | GetValueForRegister(Location::AH) << 8;
			uint32_t ebxValue = GetValueForRegister(Location::EBX) | GetValueForRegister(Location::BL) | GetValueForRegister(Location::BH) << 8;
			uint32_t ecxValue = GetValueForRegister(Location::ECX) | GetValueForRegister(Location::CL) | GetValueForRegister(Location::CH) << 8;
			uint32_t edxValue = GetValueForRegister(Location::EDX) | GetValueForRegister(Location::DL) | GetValueForRegister(Location::DH) << 8;
			uint32_t esiValue = GetValueForRegister(Location::ESI) | GetValueForRegister(Location::SIL);
			uint32_t ediValue = GetValueForRegister(Location::EDI) | GetValueForRegister(Location::DIL);

			float st0Value = GetValueForFPURegister(Location::ST0);
			float st1Value = GetValueForFPURegister(Location::ST1);
			float st2Value = GetValueForFPURegister(Location::ST2);
			float st3Value = GetValueForFPURegister(Location::ST3);
			float st4Value = GetValueForFPURegister(Location::ST4);
			float st5Value = GetValueForFPURegister(Location::ST5);
			float st6Value = GetValueForFPURegister(Location::ST6);
			float st7Value = GetValueForFPURegister(Location::ST7);

			auto stackArgumentsPointer = (uintptr_t)stackArguments;
			auto functionAddress = this->address;
			__asm {
				pushad

				sub esp, byteSizeOfStackArguments
				mov ecx, stackArgumentCount
				mov esi, stackArgumentsPointer
				mov edi, esp
				rep movsd

				mov eax, eaxValue
				mov ebx, ebxValue
				mov ecx, ecxValue
				mov edx, edxValue
				mov esi, esiValue
				mov edi, ediValue
				fld st7Value
				fld st6Value
				fld st5Value
				fld st4Value
				fld st3Value
				fld st2Value
				fld st1Value
				fld st0Value

				call functionAddress
				add esp, argumentCleanupByteSize

				mov eaxValue, eax
				mov ebxValue, ebx
				mov ecxValue, ecx
				mov edxValue, edx
				mov esiValue, esi
				mov ediValue, edi
				fstp st0Value
				fstp st1Value
				fstp st2Value
				fstp st3Value
				fstp st4Value
				fstp st5Value
				fstp st6Value
				fstp st7Value

				popad
			}

			free(stackArguments);

			switch(signature.GetReturnValueLocation())
			{
			case Location::EAX:
				return *(ReturnType*)&eaxValue;
			case Location::EBX:
				return *(ReturnType*)&ebxValue;
			case Location::ECX:
				return *(ReturnType*)&ecxValue;
			case Location::EDX:
				return *(ReturnType*)&edxValue;
			case Location::ESI:
				return *(ReturnType*)&esiValue;
			case Location::EDI:
				return *(ReturnType*)&ediValue;

			case Location::AL:
				return (ReturnType)Utils::GetLowByte(eaxValue);
			case Location::AH:
				return (ReturnType)Utils::GetHighByte(eaxValue);
			case Location::BL:
				return (ReturnType)Utils::GetLowByte(ebxValue);
			case Location::BH:
				return (ReturnType)Utils::GetHighByte(ebxValue);
			case Location::CL:
				return (ReturnType)Utils::GetLowByte(ecxValue);
			case Location::CH:
				return (ReturnType)Utils::GetHighByte(ecxValue);
			case Location::DL:
				return (ReturnType)Utils::GetLowByte(edxValue);
			case Location::DH:
				return (ReturnType)Utils::GetHighByte(edxValue);
			case Location::SIL:
				return (ReturnType)Utils::GetLowByte(esiValue);
			case Location::DIL:
				return (ReturnType)Utils::GetLowByte(ediValue);

			case Location::ST0:
				return (ReturnType)st0Value;
			case Location::ST1:
				return (ReturnType)st1Value;
			case Location::ST2:
				return (ReturnType)st2Value;
			case Location::ST3:
				return (ReturnType)st3Value;
			case Location::ST4:
				return (ReturnType)st4Value;
			case Location::ST5:
				return (ReturnType)st5Value;
			case Location::ST6:
				return (ReturnType)st6Value;
			case Location::ST7:
				return (ReturnType)st7Value;

			case Location::Stack:
				throw std::exception("Return value cannot be on stack");
			default:
				throw std::exception("Not implemented");
			}
		}
	private:
		uintptr_t address;
		FunctionSignature<ReturnType, ArgumentTypes...> signature;
	};
	
	template<typename ReturnType, typename... ArgumentTypes>
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
			Function<ReturnType, ArgumentTypes...> trampolineFunction(trampolineAddress, originalFunction.GetSignature());
			return trampolineFunction.Call(arguments...);
		}

		Hook() : isInitialized(false), isInstalled(false), opCodeSize(0), originalFunction(Function<ReturnType, ArgumentTypes...>()), userHookFunctionAddress(0),
		         trampolineAddress(0),
		         hookWrapperAddress(0),
		         tempStorage{}
		{
		}

		Hook(Function<ReturnType, ArgumentTypes...> originalFunction, uintptr_t hookFunctionAddress, const uint8_t opCodeSize)
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
		Function<ReturnType, ArgumentTypes...> originalFunction;
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

			const uint32_t stackArgumentCount = originalFunction.GetSignature().GetStackArgumentIndices().size();
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
			auto argumentLocations = originalFunction.GetSignature().GetArgumentLocations();
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
			auto returnValueLocation = CallingConvention(CallingConvention::Convention::Cdecl).GetReturnValueLocation(std::is_floating_point<ReturnType>());
			if (returnValueLocation == Location::EAX)
			{
				switch (originalFunction.GetSignature().GetReturnValueLocation())
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

				switch (originalFunction.GetSignature().GetReturnValueLocation())
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