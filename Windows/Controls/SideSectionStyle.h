#pragma once
#include "afxwin.h"
#include "SideSection.h"
#include "TextureControl.h"
#include "NumericEdit.h"

class SideSectionStyle : public SideSection
{
public:
	CString headerString() override { return _T("Style"); }
	CString wndClassName() override { return _T("SideSectionStyle"); }
	void Update(bool full) override;

	int  OnCreate(LPCREATESTRUCT cs);

	void            OnHScroll(UINT code, UINT pos, CScrollBar *sb);

	CStatic         aaModeLabel; CComboBox aaMode;
	void            OnAAMode();

	CStatic         transparencyModeLabel; CComboBox transparencyMode;
	void            OnTransparencyMode();

	CStatic         fogLabel;       CSliderCtrl fog;
	CStatic         lineWidthLabel; CSliderCtrl lineWidth;
	CStatic         shinynessLabel; CSliderCtrl shinyness;

	CStatic         bgLabel, fillLabel, axisLabel, gridLabel;
	CMFCColorButton bgColor, fillColor, axisColor, gridColor;
	CSliderCtrl     bgAlpha, fillAlpha, axisAlpha, gridAlpha;
	void            OnBgColor();
	void            OnFillColor();
	void            OnAxisColor();
	void            OnGridColor();
	CStatic         fontLabel;
	CButton         font;
	void            OnFont();

	CStatic         textureLabel, reflectionLabel;
	TextureControl  texture, reflection;
	void            OnChangeTexture(int i);
	CSliderCtrl     textureStrength, reflectionStrength;
	void            OnVScroll(UINT code, UINT pos, CScrollBar *sb);
	CComboBox       textureMode;
	void            OnTextureMode();
	void            OnCycleTextureMode(int d);

	DECLARE_MESSAGE_MAP()
};

