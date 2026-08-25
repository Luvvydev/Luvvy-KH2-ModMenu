#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define WINAPI __stdcall
#define CALLBACK __stdcall
#define APIENTRY WINAPI
#define DLLIMPORT __declspec(dllimport)

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned short wchar_t;

typedef void* HANDLE;
typedef HANDLE HINSTANCE;
typedef HINSTANCE HMODULE;
typedef HANDLE HWND;
typedef HANDLE HBRUSH;
typedef HANDLE HGDIOBJ;
typedef HANDLE HDC;
typedef HANDLE HCURSOR;
typedef HANDLE HICON;
typedef HANDLE HMENU;
typedef const void* LPCVOID;
typedef void* LPVOID;
typedef const char* LPCSTR;
typedef char* LPSTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t* LPWSTR;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long LONG;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef unsigned long long ULONG_PTR;
typedef unsigned long long SIZE_T;
typedef long long LPARAM;
typedef unsigned long long WPARAM;
typedef long long LRESULT;
typedef int BOOL;
typedef unsigned int COLORREF;
typedef void* FARPROC;
typedef DWORD* LPDWORD;

typedef struct tagPOINT { LONG x; LONG y; } POINT;
typedef struct tagRECT { LONG left; LONG top; LONG right; LONG bottom; } RECT;
typedef struct tagPAINTSTRUCT {
    HDC hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    BYTE rgbReserved[32];
} PAINTSTRUCT;

typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
    DWORD lPrivate;
} MSG;

typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL (CALLBACK *WNDENUMPROC)(HWND, LPARAM);
typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);

typedef struct tagWNDCLASSEXW {
    UINT cbSize;
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCWSTR lpszMenuName;
    LPCWSTR lpszClassName;
    HICON hIconSm;
} WNDCLASSEXW;

#define TRUE 1
#define FALSE 0
#define MAX_PATH 260

#define DLL_PROCESS_ATTACH 1

#define WM_DESTROY 0x0002
#define WM_PAINT 0x000F
#define WM_CLOSE 0x0010
#define WM_ERASEBKGND 0x0014
#define WM_NCHITTEST 0x0084
#define HTTRANSPARENT ((LRESULT)-1)

#define PM_REMOVE 0x0001

#define WS_POPUP 0x80000000UL
#define WS_EX_TOPMOST 0x00000008UL
#define WS_EX_TOOLWINDOW 0x00000080UL
#define WS_EX_LAYERED 0x00080000UL
#define WS_EX_TRANSPARENT 0x00000020UL
#define WS_EX_NOACTIVATE 0x08000000UL

#define SW_HIDE 0
#define SW_SHOWNOACTIVATE 4

#define SWP_NOSIZE 0x0001
#define SWP_NOACTIVATE 0x0010
#define SWP_SHOWWINDOW 0x0040

#define LWA_ALPHA 0x00000002

#define DT_LEFT 0x00000000
#define DT_TOP 0x00000000
#define DT_SINGLELINE 0x00000020
#define DT_VCENTER 0x00000004
#define DT_NOPREFIX 0x00000800
#define DT_WORDBREAK 0x00000010

#define TRANSPARENT 1

#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_END 0x23
#define VK_SPACE 0x20
#define VK_CONTROL 0x11
#define VK_SHIFT 0x10
#define VK_W 0x57
#define VK_A 0x41
#define VK_S 0x53
#define VK_D 0x44
#define VK_UP 0x26
#define VK_DOWN 0x28
#define VK_RETURN 0x0D

#define GENERIC_WRITE 0x40000000UL
#define FILE_APPEND_DATA 0x00000004UL
#define FILE_SHARE_READ 0x00000001UL
#define OPEN_ALWAYS 4
#define FILE_ATTRIBUTE_NORMAL 0x00000080UL
#define FILE_END 2
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)

#define INFINITE 0xFFFFFFFFUL

#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)) | ((WORD)((BYTE)(g))) << 8 | (((DWORD)(BYTE)(b)) << 16)))
#define HWND_TOPMOST ((HWND)(long long)-1)

