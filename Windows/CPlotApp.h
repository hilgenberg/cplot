#pragma once

class CPlotApp : public CWinAppEx
{
public:
	CPlotApp();

	BOOL InitInstance() override;
	int  ExitInstance() override;

	void PreLoadState() override;
	void LoadCustomState() override;
	void SaveCustomState() override;

	void OnAppAbout();
	void OnViewProperties();
	void OnUpdateViewProperties(CCmdUI *mi);

private:
	ULONG_PTR gdiplusToken;

	DECLARE_MESSAGE_MAP()
};

extern CPlotApp theApp;
