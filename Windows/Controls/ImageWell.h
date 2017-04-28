#pragma once
#include "afxwin.h"

class ImageWell: public CStatic
{
public:
	ImageWell();
	~ImageWell();

	BOOL PreCreateWindow(CREATESTRUCT &cs) override;
	int  OnCreate(LPCREATESTRUCT cs);

	DECLARE_MESSAGE_MAP()
};

