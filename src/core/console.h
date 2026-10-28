#pragma once

#include <windows.h>
#include <cstdio>
#include <cstring>

namespace RR::Console {

	enum class Level { Plain, Info, Good, Warn, Bad, Dim };

	inline HANDLE g_out = INVALID_HANDLE_VALUE;
	inline bool g_active = false;
	inline bool g_vt = false;
	inline bool g_ownWindow = false;
	inline CRITICAL_SECTION g_lock;
	inline bool g_lockReady = false;

	inline void Lock() { if (g_lockReady) EnterCriticalSection(&g_lock); }
	inline void Unlock() { if (g_lockReady) LeaveCriticalSection(&g_lock); }

	inline const char* Ansi(Level l) {
		switch (l) {
		case Level::Info: return "\x1b[36m";
		case Level::Good: return "\x1b[32m";
		case Level::Warn: return "\x1b[33m";
		case Level::Bad:  return "\x1b[31m";
		case Level::Dim:  return "\x1b[90m";
		default:          return "\x1b[0m";
		}
	}

	inline WORD Attr(Level l) {
		switch (l) {
		case Level::Info: return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		case Level::Good: return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		case Level::Warn: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		case Level::Bad:  return FOREGROUND_RED | FOREGROUND_INTENSITY;
		case Level::Dim:  return FOREGROUND_INTENSITY;
		default:          return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		}
	}

	inline void WriteRaw(const char* p, size_t n) {
		if (!g_active || !p || !n) return;
		DWORD written = 0;
		if (!WriteConsoleA(g_out, p, (DWORD)n, &written, nullptr))
			WriteFile(g_out, p, (DWORD)n, &written, nullptr);
	}

	inline Level Classify(const char* line) {
		if (!line) return Level::Plain;
		if (strstr(line, "FAILED") || strstr(line, "failed") || strstr(line, "WARNING")) return Level::Bad;
		if (strstr(line, "SUPPRESSED")) return Level::Warn;
		if (strstr(line, "passed through")) return Level::Bad;

		if (strncmp(line, "[DllMain]", 9) == 0) return Level::Info;
		if (strncmp(line, "[Patch]", 7) == 0) return Level::Good;
		if (strncmp(line, "[Quit]", 6) == 0) return Level::Warn;
		if (strncmp(line, "[CheatMgr]", 10) == 0) return Level::Warn;
		if (strncmp(line, "[Config]", 8) == 0) return Level::Info;
		if (strncmp(line, "[Console]", 9) == 0) return Level::Info;
		if (strncmp(line, "[Resolve]", 9) == 0) return Level::Dim;
		if (strncmp(line, "[SendRequest]", 13) == 0) return Level::Dim;
		if (strncmp(line, "[DNS]", 5) == 0) return Level::Dim;
		if (strncmp(line, "[Photon]", 8) == 0) return Level::Dim;
		return Level::Plain;
	}

	inline void Log(const char* line) {
		if (!g_active || !line) return;
		Lock();
		const Level lvl = Classify(line);
		if (g_vt) {
			WriteRaw(Ansi(lvl), strlen(Ansi(lvl)));
			WriteRaw(line, strlen(line));
			WriteRaw("\x1b[0m", 4);
			WriteRaw("\r\n", 2);
		}
		else {
			CONSOLE_SCREEN_BUFFER_INFO info{};
			WORD saved = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			if (GetConsoleScreenBufferInfo(g_out, &info)) saved = info.wAttributes;
			SetConsoleTextAttribute(g_out, Attr(lvl));
			WriteRaw(line, strlen(line));
			SetConsoleTextAttribute(g_out, saved);
			WriteRaw("\r\n", 2);
		}
		Unlock();
	}

	inline void Line(const char* text, Level lvl = Level::Plain) {
		if (!g_active || !text) return;
		Lock();
		if (g_vt) {
			WriteRaw(Ansi(lvl), strlen(Ansi(lvl)));
			WriteRaw(text, strlen(text));
			WriteRaw("\x1b[0m", 4);
			WriteRaw("\r\n", 2);
		}
		else {
			CONSOLE_SCREEN_BUFFER_INFO info{};
			WORD saved = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			if (GetConsoleScreenBufferInfo(g_out, &info)) saved = info.wAttributes;
			SetConsoleTextAttribute(g_out, Attr(lvl));
			WriteRaw(text, strlen(text));
			SetConsoleTextAttribute(g_out, saved);
			WriteRaw("\r\n", 2);
		}
		Unlock();
	}

	inline void Blank() { Line("", Level::Plain); }

	inline BOOL WINAPI CtrlHandler(DWORD type) {
		if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT) return TRUE;
		return FALSE;
	}

	struct FocusSearch { HWND found; };
	inline BOOL CALLBACK FindGameWindowProc(HWND hwnd, LPARAM param) {
		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);
		if (pid == GetCurrentProcessId() && IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == nullptr) {
			char title[128] = {};
			if (GetWindowTextA(hwnd, title, sizeof(title)) > 0) {
				reinterpret_cast<FocusSearch*>(param)->found = hwnd;
				return FALSE;
			}
		}
		return TRUE;
	}

	inline bool Init(bool enable) {
		if (!enable) return false;
		if (g_active) return true;

		InitializeCriticalSection(&g_lock);
		g_lockReady = true;

		bool haveConsole = GetConsoleWindow() != nullptr;
		if (!haveConsole) {
			if (AttachConsole(ATTACH_PARENT_PROCESS)) haveConsole = true;
			else if (GetLastError() == ERROR_ACCESS_DENIED) haveConsole = true;
		}

		if (!haveConsole) {
			if (!AllocConsole()) return false;
			g_ownWindow = true;
		}

		g_out = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (g_out == INVALID_HANDLE_VALUE) return false;

		SetStdHandle(STD_OUTPUT_HANDLE, g_out);
		SetStdHandle(STD_ERROR_HANDLE, g_out);
		FILE* f = nullptr;
		freopen_s(&f, "CONOUT$", "w", stdout);

		DWORD mode = 0;
		if (GetConsoleMode(g_out, &mode)) {
			DWORD want = mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			g_vt = SetConsoleMode(g_out, want) != 0;
			if (!g_vt) SetConsoleMode(g_out, mode);
		}

		g_active = true;
		SetConsoleCtrlHandler(CtrlHandler, TRUE);

		if (g_ownWindow) {

			HWND con = GetConsoleWindow();
			if (con) {
				ShowWindow(con, SW_SHOWNOACTIVATE);
				SetWindowPos(con, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				SetConsoleTitleA("Rift (20240802.02)");
			}
			FocusSearch search{ nullptr };
			EnumWindows(FindGameWindowProc, reinterpret_cast<LPARAM>(&search));
			if (search.found) {
				ShowWindow(search.found, SW_SHOW);
				SetForegroundWindow(search.found);
				SetActiveWindow(search.found);
			}
		}

		return true;
	}
}
