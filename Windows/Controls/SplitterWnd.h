#pragma once

class SplitterWnd : public CSplitterWnd
{
public:
	SplitterWnd();

	void Show();
	void Hide();
	void Toggle() { if (hidden) Show(); else Hide(); }
	bool Hidden() { return hidden; }

	BOOL OnEraseBkgnd(CDC *dc);
	BOOL PreCreateWindow(CREATESTRUCT &cs);

private:
	bool hidden;

	DECLARE_MESSAGE_MAP()
};
