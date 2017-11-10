
CFont &controlFont(bool bold)
{
	static CFont font, boldFont;

	if (!font.GetSafeHandle())
	{
		font.CreatePointFont(80, _T("MS Shell Dlg"));

		LOGFONT lf;
		font.GetLogFont(&lf);
		lf.lfWeight = FW_SEMIBOLD;
		boldFont.CreateFontIndirect(&lf);
	}
	
	return bold ? boldFont : font;
}
