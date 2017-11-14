#pragma once
#include "afxwin.h"
#include "../Util/DropHandler.h"
#include <functional>
struct GL_Image;

class TextureControl: public CWnd
{
public:
	TextureControl();
	BOOL Create(DWORD style, const RECT &rect, CWnd *parent, UINT ID);

	std::function<void(void)> OnChange;
	void SetImage(GL_Image *im, bool forceReload); // NULL to clear

	int  OnCreate(LPCREATESTRUCT cs);
	BOOL OnEraseBkgnd(CDC *dc);
	void OnPaint();
	void OnRButtonUp(UINT flags, CPoint p);
	void OnContextItem(UINT which);
	bool load(const CString &imageFile);

private:
	GL_Image   *im;
	CBitmap     bmp;
	CMenu       cm; // context menu
	DropHandler drop;

	DECLARE_MESSAGE_MAP()
};
