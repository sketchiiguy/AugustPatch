#pragma once
#include "prelude.h"

inline uintptr_t GA = (uintptr_t)GetModuleHandleA("GameAssembly.dll");
inline const IMAGE_NT_HEADERS* const NtHeaders = GA
? reinterpret_cast<IMAGE_NT_HEADERS*>(GA + reinterpret_cast<IMAGE_DOS_HEADER*>(GA)->e_lfanew)
    : nullptr;

inline uint64_t* RetAddr = nullptr;

template <typename T>
T read(void* obj, std::ptrdiff_t offset) {
    return *(T*)((std::uint8_t*)obj + offset);
}

template <typename T>
void set(void* obj, std::ptrdiff_t offset, T val) {
    *(T*)((std::uint8_t*)obj + offset) = val;
}

inline bool ReadIl2CppString(Il2cppString* il2cppString, char* out, size_t cch, bool* truncated = nullptr) {
    if (truncated) *truncated = false;
    if (!out || !cch) return false;
    out[0] = '\0';
    if (il2cppString == nullptr) return false;

    size_t convertedChars = 0;
    errno_t result = wcstombs_s(&convertedChars, out, cch, reinterpret_cast<wchar_t*>(il2cppString) + 0xA, _TRUNCATE);

    if (result == STRUNCATE) {
        if (truncated) *truncated = true;
        return true;
    }
    if (result != 0) {
        out[0] = '\0';
        return false;
    }
    return true;
}
