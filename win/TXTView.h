// View.h : interface of the CTxtView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#define IsSpaceCharacter(p)		(((p) == L' ') || ((p) == L'\t') || ((p) == L'\n') || ((p) == L'\r'))
#define IsAlphabet(p)		((((p) >= L'0') && ((p) <= L'9')) || (((p) >= L'a') && ((p) <= L'z')) || (((p) >= L'A') && ((p) <= L'Z')))

WCHAR url[MAX_PATH + 1] = { 0 };

WCHAR greeting_[] =
{
	0xd83d,0xde42,0x000a,0x0048,0x0065,0x006c,0x006c,0x006f,
	0x0021,0x0020,0x0049,0x0066,0x0020,0x0079,0x006f,0x0075,
	0x0020,0x0068,0x0061,0x0076,0x0065,0x0020,0x0061,0x006e,
	0x0079,0x0020,0x0071,0x0075,0x0065,0x0073,0x0074,0x0069,
	0x006f,0x006e,0x002c,0x0020,0x0070,0x006c,0x0065,0x0061,
	0x0073,0x0065,0x0020,0x0074,0x0079,0x0070,0x0065,0x0020,
	0x0069,0x006e,0x0020,0x0074,0x0068,0x0065,0x0020,0x0062,
	0x0065,0x006c,0x006f,0x0077,0x0020,0x0077,0x0069,0x006e,
	0x0064,0x006f,0x0077,0x0020,0x0061,0x006e,0x0064,0x0020,
	0x0070,0x0072,0x0065,0x0073,0x0073,0x0020,0x0045,0x004e,
	0x0054,0x0045,0x0052,0x0020,0x006b,0x0065,0x0079,0x002e,
	0x000a,0x000a,0x0000
};

#define TEXT_BUFFER_BLOCK		(1<<20)

#define DECLARE_WND_CLASS_TEXT(WndClassName) \
static ATL::CWndClassInfo& GetWndClassInfo() \
{ \
	static ATL::CWndClassInfo wc = \
	{ \
		{ sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, StartWindowProc, \
		  0, 0, NULL, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, WndClassName, NULL }, \
		NULL, NULL, IDC_IBEAM, TRUE, 0, _T("") \
	}; \
	return wc; \
}

// look for "https://"
U32 FindWordInText(WCHAR* data, U32 length, U32 pos, U32& startPos, U32& endPos)
{
	if (pos < length && data)
	{
		WCHAR* p;
		U32 i;
		i = pos;
		p = data + pos;
		while (i > 0)
		{
			if (!IsAlphabet(*p))
				break;
				p--;
				i--;
		}
		startPos = i+1;
		i = pos;
		p = data + pos;
		while (i < length)
		{
			if (!IsAlphabet(*p))
				break;
			p++;
			i++;
		}
		endPos = i;
	}
	return 0;
}

// look for "https://"
bool IsLinkText(WCHAR* data, U32 length, U32 pos, U32& startPos, U32& endPos)
{
	bool bRet = false;
	startPos = 0;
	endPos = 0;
	if (data&& pos < length)
	{
		if (data[pos] != L' ' && data[pos] != L'\t' && data[pos] != L'\n' && data[pos] != L'\r')
		{
			U32 idx;
			for (idx = pos; idx > 0; idx--)
			{
				if (data[idx] == L' ' || data[idx] == L'\t' || data[idx] == L'\n' || data[idx] == L'\r')
					break;
			}
			if (data[idx] == L' ' || data[idx] == L'\t' || data[idx] == L'\n' || data[idx] == L'\r')
				idx++;
			if (idx < length - 10)
			{
				if (data[idx + 0] == L'h' &&
					data[idx + 1] == L't' &&
					data[idx + 2] == L't' &&
					data[idx + 3] == L'p' &&
					data[idx + 4] == L's' &&
					data[idx + 5] == L':' &&
					data[idx + 6] == L'/' &&
					data[idx + 7] == L'/')
				{
					startPos = idx;
					for (idx = startPos; idx < length; idx++)
					{
						if (data[idx] == L' ' || data[idx] == L'\t' || data[idx] == L'\n' || data[idx] == L'\r')
							break;
					}
					endPos = idx - 1;
					bRet = true;
				}
			}
		}
	}
	return bRet;
}

