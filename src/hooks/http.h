#pragma once
#include "common.h"

void (*SendRequest_O)(void*, void*);
void SendRequest_H(void* request, void* mi) {
	static long s_n = 0;
	long n = ++s_n;
	TraceLog("[SendRequest] #%ld enter request=%p", n, request);

	try {
		void* uri = request ? read<void*>(request, RR::Offsets::Request_Uri) : nullptr;

		if (uri && RetAddr) {
			using ToStringFn = Il2cppString * (*)(void*, void*);
			auto fn = reinterpret_cast<ToStringFn>(GA + RR::Methods::System::Uri::ToString);

			Il2cppString* Uri = spoof_call(RetAddr, fn, uri, (void*)nullptr);

			char raw[8192];
			bool truncated = false;
			if (!ReadIl2CppString(Uri, raw, sizeof raw, &truncated) || truncated) {
				PatchLog("[SendRequest] #%ld uri unreadable or too long, passing through", n);
			}
			else {
				std::string url(raw);

				TraceLog("[SendRequest] #%ld url=%s", n, url.c_str());

				std::string replace = RR::Config::ApiHost;
				const char* find = "ns.rec.net";

				if (!replace.empty() && url.find(find) != std::string::npos) {
					url.replace(url.find(find), strlen(find), replace);
					PatchLog("[SendRequest] #%ld REWRITE -> %s", n, url.c_str());

					using Il2cppStringNewFn = Il2cppString * (*)(const char*, void*);
					auto fn2 = reinterpret_cast<Il2cppStringNewFn>(GA + RR::Methods::Il2cpp::il2cpp_string_new);
					Il2cppString* NewStr = spoof_call(RetAddr, fn2, url.c_str(), (void*)nullptr);

					using ObjectGetClassFn = void* (*)(void*, void*);
					auto getClass = reinterpret_cast<ObjectGetClassFn>(GA + RR::Methods::Il2cpp::il2cpp_object_get_class);
					void* UriClass = spoof_call(RetAddr, getClass, uri, (void*)nullptr);

					using ObjectNewFn = void* (*)(void*, void*);
					auto objNew = reinterpret_cast<ObjectNewFn>(GA + RR::Methods::Il2cpp::il2cpp_object_new);
					void* NewUri = UriClass ? spoof_call(RetAddr, objNew, UriClass, (void*)nullptr) : nullptr;

					if (NewUri && NewStr) {
						using UriCtor = void (*)(void*, Il2cppString*, void*);
						auto ctor = reinterpret_cast<UriCtor>(GA + RR::Methods::System::Uri::ctor);

						spoof_call(RetAddr, ctor, NewUri, NewStr, (void*)nullptr);

						set<void*>(request, RR::Offsets::Request_Uri, NewUri);
						TraceLog("[SendRequest] #%ld new uri object=%p", n, NewUri);
					}
					else {
						PatchLog("[SendRequest] #%ld could not build replacement Uri, sending original", n);
					}
				}
			}
		}
	}
	catch (...) {

		PatchLog("[SendRequest] #%ld exception in hook, passing through unchanged", n);
	}

	TraceLog("[SendRequest] #%ld -> original", n);
	SendRequest_O(request, mi);
	TraceLog("[SendRequest] #%ld returned", n);
}

void (*CallCallback_O)(void*, void*);
void CallCallback_H(void* request, void* mi) {

	CallCallback_O(request, mi);

	__try {
		if (request) {
			void* resp = read<void*>(request, RR::Offsets::Request_Response);
			int status = resp ? read<int32_t>(resp, RR::Offsets::Response_StatusCode) : -1;
			int len = 0;
			char preview[513] = {};
			if (resp) {
				void* data = read<void*>(resp, RR::Offsets::Response_Data);
				len = read<int32_t>(resp, RR::Offsets::Response_DataLength);
				if (data && len > 0) {
					const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data) + RR::Offsets::Array_Data;
					int n = len < 512 ? len : 512;
					for (int i = 0; i < n; i++) {
						char c = static_cast<char>(bytes[i]);
						preview[i] = (c >= 32 && c < 127) ? c : '.';
					}
					preview[n] = 0;
				}
				else {

					Il2cppString* txt = read<Il2cppString*>(resp, RR::Offsets::Response_DataAsText);
					if (txt) {
						char s[513];
						if (ReadIl2CppString(txt, s, sizeof s)) {
							int i = 0; for (; s[i] && i < 512; i++) preview[i] = s[i];
							preview[i] = 0; len = i;
						}
					}
				}
			}
			PatchLog("[HTTP<-] request=%p status=%d len=%d body=%s", request, status, len, preview);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}
