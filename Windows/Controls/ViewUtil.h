#pragma once
#include "../Util/Layout.h"

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
const DWORD buttonStyle = WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP;

#define START_CREATE \
	DS0;\
	CComboBox *currentPopup = NULL; int currentIdx = 0;

#define CREATE(c, h, ...)      c.Create(__VA_ARGS__, CRect(0, 0, 80, DS(h)), this, ID_##c); c.SetFont(&controlFont())
#define CREATEI(c, h, ID, ...) c.Create(__VA_ARGS__, CRect(0, 0, 80, DS(h)), this, ID);     c.SetFont(&controlFont())

#define LABEL(c, s)   CREATE(c, 14, _T(s),  labelStyle)
#define RLABEL(c, s)  CREATE(c, 14, _T(s),  rlabelStyle)
#define LLABEL(c, s)  CREATE(c, 14, _T(s),  llabelStyle)
#define SECTION(c, s) CREATE(c, 20, _T(s),  sectionStyle); c.SetCheck(TRUE); c.SetOwner(this)
#define COLOR(c)      CREATE(c, 20, _T(""), colorStyle); c.EnableOtherButton(_T("More Colors..."))
#define SLIDER(c, m)  CREATE(c, 20, sliderStyle); c.SetRange(0, m)
#define VSLIDER(c, m) CREATE(c, 20, vSliderStyle); c.SetRange(0, m)
#define DELTA(c)      CREATE(c, 20, deltaStyle); c.stateChange = [this](bool a){ parent().AnimStateChanged(a); }
#define DELTA0(c)     CREATE(c, 20, deltaStyle)
#define CHECK(c, s)   CREATE(c, 20, _T(s), checkStyle)
#define BUTTON(c, s)  CREATE(c, 20, _T(s), buttonStyle)
#define POPUP(c)      CREATE(c, 21, popupStyle); currentPopup = &c; currentIdx = 0
#define OPTION(s)     currentPopup->InsertString(currentIdx++, _T(s))
#define DATA(s)       currentPopup->SetItemData(currentIdx-1, (DWORD_PTR)s)
#define EDIT(c)       CREATE(c, 20, editStyle)

#define BUTTONLABEL(c)  CREATE(c, 20, _T(""), WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP);\
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

CFont &controlFont(bool bold = false);
