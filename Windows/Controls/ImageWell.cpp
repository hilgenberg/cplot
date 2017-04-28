#include "ImageWell.h"

BEGIN_MESSAGE_MAP(ImageWell, CStatic)
	ON_WM_CREATE()
END_MESSAGE_MAP()


BOOL ImageWell::PreCreateWindow(CREATESTRUCT &cs)
{
	if (!CStatic::PreCreateWindow(cs)) return FALSE;
	cs.style |= WS_BORDER | SS_BITMAP | SS_CENTERIMAGE | SS_REALSIZEIMAGE | SS_SUNKEN;
	return TRUE;
}

int ImageWell::OnCreate(LPCREATESTRUCT cs)
{
	if (CStatic::OnCreate(cs) < 0) return -1;
	return 0;
}

ImageWell::ImageWell()
{
}


ImageWell::~ImageWell()
{
}
