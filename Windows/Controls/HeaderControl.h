#pragma once
#include "afxwin.h"

class HeaderControl : public CButton
{
public:
	HeaderControl() : canAdd(Hidden), canRemove(Hidden) {}

	enum State{Hidden, Inactive, Active};

	void SetCanAdd   (State s) { if (canAdd    == s) return;  canAdd    = s; Invalidate(); }
	void SetCanRemove(State s) { if (canRemove == s) return;  canRemove = s; Invalidate(); }

	void OnPaint();
	void OnLButtonDown(UINT flags, CPoint p);

private:
	State canAdd, canRemove;

	DECLARE_MESSAGE_MAP()
};

