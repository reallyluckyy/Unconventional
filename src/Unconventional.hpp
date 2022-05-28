#pragma once

#include <stdexcept>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <array>
#include <vector>
#include <any>

namespace Unconventional
{
	enum class CallingConvention
	{
		CDECL,
		// TODO: Support more calling conventions
	};

	enum class Location
	{
		STACK,
		EAX, EBX, ECX, EDX, ESI, EDI,
		AH, AL, BH, BL, CH, CL, DH, DL, SIL, DIL,
		ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7

		// TODO: Support 16-bit registers
		// TODO: Support XMM registers
	};

	template<typename ReturnType, typename... ArgumentTypes>
	class FunctionSignature
	{
	public:
		FunctionSignature(const CallingConvention callingConvention = CallingConvention::CDECL) :
			callingConvention(callingConvention), returnValueLocation(), argumentLocations(std::array<Location, sizeof...(ArgumentTypes)>())
		{
		}

		FunctionSignature(Location returnValueLocation, const CallingConvention callingConvention = CallingConvention::CDECL)
			: callingConvention(callingConvention), returnValueLocation(returnValueLocation), argumentLocations(std::array<Location, sizeof...(ArgumentTypes)>())
		{
		}

		FunctionSignature(Location returnValueLocation, std::array<Location, sizeof...(ArgumentTypes)> argumentLocations, const CallingConvention callingConvention = CallingConvention::CDECL)
			: callingConvention(callingConvention), returnValueLocation(returnValueLocation), argumentLocations(argumentLocations)
		{
			VerifyReturnValueLocation();
			VerifyArgumentLocations();
		}

		FunctionSignature(std::array<Location, sizeof...(ArgumentTypes)> argumentLocations, const CallingConvention callingConvention = CallingConvention::CDECL)
			: callingConvention(callingConvention), returnValueLocation(), argumentLocations(argumentLocations)
		{
			VerifyReturnValueLocation();
			VerifyArgumentLocations();
		}

		Location GetReturnValueLocation() const
		{
			if (returnValueLocation.has_value())
				return returnValueLocation.value();
			return GetCallingConventionReturnValueLocation();
		}

		bool ConventionSpecifiesCallerCleanup() const
		{
			switch(callingConvention)
			{
			case CallingConvention::CDECL:
				return true;
			}

			return false;
		}

		std::vector<int32_t> GetStackArgumentIndices()
		{
			std::vector<int32_t> indices;
			for (int32_t i = 0; i < (int32_t)sizeof...(ArgumentTypes); i++)
			{
				if (argumentLocations[i] == Location::STACK)
				{
					indices.push_back(i);
				}
			}
			return indices;
		}

		int32_t GetArgumentIndexForRegister(Location location)
		{
			if (location == Location::STACK)
				throw std::invalid_argument("Location passed to GetArgumentIndexForRegister was Location::STACK");

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
			if (location == Location::STACK)
				throw std::invalid_argument("Location passed to HasArgumentInRegister was Location::STACK");

			return this->GetArgumentIndexForRegister(location) != -1;
		}

	private:
		CallingConvention callingConvention;
		std::optional<Location> returnValueLocation;
		std::array<Location, sizeof...(ArgumentTypes)> argumentLocations;

		Location GetCallingConventionReturnValueLocation() const
		{
			switch (callingConvention)
			{
			case CallingConvention::CDECL:
				if (std::is_floating_point<ReturnType>())
					return Location::ST0;
				else
					return Location::EAX;
			default:
				throw std::exception("Not implemented");
			}
		}

		void VerifyReturnValueLocation()
		{
			if (returnValueLocation.has_value() && returnValueLocation == Location::STACK)
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
		Function(const uintptr_t address, const FunctionSignature<ReturnType, ArgumentTypes...> signature) : address(address), signature(signature) {}

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

			auto argumentCleanupByteSize = signature.ConventionSpecifiesCallerCleanup() ? byteSizeOfStackArguments : 0;

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
				return (ReturnType)GetLowByte(eaxValue);
			case Location::AH:
				return (ReturnType)GetHighByte(eaxValue);
			case Location::BL:
				return (ReturnType)GetLowByte(ebxValue);
			case Location::BH:
				return (ReturnType)GetHighByte(ebxValue);
			case Location::CL:
				return (ReturnType)GetLowByte(ecxValue);
			case Location::CH:
				return (ReturnType)GetHighByte(ecxValue);
			case Location::DL:
				return (ReturnType)GetLowByte(edxValue);
			case Location::DH:
				return (ReturnType)GetHighByte(edxValue);
			case Location::SIL:
				return (ReturnType)GetLowByte(esiValue);
			case Location::DIL:
				return (ReturnType)GetLowByte(ediValue);

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

			case Location::STACK:
				throw std::exception("Return value cannot be on stack");
			default:
				throw std::exception("Not implemented");
			}
		}
	private:
		uintptr_t address;
		FunctionSignature<ReturnType, ArgumentTypes...> signature;

		static uint8_t GetLowByte(uint32_t x)
		{
			return x & 0xFF;
		}

		static uint8_t GetHighByte(uint32_t x)
		{
			return (x >> 8) & 0xFF;
		}
	};
}