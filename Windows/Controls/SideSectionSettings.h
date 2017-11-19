#pragma once
#include "afxwin.h"
#include "SideSection.h"
#include "TextureControl.h"
#include "NumericEdit.h"

class SideSectionSettings : public SideSection
{
public:
	CString headerString() override { return _T("Settings"); }
	CString wndClassName() override { return _T("SideSectionSettings"); }
	void Update(bool full) override;

	int  OnCreate(LPCREATESTRUCT cs);

	CStatic         qualityLabel; CSliderCtrl quality;
	void            OnHScroll(UINT code, UINT pos, CScrollBar *sb);

	CStatic         discoLabel;   CButton     disco;
	void            OnDisco();

	CStatic         displayModeLabel; CComboBox displayMode, vfMode;
	void            OnDisplayMode();
	void            OnVFMode();
	void            OnCycleVFMode(int d);
	CStatic         histoModeLabel; CComboBox histoMode;
	CStatic         histoScaleLabel; NumericEdit histoScale; CSliderCtrl histoScaleSlider;
	void            OnHistoMode();
	void            OnHistoScale();

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

	CStatic         gridModeLabel, meshModeLabel;
	CComboBox       gridMode, meshMode;
	CSliderCtrl     gridDensity, meshDensity;
	void            OnGridMode();
	void            OnMeshMode();
	void            OnToggleGrid(); // for PlotView

	CButton         drawAxis;
	void            OnDrawAxis();
	CStatic         axisModeLabel;
	CButton         clip;
	void            OnClip();
	CComboBox       axisMode;
	void            OnAxisMode();

	CButton         clipCustom, clipLock, clipReset;
	CSliderCtrl     clipDistance;
	void            OnClipCustom();
	void            OnClipLock();
	void            OnClipReset();

	DECLARE_MESSAGE_MAP()
};

