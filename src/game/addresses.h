#pragma once
#include "../core/runtime.h"

namespace RR::Methods::LegacyTlsAuthentication {

	uintptr_t NotifyServerCertificate = 0x60ED240;
}

namespace RR::Methods::HTTPRequest {

	uintptr_t SendRequest = 0x60E40C0;

	uintptr_t CallCallback = 0x60E81A0;
}

namespace RR::Methods::AntiCheat {
	uintptr_t ModuleScanDetected = 0x13E1FA0;

	uintptr_t CheckForDUIDMismatch = 0x13D98D0;

	uintptr_t OnTimeCheatDetected = 0x13DB080;
	uintptr_t OnObscuredTypeCheatDetected = 0x13DAFD0;
	uintptr_t OnAdvancedMovementCheatDetected = 0x13DACD0;
}

namespace RR::Methods::EAC {
	uintptr_t GenerateChallengeResponse = 0x7003DF0;
	uintptr_t Update = 0x7004B70;
}

namespace RR::Methods::ConnGate {
	uintptr_t ConnectUsingSettings = 0x5F22BB0;
	uintptr_t PeerDisconnect = 0x5EE44D0;
	uintptr_t OnStatusChanged = 0x5F236B0;
	uintptr_t DebugReturn = 0x5F1ED10;
	uintptr_t OnOperationResponse = 0x5F24890;

	uintptr_t ClientDisconnect = 0x5F22870;
}

namespace RR::Methods::Boot {
	uintptr_t SetState = 0x1D61C40;
	uintptr_t Error = 0x1D64AC0;
}

namespace RR::Methods::LaunchValidation {
	inline constexpr uintptr_t VerifyFile = 0xA52810;

	inline constexpr uintptr_t ResultFromException = 0xA56370;

	inline constexpr uintptr_t ResultConsumer = 0xA5CAE0;

	inline constexpr uintptr_t BootPromise = 0xA452C0;

	inline constexpr uintptr_t HashCheckThread = 0xA5D530;
}

namespace RR::Methods::Promise {
	inline constexpr uintptr_t Resolved = 0x6B9E690;
}

namespace RR::Methods::SessionManager {
	uintptr_t FatalApplicationQuit = 0x1462880;
	uintptr_t TryApplicationQuit = 0x1469860;
	uintptr_t TryApplicationQuitNoArg = 0x14699E0;
	uintptr_t get_IsQuitting = 0x146B450;
	uintptr_t get_IsQuittingOrLoggingOut = 0x146B3D0;
	uintptr_t get_IsLoadingOrQuitting = 0x146B2D0;
}

namespace RR::Methods::System::Uri {
	uintptr_t ToString = 0x789A9F0;
	uintptr_t ctor = 0x789BD40;
}

namespace RR::Methods::Il2cpp {

	uintptr_t il2cpp_string_new;
	uintptr_t il2cpp_object_get_class;
	uintptr_t il2cpp_object_new;
	uintptr_t il2cpp_runtime_class_init;
}

namespace RR::Offsets {

	constexpr int Request_Uri = 16;
	constexpr int Request_Response = 112;

	constexpr int Response_StatusCode = 24;
	constexpr int Response_Data = 72;
	constexpr int Response_DataLength = 80;
	constexpr int Response_DataAsText = 96;

	constexpr int Array_Data = 32;
}

namespace RR::Methods::ImageSig {
	inline constexpr uintptr_t Verify = 0x65BFD70;
	inline constexpr uintptr_t GetPublicKey = 0x65BDF90;
}
namespace RR::Methods::CertPin {
	inline constexpr uintptr_t Validate = 0x66B5A70;
	inline constexpr uintptr_t ReportReason = 0x66B5210;
}
namespace RR::Methods::CheatReport {
	inline constexpr uintptr_t Report = 0x665E040;
}