enum SetSelectionMode
{
	SetSelectionModeLeft,               // cluster left
	SetSelectionModeRight,              // cluster right
	SetSelectionModeUp,                 // line up
	SetSelectionModeDown,               // line down
	SetSelectionModeLeftChar,           // single character left (backspace uses it)
	SetSelectionModeRightChar,          // single character right
	SetSelectionModeLeftWord,           // single word left
	SetSelectionModeRightWord,          // single word right
	SetSelectionModeHome,               // front of line
	SetSelectionModeEnd,                // back of line
	SetSelectionModeFirst,              // very first position
	SetSelectionModeLast,               // very last position
	SetSelectionModeAbsoluteLeading,    // explicit position (for mouse click)
	SetSelectionModeAbsoluteTrailing,   // explicit position, trailing edge
	SetSelectionModeAll                 // select all text
};

class CTxtView : public CScrollWindowImpl<CTxtView>
{
	////////////////////
	// Selection/Caret navigation
	///
	// caretAnchor equals caretPosition when there is no selection.
	// Otherwise, the anchor holds the point where shift was held
	// or left drag started.
	//
	// The offset is used as a sort of trailing edge offset from
	// the caret position. For example, placing the caret on the
	// trailing side of a surrogate pair at the beginning of the
	// text would place the position at zero and offset of two.
	// So to get the absolute leading position, sum the two.
	U32 m_caretAnchor = 0;
	U32 m_caretPosition = 0;
	U32 m_caretPositionOffset = 0;    // > 0 used for trailing edge

	////////////////////
	// Mouse manipulation
	bool m_currentlySelecting : 1;
	bool currentlyPanning_ : 1;
	float previousMouseX = 0;
	float previousMouseY = 0;

	enum { MouseScrollFactor = 10 };

	////////////////////
	// Current view
	float scaleX_;          // horizontal scaling
	float scaleY_;          // vertical scaling
	float angle_;           // in degrees
	float originX_;         // focused point in document (moves on panning and caret navigation)
	float originY_;
	float contentWidth_;    // page size - margin left - margin right (can be fixed or variable)
	float contentHeight_;

	WCHAR* m_dataBuf = nullptr;
	U32    m_dataLen = 0;
	U32    m_dataMax = 0;
	HCURSOR m_hCursor;
	bool  m_CursorChanged = false;

	IDWriteTextFormat* m_textFormat = nullptr;
	IDWriteTextLayout* m_textLayout = nullptr;

	//RenderTarget* renderTarget_ = nullptr;
	IDWriteFactory* dwriteFactory_ = nullptr;
	ID2D1Factory* d2dFactory_ = nullptr;

	ID2D1HwndRenderTarget* m_pD2DRenderTarget = nullptr;
	ID2D1Bitmap* m_pixelBitmap = nullptr;
	ID2D1SolidColorBrush* m_pBrush = nullptr;
	ID2D1SolidColorBrush* m_pBrushText = nullptr;
	float m_deviceScaleFactor = 1.f;

	int m_lineHeight = 18;
public:
	DECLARE_WND_CLASS_TEXT(NULL)

	CTxtView()
	{
		d2dFactory_ = g_pD2DFactory;
		dwriteFactory_ = g_pDWriteFactory;

		m_caretPosition = 0;
		m_caretAnchor = 0;
		m_caretPositionOffset = 0;

		m_currentlySelecting = false;
		currentlyPanning_ = false;
		previousMouseX = 0;
		previousMouseY = 0;

		scaleX_ = 1;
		scaleY_ = 1;
		angle_ = 0;
		originX_ = 0;
		originY_ = 0;

		m_dataLen = 0;
		m_dataMax = TEXT_BUFFER_BLOCK;
		m_dataBuf = (WCHAR*)VirtualAlloc(NULL, m_dataMax, MEM_COMMIT, PAGE_READWRITE);
		ATLASSERT(m_dataBuf);
	}

