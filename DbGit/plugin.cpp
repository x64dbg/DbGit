#include "plugin.h"
#include "stringutils.h"

static wchar_t szDbPath[1024];
static wchar_t szDbFilename[MAX_PATH];
static bool bEnabled = true;
static SYSTEMTIME sessionBegin;
static SYSTEMTIME sessionEnd;

static DWORD cmd(_In_z_ _Printf_format_string_ const wchar_t* Format, ...)
{
    va_list args;
    va_start(args, Format);

    wchar_t szCommandLine[2048];
    _vsnwprintf_s(szCommandLine, _TRUNCATE, Format, args);
    
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = { };
    DWORD dwExitCode = 1;
    if(CreateProcessW(nullptr, szCommandLine, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, szDbPath, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &dwExitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        dprintf("CreateProcessW failed with error %u\n", GetLastError());
    }

    dprintf("%S returned %u\n", szCommandLine, dwExitCode);

    va_end(args);
    return dwExitCode;
}

extern "C" __declspec(dllexport) void CBINITDEBUG(CBTYPE cbType, PLUG_CB_INITDEBUG* info)
{
    duint setting = 0;
    if(BridgeSettingGetUint("Engine", "SaveDatabaseInProgramDirectory", &setting) && setting)
    {
        if(BridgeSettingGetUint("DbGit", "UnsupportedConfigurationShown", &setting) && !setting
            && MessageBoxW(GuiGetWindowHandle(),
                L"The option 'Save Database in Program Directory' is not supported by DbGit. Do you want me to disable this?",
                L"Unsupported configuration",
                MB_YESNO | MB_ICONWARNING) == IDYES)
        {
            BridgeSettingSetUint("Engine", "SaveDatabaseInProgramDirectory", 0);
        }
        else
        {
            MessageBoxW(GuiGetWindowHandle(), L"Your database will not be versioned.", L"DbGit disabled", MB_OK);
            BridgeSettingSetUint("DbGit", "UnsupportedConfigurationShown", 1);
            bEnabled = false;
        }
    }
    else
        bEnabled = true;

    if(bEnabled)
    {
        GetSystemTime(&sessionBegin);
        auto sep = strrchr(info->szFileName, '\\');
        wcscpy_s(szDbFilename, StringUtils::Utf8ToUtf16(sep + 1).c_str());
#ifdef _WIN64
        wcscat_s(szDbFilename, L".dd64");
#else
        wcscat_s(szDbFilename, L".dd32");
#endif //_WIN64
    }
}

extern "C" __declspec(dllexport) void CBSAVEDB(CBTYPE cbType, PLUG_CB_LOADSAVEDB* info)
{
    BridgeSettingSetUint("Engine", "DisableDatabaseCompression", 1);
}

static String convertDate(SYSTEMTIME & st)
{
    return StringUtils::sprintf("%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

extern "C" __declspec(dllexport) void CBSTOPDEBUG(CBTYPE cbType, PLUG_CB_STOPDEBUG* info)
{
    if(bEnabled)
    {
        GetSystemTime(&sessionEnd);
        cmd(L"git add -A \"%s\"", szDbFilename);
        cmd(L"git commit -m \"[DbGit] debugging session from %S to %S (%s)\"", convertDate(sessionBegin).c_str(), convertDate(sessionEnd).c_str(), szDbFilename);
    }
}

bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
    GetModuleFileNameW(GetModuleHandleW(nullptr), szDbPath, _countof(szDbPath));

    auto sep = wcsrchr(szDbPath, '\\');
    if(sep)
        *sep = '\0';
    wcscat_s(szDbPath, L"\\db");
    cmd(L"git init");
    return true;
}

void pluginStop()
{
}

void pluginSetup()
{
}
