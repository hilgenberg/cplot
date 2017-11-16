#pragma once

const DWORD sectionStyle = WS_CHILD | BS_AUTOCHECKBOX;
const DWORD labelStyle   = WS_CHILD | SS_CENTER | SS_WORDELLIPSIS;
const DWORD rlabelStyle  = WS_CHILD | SS_RIGHT | SS_WORDELLIPSIS;
const DWORD llabelStyle  = WS_CHILD | SS_LEFT | SS_WORDELLIPSIS;
const DWORD colorStyle   = WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP;
const DWORD sliderStyle  = WS_CHILD | TBS_NOTICKS | TBS_BOTH | WS_TABSTOP;
const DWORD deltaStyle   = WS_CHILD | TBS_NOTICKS | TBS_BOTH;
const DWORD vSliderStyle = WS_CHILD | TBS_NOTICKS | TBS_BOTH | TBS_VERT | TBS_DOWNISLEFT | WS_TABSTOP;
const DWORD editStyle    = WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP;
const DWORD popupStyle   = WS_CHILD | WS_BORDER | CBS_DROPDOWNLIST | WS_TABSTOP;
const DWORD checkStyle   = WS_CHILD | BS_CHECKBOX | WS_TABSTOP;
const DWORD buttonStyle  = WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP;

#define START_CREATE \
	CRect whatever(20, 20, 80, 40);\
	CComboBox *currentPopup = NULL; int currentIdx = 0;

#define CREATE(c, ...)      c.Create(__VA_ARGS__, whatever, this, ID_##c); c.SetFont(&controlFont())
#define CREATEI(c, ID, ...) c.Create(__VA_ARGS__, whatever, this, ID);     c.SetFont(&controlFont())

#define LABEL(c, s)   CREATE(c, _T(s),  labelStyle)
#define RLABEL(c, s)   CREATE(c, _T(s),  rlabelStyle)
#define LLABEL(c, s)   CREATE(c, _T(s),  llabelStyle)
#define SECTION(c, s) CREATE(c, _T(s),  sectionStyle); c.SetCheck(TRUE); c.SetOwner(this)
#define COLOR(c)      CREATE(c, _T(""), colorStyle); c.EnableOtherButton(_T("More Colors..."))
#define SLIDER(c, m)  CREATE(c, sliderStyle); c.SetRange(0, m)
#define VSLIDER(c, m) CREATE(c, vSliderStyle); c.SetRange(0, m)
#define DELTA(c)      CREATE(c, deltaStyle); c.stateChange = [this](bool a){ parent().AnimStateChanged(a); }
#define CHECK(c, s)   CREATE(c, _T(s), checkStyle)
#define BUTTON(c, s)  CREATE(c, _T(s), buttonStyle)
#define POPUP(c)      CREATE(c, popupStyle); currentPopup = &c; currentIdx = 0
#define OPTION(s)     currentPopup->InsertString(currentIdx++, _T(s))
#define DATA(s)       currentPopup->SetItemData(currentIdx-1, (DWORD_PTR)s)
#define EDIT(c)       CREATE(c, editStyle)

#define BUTTONLABEL(c)  CREATE(c, _T(""), WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP);\
	c.m_nFlatStyle = CMFCButton::BUTTONSTYLE_NOBORDERS;\
	c.m_nAlignStyle = CMFCButton::ALIGN_LEFT;\
	c.m_bDrawFocus = false



#ifdef WIN10
#define DS0 const UINT dpi = ::GetDpiForWindow(GetSafeHwnd())
#else
#define DS0 \
	HDC tmp_hdc = ::GetDC(GetSafeHwnd());\
	const UINT dpi = ::GetDeviceCaps(tmp_hdc, LOGPIXELSX);\
	::ReleaseDC(NULL, tmp_hdc)
#endif

#define DS(x) MulDiv(x, dpi, 96)

static inline void HIDE(CWnd &c) { c.ShowWindow(SW_HIDE); }

static void MOVE(CWnd &c, int x0, int x1, int y0, int h, int h_row)
{
	if (x0 >= x1 || h <= 0)
	{
		HIDE(c);
	}
	else
	{
		c.ShowWindow(SW_SHOW);
		c.MoveWindow(x0, y0 + (h_row - h) / 2, x1 - x0, h);
	}
}

CFont &controlFont(bool bold = false);
