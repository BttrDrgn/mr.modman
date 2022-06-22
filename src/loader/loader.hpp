#pragma once

#include <Windows.h>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <fstream>
#include <functional>

//Deps
#include "MinHook.h"

namespace loader
{
    void load(const char* bin_name);

    template <typename T> inline std::function<T> call(std::uintptr_t callback)
    {
        return std::function<T>(reinterpret_cast<T*>(callback));
    }

    inline int run(std::uint32_t address)
    {
        return loader::call<int()>(address)();
    }

    inline auto replace(std::uint32_t address, void* function) -> void
    {
        *reinterpret_cast<std::uint8_t*>(address) = 0xE9;
        *reinterpret_cast<std::uint32_t*>(address + 1) = ((std::uint32_t)function - address - 5);
    }
}
