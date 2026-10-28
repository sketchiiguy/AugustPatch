#pragma once
#include "common.h"

bool IsPhotonHost(const std::string& node) {
	return node.find("photonengine") != std::string::npos
		|| node.find("exitgames") != std::string::npos
		|| node.find("photonindustries") != std::string::npos;
}

bool IsDeadHost(const std::string& node) {
	static const char* kDeadHosts[] = {
		"rudderstack.com",
		"backtrace.io",
		"cloud.unity3d.com",
	};
	for (const char* h : kDeadHosts) {
		if (node.find(h) != std::string::npos) return true;
	}
	return false;
}

int (WSAAPI* getaddrinfo_O)(PCSTR, PCSTR, const ADDRINFOA*, PADDRINFOA*);
int WSAAPI getaddrinfo_H(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA* pHints, PADDRINFOA* ppResult) {
	if (RR::Config::BlockDeadHosts && pNodeName && IsDeadHost(pNodeName)) {
		PatchLog("[DNS] %s -> BLOCKED (dead telemetry host)", pNodeName);
		if (ppResult) *ppResult = nullptr;
		WSASetLastError(WSAHOST_NOT_FOUND);
		return WSAHOST_NOT_FOUND;
	}

	if (*RR::Config::PhotonHost && pNodeName && IsPhotonHost(pNodeName)) {
		PatchLog("[Photon] getaddrinfo %s -> %s", pNodeName, RR::Config::PhotonHost);
		return getaddrinfo_O(RR::Config::PhotonHost, pServiceName, pHints, ppResult);
	}

	TraceLog("[DNS] %s", pNodeName ? pNodeName : "<null>");
	return getaddrinfo_O(pNodeName, pServiceName, pHints, ppResult);
}

int (WSAAPI* GetAddrInfoW_O)(PCWSTR, PCWSTR, const ADDRINFOW*, PADDRINFOW*);
int WSAAPI GetAddrInfoW_H(PCWSTR pNodeName, PCWSTR pServiceName, const ADDRINFOW* pHints, PADDRINFOW* ppResult) {
	if (pNodeName) {
		char node[NI_MAXHOST] = {};
		size_t converted = 0;
		wcstombs_s(&converted, node, pNodeName, _TRUNCATE);

		if (RR::Config::BlockDeadHosts && IsDeadHost(node)) {
			PatchLog("[DNS] %s (W) -> BLOCKED (dead telemetry host)", node);
			if (ppResult) *ppResult = nullptr;
			WSASetLastError(WSAHOST_NOT_FOUND);
			return WSAHOST_NOT_FOUND;
		}
		if (*RR::Config::PhotonHost && IsPhotonHost(node)) {
			PatchLog("[Photon] GetAddrInfoW %s -> %s", node, RR::Config::PhotonHost);
			wchar_t wideHost[NI_MAXHOST] = {};
			mbstowcs_s(&converted, wideHost, RR::Config::PhotonHost, _TRUNCATE);
			return GetAddrInfoW_O(wideHost, pServiceName, pHints, ppResult);
		}
		TraceLog("[DNS] %s (W)", node);
	}
	return GetAddrInfoW_O(pNodeName, pServiceName, pHints, ppResult);
}
