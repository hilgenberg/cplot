#include "../stdafx.h"
#include "SideSectionAxis.h"
#include "ViewUtil.h"
#include "../res/resource.h"
#include "../DefinitionController.h"
#include "DefinitionView.h"
#include "../../Graphs/Plot.h"
#include "../SideView.h"
#include "../Document.h"
#include "../MainWindow.h"
#include "../MainView.h"

enum
{
	ID_header = 2000,
	ID_centerLabel, ID_rangeLabel,
	ID_xLabel, ID_xCenter, ID_xRange, ID_xDelta,
	ID_yLabel, ID_yCenter, ID_yRange, ID_yDelta,
	ID_zLabel, ID_zCenter, ID_zRange, ID_zDelta,
	ID_xyzDelta,
	ID_uLabel, ID_uCenter, ID_uRange, ID_uDelta,
	ID_vLabel, ID_vCenter, ID_vRange, ID_vDelta,
	ID_uvDelta,
	ID_phiLabel, ID_psiLabel, ID_thetaLabel,
	ID_phi, ID_psi, ID_theta,
	ID_phiDelta, ID_psiDelta, ID_thetaDelta,
	ID_distLabel, ID_dist, ID_distDelta,
	ID_center, ID_top, ID_front
};

BEGIN_MESSAGE_MAP(SideSectionAxis, SideSection)
	ON_WM_CREATE()
	ON_BN_CLICKED(ID_center, OnCenterAxis)
	ON_BN_CLICKED(ID_top, OnTopView)
	ON_BN_CLICKED(ID_front, OnFrontView)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------------------------
// Check Boxes & Buttons
//---------------------------------------------------------------------------------------------

static inline void toggle(bool &b) { b = !b; }

void SideSectionAxis::OnCenterAxis()
{
	Plot &plot = document().plot;
	plot.axis.reset_center();
	Update(false);
	Recalc(plot);
}

void SideSectionAxis::OnEqualRanges()
{
	Plot &plot = document().plot;
	plot.axis.equal_ranges();
	Update(false);
	Recalc(plot);
}

void SideSectionAxis::ChangeView(const P3d &v)
{
	Plot &plot = document().plot;
	Camera &camera = plot.camera;
	camera.set_angles(v.x, v.y, v.z);
	Update(false);
	Redraw();
}

//---------------------------------------------------------------------------------------------
// Edit Fields
//---------------------------------------------------------------------------------------------

void SideSectionAxis::OnAxisRange(int i, NumericEdit &e)
{
	Plot &plot = document().plot;
	double x = e.GetDouble() * 0.5;
	if (!defined(x)) return;
	if (i < 0)
	{
		i = -(i + 1);
		plot.axis.in_range(i, x);
	}
	else
	{
		plot.axis.range(i, x);
	}
	Update(false);
	Recalc(plot);
}

void SideSectionAxis::OnAxisCenter(int i, NumericEdit &e)
{
	Plot &plot = document().plot;
	double x = e.GetDouble();
	if (!defined(x)) return;
	if (i < 0)
	{
		i = -(i + 1);
		plot.axis.in_center(i, x);
	}
	else
	{
		plot.axis.center(i, x);
	}
	Update(false);
	Recalc(plot);
}

void SideSectionAxis::OnAxisAngle(int i, NumericEdit &e)
{
	Camera &cam = document().plot.camera;
	double x = e.GetDouble();
	if (!defined(x)) return;
	switch (i)
	{
		case 0: cam.set_phi(x); break;
		case 1: cam.set_psi(x); break;
		case 2: cam.set_theta(x); break;
		default: assert(false); break;
	}
	Update(false);
	Redraw();
}

void SideSectionAxis::OnDistance()
{
	Camera &cam = document().plot.camera;
	double x = dist.GetDouble();
	if (!defined(x) || x <= 0.0) return;
	cam.set_zoom(1.0 / x);
	Update(false);
	Redraw();
}

//---------------------------------------------------------------------------------------------
// Animation
//---------------------------------------------------------------------------------------------

