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
	void SetImage(GL_Image *im); // NULL to clear

	int  OnCreate(LPCREATESTRUCT cs);
	void OnPaint();
	void OnRButtonUp(UINT flags, CPoint p);
	void OnContextItem(UINT which);
	bool load(const CString &imageFile);

private:
	GL_Image   *im;
	size_t      im_state; // im->state_counter from last update
	CBitmap     bmp;
	CMenu       cm; // context menu
	DropHandler drop;

	DECLARE_MESSAGE_MAP()
};
