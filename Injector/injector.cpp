#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdarg>
#include <thread>
#include <chrono>

static const wchar_t* kProcessName = L"RecRoom.exe";

static const wchar_t* kLauncherName = L"Recroom_Release.exe";

static const wchar_t* kDefaultDll  = L"2024AugustPatch.dll";
static const wchar_t* kDefaultExe  = L"RecRoom.exe";

static const wchar_t* kConsoleEnv = L"RR2024AUGUSTPATCH_CONSOLE";

static const std::chrono::milliseconds kSettle{250};

static const wchar_t* kRequiredModules[] = { L"GameAssembly.dll" };

static bool g_vt = false;

static void EnableAnsi() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (out && GetConsoleMode(out, &mode))
        g_vt = SetConsoleMode(out, mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    SetConsoleOutputCP(CP_UTF8);
}

static void Say(const char* color, const char* fmt, ...) {
    if (g_vt && color) fputs(color, stdout);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    if (g_vt) fputs("\x1b[0m", stdout);
    fputc('\n', stdout);
    fflush(stdout);
}

static const char* C_DIM   = "\x1b[90m";
static const char* C_CYAN  = "\x1b[36m";
static const char* C_GREEN = "\x1b[32m";
static const char* C_YELLOW= "\x1b[33m";
static const char* C_RED   = "\x1b[31m";
static const char* C_BOLD  = "\x1b[1;97m";

static std::string Narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string s((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

static std::wstring DirectoryOf(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring() : path.substr(0, slash + 1);
}

static std::wstring FileNameOf(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

static std::wstring ExeDirectory() {
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    return DirectoryOf(exePath);
}

static bool FileExists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static void FindProcessIds(const wchar_t* name, std::vector<DWORD>& out) {
    out.clear();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snap, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, name) == 0) out.push_back(entry.th32ProcessID);
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);
}

static bool ProcessIsAlive(DWORD pid) {
    HANDLE proc = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!proc) return false;
    const bool alive = WaitForSingleObject(proc, 0) == WAIT_TIMEOUT;
    CloseHandle(proc);
    return alive;
}

static bool ProcessHasModule(DWORD pid, const wchar_t* moduleName) {
    for (int attempt = 0; attempt < 8; ++attempt) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_BAD_LENGTH) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            return false;
        }

        MODULEENTRY32W mod{};
        mod.dwSize = sizeof(mod);
        bool found = false;
        if (Module32FirstW(snap, &mod)) {
            do {
                if (_wcsicmp(mod.szModule, moduleName) == 0) { found = true; break; }
            } while (Module32NextW(snap, &mod));
        }
        CloseHandle(snap);
        return found;
    }
    return false;
}

static bool AllRequiredModulesLoaded(DWORD pid) {
    for (const wchar_t* m : kRequiredModules)
        if (!ProcessHasModule(pid, m)) return false;
    return true;
}

static DWORD WaitForGameProcess(const std::wstring& dllName, int timeoutMs) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    std::vector<DWORD> pids;
    while (std::chrono::steady_clock::now() < deadline) {
        FindProcessIds(kProcessName, pids);
        for (DWORD candidate : pids)
            if (!ProcessHasModule(candidate, dllName.c_str())) return candidate;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    return 0;
}

