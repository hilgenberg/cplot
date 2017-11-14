#pragma once
#include "afxwin.h"
#include <functional>

class DropHandler: public COleDropTarget
{
public:
	enum Type
	{
		IMAGE,
		CPLOT
	};
	
	DropHandler(Type t, std::function<bool(const CString &)> callback)
	: type(t), callback(callback)
	{
		assert(callback);
	}

	DROPEFFECT OnDragEnter(CWnd *w, COleDataObject *data, DWORD keyState, CPoint point);
	DROPEFFECT OnDragOver(CWnd *w, COleDataObject* data, DWORD keyState, CPoint point);
	BOOL OnDrop(CWnd *w, COleDataObject *data, DROPEFFECT dropEffect, CPoint point);
	void OnDragLeave(CWnd *w);

protected:
	DROPEFFECT state;
	CString file;
	Type type;
	std::function<bool(const CString &)> callback; // path -> success
};
