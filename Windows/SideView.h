#pragma once
#include "afxwin.h"
#include "Controls/DeltaSlider.h"
#include "Controls/HeaderControl.h"
#include "Controls/ImageWell.h"
#include "Controls/FocusEdit.h"
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
	CStatic         displayModeLabel; CComboBox displayMode;
	void            OnDisplayMode();
	CStatic         bgLabel, fillLabel, axisLabel, gridLabel;
	CMFCColorButton bgColor, fillColor, axisColor, gridColor;
	CSliderCtrl     bgAlpha, fillAlpha, axisAlpha, gridAlpha;
	void            OnBgColor();
	void            OnFillColor();
	void            OnAxisColor();
	void            OnGridColor();
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
	CStatic xLabel; FocusEdit xCenter, xRange; DeltaSlider xDelta;
	CStatic yLabel; FocusEdit yCenter, yRange; DeltaSlider yDelta;
	DeltaSlider     rangeDelta;
	//--------------------------------------------------------

	DECLARE_DYNCREATE(SideView)
	DECLARE_MESSAGE_MAP()
};

