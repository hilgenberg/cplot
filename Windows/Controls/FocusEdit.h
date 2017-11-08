#pragma once
#include "afxwin.h"
#include <functional>

class FocusEdit: public CEdit
{
public:
	std::function<void(void)> OnChange;

private:
	BOOL PreTranslateMessage(MSG *m) override;
	BOOL OnKillFocus();
	DECLARE_MESSAGE_MAP()
};