static DWORD LaunchGame(const std::wstring& exePath) {
    const std::wstring dir = DirectoryOf(exePath);
    std::wstring cmdLine = L"\"" + exePath + L"\"";

    SetEnvironmentVariableW(kConsoleEnv, L"1");

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;

    PROCESS_INFORMATION pi{};
    const BOOL ok = CreateProcessW(exePath.c_str(), &cmdLine[0], nullptr, nullptr,
        TRUE  , 0, nullptr, dir.empty() ? nullptr : dir.c_str(), &si, &pi);

    const DWORD err = ok ? 0 : GetLastError();
    SetEnvironmentVariableW(kConsoleEnv, nullptr);

    if (!ok) {
        Say(C_RED, "[!] Could not start the game (Win32 error %lu).", err);
        Say(C_DIM, "    Path: %s", Narrow(exePath).c_str());
        return 0;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return pi.dwProcessId;
}

static bool Inject(DWORD pid, const std::wstring& dllPath) {
    HANDLE proc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!proc) {
        Say(C_RED, "[!] OpenProcess failed (%lu). Try running the injector as administrator.", GetLastError());
        return false;
    }

    bool ok = false;
    const SIZE_T bytes = (dllPath.size() + 1) * sizeof(wchar_t);

    LPVOID remote = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) {
        Say(C_RED, "[!] VirtualAllocEx failed (%lu).", GetLastError());
        CloseHandle(proc);
        return false;
    }

    if (!WriteProcessMemory(proc, remote, dllPath.c_str(), bytes, nullptr)) {
        Say(C_RED, "[!] WriteProcessMemory failed (%lu).", GetLastError());
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(k32, "LoadLibraryW"));

    HANDLE thread = CreateRemoteThread(proc, nullptr, 0, loadLibrary, remote, 0, nullptr);
    if (thread) {
        WaitForSingleObject(thread, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeThread(thread, &exitCode);
        CloseHandle(thread);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        const std::wstring dllName = FileNameOf(dllPath);
        ok = ProcessHasModule(pid, dllName.c_str());
        if (!ok && exitCode != 0) ok = true;
    }
    else {
        Say(C_RED, "[!] CreateRemoteThread failed (%lu).", GetLastError());
    }

    VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
    CloseHandle(proc);
    return ok;
}

