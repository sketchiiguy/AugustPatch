#pragma once
#include "common.h"

inline bool (*ImageVerify_O)(void*, void*, void*) = nullptr;
inline bool ImageVerify_H(void* data, void* sig, void* mi) {
	bool real = true;
	if (ImageVerify_O) {
		__try { real = ImageVerify_O(data, sig, mi); }
		__except (EXCEPTION_EXECUTE_HANDLER) { real = false; }
	}
	static int shown = 0;
	if (!real && shown < 5) { ++shown; PatchLog("[IMAGES] signature check failed -> forced TRUE"); }
	return true;
}

inline bool CertValidate_H(void*, void*, void*, void*) { return true; }

inline void CertReport_H(int reason, Il2cppString* detail, void*  ) {
	static int shown = 0;
	if (shown < 5) {
		++shown;
		char buf[256];
		const bool ok = ReadIl2CppString(detail, buf, sizeof buf);
		PatchLog("[CertPin] report reason=%d detail=\"%s\" -> SUPPRESSED", reason, ok ? buf : "");
	}
}

inline void* (*CheatReport_O)(int, Il2cppString*, uint64_t, bool, void*) = nullptr;
inline void* CheatReport_H(int reason, Il2cppString* detail, uint64_t nullableInt, bool flag, void* mi) {
	char buf[256];
	const bool ok = ReadIl2CppString(detail, buf, sizeof buf);
	PatchLog("[Report] cheat report reason=%d detail=\"%s\" caller GA+0x%llX", reason, ok ? buf : "",
		(unsigned long long)((uintptr_t)_ReturnAddress() - GA));
	return CheatReport_O ? CheatReport_O(reason, detail, nullableInt, flag, mi) : nullptr;
}
