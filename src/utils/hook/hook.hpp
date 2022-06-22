#pragma once

class hook
{
public:
	template <typename T> static void jump(std::uint32_t address, T function)
	{
		*(std::uint8_t*)(address) = 0xE9;
		*(std::uint32_t*)(address + 1) = (std::uint32_t(function) - address - 5);
	};

	template <typename T> static void retn_value(std::uint32_t address, T value)
	{
		*(std::uint8_t*)(address) = 0xB8;
		*(std::uint32_t*)(address + 1) = std::uint32_t(value);
		*(std::uint8_t*)(address + 5) = 0xC3;
	}

	template <typename T> static void set(std::uint32_t address, T value)
	{
		*reinterpret_cast<T*>(address) = value;
	}

	static void retn(std::uint32_t address)
	{
		*(std::uint8_t*)(address) = 0xC3;
	};
};
