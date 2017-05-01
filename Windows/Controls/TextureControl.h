#pragma once
#include "afxwin.h"
#include <functional>
struct GL_Image;

class TextureControl: public CWnd
{
public:
	TextureControl();
	~TextureControl();
	BOOL Create(DWORD style, const RECT &rect, CWnd *parent, UINT ID);

	std::function<void(void)> OnChange;
	void SetImage(GL_Image *im); // NULL to clear

	BOOL OnEraseBkgnd(CDC *dc);
	void OnPaint();
	void OnRButtonUp(UINT flags, CPoint p);
	void OnContextItem(UINT which);

private:
	GL_Image *im;
	CBitmap bmp;
	CMenu   cm; // context menu
	std::vector<uint32_t> buf; // premultiplied copy of im
	DECLARE_MESSAGE_MAP()
};
