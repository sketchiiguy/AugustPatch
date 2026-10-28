#pragma once
#include "common.h"

void CheatQuit_H(void* self, void* mi) {
	__try {
		Il2cppString* fn = self ? read<Il2cppString*>(self, 0x10) : nullptr;
		char s[1024];
		const bool ok = ReadIl2CppString(fn, s, sizeof s);
		PatchLog("[CheatMgr] module scan flagged \"%s\" -> FatalApplicationQuit SUPPRESSED", ok ? s : "<null>");
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}

}

void Detector_H(void* self, void* mi) {

	static int logged = 0;
	if (logged < 3) {
		++logged;
		PatchLog("[CheatMgr] detector fired (caller GA+0x%llX) -> suppressed",
			(unsigned long long)((uintptr_t)_ReturnAddress() - GA));
	}
	(void)self; (void)mi;
}

bool (*DuidMismatch_O)(void*, Il2cppString**, Il2cppString**, void*);
bool DuidMismatch_H(void* self, Il2cppString** storedA, Il2cppString** storedB, void* mi) {
	bool orig = false;
	Il2cppString* gotA = nullptr;
	Il2cppString* gotB = nullptr;
	__try { orig = DuidMismatch_O(self, &gotA, &gotB, mi); }
	__except (EXCEPTION_EXECUTE_HANDLER) { orig = false; gotA = gotB = nullptr; }

	static bool loggedOnce = false, loggedMismatch = false;
	if (!loggedOnce || (orig && !loggedMismatch)) {
		loggedOnce = true;
		if (orig) loggedMismatch = true;
		__try {
			char a[1024], b[1024];
			const bool okA = ReadIl2CppString(gotA, a, sizeof a);
			const bool okB = ReadIl2CppString(gotB, b, sizeof b);
			PatchLog("[DUID] CheatManager.CheckForDUIDMismatch original=%d storedA=\"%s\" storedB=\"%s\" -> %s",
				orig ? 1 : 0, okA ? a : "<null>", okB ? b : "<null>",
				RR::Config::SuppressDuidMismatch ? "forcing FALSE (suppressed)" : "passing through");
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
	}

	if (!RR::Config::SuppressDuidMismatch) {
		if (storedA) *storedA = gotA;
		if (storedB) *storedB = gotB;
		return orig;
	}
	if (storedA) *storedA = nullptr;
	if (storedB) *storedB = nullptr;
	return false;
}