void SideSectionAxis::Animate(double t)
{
	Plot &plot = document().plot;
	Axis &axis = plot.axis;
	Camera &camera = plot.camera;

	bool recalc = false;

	double dx = xDelta.evolve(t);
	double dy = yDelta.evolve(t);
	double dz = zDelta.evolve(t);
	double dr = xyzDelta.evolve(t);
	if (dx || dy || dz)
	{
		double scale = 0.05;
		dx *= axis.range(0) * scale;
		dy *= axis.range(1) * scale;
		dz *= axis.range(2) * scale;
		axis.move(dx, dy, dz);
		recalc = true;
	}
	if (dr)
	{
		axis.zoom(1.0 + 0.05 * dr);
		recalc = true;
	}

	double du = uDelta.evolve(t);
	double dv = vDelta.evolve(t);
	double duv = uvDelta.evolve(t);
	if (du || dv)
	{
		double scale = 0.05;
		du *= axis.in_range(0) * scale;
		dv *= axis.in_range(1) * scale;
		axis.in_move(du, dv);
		recalc = true;
	}
	if (duv)
	{
		axis.in_zoom(1.0 + 0.05 * duv);
		recalc = true;
	}

	double dphi = phiDelta.evolve(t);
	double dpsi = psiDelta.evolve(t);
	double dtheta = thetaDelta.evolve(t);
	if (dphi) camera.set_phi(camera.phi() + dphi * 10.0);
	if (dpsi) camera.set_psi(camera.psi() + dpsi * 10.0);
	if (dtheta) camera.set_theta(camera.theta() + dtheta * 10.0);

	double dd = distDelta.evolve(t);
	if (dd) camera.zoom(1.0 + 0.05 * dd);

	if (recalc) Recalc(plot);
	Update(false);
	UpdateWindow(); // repaint
}

//---------------------------------------------------------------------------------------------
// Create & Update
//---------------------------------------------------------------------------------------------

int SideSectionAxis::OnCreate(LPCREATESTRUCT cs)
{
	if (SideSection::OnCreate(cs) < 0) return -1;

	START_CREATE;

	LABEL(centerLabel, "Center"); LABEL(rangeLabel, "Range");
	LABEL(xLabel, "x:"); EDIT(xCenter); EDIT(xRange); DELTA(xDelta);
	LABEL(yLabel, "y:"); EDIT(yCenter); EDIT(yRange); DELTA(yDelta);
	LABEL(zLabel, "z:"); EDIT(zCenter); EDIT(zRange); DELTA(zDelta);
	DELTA(xyzDelta);
	LABEL(uLabel, "u:"); EDIT(uCenter); EDIT(uRange); DELTA(uDelta);
	LABEL(vLabel, "v:"); EDIT(vCenter); EDIT(vRange); DELTA(vDelta);
	DELTA(uvDelta);
	LABEL(phiLabel, "phi"); LABEL(psiLabel, "psi"); LABEL(thetaLabel, "theta");
	EDIT(phi); EDIT(psi); EDIT(theta);
	DELTA(phiDelta); DELTA(psiDelta); DELTA(thetaDelta);
	LABEL(distLabel, "Zoom:"); EDIT(dist); DELTA(distDelta);
	BUTTON(center, "Center"); BUTTON(top, "Upright"); BUTTON(front, "Front");

	xCenter.OnChange = [this] { OnAxisCenter(0, xCenter); };
	yCenter.OnChange = [this] { OnAxisCenter(1, yCenter); };
	zCenter.OnChange = [this] { OnAxisCenter(2, zCenter); };
	uCenter.OnChange = [this] { OnAxisCenter(-1, uCenter); };
	vCenter.OnChange = [this] { OnAxisCenter(-2, vCenter); };

	xRange.OnChange = [this] { OnAxisRange(0, xRange); };
	yRange.OnChange = [this] { OnAxisRange(1, yRange); };
	zRange.OnChange = [this] { OnAxisRange(2, zRange); };
	uRange.OnChange = [this] { OnAxisRange(-1, uRange); };
	vRange.OnChange = [this] { OnAxisRange(-2, vRange); };

	phi.OnChange = [this] { OnAxisAngle(0, phi); };
	psi.OnChange = [this] { OnAxisAngle(1, psi); };
	theta.OnChange = [this] { OnAxisAngle(2, theta); };

	dist.OnChange = [this] { OnDistance(); };

	return 0;
}

