#pragma once

#pragma warning(disable:4800) /* disable the idiotic performance warnings for bool casts*/
#pragma warning(disable:4100) /* unreferenced function arguments are ok */

#include "../pch.h"

#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#undef min
#undef max
#undef near
#undef far
#undef FAR
#undef NEAR

inline COLORREF GREY(BYTE y) { return RGB(y, y, y); }

#undef TRANSPARENT
#undef OPAQUE
#define MFC_TRANSPARENT         1
#define MFC_OPAQUE              2

#define strncasecmp _strnicmp
#undef ERROR

#include <glew.h>
#include <wglew.h>

#include "Conversions.h"
