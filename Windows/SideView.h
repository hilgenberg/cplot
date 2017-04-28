#pragma once
#include "afxwin.h"
#include "Controls/DeltaSlider.h"
#include "Controls/HeaderControl.h"
#include "Controls/ImageWell.h"
class Document;

class SideView : public CFormView
{
public:
	SideView();
	~SideView();

	void SetDoc(Document *d);
	void Redraw();

	int  OnCreate(LPCREATESTRUCT cs);
	BOOL PreCreateWindow(CREATESTRUCT &cs) override;
	BOOL OnEraseBkgnd(CDC *dc);
	void OnSize(UINT type, int w, int h);
	void OnDraw(CDC *dc);
	void OnInitialUpdate();
	void Update();
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
	CStatic         discoLabel;   CButton     disco;
	void            OnDisco();
	CStatic         displayModeLabel; CComboBox displayMode;
	CStatic         colorLabel1, colorLabel2;
	CMFCColorButton bgColor, fillColor, axisColor, gridColor;
	CStatic         textureLabel, reflectionLabel;
	ImageWell       texture, reflection;
	CSliderCtrl     textureStrength, reflectionStrength;
	CButton         riemannTexture;
	CButton         drawAxis;
	void            OnDrawAxis();
	//--------------------------------------------------------
	HeaderControl   axis;
	void            OnAxis();
	CStatic         center, range;
	CStatic xLabel; CEdit xCenter, xRange; DeltaSlider xDelta;
	CStatic yLabel; CEdit yCenter, yRange; DeltaSlider yDelta;
	DeltaSlider     rangeDelta;
	//--------------------------------------------------------

	DECLARE_DYNCREATE(SideView)
	DECLARE_MESSAGE_MAP()
};

