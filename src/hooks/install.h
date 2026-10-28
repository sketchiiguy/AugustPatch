#pragma once
#include "tls.h"
#include "http.h"
#include "anticheat.h"
#include "quit.h"
#include "eac.h"
#include "photon.h"
#include "validation.h"
#include "boot.h"
#include "integrity.h"
#include "dns.h"

namespace RR::Patches {

	static void InstallHook(const char* name, uintptr_t rva, void* detour, void** original) {
		if (!GA || !rva) {
			PatchLog("[Patch] %s SKIPPED (GA=%p rva=0x%llX)", name, (void*)GA, (unsigned long long)rva);
			return;
		}
		void* target = reinterpret_cast<void*>(GA + rva);
		MH_STATUS st = MH_CreateHook(target, detour, original);
		if (st == MH_OK) st = MH_QueueEnableHook(target);
		if (st != MH_OK)
			PatchLog("[Patch] %s FAILED at GA+0x%llX (MH_STATUS %d)", name, (unsigned long long)rva, (int)st);
		else
			PatchLog("[Patch] %s hooked at GA+0x%llX", name, (unsigned long long)rva);
	}

	void Resolve() {
		if (!GA || !NtHeaders) {
			PatchLog("[Resolve] GameAssembly.dll not loaded (GA=%p) -- nothing to patch", (void*)GA);
			return;
		}

		RetAddr = PatternScan<uint64_t*>("FF 23", GA, NtHeaders->OptionalHeader.SizeOfImage);
		if (!RetAddr)
			PatchLog("[Resolve] WARNING: return-address gadget (FF 23) not found -- API rewrite will be skipped");

		auto assm = GetModuleHandleA("GameAssembly.dll");

		auto getExport = [assm](const char* name) -> uintptr_t {
			if (RetAddr) return reinterpret_cast<uintptr_t>(spoof_call(RetAddr, GetProcAddress, assm, name));
			return reinterpret_cast<uintptr_t>(GetProcAddress(assm, name));
			};

		RR::Methods::Il2cpp::il2cpp_string_new = getExport("il2cpp_string_new") - GA;
		RR::Methods::Il2cpp::il2cpp_object_get_class = getExport("il2cpp_object_get_class") - GA;
		RR::Methods::Il2cpp::il2cpp_object_new = getExport("il2cpp_object_new") - GA;
		RR::Methods::Il2cpp::il2cpp_runtime_class_init = getExport("il2cpp_runtime_class_init") - GA;

		PatchLog("[Resolve] RetAddr=%p il2cpp_string_new=0x%llX il2cpp_object_get_class=0x%llX il2cpp_object_new=0x%llX il2cpp_runtime_class_init=0x%llX",
			(void*)RetAddr,
			(unsigned long long)RR::Methods::Il2cpp::il2cpp_string_new,
			(unsigned long long)RR::Methods::Il2cpp::il2cpp_object_get_class,
			(unsigned long long)RR::Methods::Il2cpp::il2cpp_object_new,
			(unsigned long long)RR::Methods::Il2cpp::il2cpp_runtime_class_init);
	}