	~CTxtView()
	{
		if (m_dataBuf)
		{
			VirtualFree(m_dataBuf, 0, MEM_RELEASE);
			m_dataBuf = nullptr;
			m_dataLen = 0;
			m_dataMax = 0;
		}
	}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	int GetFirstIntegralMultipleDeviceScaleFactor() const noexcept
	{
		return static_cast<int>(std::ceil(m_deviceScaleFactor));
	}

	D2D1_SIZE_U GetSizeUFromRect(const RECT& rc, const int scaleFactor) noexcept
	{
		const long width = rc.right - rc.left;
		const long height = rc.bottom - rc.top;
		const U32 scaledWidth = width * scaleFactor;
		const U32 scaledHeight = height * scaleFactor;
		return D2D1::SizeU(scaledWidth, scaledHeight);
	}

	HRESULT CreateDeviceDependantResource(int left, int top, int right, int bottom)
	{
		U8 result = 0;
		HRESULT hr = S_OK;
		if (nullptr == m_pD2DRenderTarget)  // Create a Direct2D render target.
		{
			RECT rc = { 0 };
			const int integralDeviceScaleFactor = GetFirstIntegralMultipleDeviceScaleFactor();
			D2D1_RENDER_TARGET_PROPERTIES drtp{};
			drtp.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
			drtp.usage = D2D1_RENDER_TARGET_USAGE_NONE;
			drtp.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
			drtp.dpiX = 96.f * integralDeviceScaleFactor;
			drtp.dpiY = 96.f * integralDeviceScaleFactor;
			// drtp.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN);
			drtp.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);

			rc.left = left; rc.top = top; rc.right = right; rc.bottom = bottom;
			D2D1_HWND_RENDER_TARGET_PROPERTIES dhrtp{};
			dhrtp.hwnd = m_hWnd;
			dhrtp.pixelSize = GetSizeUFromRect(rc, integralDeviceScaleFactor);
			dhrtp.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

			ReleaseUnknown(m_pBrush);
			ReleaseUnknown(m_pBrushText);
			ReleaseUnknown(m_pixelBitmap);

			ATLASSERT(nullptr != g_pD2DFactory);

			//hr = g_pD2DFactory->CreateHwndRenderTarget(renderTargetProperties, hwndRenderTragetproperties, &m_pD2DRenderTarget);
			hr = g_pD2DFactory->CreateHwndRenderTarget(drtp, dhrtp, &m_pD2DRenderTarget);
			if (hr == S_OK && m_pD2DRenderTarget)
			{
				//D2D1::ColorF::LightSkyBlue
				hr = m_pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x111111), &m_pBrushText);
				if (hr == S_OK && m_pBrushText)
				{
					hr = m_pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x87CEFA), &m_pBrush);
				}
			}
		}
		return hr;
	}

	DWRITE_TEXT_RANGE GetSelectionRange()
	{
		// Returns a valid range of the current selection,
		// regardless of whether the caret or anchor is first.
		U32 caretBegin = m_caretAnchor;
		U32 caretEnd = m_caretPosition + m_caretPositionOffset;
		if (caretBegin > caretEnd)
			std::swap(caretBegin, caretEnd);

		// Limit to actual text length.
		U32 textLength = m_dataLen;
		caretBegin = std::min(caretBegin, textLength);
		caretEnd = std::min(caretEnd, textLength);

		DWRITE_TEXT_RANGE textRange = { caretBegin, caretEnd - caretBegin };
		return textRange;
	}

	void DoPaint(CDCHandle dc)
	{
		POINT pt = { 0 };
		RECT rc = { 0 };
		GetClientRect(&rc);
		GetScrollOffset(pt);

		HRESULT hr = CreateDeviceDependantResource(rc.left, rc.top, rc.right, rc.bottom);
		if (S_OK == hr && nullptr != m_pD2DRenderTarget)
		{
			m_pD2DRenderTarget->BeginDraw();
			////////////////////////////////////////////////////////////////////////////////////////////////////
			m_pD2DRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
			m_pD2DRenderTarget->Clear(D2D1::ColorF(0xFFFFFF));
			if (m_textLayout)
			{
				DWRITE_TEXT_RANGE caretRange = GetSelectionRange();
				U32 actualHitTestCount = 0;
				if (caretRange.length > 0)
				{
					m_textLayout->HitTestTextRange(
						caretRange.startPosition,
						caretRange.length,
						0, // x
						0, // y
						NULL,
						0, // metrics count
						&actualHitTestCount
					);
				}
				// Allocate enough room to return all hit-test metrics.
				std::vector<DWRITE_HIT_TEST_METRICS> hitTestMetrics(actualHitTestCount);

				if (caretRange.length > 0)
				{
					m_textLayout->HitTestTextRange(
						caretRange.startPosition,
						caretRange.length,
						0, // x
						0, // y
						&hitTestMetrics[0],
						static_cast<U32>(hitTestMetrics.size()),
						&actualHitTestCount
					);
				}

				// Draw the selection ranges behind the text.
				if (actualHitTestCount > 0)
				{
					// Note that an ideal layout will return fractional values,
					// so you may see slivers between the selection ranges due
					// to the per-primitive antialiasing of the edges unless
					// it is disabled (better for performance anyway).
					m_pD2DRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
					for (size_t i = 0; i < actualHitTestCount; ++i)
					{
						const DWRITE_HIT_TEST_METRICS& htm = hitTestMetrics[i];
						D2D1_RECT_F highlightRect = {
							htm.left,
							htm.top - pt.y,
							(htm.left + htm.width),
							(htm.top + htm.height - pt.y)
						};
						m_pD2DRenderTarget->FillRectangle(highlightRect, m_pBrush);
					}
					m_pD2DRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
				}

				m_pD2DRenderTarget->DrawTextLayout(D2D1::Point2F(-pt.x, -pt.y), m_textLayout,
					m_pBrushText, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
			}
			hr = m_pD2DRenderTarget->EndDraw();
			////////////////////////////////////////////////////////////////////////////////////////////////////
			if (FAILED(hr) || D2DERR_RECREATE_TARGET == hr)
			{
				ReleaseUnknown(m_pD2DRenderTarget);
			}
		}
	}

	BEGIN_MSG_MAP(CTxtView)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
		MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
		MESSAGE_HANDLER(WM_MOUSEHOVER, OnMouseHover)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_RBUTTONUP, OnRBtnUp)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLBtnDbClk)
		MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, OnEditSelectAll)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
		CHAIN_MSG_MAP(CScrollWindowImpl<CTxtView>)
	END_MSG_MAP()

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// handled, no background painting needed
		return 1;
	}

	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		ReleaseUnknown(m_textLayout);
		ReleaseUnknown(m_textFormat);
