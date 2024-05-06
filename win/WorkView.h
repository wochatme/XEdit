/////////////////////////////////////////////////////////////////////////////

#pragma once

struct PROCESS_INFO
{
	DWORD dwProcessId;
	HWND hWnd;
};

BOOL CALLBACK EnumWindowCallBack(HWND hWnd, LPARAM lParam)
{
	PROCESS_INFO* pProcessWindow = (PROCESS_INFO*)lParam;

	DWORD dwProcessId;
	GetWindowThreadProcessId(hWnd, &dwProcessId);

	if (pProcessWindow->dwProcessId == dwProcessId && IsWindowEnabled(hWnd) && GetParent(hWnd) == NULL)
	{
		pProcessWindow->hWnd = hWnd;
		return FALSE;
	}

	return TRUE;
}

class CWorkView
{
public:
    HWND m_hWnd = nullptr;
#if 10
	HWND Create(
		_In_opt_ HWND hWndParent,
		_In_ _U_RECT rect = NULL,
		_In_opt_z_ LPCTSTR szWindowName = NULL,
		_In_ DWORD dwStyle = 0,
		_In_ DWORD dwExStyle = 0,
		_In_ _U_MENUorID MenuOrID = 0U,
		_In_opt_ LPVOID lpCreateParam = NULL)
	{
		STARTUPINFO si = { 0 };
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi = { 0 };
		ATLASSERT(::IsWindow(m_hWnd) == FALSE);

		if (CreateProcess(L"C:\\Windows\\System32\\cmd.exe", NULL, NULL, NULL, false, 0, NULL, NULL, &si, &pi))
		{
			U32 tries = 10;
			PROCESS_INFO procwin;
			procwin.dwProcessId = pi.dwProcessId;
			procwin.hWnd = NULL;

			WaitForInputIdle(pi.hProcess, 100);

			EnumWindows(EnumWindowCallBack, (LPARAM)&procwin);
			while (NULL == procwin.hWnd && tries)
			{
				Sleep(100);
				tries--;
				EnumWindows(EnumWindowCallBack, (LPARAM)&procwin);
			}
			m_hWnd =  procwin.hWnd;
		}

		if (IsWindow())
		{
			LONG_PTR dwStyle;
			dwStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
			dwStyle &= ~(WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_OVERLAPPED | WS_SIZEBOX | WS_SYSMENU | WS_THICKFRAME);
			dwStyle |= WS_CHILD;
			::SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);
			::SetWindowLong(m_hWnd, GWL_STYLE, ::GetWindowLong(m_hWnd, GWL_STYLE) & ~WS_POPUP);
			::SetWindowLong(m_hWnd, GWL_STYLE, ::GetWindowLong(m_hWnd, GWL_STYLE) & WS_CHILD);
			::SetParent(m_hWnd, hWndParent);
			::ShowWindow(m_hWnd, SW_SHOW);
			::InvalidateRect(m_hWnd, NULL, 1);
			::UpdateWindow(m_hWnd);
		}

		return m_hWnd;
	}
#endif 
	operator HWND() const throw()
	{
		return m_hWnd;
	}

	BOOL IsWindow() const throw()
	{
		return ::IsWindow(m_hWnd);
	}

#if 0
	HWND Create(
		_In_opt_ HWND hWndParent,
		_In_ _U_RECT rect = NULL,
		_In_opt_z_ LPCTSTR szWindowName = NULL,
		_In_ DWORD dwStyle = 0,
		_In_ DWORD dwExStyle = 0,
		_In_ _U_MENUorID MenuOrID = 0U,
		_In_opt_ LPVOID lpCreateParam = NULL)
	{
		m_hWnd = CreateWindowExW(0, L"Scintilla", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
			0, 0, 16, 16, hWndParent, NULL, HINST_THISCOMPONENT, NULL);
		return m_hWnd;
	}
#endif
	BOOL SetWindowPos(
		HWND hWndInsertAfter,
		int  X,
		int  Y,
		int  cx,
		int  cy,
		UINT uFlags
	)
	{
		BOOL bRet = FALSE;
		if (IsWindow())
		{
			bRet = ::SetWindowPos(m_hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
		}
		return bRet;
	}

#if 0
	HWND GetWindowHandle()
	{
		return m_hWnd;
	}

	int sci_SetTechnology(int mode)
	{
		if (IsWindow())
		{
			::SendMessage(m_hWnd, SCI_SETTECHNOLOGY, mode, 0);
		}
		return 0;
	}

	int sci_getTextLength(U32& length)
	{
		length = 0;
		if (IsWindow())
		{
			length = (U32)::SendMessage(m_hWnd, SCI_GETTEXTLENGTH, 0, 0);
		}
		return 0;
	}

	int sci_SetText(const char* text, U32 length)
	{
		if (IsWindow() && text)
		{
			::SendMessage(m_hWnd, SCI_SETTEXT, 0, (LPARAM)text);
		}
		return 0;
	}

	int sci_AppendText(const char* text, U32 length)
	{
		if (IsWindow())
		{
			int readonly = (int)::SendMessage(m_hWnd, SCI_GETREADONLY, 0, 0);
			::SendMessage(m_hWnd, EM_SETREADONLY, FALSE, 0);
			::SendMessage(m_hWnd, SCI_APPENDTEXT, length, (LPARAM)text);
			if(readonly)
				::SendMessage(m_hWnd, SCI_GETREADONLY, TRUE, 0);
		}
		return 0;
	}

	int sci_SetCodePage(int codepage)
	{
		if (IsWindow())
		{
			::SendMessage(m_hWnd, SCI_SETCODEPAGE, codepage, 0);
		}
		return 0;
	}

	int sci_SetEOLMode(int eol)
	{
		if (IsWindow())
		{
			::SendMessage(m_hWnd, SCI_SETEOLMODE, eol, 0);
		}
		return 0;
	}

	int sci_SetWrapMode(int wrapmode)
	{
		if (IsWindow())
		{
			::SendMessage(m_hWnd, SCI_SETWRAPMODE, wrapmode, 0);
		}
		return 0;
	}

	int sci_SetReadOnly(BOOL readonly = TRUE)
	{
		if (IsWindow())
		{
			::SendMessage(m_hWnd, EM_SETREADONLY, readonly, 0);
		}
		return 0;
	}

	int sci_StyleSetFont(int style, const char* fontName)
	{
		if (IsWindow() && fontName)
		{
			::SendMessage(m_hWnd, SCI_STYLESETFONT, style, (LPARAM)fontName);
		}
		return 0;
	}

	int sci_StyleSetFontSize(int style, int pointSize)
	{
		if (IsWindow())
		{
			::SendMessage(m_hWnd, SCI_STYLESETSIZEFRACTIONAL, style, pointSize * 100);
		}
		return 0;
	}

	int sci_GetCurrentPosition()
	{
		int r = 0;
		if (IsWindow())
		{
			r = (int)::SendMessage(m_hWnd, SCI_GETCURRENTPOS, 0, 0);
		}
		return r;
	}

	char sci_GetCharAtPosition(int pos)
	{
		char ch = '\0';
		if (IsWindow())
		{
			ch = (char)::SendMessage(m_hWnd, SCI_GETCHARAT, pos, 0);
		}
		return ch;
	}
#endif

	bool DoEditCopy()
	{
		return true;
	}

};
