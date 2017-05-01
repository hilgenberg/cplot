#pragma once
#include "afxcmn.h"

class DeltaSlider: public CSliderCtrl
{
public:
	DeltaSlider();
	~DeltaSlider() override;

	BOOL PreCreateWindow(CREATESTRUCT &cs) override;
	int  OnCreate(LPCREATESTRUCT cs);
	void HScroll(UINT code, UINT pos);
	void OnTimer(UINT_PTR timerID);

	DECLARE_MESSAGE_MAP()
};