	void Patch() {
		if (!GA) {
			PatchLog("[Patch] skipping -- GameAssembly.dll not loaded (GA=%p)", (void*)GA);
			return;
		}

		MH_STATUS init = MH_Initialize();
		if (init != MH_OK) {
			PatchLog("[Patch] MH_Initialize FAILED (MH_STATUS %d) -- no hooks installed", (int)init);
			return;
		}

		InstallHook("LegacyTlsAuthentication.NotifyServerCertificate",
			RR::Methods::LegacyTlsAuthentication::NotifyServerCertificate, &nop_H, (void**)&nop_O);

		InstallHook("HTTPManager.SendRequest",
			RR::Methods::HTTPRequest::SendRequest, &SendRequest_H, (void**)&SendRequest_O);

		InstallHook("CheatManager.ModuleScanDetected",
			RR::Methods::AntiCheat::ModuleScanDetected, &CheatQuit_H, nullptr);

		InstallHook("CheatManager.OnTimeCheatDetected",
			RR::Methods::AntiCheat::OnTimeCheatDetected, &Detector_H, nullptr);
		InstallHook("CheatManager.OnObscuredTypeCheatDetected",
			RR::Methods::AntiCheat::OnObscuredTypeCheatDetected, &Detector_H, nullptr);
		InstallHook("CheatManager.OnAdvancedMovementCheatDetected",
			RR::Methods::AntiCheat::OnAdvancedMovementCheatDetected, &Detector_H, nullptr);

		InstallHook("SessionManager.FatalApplicationQuit",
			RR::Methods::SessionManager::FatalApplicationQuit, &FatalQuit_H, (void**)&FatalQuit_O);
		InstallHook("SessionManager.TryApplicationQuit",
			RR::Methods::SessionManager::TryApplicationQuit, &TryQuit_H, (void**)&TryQuit_O);

		InstallHook("SessionManager.get_IsQuitting",
			RR::Methods::SessionManager::get_IsQuitting, &GetIsQuitting_H, (void**)&GetIsQuitting_O);
		InstallHook("SessionManager.get_IsQuittingOrLoggingOut",
			RR::Methods::SessionManager::get_IsQuittingOrLoggingOut, &GetIsQuittingOrLoggingOut_H, (void**)&GetIsQuittingOrLoggingOut_O);
		InstallHook("SessionManager.get_IsLoadingOrQuitting",
			RR::Methods::SessionManager::get_IsLoadingOrQuitting, &GetIsLoadingOrQuitting_H, (void**)&GetIsLoadingOrQuitting_O);

		InstallHook("CheatManager.CheckForDUIDMismatch",
			RR::Methods::AntiCheat::CheckForDUIDMismatch, &DuidMismatch_H, (void**)&DuidMismatch_O);

		InstallHook("EACManager.GenerateChallengeResponse",
			RR::Methods::EAC::GenerateChallengeResponse, &EACChallenge_H, (void**)&EACChallenge_O);

		InstallHook("EACManager.Update",
			RR::Methods::EAC::Update, &EACUpdate_H, (void**)&EACUpdate_O);

		InstallHook("PUNNetworkManager.ConnectUsingSettings",
			RR::Methods::ConnGate::ConnectUsingSettings, &ConnectUsingSettings_H, (void**)&ConnectUsingSettings_O);
		InstallHook("PhotonPeer.Disconnect",
			RR::Methods::ConnGate::PeerDisconnect, &PeerDisconnect_H, (void**)&PeerDisconnect_O);
		InstallHook("CHPIGCHHMNK.OnStatusChanged",
			RR::Methods::ConnGate::OnStatusChanged, &OnStatusChanged_H, (void**)&OnStatusChanged_O);
		InstallHook("CHPIGCHHMNK.DebugReturn",
			RR::Methods::ConnGate::DebugReturn, &DebugReturn_H, (void**)&DebugReturn_O);
		InstallHook("CHPIGCHHMNK.OnOperationResponse",
			RR::Methods::ConnGate::OnOperationResponse, &OnOperationResponse_H, (void**)&OnOperationResponse_O);

		InstallHook("LoadBalancingClient.Disconnect",
			RR::Methods::ConnGate::ClientDisconnect, &ClientDisconnect_H, (void**)&ClientDisconnect_O);

		InstallHook("Images.VerifySignature",
			RR::Methods::ImageSig::Verify, &ImageVerify_H, (void**)&ImageVerify_O);

		InstallHook("LaunchValidation.HashCheckThread",
			RR::Methods::LaunchValidation::HashCheckThread, &HashCheckThread_H, nullptr);

		InstallHook("LaunchValidation.ResultFromException",
			RR::Methods::LaunchValidation::ResultFromException, &LaunchResultFail_H, (void**)&LaunchResultFail_O);
		InstallHook("LaunchValidation.ResultConsumer",
			RR::Methods::LaunchValidation::ResultConsumer, &LaunchConsumer_H, (void**)&LaunchConsumer_O);
		InstallHook("LaunchValidation.BootPromise",
			RR::Methods::LaunchValidation::BootPromise, &BootPromise_H, (void**)&BootPromise_O);
		InstallHook("BootSequence.Error", RR::Methods::Boot::Error, &BootError_H, nullptr);
		InstallHook("BootSequence.SetState", RR::Methods::Boot::SetState, &BootState_H, (void**)&BootState_O);

		InstallHook("CertPin.Validate",
			RR::Methods::CertPin::Validate, &CertValidate_H, nullptr);
		InstallHook("CertPin.ReportReason",
			RR::Methods::CertPin::ReportReason, &CertReport_H, nullptr);

		InstallHook("PlayerReporting.CheatReport",
			RR::Methods::CheatReport::Report, &CheatReport_H, (void**)&CheatReport_O);

		LoadLibraryA("ws2_32.dll");

		void* getaddrinfoTarget = nullptr;
		MH_STATUS dnsA = MH_CreateHookApiEx(L"ws2_32", "getaddrinfo", &getaddrinfo_H, (LPVOID*)&getaddrinfo_O, &getaddrinfoTarget);
		if (dnsA == MH_OK) dnsA = MH_QueueEnableHook(getaddrinfoTarget);
		PatchLog("[Patch] getaddrinfo %s (MH_STATUS %d)", dnsA == MH_OK ? "hooked" : "FAILED", (int)dnsA);

		void* GetAddrInfoWTarget = nullptr;
		MH_STATUS dnsW = MH_CreateHookApiEx(L"ws2_32", "GetAddrInfoW", &GetAddrInfoW_H, (LPVOID*)&GetAddrInfoW_O, &GetAddrInfoWTarget);
		if (dnsW == MH_OK) dnsW = MH_QueueEnableHook(GetAddrInfoWTarget);
		PatchLog("[Patch] GetAddrInfoW %s (MH_STATUS %d)", dnsW == MH_OK ? "hooked" : "FAILED", (int)dnsW);

		if (RR::Config::EnableTracing)
			InstallHook("HTTPRequest.CallCallback",
				RR::Methods::HTTPRequest::CallCallback, &CallCallback_H, (void**)&CallCallback_O);

		MH_STATUS applied = MH_ApplyQueued();
		PatchLog("[Patch] hooks switched on (MH_STATUS %d)", (int)applied);

		PatchLog("[Patch] done: TLS, SendRequest, anti-cheat (module scan, detectors, quit funnel), EAC challenge, DUID, DNS%s",
			RR::Config::EnableTracing ? ", response tracing" : "");
	}
}
