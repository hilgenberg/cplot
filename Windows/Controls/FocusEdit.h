#pragma once
#include "afxwin.h"
#include <functional>

class FocusEdit: public CEdit
{
public:
	std::function<void(void)> f;

	BOOL PreTranslateMessage(MSG *m) override;

private:
	BOOL OnKillFocus();
	DECLARE_MESSAGE_MAP()
};