DLLIMPORT HMODULE WINAPI LoadLibraryW(LPCWSTR);
DLLIMPORT FARPROC WINAPI GetProcAddress(HMODULE, LPCSTR);
DLLIMPORT BOOL WINAPI FreeLibrary(HMODULE);
DLLIMPORT void WINAPI FreeLibraryAndExitThread(HMODULE, DWORD);
DLLIMPORT UINT WINAPI GetSystemDirectoryW(LPWSTR, UINT);
DLLIMPORT DWORD WINAPI GetModuleFileNameW(HMODULE, LPWSTR, DWORD);
DLLIMPORT HMODULE WINAPI GetModuleHandleW(LPCWSTR);
DLLIMPORT HANDLE WINAPI CreateThread(LPVOID, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
DLLIMPORT BOOL WINAPI CloseHandle(HANDLE);
DLLIMPORT void WINAPI Sleep(DWORD);
DLLIMPORT BOOL WINAPI DisableThreadLibraryCalls(HMODULE);
DLLIMPORT DWORD WINAPI GetCurrentProcessId(void);
DLLIMPORT DWORD WINAPI GetLastError(void);
DLLIMPORT HANDLE WINAPI CreateFileW(LPCWSTR, DWORD, DWORD, LPVOID, DWORD, DWORD, HANDLE);
DLLIMPORT BOOL WINAPI WriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, LPVOID);
DLLIMPORT DWORD WINAPI SetFilePointer(HANDLE, LONG, LONG*, DWORD);

DLLIMPORT BOOL WINAPI EnumWindows(WNDENUMPROC, LPARAM);
DLLIMPORT BOOL WINAPI IsWindowVisible(HWND);
DLLIMPORT DWORD WINAPI GetWindowThreadProcessId(HWND, LPDWORD);
DLLIMPORT BOOL WINAPI GetWindowRect(HWND, RECT*);
DLLIMPORT unsigned short WINAPI RegisterClassExW(const WNDCLASSEXW*);
DLLIMPORT HWND WINAPI CreateWindowExW(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
DLLIMPORT BOOL WINAPI DestroyWindow(HWND);
DLLIMPORT LRESULT WINAPI DefWindowProcW(HWND, UINT, WPARAM, LPARAM);
DLLIMPORT BOOL WINAPI ShowWindow(HWND, int);
DLLIMPORT BOOL WINAPI UpdateWindow(HWND);
DLLIMPORT BOOL WINAPI SetWindowPos(HWND, HWND, int, int, int, int, UINT);
DLLIMPORT BOOL WINAPI SetLayeredWindowAttributes(HWND, COLORREF, BYTE, DWORD);
DLLIMPORT BOOL WINAPI PeekMessageW(MSG*, HWND, UINT, UINT, UINT);
DLLIMPORT BOOL WINAPI TranslateMessage(const MSG*);
DLLIMPORT LRESULT WINAPI DispatchMessageW(const MSG*);
DLLIMPORT void WINAPI PostQuitMessage(int);
DLLIMPORT short WINAPI GetAsyncKeyState(int);
DLLIMPORT BOOL WINAPI InvalidateRect(HWND, const RECT*, BOOL);
DLLIMPORT HDC WINAPI BeginPaint(HWND, PAINTSTRUCT*);
DLLIMPORT BOOL WINAPI EndPaint(HWND, const PAINTSTRUCT*);
DLLIMPORT int WINAPI FillRect(HDC, const RECT*, HBRUSH);
DLLIMPORT int WINAPI DrawTextW(HDC, LPCWSTR, int, RECT*, UINT);
DLLIMPORT int WINAPI SetBkMode(HDC, int);
DLLIMPORT COLORREF WINAPI SetTextColor(HDC, COLORREF);

DLLIMPORT HBRUSH WINAPI CreateSolidBrush(COLORREF);
DLLIMPORT BOOL WINAPI DeleteObject(HGDIOBJ);

#ifdef __cplusplus
}
#endif
