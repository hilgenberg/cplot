#pragma once
#include "afxwin.h"
#include "Controls/DeltaSlider.h"
#include "Controls/HeaderControl.h"
#include "Controls/FocusEdit.h"
#include "Controls/TextureControl.h"
#include "Controls/NumericEdit.h"
#include "Controls/ParameterView.h"
#include "Controls/DefinitionView.h"
#include "Controls/GraphView.h"
#include "../Graphs/Geometry/Vector.h"
class Document;
class Graph;
struct Plot;
class Parameter;
class UserFunction;

union BoxState
{
	struct
	{
		bool params   : 1;
		bool defs     : 1;
		bool graphs   : 1;
		bool settings : 1;
		bool axis     : 1;
	};
	uint32_t all;
};

class SideView : public CFormView
{
public:
	SideView();
	~SideView();
	
	Document &document() const { assert(doc); return *doc; }
	BoxState GetBoxState() const;
	void SetBoxState(BoxState b);
	void OnBoxChange();

	void SetDoc(Document *d);
	void Redraw();
	void Recalc(Plot &plot);
	void Recalc(Graph *g);
	void Update();
	void UpdateAxis();

	bool Animating() const;
	void Animate();

	int  OnCreate(LPCREATESTRUCT cs);
	BOOL PreCreateWindow(CREATESTRUCT &cs) override;
	BOOL OnEraseBkgnd(CDC *dc);
	void OnSize(UINT type, int w, int h);
	void OnDraw(CDC *dc);
	void OnInitialUpdate();
	void OnAdd   (HeaderControl *sender);
	void OnRemove(HeaderControl *sender);
	void OnEdit(Parameter *p);
	void OnEdit(UserFunction *f);
	BOOL OnMouseWheel(UINT flags, short dz, CPoint p);

private:
	int  active_anims;
	void AnimStateChanged(bool active);

	friend class PlotView; // access to OnXxx methods
	friend class ParameterView; // access to AnimStateChanged

	Document *doc;

	//--------------------------------------------------------
	HeaderControl   parameters;
	std::vector<ParameterView*> params;
	//--------------------------------------------------------
	HeaderControl   definitions;
	std::vector<DefinitionView*> defs;
	//--------------------------------------------------------
	HeaderControl   graphs;
	std::vector<GraphView*> gdefs;
	//--------------------------------------------------------
	HeaderControl   settings;
	CStatic         qualityLabel; CSliderCtrl quality;
	void            OnHScroll(UINT code, UINT pos, CScrollBar *sb);
	
	CStatic         discoLabel;   CButton     disco;
	void            OnDisco();
	
	CStatic         displayModeLabel; CComboBox displayMode, vfMode;
	void            OnDisplayMode();
	void            OnVFMode();
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
	CStatic         centerLabel, rangeLabel;
	CStatic xLabel; NumericEdit xCenter, xRange; DeltaSlider xDelta;
	CStatic yLabel; NumericEdit yCenter, yRange; DeltaSlider yDelta;
	CStatic zLabel; NumericEdit zCenter, zRange; DeltaSlider zDelta;
	DeltaSlider     xyzDelta;
	CStatic uLabel; NumericEdit uCenter, uRange; DeltaSlider uDelta;
	CStatic vLabel; NumericEdit vCenter, vRange; DeltaSlider vDelta;
	DeltaSlider     uvDelta;
	CStatic         phiLabel, psiLabel, thetaLabel;
	NumericEdit     phi, psi, theta;
	DeltaSlider     phiDelta, psiDelta, thetaDelta;
	CStatic         distLabel; NumericEdit dist; DeltaSlider distDelta;
	CButton         center, top, front;
	void            OnCenterAxis();
	void            OnEqualRanges();
	void OnAxisRange(int i, NumericEdit &e);
	void OnAxisCenter(int i, NumericEdit &e);
	void OnAxisAngle(int i, NumericEdit &e);
	void OnDistance();
	void ChangeView(const P3d &v);
	void OnTopView   () { ChangeView(P3d(  0.0,  90.0, 0.0)); }
	void OnFrontView () { ChangeView(P3d(  0.0,   0.3, 0.0)); }
	void OnLeftView  () { ChangeView(P3d( 90.0,   0.3, 0.0)); }
	void OnRightView () { ChangeView(P3d(-90.0,   0.3, 0.0)); }
	void OnBackView  () { ChangeView(P3d(180.0,   0.3, 0.0)); }
	void OnBottomView() { ChangeView(P3d(  0.0, -90.0, 0.0)); }

	//--------------------------------------------------------
	DECLARE_DYNCREATE(SideView)
	DECLARE_MESSAGE_MAP()
};

