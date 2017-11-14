#include "DropHandler.h"

DROPEFFECT DropHandler::OnDragEnter(CWnd *w, COleDataObject *data, DWORD keyState, CPoint point)
{
	state = DROPEFFECT_NONE;

	HGLOBAL hg = data->GetGlobalData(CF_HDROP);
	if (!hg) return false;
	HDROP hdrop = (HDROP)GlobalLock(hg);
	if (!hdrop) { GlobalUnlock(hg); return false; }

	// Get the number of files dropped 
	if (DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0) == 1)
	{
		auto bufsize = 1 + DragQueryFile(hdrop, 0, NULL, 0);
		DragQueryFile(hdrop, 0, file.GetBuffer(bufsize), bufsize);
		file.ReleaseBuffer();

		int pos = file.ReverseFind('.');
		if (pos > 0)
		{
			CString ext = file.Right(file.GetLength() - pos - 1).MakeLower();
			switch (type)
			{
				case DropHandler::IMAGE:
					if (ext == _T("png") || ext == _T("jpg") || ext == _T("jpeg") ||
						ext == _T("gif") || ext == _T("bmp"))
						state = DROPEFFECT_COPY;
					break;
				case DropHandler::CPLOT:
					if (ext == _T("cplot"))
						state = DROPEFFECT_COPY;
					break;
			}
		}
	}

	GlobalUnlock(hg);

	return state;
}
DROPEFFECT DropHandler::OnDragOver(CWnd *w, COleDataObject* data, DWORD keyState, CPoint point)
{
	return state;
}
BOOL DropHandler::OnDrop(CWnd *w, COleDataObject *data, DROPEFFECT dropEffect, CPoint point)
{
	return state == DROPEFFECT_NONE ? false : callback(file);
}
void DropHandler::OnDragLeave(CWnd *w)
{
}
