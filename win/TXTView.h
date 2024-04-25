// View.h : interface of the CTxtView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

WCHAR title[MAX_PATH + 1] = { 0 };
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

inline D2D1::Matrix3x2F& Cast(DWRITE_MATRIX& matrix)
{
	// DWrite's matrix, D2D's matrix, and GDI's XFORM
	// are all compatible.
	return *reinterpret_cast<D2D1::Matrix3x2F*>(&matrix);
}

inline DWRITE_MATRIX& Cast(D2D1::Matrix3x2F& matrix)
{
	return *reinterpret_cast<DWRITE_MATRIX*>(&matrix);
}

#ifndef M_PI
// No M_PI in math.h on windows? No problem.
#define M_PI 3.14159265358979323846
#endif

inline double DegreesToRadians(float degrees)
{
	return degrees * M_PI * 2.0f / 360.0f;
}

inline float GetDeterminant(DWRITE_MATRIX const& matrix)
{
	return matrix.m11 * matrix.m22 - matrix.m12 * matrix.m21;
}

void ComputeInverseMatrix(
	DWRITE_MATRIX const& matrix,
	OUT DWRITE_MATRIX& result
)
{
	// Used for hit-testing, mouse scrolling, panning, and scroll bar sizing.

	float invdet = 1.f / GetDeterminant(matrix);
	result.m11 = matrix.m22 * invdet;
	result.m12 = -matrix.m12 * invdet;
	result.m21 = -matrix.m21 * invdet;
	result.m22 = matrix.m11 * invdet;
	result.dx = (matrix.m21 * matrix.dy - matrix.dx * matrix.m22) * invdet;
	result.dy = (matrix.dx * matrix.m12 - matrix.m11 * matrix.dy) * invdet;
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
	UINT32 caretAnchor_ = 0;
	UINT32 caretPosition_ = 0;
	UINT32 caretPositionOffset_ = 0;    // > 0 used for trailing edge

	////////////////////
	// Mouse manipulation
	bool currentlySelecting_ : 1;
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

public:
	DECLARE_WND_CLASS_TEXT(NULL)

	CTxtView()
	{
		d2dFactory_ = g_pD2DFactory;
		dwriteFactory_ = g_pDWriteFactory;

		caretPosition_ = 0;
		caretAnchor_ = 0;
		caretPositionOffset_ = 0;

		currentlySelecting_ = false;
		currentlyPanning_ = false;
		previousMouseX = 0;
		previousMouseY = 0;

		scaleX_ = 1;
		scaleY_ = 1;
		angle_ = 0;
		originX_ = 0;
		originY_ = 0;

		m_dataLen = 0;
		m_dataMax = (1 << 20);
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
		const UINT32 scaledWidth = width * scaleFactor;
		const UINT32 scaledHeight = height * scaleFactor;
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
#if 0
		caretAnchor_ = 130;
		caretPosition_ = 250;
		caretPositionOffset_ = 0;
#endif 
		UINT32 caretBegin = caretAnchor_;
		UINT32 caretEnd = caretPosition_ + caretPositionOffset_;
		if (caretBegin > caretEnd)
			std::swap(caretBegin, caretEnd);

		// Limit to actual text length.
		UINT32 textLength = m_dataLen;
		caretBegin = std::min(caretBegin, textLength);
		caretEnd = std::min(caretEnd, textLength);

		DWRITE_TEXT_RANGE textRange = { caretBegin, caretEnd - caretBegin };
		return textRange;
	}

	void DoPaint(CDCHandle dc)
	{
#if 10
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
				UINT32 actualHitTestCount = 0;
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
						static_cast<UINT32>(hitTestMetrics.size()),
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
#endif 
	}

	BEGIN_MSG_MAP(CTxtView)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
		MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
		MESSAGE_HANDLER(WM_MOUSEHOVER, OnMouseHover)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
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
#if 0
		ATLASSERT(m_dataBuf == nullptr);

		m_dataLen = 0;
		m_dataMax = (1 << 20);
		m_dataBuf = (WCHAR*)VirtualAlloc(NULL, m_dataMax, MEM_COMMIT, PAGE_READWRITE);
		ATLASSERT(m_dataBuf);
		hr = CreateRenderTarget();
		ATLASSERT(hr == S_OK);
#endif
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
			ReleaseUnknown(m_textLayout);
			if (m_dataLen && m_dataBuf && m_textFormat)
			{
				dwriteFactory_->CreateTextLayout(m_dataBuf, m_dataLen, m_textFormat,
					static_cast<FLOAT>(rc.right - rc.left),
					static_cast<FLOAT>(rc.bottom - rc.top),
					&m_textLayout);
				MakeLinkTextUnderline();
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
#if 0
		float xPos = static_cast<float>(GET_X_LPARAM(lParam) - pt.x);
		float yPos = static_cast<float>(GET_Y_LPARAM(lParam) - pt.y);
#endif 
		float xPos = static_cast<float>(GET_X_LPARAM(lParam));
		float yPos = static_cast<float>(GET_Y_LPARAM(lParam));

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
#if 0
				if (e - s > 0)
				{
					U8 i = 0;
					for (i = 0; i <= (e - s); i++)
						title[i] = m_dataBuf[s + i];
					title[i] = L'\0';
				}
				else
				{
					swprintf((wchar_t*)title, MAX_PATH, L"XEdit - tp[%u],len[%u],left[%f],top[%f],w[%f],h[%f],bi[%u],it[%d],trim[%d]",
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
				::SetWindowTextW(GetParent(), title);
#endif 
			}
		}

		//MirrorXCoordinate(x);

		if (currentlySelecting_)
		{
			// Drag current selection.
			//SetSelectionFromPoint(xPos, yPos, true);
			BOOL isTrailingHit = FALSE;
			BOOL isInside = FALSE;
			DWRITE_HIT_TEST_METRICS cm = { 0 };
			HRESULT hr = m_textLayout->HitTestPoint(xPos, yPos, &isTrailingHit, &isInside, &cm);
			if (hr == S_OK)
			{
				caretPosition_ = cm.textPosition;
				caretPositionOffset_ = 0;
				Invalidate();
			}
		}
		else if (currentlyPanning_)
		{
			DWRITE_MATRIX matrix;
			GetInverseViewMatrix(&matrix);

			float xDif = xPos - previousMouseX;
			float yDif = yPos - previousMouseY;
			previousMouseX = xPos;
			previousMouseY = yPos;

			originX_ -= (xDif * matrix.m11 + yDif * matrix.m21);
			originY_ -= (xDif * matrix.m12 + yDif * matrix.m22);
			//ConstrainViewOrigin();

			//RefreshView();
			Invalidate();
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
		float xPos = static_cast<float>(GET_X_LPARAM(lParam));
		float yPos = static_cast<float>(GET_Y_LPARAM(lParam));

		SetFocus();
		SetCapture();

		// Start dragging selection.
		currentlySelecting_ = true;
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
				caretAnchor_ = cm.textPosition;
				caretPosition_ = caretAnchor_;
				caretPositionOffset_ = 0;
				Invalidate();
			}
		}
		return 1;
	}

	LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		float xPos = static_cast<float>(GET_X_LPARAM(lParam));
		float yPos = static_cast<float>(GET_Y_LPARAM(lParam));
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
#if 0
					U8 i = 0;
					for (i = 0; i <= (e - s); i++)
						title[i] = m_dataBuf[s + i];
					title[i] = L'\0';
