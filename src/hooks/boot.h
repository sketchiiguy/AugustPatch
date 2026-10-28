#pragma once
#include "common.h"

void BootError_H(void* self, Il2cppString* message, void* mi) {
	char text[512] = {};
	bool ok = false;
	__try { ok = ReadIl2CppString(message, text, sizeof text); } __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
	PatchLog("[Boot] ERROR suppressed: \"%s\" (from GA+0x%llX)", ok ? text : "<unreadable>",
		(unsigned long long)((uintptr_t)_ReturnAddress() - GA));
}

static const char* BootStateName(int32_t s) {
	switch (s) {
	case -1: return "INVALID";
	case 0: return "APP_SETUP";
	case 1: return "PRE_INITIALIZATION";
	case 2: return "LOGIN";
	case 3: return "POST_LOGIN_RECNET_OPERATIONS";
	case 4: return "CALIBRATION";
	case 5: return "POST_LOGIN_INITIALIZATION";
	case 100: return "LOAD_INITIAL_SCENE";
	case 101: return "POST_LOAD_INITIAL_SCENE";
	case 1000: return "ERROR";
	default: return "?";
	}
}
void (*BootState_O)(void*, int32_t, void*) = nullptr;
void BootState_H(void* self, int32_t state, void* mi) {
	PatchLog("[Boot] state -> %s (%d)", BootStateName(state), state);
	if (BootState_O) BootState_O(self, state, mi);
}
