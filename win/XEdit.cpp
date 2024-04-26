// XEdit.cpp : main source file for XEdit.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlscrl.h>

#include "resource.h"
#include "XEdit.h"
#include "TXTView.h"
#include "WorkView.h"
#include "InputView.h"
#include "View.h"
#include "aboutdlg.h"
#include "ConfDlg.h"
#include "MainFrm.h"

HWND g_hWnd = nullptr;
HINSTANCE g_hInstance = nullptr;
WCHAR     g_dbFilePath[MAX_PATH + 1] = { 0 };

/* Direct2D and DirectWrite factory */
ID2D1Factory* g_pD2DFactory = nullptr;
IDWriteFactory* g_pDWriteFactory = nullptr;
UINT g_nDPI = 96;

static D2D1_DRAW_TEXT_OPTIONS d2dDrawTextOptions = D2D1_DRAW_TEXT_OPTIONS_NONE;
static HMODULE hDLLD2D{};
static HMODULE hDLLDWrite{};

static XConfig X_CONFIGURATION = { 0 };

volatile LONG g_threadCount = 0;
volatile LONG g_Quit = 0;
volatile LONG g_QuitNetworkThread = 0;
volatile LONG g_NetworkStatus = 0;

CAppModule _Module;

static void LoadD2DOnce()
{
	DWORD loadLibraryFlags = 0;
	HMODULE kernel32 = ::GetModuleHandleW(L"kernel32.dll");

	if (kernel32)
	{
		if (::GetProcAddress(kernel32, "SetDefaultDllDirectories"))
		{
			// Availability of SetDefaultDllDirectories implies Windows 8+ or
			// that KB2533623 has been installed so LoadLibraryEx can be called
			// with LOAD_LIBRARY_SEARCH_SYSTEM32.
			loadLibraryFlags = LOAD_LIBRARY_SEARCH_SYSTEM32;
		}
	}

	typedef HRESULT(WINAPI* D2D1CFSig)(D2D1_FACTORY_TYPE factoryType, REFIID riid, CONST D2D1_FACTORY_OPTIONS* pFactoryOptions, IUnknown** factory);
	typedef HRESULT(WINAPI* DWriteCFSig)(DWRITE_FACTORY_TYPE factoryType, REFIID iid, IUnknown** factory);

	hDLLD2D = ::LoadLibraryEx(TEXT("D2D1.DLL"), 0, loadLibraryFlags);
	D2D1CFSig fnD2DCF = DLLFunction<D2D1CFSig>(hDLLD2D, "D2D1CreateFactory");

	if (fnD2DCF)
	{
		// A single threaded factory as Scintilla always draw on the GUI thread
		fnD2DCF(D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory),
			nullptr,
			reinterpret_cast<IUnknown**>(&g_pD2DFactory));
	}

	hDLLDWrite = ::LoadLibraryEx(TEXT("DWRITE.DLL"), 0, loadLibraryFlags);
	DWriteCFSig fnDWCF = DLLFunction<DWriteCFSig>(hDLLDWrite, "DWriteCreateFactory");
	if (fnDWCF)
	{
		const GUID IID_IDWriteFactory2 = // 0439fc60-ca44-4994-8dee-3a9af7b732ec
		{ 0x0439fc60, 0xca44, 0x4994, { 0x8d, 0xee, 0x3a, 0x9a, 0xf7, 0xb7, 0x32, 0xec } };

		const HRESULT hr = fnDWCF(DWRITE_FACTORY_TYPE_SHARED,
			IID_IDWriteFactory2,
			reinterpret_cast<IUnknown**>(&g_pDWriteFactory));

		if (SUCCEEDED(hr))
		{
			// D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
			d2dDrawTextOptions = static_cast<D2D1_DRAW_TEXT_OPTIONS>(0x00000004);
		}
		else
		{
			fnDWCF(DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory),
				reinterpret_cast<IUnknown**>(&g_pDWriteFactory));
		}
	}
}

static bool LoadD2D()
{
	static std::once_flag once;

	ReleaseUnknown(g_pD2DFactory);
	ReleaseUnknown(g_pDWriteFactory);

	std::call_once(once, LoadD2DOnce);

	return g_pDWriteFactory && g_pD2DFactory;
}

