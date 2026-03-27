#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>

int main(int argc, char *argv[])
{
    std::wstring passManagerPath;

    if (argc > 1) {
        // Path passed as command-line argument
        int wlen = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, nullptr, 0);
        passManagerPath.resize(wlen);
        MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, &passManagerPath[0], wlen);
    } else {
        // Default: look for Lab1_PassManager.exe next to this exe
        wchar_t selfPath[MAX_PATH];
        GetModuleFileNameW(NULL, selfPath, MAX_PATH);
        std::wstring dir(selfPath);
        size_t pos = dir.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            dir = dir.substr(0, pos + 1);
        }
        passManagerPath = dir + L"Lab1_PassManager.exe";
    }

    std::wcout << L"[Protector] Starting: " << passManagerPath << std::endl;

    // 1. Create the password manager process
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::wstring cmdLine = passManagerPath;

    if (!CreateProcessW(
            cmdLine.c_str(), NULL,
            NULL, NULL,
            FALSE, 0,
            NULL, NULL,
            &si, &pi)) {
        DWORD err = GetLastError();
        std::cerr << "[Protector] CreateProcessW() FAILED, error = "
                  << err << std::endl;
        return 1;
    }
    std::cout << "[Protector] CreateProcessW() success, PID = "
              << pi.dwProcessId << std::endl;

    // 2. Attach as debugger
    if (!DebugActiveProcess(pi.dwProcessId)) {
        DWORD err = GetLastError();
        std::cerr << "[Protector] DebugActiveProcess() FAILED, error = 0x"
                  << std::hex << err << std::endl;
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 1;
    }
    std::cout << "[Protector] DebugActiveProcess() success — attached as debugger"
              << std::endl;

    // 3. Debug event loop: pass through all debug events
    DEBUG_EVENT debugEvent;
    DWORD continueStatus;

    while (true) {
        if (!WaitForDebugEvent(&debugEvent, INFINITE)) {
            std::cerr << "[Protector] WaitForDebugEvent() failed" << std::endl;
            break;
        }

        continueStatus = DBG_CONTINUE;

        switch (debugEvent.dwDebugEventCode) {
        case EXCEPTION_DEBUG_EVENT:
            if (debugEvent.u.Exception.ExceptionRecord.ExceptionCode
                != EXCEPTION_BREAKPOINT) {
                continueStatus = DBG_EXCEPTION_NOT_HANDLED;
            }
            break;

        case EXIT_PROCESS_DEBUG_EVENT:
            std::cout << "[Protector] Password manager exited (code "
                      << debugEvent.u.ExitProcess.dwExitCode << ")" << std::endl;
            ContinueDebugEvent(debugEvent.dwProcessId,
                               debugEvent.dwThreadId,
                               continueStatus);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 0;

        default:
            break;
        }

        ContinueDebugEvent(debugEvent.dwProcessId,
                           debugEvent.dwThreadId,
                           continueStatus);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}

#else
// Non-Windows stub
int main()
{
    std::cerr << "Lab1_Protector requires Windows (WinAPI DebugActiveProcess)"
              << std::endl;
    return 1;
}
#endif