#endif 
					MessageBox(L"LINK", L"LINK", MB_OK);
				}
				caretPosition_ = cm.textPosition;
				caretPositionOffset_ = 0;
				Invalidate();
			}
		}
#endif 
		//MirrorXCoordinate(x);

		currentlySelecting_ = false;
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
					static_cast<FLOAT>(rc.right - rc.left),
					static_cast<FLOAT>(1),
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

	void AlignCaretToNearestCluster(bool isTrailingHit = false, bool skipZeroWidth = false)
	{
		// Uses hit-testing to align the current caret position to a whole cluster,
		// rather than residing in the middle of a base character + diacritic,
		// surrogate pair, or character + UVS.

		DWRITE_HIT_TEST_METRICS hitTestMetrics;
		float caretX, caretY;

		// Align the caret to the nearest whole cluster.
		m_textLayout->HitTestTextPosition(
			caretPosition_,
			false,
			&caretX,
			&caretY,
			&hitTestMetrics
		);

		// The caret position itself is always the leading edge.
		// An additional offset indicates a trailing edge when non-zero.
		// This offset comes from the number of code-units in the
		// selected cluster or surrogate pair.
		caretPosition_ = hitTestMetrics.textPosition;
		caretPositionOffset_ = (isTrailingHit) ? hitTestMetrics.length : 0;

		// For invisible, zero-width characters (like line breaks
		// and formatting characters), force leading edge of the
		// next position.
		if (skipZeroWidth && hitTestMetrics.width == 0)
		{
			caretPosition_ += caretPositionOffset_;
			caretPositionOffset_ = 0;
		}
	}

	void GetLineFromPosition(
		const DWRITE_LINE_METRICS* lineMetrics, // [lineCount]
		UINT32 lineCount,
		UINT32 textPosition,
		OUT UINT32* lineOut,
		OUT UINT32* linePositionOut
	)
	{
		// Given the line metrics, determines the current line and starting text
		// position of that line by summing up the lengths. When the starting
		// line position is beyond the given text position, we have our line.

		UINT32 line = 0;
		UINT32 linePosition = 0;
		UINT32 nextLinePosition = 0;
		for (; line < lineCount; ++line)
		{
			linePosition = nextLinePosition;
			nextLinePosition = linePosition + lineMetrics[line].length;
			if (nextLinePosition > textPosition)
			{
				// The next line is beyond the desired text position,
				// so it must be in the current line.
				break;
			}
		}
		*linePositionOut = linePosition;
		*lineOut = std::min(line, lineCount - 1);
		return;
	}

	void GetLineMetrics(
		OUT std::vector<DWRITE_LINE_METRICS>& lineMetrics
	)
	{
		// Retrieves the line metrics, used for caret navigation, up/down and home/end.

		DWRITE_TEXT_METRICS textMetrics;
		m_textLayout->GetMetrics(&textMetrics);

		lineMetrics.resize(textMetrics.lineCount);
		m_textLayout->GetLineMetrics(&lineMetrics.front(), textMetrics.lineCount, &textMetrics.lineCount);
	}

	bool SetSelection(SetSelectionMode moveMode, UINT32 advance, bool extendSelection, bool updateCaretFormat = true)
	{
		// Moves the caret relatively or absolutely, optionally extending the
		// selection range (for example, when shift is held).
		UINT32 line = UINT32_MAX; // current line number, needed by a few modes
		UINT32 absolutePosition = caretPosition_ + caretPositionOffset_;
		UINT32 oldAbsolutePosition = absolutePosition;
		UINT32 oldCaretAnchor = caretAnchor_;

		switch (moveMode)
		{
		case SetSelectionModeLeft:
			caretPosition_ += caretPositionOffset_;
			if (caretPosition_ > 0)
			{
				--caretPosition_;
				AlignCaretToNearestCluster(false, true);

				// special check for CR/LF pair
				absolutePosition = caretPosition_ + caretPositionOffset_;
				if (absolutePosition >= 1
					&& absolutePosition < m_dataLen
					&& m_dataBuf[absolutePosition - 1] == '\r'
					&& m_dataBuf[absolutePosition] == '\n')
				{
					caretPosition_ = absolutePosition - 1;
					AlignCaretToNearestCluster(false, true);
				}
			}
			break;

		case SetSelectionModeRight:
			caretPosition_ = absolutePosition;
			AlignCaretToNearestCluster(true, true);

			// special check for CR/LF pair
			absolutePosition = caretPosition_ + caretPositionOffset_;
			if (absolutePosition >= 1
				&& absolutePosition < m_dataLen
				&& m_dataBuf[absolutePosition - 1] == '\r'
				&& m_dataBuf[absolutePosition] == '\n')
			{
				caretPosition_ = absolutePosition + 1;
				AlignCaretToNearestCluster(false, true);
			}
			break;

		case SetSelectionModeLeftChar:
			caretPosition_ = absolutePosition;
			caretPosition_ -= std::min(advance, absolutePosition);
			caretPositionOffset_ = 0;
			break;

		case SetSelectionModeRightChar:
			caretPosition_ = absolutePosition + advance;
			caretPositionOffset_ = 0;
			{
				// Use hit-testing to limit text position.
				DWRITE_HIT_TEST_METRICS hitTestMetrics;
				float caretX, caretY;

				m_textLayout->HitTestTextPosition(
					caretPosition_,
					false,
					&caretX,
					&caretY,
					&hitTestMetrics
				);
				caretPosition_ = std::min(caretPosition_, hitTestMetrics.textPosition + hitTestMetrics.length);
			}
			break;

		case SetSelectionModeUp:
		case SetSelectionModeDown:
		{
			// Retrieve the line metrics to figure out what line we are on.
			std::vector<DWRITE_LINE_METRICS> lineMetrics;
			GetLineMetrics(lineMetrics);

			UINT32 linePosition;
			GetLineFromPosition(
				&lineMetrics.front(),
				static_cast<UINT32>(lineMetrics.size()),
				caretPosition_,
				&line,
				&linePosition
			);

			// Move up a line or down
			if (moveMode == SetSelectionModeUp)
			{
				if (line <= 0)
					break; // already top line
				line--;
				linePosition -= lineMetrics[line].length;
			}
			else
			{
				linePosition += lineMetrics[line].length;
				line++;
				if (line >= lineMetrics.size())
					break; // already bottom line
			}

			// To move up or down, we need three hit-testing calls to determine:
			// 1. The x of where we currently are.
			// 2. The y of the new line.
			// 3. New text position from the determined x and y.
			// This is because the characters are variable size.

			DWRITE_HIT_TEST_METRICS hitTestMetrics;
			float caretX, caretY, dummyX;

			// Get x of current text position
			m_textLayout->HitTestTextPosition(
				caretPosition_,
				caretPositionOffset_ > 0, // trailing if nonzero, else leading edge
				&caretX,
				&caretY,
				&hitTestMetrics
			);

			// Get y of new position
			m_textLayout->HitTestTextPosition(
				linePosition,
				false, // leading edge
				&dummyX,
				&caretY,
				&hitTestMetrics
			);

			// Now get text position of new x,y.
			BOOL isInside, isTrailingHit;
			m_textLayout->HitTestPoint(
				caretX,
				caretY,
				&isTrailingHit,
				&isInside,
				&hitTestMetrics
			);

			caretPosition_ = hitTestMetrics.textPosition;
			caretPositionOffset_ = isTrailingHit ? (hitTestMetrics.length > 0) : 0;
		}
		break;

		case SetSelectionModeLeftWord:
		case SetSelectionModeRightWord:
		{
			// To navigate by whole words, we look for the canWrapLineAfter
			// flag in the cluster metrics.

			// First need to know how many clusters there are.
			std::vector<DWRITE_CLUSTER_METRICS> clusterMetrics;
			UINT32 clusterCount;
			m_textLayout->GetClusterMetrics(NULL, 0, &clusterCount);

			if (clusterCount == 0)
				break;

			// Now we actually read them.
			clusterMetrics.resize(clusterCount);
			m_textLayout->GetClusterMetrics(&clusterMetrics.front(), clusterCount, &clusterCount);

			caretPosition_ = absolutePosition;

			UINT32 clusterPosition = 0;
			UINT32 oldCaretPosition = caretPosition_;

			if (moveMode == SetSelectionModeLeftWord)
			{
				// Read through the clusters, keeping track of the farthest valid
				// stopping point just before the old position.
				caretPosition_ = 0;
				caretPositionOffset_ = 0; // leading edge
				for (UINT32 cluster = 0; cluster < clusterCount; ++cluster)
				{
					clusterPosition += clusterMetrics[cluster].length;
					if (clusterMetrics[cluster].canWrapLineAfter)
					{
						if (clusterPosition >= oldCaretPosition)
							break;

						// Update in case we pass this point next loop.
						caretPosition_ = clusterPosition;
					}
				}
			}
			else // SetSelectionModeRightWord
			{
				// Read through the clusters, looking for the first stopping point
				// after the old position.
				for (UINT32 cluster = 0; cluster < clusterCount; ++cluster)
				{
					UINT32 clusterLength = clusterMetrics[cluster].length;
					caretPosition_ = clusterPosition;
					caretPositionOffset_ = clusterLength; // trailing edge
					if (clusterPosition >= oldCaretPosition && clusterMetrics[cluster].canWrapLineAfter)
						break; // first stopping point after old position.

					clusterPosition += clusterLength;
				}
			}
		}
		break;

		case SetSelectionModeHome:
		case SetSelectionModeEnd:
		{
			// Retrieve the line metrics to know first and last position
			// on the current line.
			std::vector<DWRITE_LINE_METRICS> lineMetrics;
			GetLineMetrics(lineMetrics);

			GetLineFromPosition(
				&lineMetrics.front(),
				static_cast<UINT32>(lineMetrics.size()),
				caretPosition_,
				&line,
				&caretPosition_
			);

			caretPositionOffset_ = 0;
			if (moveMode == SetSelectionModeEnd)
			{
				// Place the caret at the last character on the line,
				// excluding line breaks. In the case of wrapped lines,
				// newlineLength will be 0.
				UINT32 lineLength = lineMetrics[line].length - lineMetrics[line].newlineLength;
				caretPositionOffset_ = std::min(lineLength, 1u);
				caretPosition_ += lineLength - caretPositionOffset_;
				AlignCaretToNearestCluster(true);
			}
		}
		break;

		case SetSelectionModeFirst:
			caretPosition_ = 0;
			caretPositionOffset_ = 0;
			break;

		case SetSelectionModeAll:
			caretAnchor_ = 0;
			extendSelection = true;
			__fallthrough;

		case SetSelectionModeLast:
			caretPosition_ = UINT32_MAX;
			caretPositionOffset_ = 0;
			AlignCaretToNearestCluster(true);
			break;

		case SetSelectionModeAbsoluteLeading:
			caretPosition_ = advance;
			caretPositionOffset_ = 0;
			break;

		case SetSelectionModeAbsoluteTrailing:
			caretPosition_ = advance;
			AlignCaretToNearestCluster(true);
			break;
		}

		absolutePosition = caretPosition_ + caretPositionOffset_;

		if (!extendSelection)
			caretAnchor_ = absolutePosition;

		bool caretMoved = (absolutePosition != oldAbsolutePosition)
			|| (caretAnchor_ != oldCaretAnchor);

		if (caretMoved)
		{
#if 0
			// update the caret formatting attributes
			if (updateCaretFormat)
				UpdateCaretFormatting();

			PostRedraw();

			RectF rect;
			GetCaretRect(rect);
			UpdateSystemCaret(rect);
#endif
		}

		return caretMoved;
	}

	void GetViewMatrix(OUT DWRITE_MATRIX* matrix) const
	{
		// Generates a view matrix from the current origin, angle, and scale.
		// Need the editor size for centering.
		RECT rect;
		GetClientRect(&rect);

		// Translate the origin to 0,0
		DWRITE_MATRIX translationMatrix = {
			1, 0,
			0, 1,
			-originX_, -originY_
		};
		// Scale and rotate
		double radians = DegreesToRadians(fmod(angle_, 360.0f));
		double cosValue = cos(radians);
		double sinValue = sin(radians);

		// If rotation is a quarter multiple, ensure sin and cos are exactly one of {-1,0,1}
		if (fmod(angle_, 90.0f) == 0)
		{
			cosValue = floor(cosValue + .5);
			sinValue = floor(sinValue + .5);
		}

		DWRITE_MATRIX rotationMatrix = {
			float(cosValue * scaleX_), float(sinValue * scaleX_),
			float(-sinValue * scaleY_), float(cosValue * scaleY_),
			0, 0
		};

		// Set the origin in the center of the window
		float centeringFactor = .5f;
		DWRITE_MATRIX centerMatrix = {
			1, 0,
			0, 1,
			floor(float(rect.right * centeringFactor)), floor(float(rect.bottom * centeringFactor))
		};

		D2D1::Matrix3x2F resultA, resultB;

		resultB.SetProduct(Cast(translationMatrix), Cast(rotationMatrix));
		resultA.SetProduct(resultB, Cast(centerMatrix));

		// For better pixel alignment (less blurry text)
		resultA._31 = floor(resultA._31);
		resultA._32 = floor(resultA._32);

		*matrix = *reinterpret_cast<DWRITE_MATRIX*>(&resultA);
	}

	void GetInverseViewMatrix(OUT DWRITE_MATRIX* matrix) const
	{
		// Inverts the view matrix for hit-testing and scrolling.
		DWRITE_MATRIX viewMatrix;
		GetViewMatrix(&viewMatrix);
		ComputeInverseMatrix(viewMatrix, *matrix);
	}

	bool SetSelectionFromPoint(float x, float y, bool extendSelection)
	{
		// Returns the text position corresponding to the mouse x,y.
		// If hitting the trailing side of a cluster, return the
		// leading edge of the following text position.
		BOOL isTrailingHit;
		BOOL isInside;
		DWRITE_HIT_TEST_METRICS caretMetrics;
#if 10
		// Remap display coordinates to actual.
		DWRITE_MATRIX matrix;
		GetInverseViewMatrix(&matrix);
#endif 
		float transformedX = (x * matrix.m11 + y * matrix.m21 + matrix.dx);
		float transformedY = (x * matrix.m12 + y * matrix.m22 + matrix.dy);

		m_textLayout->HitTestPoint(
			transformedX,
			transformedY,
			&isTrailingHit,
			&isInside,
			&caretMetrics
		);

		// Update current selection according to click or mouse drag.
		SetSelection(
			isTrailingHit ? SetSelectionModeAbsoluteTrailing : SetSelectionModeAbsoluteLeading,
			caretMetrics.textPosition,
			extendSelection
		);

		return true;
	}

};