void SideSectionAxis::Update(bool full)
{
	SideSection::Update(full);

	const Plot &plot = GetPlot();
	const Graph *g = GetGraph();
	const Axis   &ax = plot.axis;
	const Camera &cam = plot.camera;

	if (!full)
	{
		if (!header.GetCheck()) return;

		const bool in3d = (plot.axis_type() != Axis::Rect);

		xCenter.SetDouble(ax.center(0));
		xRange.SetDouble(ax.range(0)*2.0);

		yCenter.SetDouble(ax.center(1));
		yRange.SetDouble(ax.range(1)*2.0);

		if (in3d)
		{
			zCenter.SetDouble(ax.center(2));
			zRange.SetDouble(ax.range(2)*2.0);
			phi.SetDouble(cam.phi());
			psi.SetDouble(cam.psi());
			theta.SetDouble(cam.theta());
			dist.SetDouble(1.0 / cam.zoom());
		}

		const int nin = g ? g->inRangeDimension() : -1;

		if (nin >= 1)
		{
			uCenter.SetDouble(ax.in_center(0));
			uRange.SetDouble(ax.in_range(0)*2.0);
		}
		if (nin >= 2)
		{
			vCenter.SetDouble(ax.in_center(1));
			vRange.SetDouble(ax.in_range(1)*2.0);
		}
		return;
	}

	//----------------------------------------------------------------------------------
	CRect bounds; GetWindowRect(bounds);
	if (bounds.Width() < 2) return;

	const bool sel = g != NULL;
	const bool vf = g && g->isVectorField();
	const bool color = g && g->isColor();
	const bool line = g && g->isLine();
	const bool histo = g && g->isHistogram();
	const bool twoD = plot.axis_type() == Axis::Rect;
	const bool points = g && g->options.shading_mode == Shading_Points;

	Layout layout(*this, 22);
	SET(20, -1, -1, -1)
	if (!header.GetCheck())
	{
		HIDE(centerLabel); HIDE(rangeLabel);
		HIDE(xLabel); HIDE(xCenter); HIDE(xRange); HIDE(xDelta);
		HIDE(yLabel); HIDE(yCenter); HIDE(yRange); HIDE(yDelta);
		HIDE(zLabel); HIDE(zCenter); HIDE(zRange); HIDE(zDelta);
		HIDE(xyzDelta);
		HIDE(uLabel); HIDE(uCenter); HIDE(uRange); HIDE(uDelta);
		HIDE(vLabel); HIDE(vCenter); HIDE(vRange); HIDE(vDelta);
		HIDE(uvDelta);
		HIDE(phiLabel); HIDE(psiLabel); HIDE(thetaLabel);
		HIDE(phi); HIDE(psi); HIDE(theta);
		HIDE(phiDelta); HIDE(psiDelta); HIDE(thetaDelta);
		HIDE(distLabel); HIDE(dist); HIDE(distDelta);
		HIDE(center); HIDE(top); HIDE(front);
	}
	else
	{
		const bool in3d = (plot.axis_type() != Axis::Rect);

		USEH(20, NULL, &centerLabel, &rangeLabel, NULL);

		USE(&xLabel, &xCenter, &xRange, &xDelta);
		xCenter.SetDouble(ax.center(0));
		xRange.SetDouble(ax.range(0)*2.0);

		USE(&yLabel, &yCenter, &yRange, &yDelta);
		yCenter.SetDouble(ax.center(1));
		yRange.SetDouble(ax.range(1)*2.0);
		yRange.EnableWindow(in3d);

		if (!in3d)
		{
			HIDE(zLabel);
			HIDE(zCenter);
			HIDE(zRange);
			HIDE(zDelta);
		}
		else
		{
			USE(&zLabel, &zCenter, &zRange, &zDelta);
			zCenter.SetDouble(ax.center(2));
			zRange.SetDouble(ax.range(2)*2.0);
		}
		USE(NULL, &xyzDelta, &xyzDelta, NULL);

		const int nin = g ? g->inRangeDimension() : -1;

		if (nin < 1)
		{
			HIDE(uLabel);
			HIDE(uCenter);
			HIDE(uRange);
			HIDE(uDelta);
		}
		else
		{
			USE(&uLabel, &uCenter, &uRange, &uDelta);
			uCenter.SetDouble(ax.in_center(0));
			uRange.SetDouble(ax.in_range(0)*2.0);
		}
		if (nin < 2)
		{
			HIDE(vLabel);
			HIDE(vCenter);
			HIDE(vRange);
			HIDE(vDelta);
		}
		else
		{
			USE(&vLabel, &vCenter, &vRange, &vDelta);
			vCenter.SetDouble(ax.in_center(1));
			vRange.SetDouble(ax.in_range(1)*2.0);
		}
		if (nin < 1)
		{
			HIDE(uvDelta);
		}
		else
		{
			USE(NULL, &uvDelta, &uvDelta, NULL);
			layout.skip(2);
		}

		SET(-1, -1, -1);
		if (!in3d)
		{
			HIDE(phiLabel); HIDE(psiLabel); HIDE(thetaLabel);
			HIDE(phi); HIDE(psi); HIDE(theta);
			HIDE(phiDelta); HIDE(psiDelta); HIDE(thetaDelta);
			HIDE(distLabel); HIDE(dist); HIDE(distDelta);
			HIDE(center); HIDE(top); HIDE(front);
		}
		else
		{
			USE(&phiLabel, &psiLabel, &thetaLabel);
			USE(&phi, &psi, &theta);
			USE(&phiDelta, &psiDelta, &thetaDelta);
			phi.SetDouble(cam.phi());
			psi.SetDouble(cam.psi());
			theta.SetDouble(cam.theta());

			USE(&distLabel, &dist, &distDelta);
			layout.skip(2);
			dist.SetDouble(1.0 / cam.zoom());

			USE(&center, &top, &front);
		}

		layout.skip();
	}

	if (full) MoveWindow(0, 0, layout.W, layout.y);
}
