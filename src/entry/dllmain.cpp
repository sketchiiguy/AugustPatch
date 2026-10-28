#include "../hooks/install.h"

static void Banner(bool consoleOn) {
	if (!consoleOn) return;

	RR::Console::Blank();
	RR::Console::Line("  +--------------------------------------------------------------+", RR::Console::Level::Info);
	RR::Console::Line("  |  2024AugustPatch  -  Rec Room 2024 (build 20240802.02)       |", RR::Console::Level::Info);
	RR::Console::Line("  |  live log  -  also written to 2024augustpatch.log            |", RR::Console::Level::Info);
	RR::Console::Line("  +--------------------------------------------------------------+", RR::Console::Level::Info);
	RR::Console::Blank();
}

static DWORD WINAPI ConsoleThread(LPVOID)
{
	Sleep(500);
	if (!RR::Console::Init(RR::Config::EnableConsole)) return 0;
	Banner(true);
	char line[256];
	sprintf_s(line, sizeof line, "[Console] live log attached (pid %lu)%s", GetCurrentProcessId(),
		RR::Console::g_ownWindow ? " in its own window" : " in the launcher's terminal");
	RR::Console::Log(line);
	return 0;
}

#include <dbghelp.h>

static void CrashWrite(const wchar_t* path, const char* text, DWORD len)
{
	HANDLE f = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (f == INVALID_HANDLE_VALUE) return;
	DWORD wrote = 0;
	WriteFile(f, text, len, &wrote, nullptr);
	CloseHandle(f);
}

static LONG WINAPI CrashFilter(EXCEPTION_POINTERS* info)
{
	wchar_t dir[MAX_PATH]{};
	GetModuleFileNameW(nullptr, dir, MAX_PATH);
	wchar_t* slash = wcsrchr(dir, L'\\');
	if (slash) slash[1] = L'\0'; else dir[0] = L'\0';

	wchar_t logPath[MAX_PATH]{}, dmpPath[MAX_PATH]{};
	swprintf_s(logPath, L"%s2024augustpatch-crash.log", dir);
	swprintf_s(dmpPath, L"%s2024augustpatch-crash.dmp", dir);

	static char text[8192];
	const EXCEPTION_RECORD* er = info ? info->ExceptionRecord : nullptr;
	const uintptr_t address = er ? reinterpret_cast<uintptr_t>(er->ExceptionAddress) : 0;
	const uintptr_t ga = reinterpret_cast<uintptr_t>(GetModuleHandleA("GameAssembly.dll"));
	const uintptr_t self = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));

	int n = sprintf_s(text, "[2024augustpatch] UNHANDLED EXCEPTION 0x%08lX at 0x%llX",
		er ? static_cast<unsigned long>(er->ExceptionCode) : 0ul,
		static_cast<unsigned long long>(address));
	if (n < 0) n = 0;
	if (ga && address >= ga && address < ga + 0x20000000ull)
		n += sprintf_s(text + n, sizeof(text) - n, " (GameAssembly+0x%llX)", static_cast<unsigned long long>(address - ga));
	else if (self && address >= self && address < self + 0x10000000ull)
		n += sprintf_s(text + n, sizeof(text) - n, " (2024AugustPatch+0x%llX)", static_cast<unsigned long long>(address - self));
	if (n < 0) n = 0;
	n += sprintf_s(text + n, sizeof(text) - n, " thread=%lu\r\n", GetCurrentThreadId());
	if (n < 0) n = 0;

	void* frames[40]{};
	const USHORT count = CaptureStackBackTrace(0, 40, frames, nullptr);
	for (USHORT i = 0; i < count && n < static_cast<int>(sizeof(text)) - 48; i++)
	{
		const uintptr_t a = reinterpret_cast<uintptr_t>(frames[i]);
		if (ga && a >= ga && a < ga + 0x20000000ull)
			n += sprintf_s(text + n, sizeof(text) - n, "  [%02u] GameAssembly+0x%llX\r\n", i, static_cast<unsigned long long>(a - ga));
		else
			n += sprintf_s(text + n, sizeof(text) - n, "  [%02u] 0x%llX\r\n", i, static_cast<unsigned long long>(a));
		if (n < 0) { n = static_cast<int>(sizeof(text)) - 1; break; }
	}
	CrashWrite(logPath, text, static_cast<DWORD>(n > 0 ? n : 0));

	if (HMODULE dbg = LoadLibraryA("dbghelp.dll"))
	{
		using MiniDumpFn = BOOL(WINAPI*)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
		if (auto fn = reinterpret_cast<MiniDumpFn>(GetProcAddress(dbg, "MiniDumpWriteDump")))
		{
			HANDLE f = CreateFileW(dmpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (f != INVALID_HANDLE_VALUE)
			{
				MINIDUMP_EXCEPTION_INFORMATION mei{ GetCurrentThreadId(), info, FALSE };
				fn(GetCurrentProcess(), GetCurrentProcessId(), f, MiniDumpWithThreadInfo, &mei, nullptr, nullptr);
				CloseHandle(f);
			}
		}
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		SetUnhandledExceptionFilter(CrashFilter);
		RR::Config::Load();
		if (RR::Config::EnableConsole)
		{
			HANDLE h = CreateThread(nullptr, 0, ConsoleThread, nullptr, 0, nullptr);
			if (h) CloseHandle(h);
		}
		else
			PatchLog("[Console] disabled (EnableConsole=false); diagnostics -> 2024augustpatch.log only");
		RR::Config::LogSummary();
		PatchLog("[DllMain] attached, resolving + patching");
		RR::Patches::Resolve();
		RR::Patches::Patch();
		PatchLog("[DllMain] patch complete");
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
