#pragma once
#include "common.h"

Il2cppString* (*EACChallenge_O)(Il2cppString*, void*);
Il2cppString* EACChallenge_H(Il2cppString* challenge, void* mi) {
	Il2cppString* real = nullptr;
	__try {
		if (EACChallenge_O) real = EACChallenge_O(challenge, mi);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { real = nullptr; }

	int32_t len = 0;
	if (real) {
		__try { len = read<int32_t>(real, 0x10); }
		__except (EXCEPTION_EXECUTE_HANDLER) { len = 0; }
	}
	if (len > 0) return real;

	if (!RR::Config::DisableAntiCheat) {
		PatchLog("[EAC] GenerateChallengeResponse returned nothing (EAC runtime not loaded) -- "
			"not faking it (DisableAntiCheat=false)");
		return real;
	}

	static char b64[173] = {};
	if (!b64[0]) {
		memset(b64, 'A', 171);
		b64[171] = '=';
		b64[172] = '\0';
	}

	Il2cppString* fake = nullptr;
	__try {
		using NewFn = Il2cppString * (*)(const char*, void*);
		auto fn = reinterpret_cast<NewFn>(GA + RR::Methods::Il2cpp::il2cpp_string_new);
		fake = RetAddr ? spoof_call(RetAddr, fn, (const char*)b64, (void*)nullptr) : fn(b64, nullptr);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { fake = nullptr; }

	PatchLog("[EAC] GenerateChallengeResponse returned nothing (EAC runtime not loaded) -> %s",
		fake ? "dummy 128-byte response, login request can now be sent"
		: "could not build dummy response, login will fail as before");
	return fake;
}

void (*EACUpdate_O)(void*, void*);
void EACUpdate_H(void* self, void* mi) {
	__try {
		if (EACUpdate_O) EACUpdate_O(self, mi);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		static bool logged = false;
		if (!logged) { logged = true; PatchLog("[EAC] Update() threw 0x%08X -- skipped", GetExceptionCode()); }
	}
}