int wmain(int argc, wchar_t** argv) {
    EnableAnsi();
    SetConsoleTitleW(L"2024AugustPatch - Rec Room 2024 (20240802.02)");

    std::wstring dllPath, exePath;
    bool doLaunch = true;
    bool stayOpen = true;

    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];
        if (arg == L"--no-launch")      doLaunch = false;
        else if (arg == L"--no-wait")   stayOpen = false;
        else if (arg == L"--help" || arg == L"-h") {
            Say(C_BOLD, "Injector.exe [--no-launch] [--no-wait] [--dll <path>] [--exe <path>]");
            Say(C_DIM,  "  default: launch Recroom_Release.exe next to this exe, inject, keep the");
            Say(C_DIM,  "           console open for the patch's live log until the game exits");
            return 0;
        }
        else if (arg == L"--dll" && i + 1 < argc) dllPath = argv[++i];
        else if (arg == L"--exe" && i + 1 < argc) exePath = argv[++i];
        else {
            Say(C_YELLOW, "[!] Unknown argument: %s", Narrow(arg).c_str());
            return 2;
        }
    }

    const std::wstring dir = ExeDirectory();

    if (dllPath.empty()) dllPath = dir + kDefaultDll;
    {
        wchar_t full[MAX_PATH]{};
        if (!GetFullPathNameW(dllPath.c_str(), MAX_PATH, full, nullptr) || !FileExists(full)) {
            Say(C_RED, "[!] Patch DLL not found: %s", Narrow(dllPath).c_str());
            Say(C_DIM, "    Put 2024AugustPatch.dll next to this injector, or pass --dll <path>.");
            return 1;
        }
        dllPath = full;
    }
    const std::wstring dllName = FileNameOf(dllPath);

    if (exePath.empty()) {
        exePath = dir + kDefaultExe;
        if (!FileExists(exePath)) {
            const std::wstring launcher = dir + kLauncherName;
            if (FileExists(launcher)) exePath = launcher;
        }
    }
    {
        wchar_t full[MAX_PATH]{};
        if (GetFullPathNameW(exePath.c_str(), MAX_PATH, full, nullptr)) exePath = full;
    }
    if (doLaunch && !FileExists(exePath)) {
        Say(C_RED, "[!] Game executable not found: %s", Narrow(exePath).c_str());
        Say(C_DIM, "    Injector.exe, 2024AugustPatch.dll and RecRoom.exe must be in one folder,");
        Say(C_DIM, "    or pass --exe <path> / use --no-launch to attach to a running client.");
        return 1;
    }

    Say(C_BOLD, "");
    Say(C_BOLD, "  2024AugustPatch launcher  -  Rec Room 2024 (build 20240802.02)");
    Say(C_DIM,  "  -------------------------------------------------------------");
    Say(C_DIM,  "  patch DLL : %s", Narrow(dllPath).c_str());
    if (doLaunch) Say(C_DIM, "  game exe  : %s", Narrow(exePath).c_str());
    else          Say(C_DIM, "  game exe  : (not launched; attaching to a running client)");
    Say(C_DIM,  "  log file  : %s2024augustpatch.log", Narrow(dir).c_str());
    Say(C_BOLD, "");

    HANDLE gate = CreateMutexW(nullptr, FALSE, L"Local\\RecRoom2024AugustPatchInjector");

    DWORD pid = 0;

    for (;;) {
        int patched = 0;
        std::vector<DWORD> pids;
        FindProcessIds(kProcessName, pids);
        for (DWORD candidate : pids) {
            if (ProcessHasModule(candidate, dllName.c_str())) ++patched;
            else if (pid == 0) pid = candidate;
        }

        if (pid != 0) {
            Say(C_GREEN, "[+] Found a running, unpatched game (pid %lu).", pid);
            if (patched > 0) Say(C_DIM, "    %d other instance(s) already patched.", patched);
            break;
        }

        if (!doLaunch) {
            Say(C_CYAN, "[*] Waiting for an unpatched %s (launch it now)...", Narrow(kProcessName).c_str());
            while (pid == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                FindProcessIds(kProcessName, pids);
                for (DWORD candidate : pids)
                    if (!ProcessHasModule(candidate, dllName.c_str())) { pid = candidate; break; }
            }
            Say(C_GREEN, "[+] Found a running, unpatched game (pid %lu).", pid);
            break;
        }

        if (patched > 0)
            Say(C_DIM, "[=] %d instance(s) already patched -- starting another one.", patched);

        Say(C_CYAN, "[*] Starting the game...");
        pid = LaunchGame(exePath);
        if (!pid) return 1;
        Say(C_GREEN, "[+] Started %s (pid %lu).", Narrow(FileNameOf(exePath)).c_str(), pid);

        if (_wcsicmp(FileNameOf(exePath).c_str(), kProcessName) != 0) {
            Say(C_CYAN, "[*] Waiting for %s to appear...", Narrow(kProcessName).c_str());
            const DWORD gamePid = WaitForGameProcess(dllName, 90000);
            if (!gamePid) {
                Say(C_RED, "[!] %s did not start (90s).", Narrow(kProcessName).c_str());
                return 1;
            }
            pid = gamePid;
            Say(C_GREEN, "[+] Game process up (pid %lu).", pid);
        }
        break;
    }

    Say(C_CYAN, "[*] Waiting for GameAssembly.dll...");
    while (!AllRequiredModulesLoaded(pid)) {
        if (!ProcessIsAlive(pid)) {
            Say(C_RED, "[!] Game process exited before it finished loading.");
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::this_thread::sleep_for(kSettle);

    Say(C_CYAN, "[*] Injecting %s ...", Narrow(dllName).c_str());
    if (gate) WaitForSingleObject(gate, INFINITE);
    const bool taken = ProcessHasModule(pid, dllName.c_str());
    const bool ok = !taken && Inject(pid, dllPath);
    if (gate) ReleaseMutex(gate);

    if (taken) {
        Say(C_YELLOW, "[=] pid %lu was patched by another injector.", pid);
        return 1;
    }
    if (!ok) {
        Say(C_RED, "[!] Injection failed. See the message above.");
        return 1;
    }

    Say(C_GREEN, "[+] Injected. The patch is loading now.");
    Say(C_DIM,   "    Its live log appears below, in this window.");
    Say(C_DIM,   "    Do not press Ctrl+C here -- it would also hit the game. Close the game to quit.");
    Say(C_BOLD,  "");

    if (!stayOpen) return 0;

    HANDLE game = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!game) {
        Say(C_YELLOW, "[=] Cannot wait for the game to exit (OpenProcess failed); leaving now.");
        Say(C_DIM,    "    The patch keeps writing to 2024augustpatch.log.");
        return 0;
    }

    const auto started = std::chrono::steady_clock::now();
    WaitForSingleObject(game, INFINITE);
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - started).count();

    DWORD exitCode = 0;
    GetExitCodeProcess(game, &exitCode);
    CloseHandle(game);

    Say(C_BOLD, "");
    Say(C_DIM,  "  -------------------------------------------------------------");
    Say(C_CYAN, "[*] Game exited (code %lu) after %llds. Log: 2024augustpatch.log", exitCode, (long long)seconds);
    return 0;
}
