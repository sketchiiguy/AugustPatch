#pragma once
#include "common.h"

constexpr int32_t kCheatQuitCodeModuleScan = 533223478;
constexpr int32_t kCheatQuitCodeReporter = 828725391;

static bool QuitDetailText(void* detail, char* out, size_t cch) {
	if (out && cch) out[0] = '\0';
	if (!detail || !out || !cch) return false;
	__try {
		return ReadIl2CppString(reinterpret_cast<Il2cppString*>(detail), out, cch);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { out[0] = '\0'; return false; }
}

void (*FatalQuit_O)(int32_t, Il2cppString*, void*);
void FatalQuit_H(int32_t code, Il2cppString* detail, void* mi) {
	char text[512];
	const bool readOk = QuitDetailText(detail, text, sizeof(text));

	const bool hasMessage = readOk && text[0] != '\0';
	const uintptr_t rva = (uintptr_t)_ReturnAddress() - GA;

	const bool knownCheat = code == kCheatQuitCodeModuleScan || code == kCheatQuitCodeReporter;
	const bool suppress = RR::Config::DisableAntiCheat && (knownCheat || !hasMessage);

	PatchLog("[Quit] FatalApplicationQuit(code=%d, detail=\"%s\", caller GA+0x%llX) -> %s",
		code, readOk ? text : "<unreadable>", (unsigned long long)rva,
		suppress ? "SUPPRESSED" : "passed through");

	if (suppress) return;
	if (FatalQuit_O) FatalQuit_O(code, detail, mi);
}

void (*TryQuit_O)(int32_t, void*);
void TryQuit_H(int32_t code, void* mi) {
	const uintptr_t rva = (uintptr_t)_ReturnAddress() - GA;
	const bool knownCheat = code == kCheatQuitCodeModuleScan || code == kCheatQuitCodeReporter;
	PatchLog("[Quit] TryApplicationQuit(code=%d, caller GA+0x%llX) -> %s",
		code, (unsigned long long)rva, knownCheat ? "SUPPRESSED" : "passed through");
	if (RR::Config::DisableAntiCheat && knownCheat) return;
	if (TryQuit_O) TryQuit_O(code, mi);
}

bool (*GetIsQuitting_O)(void*);
bool GetIsQuitting_H(void* mi) {
	bool orig = false;
	__try { if (GetIsQuitting_O) orig = GetIsQuitting_O(mi); } __except (EXCEPTION_EXECUTE_HANDLER) { orig = false; }
	static int logged = 0;
	if (orig && logged < 5) { ++logged; PatchLog("[Quit] get_IsQuitting() original=true -> %s", RR::Config::DisableAntiCheat ? "forcing FALSE" : "passing through"); }
	return RR::Config::DisableAntiCheat ? false : orig;
}
bool (*GetIsQuittingOrLoggingOut_O)(void*);
bool GetIsQuittingOrLoggingOut_H(void* mi) {
	bool orig = false;
	__try { if (GetIsQuittingOrLoggingOut_O) orig = GetIsQuittingOrLoggingOut_O(mi); } __except (EXCEPTION_EXECUTE_HANDLER) { orig = false; }
	static int logged = 0;
	if (orig && logged < 5) { ++logged; PatchLog("[Quit] get_IsQuittingOrLoggingOut() original=true -> %s", RR::Config::DisableAntiCheat ? "forcing FALSE" : "passing through"); }
	return RR::Config::DisableAntiCheat ? false : orig;
}
bool (*GetIsLoadingOrQuitting_O)(void*);
bool GetIsLoadingOrQuitting_H(void* mi) {
	bool orig = false;
	__try { if (GetIsLoadingOrQuitting_O) orig = GetIsLoadingOrQuitting_O(mi); } __except (EXCEPTION_EXECUTE_HANDLER) { orig = false; }
	static int logged = 0;
	if (orig && logged < 5) { ++logged; PatchLog("[Quit] get_IsLoadingOrQuitting() original=true -> %s", RR::Config::DisableAntiCheat ? "forcing FALSE" : "passing through"); }
	return RR::Config::DisableAntiCheat ? false : orig;
}
