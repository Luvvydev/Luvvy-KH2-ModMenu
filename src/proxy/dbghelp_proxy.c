#include "../winmini.h"

typedef BOOL (WINAPI *MiniDumpWriteDumpFn)(HANDLE, DWORD, HANDLE, DWORD, LPVOID, LPVOID, LPVOID);

static HMODULE g_self = NULL;
static HMODULE g_real_dbghelp = NULL;
static MiniDumpWriteDumpFn g_real_mini_dump = NULL;

/* Keep a real base relocation in the image so ASLR can move this DLL safely. */
__declspec(dllexport) void* g_luvvy_proxy_reloc_anchor = (void*)&g_self;

static unsigned int str_len_a(const char* s) {
    unsigned int n = 0;
    while (s && s[n]) ++n;
    return n;
}

static void path_dirname_w(wchar_t* path) {
    unsigned int i = 0, last = 0;
    while (path[i]) {
        if (path[i] == L'\\' || path[i] == L'/') last = i;
        ++i;
    }
    path[last + 1] = 0;
}

static void path_append_w(wchar_t* dst, const wchar_t* src, unsigned int cap) {
    unsigned int d = 0, s = 0;
    while (d + 1 < cap && dst[d]) ++d;
    while (d + 1 < cap && src[s]) dst[d++] = src[s++];
    dst[d] = 0;
}

static void write_proxy_log(const char* text) {
    wchar_t path[MAX_PATH];
    DWORD written = 0;
    path[0] = 0;
    if (!GetModuleFileNameW(g_self, path, MAX_PATH)) return;
    path_dirname_w(path);
    path_append_w(path, L"KH2Proxy.log", MAX_PATH);
    HANDLE f = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return;
    WriteFile(f, text, str_len_a(text), &written, NULL);
    CloseHandle(f);
}


static void write_proxy_error_code(DWORD code) {
    char buf[64];
    const char prefix[] = "[proxy] LoadLibraryW error: ";
    unsigned int i = 0;
    while (prefix[i]) { buf[i] = prefix[i]; ++i; }
    char digits[16];
    unsigned int n = 0;
    if (code == 0) digits[n++] = '0';
    while (code && n < sizeof(digits)) {
        digits[n++] = (char)('0' + (code % 10));
        code /= 10;
    }
    while (n) buf[i++] = digits[--n];
    buf[i++] = '\r'; buf[i++] = '\n'; buf[i] = 0;
    write_proxy_log(buf);
}

static void load_real_dbghelp(void) {
    if (g_real_dbghelp) return;
    wchar_t path[MAX_PATH];
    UINT n = GetSystemDirectoryW(path, MAX_PATH);
    if (!n || n >= MAX_PATH - 13) return;
    if (path[n - 1] != L'\\') path[n++] = L'\\';
    path[n] = 0;
    path_append_w(path, L"dbghelp.dll", MAX_PATH);
    g_real_dbghelp = LoadLibraryW(path);
    if (g_real_dbghelp) {
        g_real_mini_dump = (MiniDumpWriteDumpFn)GetProcAddress(g_real_dbghelp, "MiniDumpWriteDump");
    }
}

static DWORD WINAPI proxy_worker(LPVOID unused) {
    (void)unused;
    Sleep(100);
    write_proxy_log("[proxy] loaded\r\n");
    load_real_dbghelp();
    if (g_real_mini_dump) write_proxy_log("[proxy] system dbghelp ready\r\n");
    else write_proxy_log("[proxy] WARNING: system dbghelp MiniDumpWriteDump not resolved\r\n");

    wchar_t path[MAX_PATH];
    path[0] = 0;
    if (GetModuleFileNameW(g_self, path, MAX_PATH)) {
        path_dirname_w(path);
        path_append_w(path, L"KH2ModMenu.dll", MAX_PATH);
        HMODULE mod = LoadLibraryW(path);
        if (mod) write_proxy_log("[proxy] KH2ModMenu.dll loaded\r\n");
        else {
            DWORD err = GetLastError();
            write_proxy_log("[proxy] ERROR: KH2ModMenu.dll failed to load\r\n");
            write_proxy_error_code(err);
        }
    }
    return 0;
}

__declspec(dllexport) BOOL WINAPI MiniDumpWriteDump(HANDLE hProcess, DWORD processId, HANDLE hFile, DWORD dumpType, LPVOID exceptionParam, LPVOID userStreamParam, LPVOID callbackParam) {
    if (!g_real_mini_dump) load_real_dbghelp();
    if (!g_real_mini_dump) return FALSE;
    return g_real_mini_dump(hProcess, processId, hFile, dumpType, exceptionParam, userStreamParam, callbackParam);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(NULL, 0, proxy_worker, NULL, 0, NULL);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
