#pragma once
#include "afxcmn.h"

class DeltaSlider: public CSliderCtrl
{
public:
	DeltaSlider();

	std::function<void(bool)> stateChange;

	int  OnCreate(LPCREATESTRUCT cs);
	void HScroll(UINT code, UINT pos);

	bool active() const { return active_; }
	double evolve(double t); // t is current time, returns current dx

private:
	void active(bool f, bool slide = false)
	{
		if (f != active_)
		{
			active_ = f;
			v = 0.0;
			if (stateChange) stateChange(f);
		}
		if (slide != slideback_)
		{
			slideback_ = slide;
			if (slide) v = 0.0;
		}
	}
	bool slideback() const
	{
		return slideback_;
	}
	bool active_, slideback_;
	double v; // slideback velocity
	double t0;

	DECLARE_MESSAGE_MAP()
};