#if 1
		ReleaseUnknown(m_pBrush);
		ReleaseUnknown(m_pBrushText);
		ReleaseUnknown(m_pixelBitmap);
		ReleaseUnknown(m_pD2DRenderTarget);
#endif 
		//bHandled = FALSE;
		return 0;
	}

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		ATLASSERT(m_textFormat == nullptr);
		HRESULT hr = dwriteFactory_->CreateTextFormat(L"Courier New", NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			15.5f, L"en-US", &m_textFormat);

		if (hr == S_OK && m_textFormat)
		{
			size_t len = wcslen(greeting_);
			wchar_t strEnglish[2] = { 0x0058,0 };
			IDWriteTextLayout* pTextLayout = nullptr;
			hr = dwriteFactory_->CreateTextLayout(strEnglish, 1, m_textFormat, 600.f, 1.f, &pTextLayout);
			if (hr == S_OK && pTextLayout)
			{
				DWRITE_TEXT_METRICS tm = { 0 };
				if (S_OK == pTextLayout->GetMetrics(&tm))
				{
					m_lineHeight = static_cast<int>(tm.height) + 1;
				}
				ReleaseUnknown(pTextLayout);
			}
			m_dataLen = 0;
			for (size_t i = 0; i < len; i++)
			{
				m_dataBuf[i] = greeting_[i];
				m_dataLen++;
			}
			m_dataBuf[m_dataLen] = L'\0';
		}
		SetScrollSize(1, 1);
		SetScrollLine(1, m_lineHeight);
		SetScrollPage(1, m_lineHeight * 10);

		m_hCursor = ::LoadCursor(NULL, IDC_HAND);
		ATLASSERT(m_hCursor);
		return 0;
	}

	LRESULT OnSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if (wParam != SIZE_MINIMIZED)
		{
			RECT rc = { 0 };
			GetClientRect(&rc);
			if (rc.bottom && rc.right)
			{
				if (m_dataLen && m_dataBuf && m_textFormat)
				{
					ReleaseUnknown(m_textLayout);
					dwriteFactory_->CreateTextLayout(m_dataBuf, m_dataLen, m_textFormat,
						static_cast<FLOAT>(rc.right - rc.left), 1.f,
						&m_textLayout);
					if (m_textLayout)
					{
						int hI;
						DWRITE_TEXT_METRICS textMetrics;
						MakeLinkTextUnderline();
						m_textLayout->GetMetrics(&textMetrics);
						float hf = textMetrics.layoutHeight;
						if (hf < textMetrics.height)
							hf = textMetrics.height;

						hI = static_cast<int>(hf + 1);
						if (hI > rc.bottom - rc.top)
							SetScrollSize(1, hI);
					}
				}
			}
			ReleaseUnknown(m_pD2DRenderTarget);
			Invalidate();
		}
		return 1;
	}

	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		POINT pt = { 0 };
		GetScrollOffset(pt);
