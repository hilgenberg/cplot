#pragma once
#include "afxwin.h"
#include "Controls/DeltaSlider.h"
#include "Controls/HeaderControl.h"
#include "Controls/FocusEdit.h"
#include "Controls/TextureControl.h"
class Document;
class Graph;
struct Plot;

class SideView : public CFormView
{
public:
	SideView();
	~SideView();

	void SetDoc(Document *d);
	void Redraw();
	void Recalc(Plot &plot);
	void Recalc(Graph *g);
	void Update();
	void UpdateAxis();

	int  OnCreate(LPCREATESTRUCT cs);
	BOOL PreCreateWindow(CREATESTRUCT &cs) override;
	BOOL OnEraseBkgnd(CDC *dc);
	void OnSize(UINT type, int w, int h);
	void OnDraw(CDC *dc);
	void OnInitialUpdate();
	void OnAdd   (HeaderControl *sender);
	void OnRemove(HeaderControl *sender);
	BOOL OnMouseWheel(UINT flags, short dz, CPoint p);

private:
	friend class PlotView; // access to OnXxx methods

	Document *doc;

	//--------------------------------------------------------
	HeaderControl   parameters;
	//--------------------------------------------------------
	HeaderControl   definitions;
	//--------------------------------------------------------
	HeaderControl   graphs;
	//--------------------------------------------------------
	HeaderControl   settings;
	void            OnSettings();

	CStatic         qualityLabel; CSliderCtrl quality;
	void            OnHScroll(UINT code, UINT pos, CScrollBar *sb);
	
	CStatic         discoLabel;   CButton     disco;
	void            OnDisco();
	
	CStatic         displayModeLabel; CComboBox displayMode, vfMode;
	void            OnDisplayMode();
	void            OnVFMode();
	CStatic         histoModeLabel; CComboBox histoMode;
	CStatic         histoScaleLabel; FocusEdit histoScale; CSliderCtrl histoScaleSlider;
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
	
	CStatic         textureLabel, reflectionLabel;
	TextureControl  texture, reflection;
	void            OnChangeTexture(int i);
	CSliderCtrl     textureStrength, reflectionStrength;
	void            OnVScroll(UINT code, UINT pos, CScrollBar *sb);
	CComboBox       textureMode;
	void            OnTextureMode();
	
	CStatic         gridModeLabel,   meshModeLabel;
	CComboBox       gridMode,        meshMode;
	CSliderCtrl     gridDensity,     meshDensity;
	void            OnGridMode();
	void            OnMeshMode();

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
	//--------------------------------------------------------
	HeaderControl   axis;
	void            OnAxis();
	CStatic         center, range;
	CStatic xLabel; FocusEdit xCenter, xRange; DeltaSlider xDelta;
	CStatic yLabel; FocusEdit yCenter, yRange; DeltaSlider yDelta;
	DeltaSlider     rangeDelta;
	//--------------------------------------------------------

	DECLARE_DYNCREATE(SideView)
	DECLARE_MESSAGE_MAP()
};

