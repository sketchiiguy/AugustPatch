#pragma once
#include "runtime.h"

namespace RR::Config {

	constexpr const char* ApiHost = "REPLACE_URL";

	constexpr const char* PhotonHost = "REPLACE_VPS_IP";

	constexpr int PhotonPort = 0;

	constexpr bool EnableConsole = true;

	constexpr bool BlockDeadHosts = true;

	constexpr bool SuppressDuidMismatch = true;

	constexpr bool DisableAntiCheat = true;

	constexpr bool EnableTracing = true;

	inline void Load() {}

	inline void LogSummary() {
		PatchLog("[Config] compiled in: ApiHost=%s PhotonHost=%s PhotonPort=%d EnableConsole=%s"
			" BlockDeadHosts=%s SuppressDuidMismatch=%s DisableAntiCheat=%s EnableTracing=%s",
			ApiHost, PhotonHost, PhotonPort,
			EnableConsole ? "true" : "false", BlockDeadHosts ? "true" : "false",
			SuppressDuidMismatch ? "true" : "false", DisableAntiCheat ? "true" : "false",
			EnableTracing ? "true" : "false");
	}
}
