typedef void* HANDLE;
typedef HANDLE HWND;
typedef unsigned long DWORD;
typedef int BOOL;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned short wchar_t;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef const void* LPCVOID;
typedef void* LPVOID;
typedef unsigned long long ULONG_PTR;
typedef DWORD* LPDWORD;

#define WINAPI __stdcall
#define DLLIMPORT __declspec(dllimport)
#define FALSE 0
#define MAX_PATH 260
#define MB_OK 0x00000000UL
#define MB_ICONERROR 0x00000010UL
#define GENERIC_WRITE 0x40000000UL
#define FILE_SHARE_READ 0x00000001UL
#define CREATE_ALWAYS 2
#define FILE_ATTRIBUTE_NORMAL 0x00000080UL
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef struct _STARTUPINFOW {
    DWORD cb;
    LPWSTR lpReserved;
    LPWSTR lpDesktop;
    LPWSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    BYTE* lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOW;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION;

DLLIMPORT DWORD WINAPI GetModuleFileNameW(HANDLE, LPWSTR, DWORD);
DLLIMPORT BOOL WINAPI CreateProcessW(LPCWSTR, LPWSTR, LPVOID, LPVOID, BOOL, DWORD, LPVOID, LPCWSTR, STARTUPINFOW*, PROCESS_INFORMATION*);
DLLIMPORT HANDLE WINAPI CreateFileW(LPCWSTR, DWORD, DWORD, LPVOID, DWORD, DWORD, HANDLE);
DLLIMPORT BOOL WINAPI WriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, LPVOID);
DLLIMPORT BOOL WINAPI CloseHandle(HANDLE);
DLLIMPORT void WINAPI ExitProcess(DWORD);
DLLIMPORT int WINAPI MessageBoxW(HWND, LPCWSTR, LPCWSTR, unsigned int);

static void zero_bytes(void* p, unsigned long long n) {
    BYTE* b = (BYTE*)p;
    while (n--) *b++ = 0;
}

static void copy_w(wchar_t* dst, const wchar_t* src, unsigned int cap) {
    unsigned int i = 0;
    if (!cap) return;
    while (i + 1 < cap && src[i]) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}

static void dirname_w(wchar_t* path) {
    unsigned int i = 0, last = 0;
    while (path[i]) {
        if (path[i] == L'\\' || path[i] == L'/') last = i;
        ++i;
    }
    path[last + 1] = 0;
}

static void append_w(wchar_t* dst, const wchar_t* src, unsigned int cap) {
    unsigned int d = 0, s = 0;
    while (d + 1 < cap && dst[d]) ++d;
    while (d + 1 < cap && src[s]) dst[d++] = src[s++];
    dst[d] = 0;
}

static void ensure_steam_appid(const wchar_t* root) {
    wchar_t path[MAX_PATH];
    DWORD written = 0;
    static const char appid[] = "2552430\r\n";
    copy_w(path, root, MAX_PATH);
    append_w(path, L"steam_appid.txt", MAX_PATH);
    HANDLE f = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return;
    WriteFile(f, appid, 9, &written, NULL);
    CloseHandle(f);
}

void WINAPI LauncherEntry(void) {
    wchar_t root[MAX_PATH];
    wchar_t game[MAX_PATH];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    zero_bytes(&si, sizeof(si));
    zero_bytes(&pi, sizeof(pi));
    si.cb = sizeof(si);

    root[0] = 0;
    if (!GetModuleFileNameW(NULL, root, MAX_PATH)) {
        MessageBoxW(NULL, L"Could not determine the Luvvy launcher folder.", L"Luvvy KH2 Launcher", MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }
    dirname_w(root);
    ensure_steam_appid(root);

    copy_w(game, root, MAX_PATH);
    append_w(game, L"KINGDOM HEARTS II FINAL MIX.exe", MAX_PATH);

    if (!CreateProcessW(game, NULL, NULL, NULL, FALSE, 0, NULL, root, &si, &pi)) {
        MessageBoxW(NULL,
                    L"Could not launch KINGDOM HEARTS II FINAL MIX.exe.\n\nKeep this launcher in the KH 1.5+2.5 game folder and make sure Steam is running.",
                    L"Luvvy KH2 Launcher",
                    MB_OK | MB_ICONERROR);
        ExitProcess(2);
    }

    if (pi.hThread) CloseHandle(pi.hThread);
    if (pi.hProcess) CloseHandle(pi.hProcess);
    ExitProcess(0);
}
