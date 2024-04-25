// XEdit.h
#pragma once

#include "wt/wt_utils.h"
#include "wt/wt_sha256.h"
#include "wt/wt_unicode.h"
#include "wt/wt_mempool.h"
#include "wt/wt_render.h"

#include "config.h"
#include "network.h"

#define INPUT_BUF_INPUT_MAX     (1<<18)
/*
 * libCurl Proxy type. Please check: https://curl.se/libcurl/c/CURLOPT_PROXYTYPE.html
 * 0 - No Proxy
 * 1 - CURLPROXY_HTTP
 * 2 - CURLPROXY_HTTPS
 * 3 - CURLPROXY_HTTPS2
 * 4 - CURLPROXY_HTTP_1_0
 * 5 - CURLPROXY_SOCKS4
 * 6 - CURLPROXY_SOCKS4A
 * 7 - CURLPROXY_SOCKS5
 * 8 - CURLPROXY_SOCKS5_HOSTNAME
 */
#define AI_CURLPROXY_NO_PROXY          0
#define AI_CURLPROXY_HTTP              1
#define AI_CURLPROXY_HTTPS             2
#define AI_CURLPROXY_HTTPS2            3
#define AI_CURLPROXY_HTTP_1_0          4
#define AI_CURLPROXY_SOCKS4            5
#define AI_CURLPROXY_SOCKS4A           6
#define AI_CURLPROXY_SOCKS5            7
#define AI_CURLPROXY_SOCKS5_HOSTNAME   8
#define AI_CURLPROXY_TYPE_MAX          AI_CURLPROXY_SOCKS5_HOSTNAME


// Release an IUnknown* and set to nullptr.
// While IUnknown::Release must be noexcept, it isn't marked as such so produces
// warnings which are avoided by the catch.
template <class T>
inline void ReleaseUnknown(T*& ppUnknown) noexcept {
    if (ppUnknown) {
        try {
            ppUnknown->Release();
        }
        catch (...) {
            // Never occurs
        }
        ppUnknown = nullptr;
    }
}

/// Find a function in a DLL and convert to a function pointer.
/// This avoids undefined and conditionally defined behaviour.
template<typename T>
inline T DLLFunction(HMODULE hModule, LPCSTR lpProcName) noexcept {
    if (!hModule) {
        return nullptr;
    }
    FARPROC function = ::GetProcAddress(hModule, lpProcName);
    static_assert(sizeof(T) == sizeof(function));
    T fp{};
    memcpy(&fp, &function, sizeof(T));
    return fp;
}

extern HINSTANCE g_hInstance;
extern WCHAR g_dbFilePath[MAX_PATH + 1];

extern ID2D1Factory* g_pD2DFactory;
extern IDWriteFactory* g_pDWriteFactory;

extern UINT  g_nDPI;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