static int App_Init(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpstrCmdLine, int nCmdShow)
{
	int ret = 0;

	g_hInstance = hInstance;
	g_Quit = 0;           /* this is global signal to let some threads to quit gracefully */
	g_threadCount = 0;           /* we use this counter to be sure all threads exit gracefully in the end */
	g_nDPI = 96;

	if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) /* initialize libCURL */
		ret = 2;

	if (Scintilla_RegisterClasses(hInstance) == 0) /* initalize Scintialla */
		ret = 3;

	if (!LoadD2D())
		ret = 4;
#if	0
	if (zt_LoadConfiguration(hInstance, &X_CONFIGURATION, g_dbFilePath, MAX_PATH) == false)
		ret = 1;
#endif
#if 0
	g_ztConf = &ZTCONFIGURATION;

	g_NetworkStatus = AI_NETWORK_IS_BAD; /* at first we assume the network is not connectable */

	g_hWndTerm = nullptr;

	InitializeCriticalSection(&g_csSendMsg);  /* these two are Critial Sections to sync different threads */
	InitializeCriticalSection(&g_csReceMsg);
	InitializeCriticalSection(&g_csWorkMsg);

	if (zt_LoadConfiguration(hInstance, g_ztConf, g_dbFilePath, MAX_PATH) == false)
		ret = 1;

	if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) /* initialize libCURL */
		ret = 2;

	if (Scintilla_RegisterClasses(hInstance) == 0) /* initalize Scintialla */
		ret = 3;

	if (!LoadD2D())
		ret = 4;

	g_send_msg_MemPool = wt_mempool_create("SendMsgPool", ALLOCSET_DEFAULT_SIZES);
	if (g_send_msg_MemPool == nullptr)
		ret = 5;

	g_rece_msg_MemPool = wt_mempool_create("ReceMsgPool", ALLOCSET_DEFAULT_SIZES);
	if (g_rece_msg_MemPool == nullptr)
		ret = 6;

	g_ttyScreenMax = INPUT_BUF_SCREEN_MAX;
	g_ttyScreenBuf = (wchar_t*)VirtualAlloc(nullptr, g_ttyScreenMax, MEM_COMMIT, PAGE_READWRITE);
	if (g_ttyScreenBuf == nullptr)
		ret = 7;

	g_inputBuffer = (U8*)VirtualAlloc(nullptr, INPUT_BUF_INPUT_MAX, MEM_COMMIT, PAGE_READWRITE);
	if (g_inputBuffer == nullptr)
		ret = 8;

	PTerm_Init(hInstance, hPrevInstance, "", nCmdShow);
#endif
	return ret;
}

static void App_Term()
{
	UINT tries;

	/* tell all threads to quit gracefully */
	InterlockedIncrement(&g_Quit);

	/* wait the threads to quit */
	tries = 10;
	while (g_threadCount && tries > 0)
	{
		Sleep(1000);
		tries--;
	}

	ReleaseUnknown(g_pDWriteFactory);
	ReleaseUnknown(g_pD2DFactory);

	if (hDLLDWrite)
	{
		FreeLibrary(hDLLDWrite);
		hDLLDWrite = {};
	}
	if (hDLLD2D)
	{
		FreeLibrary(hDLLD2D);
		hDLLD2D = {};
	}

	Scintilla_ReleaseResources();
	curl_global_cleanup();

#if 0
	g_sendQueue = nullptr;
	g_receQueue = nullptr;
	wt_mempool_destroy(g_send_msg_MemPool);
	wt_mempool_destroy(g_rece_msg_MemPool);


	DeleteCriticalSection(&g_csSendMsg);
	DeleteCriticalSection(&g_csReceMsg);
	DeleteCriticalSection(&g_csWorkMsg);

	PTerm_Exit();
	ATLASSERT(g_inputBuffer);
	VirtualFree(g_inputBuffer, 0, MEM_RELEASE);
	g_inputBuffer = nullptr;

	ATLASSERT(g_ttyScreenBuf);
	VirtualFree(g_ttyScreenBuf, 0, MEM_RELEASE);
	g_ttyScreenBuf = nullptr;
	g_ttyScreenMax = 0;
#endif
}


int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain;

	if(wndMain.CreateEx() == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpstrCmdLine, _In_ int nCmdShow)
{
	int nRet = 0;
	HRESULT hRes = ::CoInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	nRet = App_Init(hInstance, hPrevInstance, lpstrCmdLine, nCmdShow);

	if (nRet == 0)
	{
		nRet = Run(lpstrCmdLine, nCmdShow);
	}

	App_Term();

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
