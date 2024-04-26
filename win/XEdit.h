// XEdit.h
#pragma once

#include "wt/wt_utils.h"
#include "wt/wt_sha256.h"
#include "wt/wt_unicode.h"
#include "wt/wt_mempool.h"
#include "wt/wt_render.h"
#include "wt/wt_aes256.h"

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

#define AI_PUB_KEY_LENGTH   33
#define AI_SEC_KEY_LENGTH   32
#define AI_HASH256_LENGTH   32
#define AI_SESSION_LENGTH   64
#define AI_NET_URL_LENGTH   256
#define AI_DATA_CACHE_LEN   MAX_PATH
#define AI_FONTNAMELENGTH   32

#define AI_NETWORK_IS_BAD   0
#define AI_NETWORK_IS_GOOD  1

#define AI_TIMER_ASKROB     999

#define AI_NETWORK_TIMEOUT_MIN_IN_SECONDS   5
#define AI_NETWORK_TIMEOUT_MAX_IN_SECONDS   600

#define AI_LAYOUT_LEFT      0
#define AI_LAYOUT_RIGHT     1

#define WM_NETWORK_THREAD_MSG  (WM_USER + 1)
#define WM_FROM_CHILD_WIN_MSG  (WM_USER + 2)

typedef struct TTYConfig
{
    U8* anything;
} TTYConfig;

#define AI_PROP_STARTUP     0x00000001
#define AI_PROP_IS_LEFT     0x00000002
#define AI_PROP_SSCREEN     0x00000004
#define AI_PROP_AUTOLOG     0x00000008
#define AI_DEFAULT_PROP     (AI_PROP_STARTUP | AI_PROP_IS_LEFT | AI_PROP_SSCREEN | AI_PROP_AUTOLOG)

typedef struct XConfig
{
    U32 property;
    U8  serverKey[AI_PUB_KEY_LENGTH];
    U8  my_pubKey[AI_PUB_KEY_LENGTH];
    U8  my_secKey[AI_SEC_KEY_LENGTH];
    U8  sessionId[AI_SESSION_LENGTH];

    U8  serverURL[AI_NET_URL_LENGTH + 1];
    U8  dataCache[AI_DATA_CACHE_LEN + 1];
    U8  proxy_Str[AI_NET_URL_LENGTH + 1];
    U8  font_Name[AI_FONTNAMELENGTH + 1];
    U8  font_Size;
    U16 networkTimout;
    U8  proxy_Type;
    TTYConfig ttyConf;
} XConfig;

typedef struct MessageTask
{
    struct MessageTask* next;
    volatile LONG  msg_state;
    U8* msg_body;
    U32 msg_length;
} MessageTask;

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

extern HWND g_hWnd;
extern HINSTANCE g_hInstance;
extern WCHAR g_dbFilePath[MAX_PATH + 1];

extern ID2D1Factory* g_pD2DFactory;
extern IDWriteFactory* g_pDWriteFactory;

extern UINT  g_nDPI;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

U32 zt_LoadConfiguration(HINSTANCE hInstance, XConfig* cf, WCHAR* path, U16 max_len);