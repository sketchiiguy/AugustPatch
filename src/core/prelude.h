#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>
#include <cstdint>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdarg>
#include <share.h>
#include <new>
#include <string>

#include "Console.h"

inline void PatchLog(const char* fmt, ...) {
	static FILE* lf = nullptr;
	if (!lf) {
		char path[MAX_PATH];
		GetModuleFileNameA(nullptr, path, MAX_PATH);
		char* slash = strrchr(path, '\\');
		if (slash) strcpy_s(slash + 1, MAX_PATH - (slash + 1 - path), "2024augustpatch.log");

		lf = _fsopen(path, "w", _SH_DENYWR);
		if (!lf) {
			char* dot = strrchr(path, '.');
			if (dot) sprintf_s(dot, sizeof(path) - (dot - path), ".%lu.log", GetCurrentProcessId());
			lf = _fsopen(path, "w", _SH_DENYWR);
		}
		if (!lf) return;
	}

	char line[2048];
	SYSTEMTIME st; GetLocalTime(&st);
	int n = sprintf_s(line, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	if (n < 0) n = 0;

	va_list ap; va_start(ap, fmt);
	vsnprintf_s(line + n, sizeof(line) - n, _TRUNCATE, fmt, ap);
	va_end(ap);

	fputs(line, lf);
	fputc('\n', lf);
	fflush(lf);

	RR::Console::Log(line);
}

#include "../vendor/minhook/include/MinHook.h"

struct Il2cppString {
	int Length;
	wchar_t s[1024];
};
