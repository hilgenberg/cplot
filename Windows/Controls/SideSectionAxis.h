#pragma once
#include "afxwin.h"
#include "SideSection.h"
#include "DeltaSlider.h"
#include "NumericEdit.h"
#include "../../Graphs/Geometry/Vector.h"

class SideSectionAxis : public SideSection
{
public:
	CString headerString() override { return _T("Axis"); }
	CString wndClassName() override { return _T("SideSectionAxis"); }
	void Update(bool full) override;

	int  OnCreate(LPCREATESTRUCT cs);

	void Animate(double t);

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
	void OnTopView() { ChangeView(P3d(0.0, 90.0, 0.0)); }
	void OnFrontView() { ChangeView(P3d(0.0, 0.3, 0.0)); }
	void OnLeftView() { ChangeView(P3d(90.0, 0.3, 0.0)); }
	void OnRightView() { ChangeView(P3d(-90.0, 0.3, 0.0)); }
	void OnBackView() { ChangeView(P3d(180.0, 0.3, 0.0)); }
	void OnBottomView() { ChangeView(P3d(0.0, -90.0, 0.0)); }

	DECLARE_MESSAGE_MAP()
};