#if 10
		float xPos = static_cast<float>(GET_X_LPARAM(lParam) + pt.x);
		float yPos = static_cast<float>(GET_Y_LPARAM(lParam) + pt.y);
#else
		float xPos = static_cast<float>(GET_X_LPARAM(lParam));
		float yPos = static_cast<float>(GET_Y_LPARAM(lParam));
#endif 
		m_CursorChanged = false;
		if (m_textLayout)
		{
			BOOL isTrailingHit = FALSE;
			BOOL isInside = FALSE;
			DWRITE_HIT_TEST_METRICS cm = { 0 };

			HRESULT hr = m_textLayout->HitTestPoint(xPos, yPos, &isTrailingHit, &isInside, &cm);
			if (hr == S_OK)
			{
				U32 s = 0, e = 0;
				if (IsLinkText(m_dataBuf, m_dataLen, cm.textPosition, s, e))
				{
					::SetCursor(m_hCursor);
					m_CursorChanged = true;
				}
#if 10
				if (e - s > 0)
				{
					U8 i = 0;
					for (i = 0; i <= (e - s); i++)
						url[i] = m_dataBuf[s + i];
					url[i] = L'\0';
				}
				else
				{
					swprintf((wchar_t*)url, MAX_PATH, L"XEdit - x:%f, y:%f tp[%u],len[%u],left[%f],top[%f],w[%f],h[%f],bi[%u],it[%d],trim[%d]",
						xPos, yPos,
						cm.textPosition,
						cm.length,
						cm.left,
						cm.top,
						cm.width,
						cm.height,
						cm.bidiLevel,
						cm.isText,
						cm.isTrimmed
					);
				}
				if (::IsWindow(g_hWnd))
				{
					HWND hWnd = GetTopWindow();
					::PostMessage(g_hWnd, WM_FROM_CHILD_WIN_MSG, (WPARAM)url, 0);
				}
#endif 
			}
		}

		if (m_currentlySelecting)
		{
			// Drag current selection.
			//SetSelectionFromPoint(xPos, yPos, true);
			BOOL isTrailingHit = FALSE;
			BOOL isInside = FALSE;
			DWRITE_HIT_TEST_METRICS cm = { 0 };
			HRESULT hr = m_textLayout->HitTestPoint(xPos, yPos, &isTrailingHit, &isInside, &cm);
			if (hr == S_OK)
			{
				m_caretPosition = cm.textPosition;
				m_caretPositionOffset = 0;
				Invalidate();
			}
		}
		return 0;
	}

	LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_CursorChanged == false)
		{
			bHandled = FALSE;
		}
		m_CursorChanged = false;
		return 0;
	}

	LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		if (delta >= 0)
		{
			ScrollLineUp();
		}
		else
		{
			ScrollLineDown();
		}
		return 1;
	}

	LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 1;
	}

	LRESULT OnMouseHover(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 1;
	}

	LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		POINT pt = { 0 };
		GetScrollOffset(pt);
		float xPos = static_cast<float>(GET_X_LPARAM(lParam) + pt.x);
		float yPos = static_cast<float>(GET_Y_LPARAM(lParam) + pt.y);

		SetFocus();
		SetCapture();

		// Start dragging selection.
		m_currentlySelecting = true;
		bool heldShift = (GetKeyState(VK_SHIFT) & 0x80) != 0;
		//SetSelectionFromPoint(xPos, yPos, heldShift);
		if (m_textLayout)
		{
			BOOL isTrailingHit = FALSE;
			BOOL isInside = FALSE;
			DWRITE_HIT_TEST_METRICS cm = { 0 };
			HRESULT hr = m_textLayout->HitTestPoint(xPos, yPos, &isTrailingHit, &isInside, &cm);
			if (hr == S_OK)
			{
				m_caretAnchor = cm.textPosition;
				m_caretPosition = m_caretAnchor;
				m_caretPositionOffset = 0;
				Invalidate();
			}
		}
		return 1;
	}

	LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		POINT pt = { 0 };
		GetScrollOffset(pt);
		float xPos = static_cast<float>(GET_X_LPARAM(lParam) + pt.x);
		float yPos = static_cast<float>(GET_Y_LPARAM(lParam) + pt.y);
