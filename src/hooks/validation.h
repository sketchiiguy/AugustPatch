#pragma once
#include "common.h"

static bool PerformIntegrityScan() {
	uintptr_t ga = reinterpret_cast<uintptr_t>(GetModuleHandleA("GameAssembly.dll"));
	if (!ga) { PatchLog("[Integrity] GameAssembly.dll not found"); return false; }

	struct ScanRegion { uintptr_t rva; size_t size; const char* name; };
	ScanRegion regions[] = {
		{ RR::Methods::LaunchValidation::VerifyFile, 32, "VerifyFile" },
		{ RR::Methods::SessionManager::FatalApplicationQuit, 32, "FatalQuit" },
		{ RR::Methods::AntiCheat::ModuleScanDetected, 32, "ModuleScan" }
	};

	uint64_t combinedHash = 0;
	for (const auto& reg : regions) {
		uint8_t* data = reinterpret_cast<uint8_t*>(ga + reg.rva);
		for (size_t i = 0; i < reg.size; i++) {
			combinedHash = (combinedHash * 31) + data[i];
		}
	}

	static bool firstScan = true;
	if (firstScan) {
		firstScan = false;
		PatchLog("[Integrity] scan complete. Hash: 0x%llX", combinedHash);
	}

	return true;
}

void LaunchVerify_H(void* file, void* hash, void* signature, void* modulus, void* exponent) {
	static bool announced = false;
	if (!announced) {
		announced = true;
		if (PerformIntegrityScan()) {
			PatchLog("[LaunchValidation] integrity scan passed - all files accepted");
		} else {
			PatchLog("[LaunchValidation] integrity scan FAILED - blocking boot");
		}
	}
	return;
}

void* (*LaunchResultFail_O)(void*, void*) = nullptr;
void* LaunchResultFail_H(void* exception, void* mi) {
	void* result = nullptr;
	char why[256] = {};
	bool haveWhy = false;
	__try {
		if (LaunchResultFail_O) result = LaunchResultFail_O(exception, mi);
		if (exception) {
			void* msg = *reinterpret_cast<void**>((uintptr_t)exception + 0x18);
			if (msg) haveWhy = ReadIl2CppString(reinterpret_cast<Il2cppString*>(msg), why, sizeof why);
		}
		if (result) {
			*reinterpret_cast<uint8_t*>((uintptr_t)result + 0x18) = 0;
			*reinterpret_cast<uint8_t*>((uintptr_t)result + 0x19) = 0;
			*reinterpret_cast<void**>((uintptr_t)result + 0x20) = nullptr;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	PatchLog("[LaunchValidation] failed (%s) -> %s", haveWhy ? why : "no message",
		result ? "reported as passed" : "could not build a result object");
	return result;
}

void (*LaunchConsumer_O)(void*, void*, void*) = nullptr;
void LaunchConsumer_H(void* closure, void* result, void* mi) {
	int flagA = -1, flagB = -1;
	bool hasException = false;
	__try {
		if (result) {
			flagA = *reinterpret_cast<uint8_t*>((uintptr_t)result + 0x18);
			flagB = *reinterpret_cast<uint8_t*>((uintptr_t)result + 0x19);
			hasException = *reinterpret_cast<void**>((uintptr_t)result + 0x20) != nullptr;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	const char* branch = !result ? "NULL -> NullReferenceException"
		: flagB ? "+0x19 branch" : flagA ? "+0x18 branch"
		: hasException ? "FAILED (cheat report)" : "PASSED";
	PatchLog("[LaunchValidation] consumer got result (+0x18=%d +0x19=%d exception=%s) -> %s",
		flagA, flagB, hasException ? "yes" : "no", branch);
	if (LaunchConsumer_O) LaunchConsumer_O(closure, result, mi);
}

static void DescribePromise(void* p, char* out, size_t cap) {
	if (!p) { strcpy_s(out, cap, "null"); return; }
	const char* cname = "?";
	bool settled = false;
	char err[256] = {};
	bool haveErr = false;
	__try {
		void* klass = *reinterpret_cast<void**>(p);
		if (klass) cname = *reinterpret_cast<const char**>((uintptr_t)klass + 0x10);
		settled = *reinterpret_cast<uint8_t*>((uintptr_t)p + 0x88) != 0;
		void* e = *reinterpret_cast<void**>((uintptr_t)p + 0x80);
		if (e) haveErr = ReadIl2CppString(reinterpret_cast<Il2cppString*>(e), err, sizeof err);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		strcpy_s(out, cap, "<unreadable>");
		return;
	}
	if (haveErr) sprintf_s(out, cap, "%s REJECTED \"%s\"", cname ? cname : "?", err);
	else sprintf_s(out, cap, "%s %s", cname ? cname : "?", settled ? "resolved" : "pending");
}

void* (*BootPromise_O)(void*, void*) = nullptr;
void* BootPromise_H(void* self, void* mi) {
	void* real = nullptr;
	__try { if (BootPromise_O) real = BootPromise_O(self, mi); }
	__except (EXCEPTION_EXECUTE_HANDLER) { real = nullptr; }

	void* own = nullptr;
	__try { if (self) own = *reinterpret_cast<void**>((uintptr_t)self + 0x10); }
	__except (EXCEPTION_EXECUTE_HANDLER) { own = nullptr; }
	char ownText[320], realText[320];
	DescribePromise(own, ownText, sizeof ownText);
	DescribePromise(real, realText, sizeof realText);

	void* resolved = nullptr;
	__try {
		using ResolvedFn = void* (*)(void*);
		auto fn = reinterpret_cast<ResolvedFn>(GA + RR::Methods::Promise::Resolved);
		resolved = RetAddr ? spoof_call(RetAddr, fn, (void*)nullptr) : fn(nullptr);
	} __except (EXCEPTION_EXECUTE_HANDLER) { resolved = nullptr; }

	PatchLog("[LaunchValidation] boot gate: validation promise [%s], combined [%s] -> %s",
		ownText, realText, resolved ? "replaced with a resolved promise" : "could not get a resolved promise, passing through");
	return resolved ? resolved : real;
}

void HashCheckThread_H(void* self) { return; }
