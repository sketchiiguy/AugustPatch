#pragma once
#include "common.h"

bool (*ConnectUsingSettings_O)(void*, void*, void*);
bool ConnectUsingSettings_H(void* self, void* settings, void* mi) {
	char realtimeId[128] = {}, chatId[128] = {}, voiceId[128] = {};
	__try {
		if (settings) {
			void* s1 = *reinterpret_cast<void**>((uintptr_t)settings + 0x10);
			void* s2 = *reinterpret_cast<void**>((uintptr_t)settings + 0x18);
			void* s3 = *reinterpret_cast<void**>((uintptr_t)settings + 0x20);
			ReadIl2CppString(reinterpret_cast<Il2cppString*>(s1), realtimeId, sizeof realtimeId);
			ReadIl2CppString(reinterpret_cast<Il2cppString*>(s2), chatId, sizeof chatId);
			ReadIl2CppString(reinterpret_cast<Il2cppString*>(s3), voiceId, sizeof voiceId);
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	const uintptr_t callerRva = (uintptr_t)_ReturnAddress() - GA;
	PatchLog("[ConnGate] ConnectUsingSettings(realtime='%s' chat='%s' voice='%s') called from GA+0x%llX",
		realtimeId, chatId, voiceId, (unsigned long long)callerRva);
	bool result = false;
	__try { if (ConnectUsingSettings_O) result = ConnectUsingSettings_O(self, settings, mi); }
	__except (EXCEPTION_EXECUTE_HANDLER) { result = false; }
	PatchLog("[ConnGate] ConnectUsingSettings -> %s", result ? "true" : "false");
	return result;
}

void (*PeerDisconnect_O)(void*, void*);
void PeerDisconnect_H(void* self, void* mi) {
	const uintptr_t callerRva = (uintptr_t)_ReturnAddress() - GA;
	PatchLog("[ConnGate] PhotonPeer.Disconnect() called from GA+0x%llX", (unsigned long long)callerRva);
	if (PeerDisconnect_O) PeerDisconnect_O(self, mi);
}

void (*OnStatusChanged_O)(void*, int32_t, void*);
void OnStatusChanged_H(void* self, int32_t statusCode, void* mi) {
	PatchLog("[ConnGate] OnStatusChanged(code=%d)", statusCode);
	if (OnStatusChanged_O) OnStatusChanged_O(self, statusCode, mi);
}

void (*DebugReturn_O)(void*, int32_t, Il2cppString*, void*);
void DebugReturn_H(void* self, int32_t level, Il2cppString* msg, void* mi) {
	char text[512] = {};
	bool ok = false;
	__try { ok = ReadIl2CppString(msg, text, sizeof text); } __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
	PatchLog("[ConnGate] DebugReturn(level=%d): %s", level, ok ? text : "<unreadable>");
	if (DebugReturn_O) DebugReturn_O(self, level, msg, mi);
}

void (*OnOperationResponse_O)(void*, void*, void*);
void OnOperationResponse_H(void* self, void* response, void* mi) {
	uint8_t opCode = 0; int16_t returnCode = 0; char debugMsg[256] = {};
	bool haveDebug = false;
	__try {
		if (response) {
			opCode = *reinterpret_cast<uint8_t*>((uintptr_t)response + 0x10);
			returnCode = *reinterpret_cast<int16_t*>((uintptr_t)response + 0x12);
			void* dbg = *reinterpret_cast<void**>((uintptr_t)response + 0x18);
			if (dbg) haveDebug = ReadIl2CppString(reinterpret_cast<Il2cppString*>(dbg), debugMsg, sizeof debugMsg);
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	PatchLog("[ConnGate] OperationResponse(opCode=%u returnCode=%d debug='%s')",
		opCode, (int)returnCode, haveDebug ? debugMsg : "");
	if (OnOperationResponse_O) OnOperationResponse_O(self, response, mi);
}

void (*ClientDisconnect_O)(void*, int32_t, void*);
void ClientDisconnect_H(void* self, int32_t cause, void* mi) {
	void* frames[32] = {};
	const USHORT n = RtlCaptureStackBackTrace(0, 32, frames, nullptr);
	const uintptr_t lo = GA, hi = GA + (NtHeaders ? NtHeaders->OptionalHeader.SizeOfImage : 0);
	char stack[900] = {};
	int used = 0;
	for (USHORT i = 0; i < n && used >= 0 && (size_t)used < sizeof stack - 24; ++i) {
		const uintptr_t a = (uintptr_t)frames[i];
		if (a < lo || a >= hi) continue;
		const int w = sprintf_s(stack + used, sizeof stack - used, " 0x%llX", (unsigned long long)(a - lo));
		if (w < 0) break;
		used += w;
	}
	PatchLog("[Net] LoadBalancingClient.Disconnect(cause=%d) GA stack:%s", cause, stack);
	if (ClientDisconnect_O) ClientDisconnect_O(self, cause, mi);
}