#if 10
		ReleaseCapture();
		m_CursorChanged = false;
		if (m_textLayout)
		{
			BOOL isTrailingHit = FALSE;
			BOOL isInside = FALSE;
			DWRITE_HIT_TEST_METRICS cm = { 0 };

			HRESULT hr = m_textLayout->HitTestPoint(xPos, yPos, &isTrailingHit, &isInside, &cm);
			if (hr == S_OK)
			{
				U32 s = 0, e = 0;
				if (IsLinkText(m_dataBuf, m_dataLen, cm.textPosition, s, e))
				{
					::SetCursor(m_hCursor);
					m_CursorChanged = true;
				}
				if (e - s > 0)
				{
					U8 i = 0;
					for (i = 0; i <= (e - s); i++)
					{
						url[i] = m_dataBuf[s + i];
					}
					url[i] = L'\0';
					ShellExecute(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
#if 0
					MessageBox(L"LINK", L"LINK", MB_OK);
#endif 
				}
				m_caretPosition = cm.textPosition;
				m_caretPositionOffset = 0;
				Invalidate();
			}
		}
#endif 
		//MirrorXCoordinate(x);

		m_currentlySelecting = false;
		return 0;
	}

	LRESULT OnLBtnDbClk(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		POINT pt = { 0 };
		GetScrollOffset(pt);
		float xPos = static_cast<float>(GET_X_LPARAM(lParam) + pt.x);
		float yPos = static_cast<float>(GET_Y_LPARAM(lParam) + pt.y);

		if (m_textLayout)
		{
			BOOL isTrailingHit = FALSE;
			BOOL isInside = FALSE;
			DWRITE_HIT_TEST_METRICS cm = { 0 };

			HRESULT hr = m_textLayout->HitTestPoint(xPos, yPos, &isTrailingHit, &isInside, &cm);
			if (hr == S_OK && cm.textPosition < m_dataLen)
			{
				U32 s = 0, e = 0;
				FindWordInText(m_dataBuf, m_dataLen, cm.textPosition, s, e);
				m_caretAnchor = s;
				m_caretPosition = e;
				m_caretPositionOffset = 0;
				Invalidate();
			}
		}
		return 0;
	}

	LRESULT OnRBtnUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		bool bSelected = (m_caretAnchor != m_caretPosition + m_caretPositionOffset) ? true : false;
		bool bHaveContext = (m_dataLen) ? true : false;

		if (bSelected || bHaveContext)
		{
			POINT pt;
			HMENU hMenu;
			GetCursorPos(&pt);
			hMenu = CreatePopupMenu();

			if(bSelected)
				AppendMenu(hMenu, MF_STRING | MF_ENABLED, ID_EDIT_COPY, _T("Copy"));
			if(bHaveContext)
				AppendMenu(hMenu, MF_STRING | MF_ENABLED, ID_EDIT_SELECT_ALL, _T("Select All"));
			TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
		}
		return 0;
	}

	int AppendText(const char* text, U32 length)
	{
		int r = 0;
		if (text && length && m_dataBuf)
		{
			U32 utf16len = 0;
			U32 status = wt_UTF8ToUTF16((U8*)text, length, NULL, &utf16len);
			if (status == WT_OK && utf16len && utf16len < m_dataMax - m_dataLen - 1)
			{
				RECT rc;
				GetClientRect(&rc);
				WCHAR* p = m_dataBuf + m_dataLen;

				status = wt_UTF8ToUTF16((U8*)text, length, (U16*)p, NULL);
				ATLASSERT(status == WT_OK);
				m_dataLen += utf16len;
				m_dataBuf[m_dataLen] = L'\0';

				ReleaseUnknown(m_textLayout);
				ATLASSERT(m_textFormat);
				dwriteFactory_->CreateTextLayout(m_dataBuf, m_dataLen, m_textFormat,
					static_cast<FLOAT>(rc.right - rc.left), 1.f,
					&m_textLayout);

				if (m_textLayout)
				{
					int hI;
					DWRITE_TEXT_METRICS textMetrics;

					MakeLinkTextUnderline();
					m_textLayout->GetMetrics(&textMetrics);
					float hf = textMetrics.layoutHeight;
					if (hf < textMetrics.height)
						hf = textMetrics.height;

					hI = static_cast<int>(hf + 1);
					if (hI > rc.bottom - rc.top)
						SetScrollSize(1, hI);
				}
				Invalidate();
			}
		}
		return r;
	}

	// search https://
	void MakeLinkTextUnderline()
	{
		WCHAR* w;
		WCHAR* p = m_dataBuf;
		U32 i, pos = 0;
		DWRITE_TEXT_RANGE tr = { 0 };

		WCHAR* q = wcsstr(p, L"https://");
		while (q)
		{
			pos = q - m_dataBuf;
			w = q + 8;
			while (*w != L'\0' && *w != L' ' && *w != L'\t' && *w != L'\n' && *w != L'\r' && *w != L'"' && *w != L'\'')
				w++;

			tr.startPosition = pos;
			tr.length = w - q;
			m_textLayout->SetUnderline(TRUE, tr);

			if (p < m_dataBuf + m_dataLen)
			{
				p = q + 1;
				q = wcsstr(p, L"https://");
			}
			else
			{
				q = nullptr;
				break;
			}
		}
	}

	bool DoEditCopy()
	{
		return true;
	}

	LRESULT OnEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ATLASSERT(m_dataLen);
		m_caretAnchor = 0;
		m_caretPosition = m_dataLen - 1;
		m_caretPositionOffset = 0;
		
		Invalidate();

		return true;
	}

	LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ATLASSERT(m_dataLen);
		DWRITE_TEXT_RANGE selectionRange = GetSelectionRange();

		if (selectionRange.length > 0)
		{
			// Open and empty existing contents.
			if (OpenClipboard())
			{
				if (EmptyClipboard())
				{
					// Allocate room for the text
					size_t byteSize = sizeof(wchar_t) * (selectionRange.length + 1);
					HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, byteSize);
					if (hClipboardData != NULL)
					{
						void* memory = GlobalLock(hClipboardData);  // [byteSize] in bytes

						if (memory != NULL)
						{
							// Copy text to memory block.
							memcpy(memory, m_dataBuf + selectionRange.startPosition, byteSize);
							GlobalUnlock(hClipboardData);
							if (SetClipboardData(CF_UNICODETEXT, hClipboardData) != NULL)
							{
								hClipboardData = NULL; // system now owns the clipboard, so don't touch it.
							}
						}
						GlobalFree(hClipboardData); // free if failed
					}
				}
				CloseClipboard();
			}
		}
		return true;
	}
};
