#include "../stdafx.h"
#include "SideSectionParams.h"
#include "ViewUtil.h"
#include "../res/resource.h"
#include "../ParameterController.h"
#include "ParameterView.h"
#include "../../Graphs/Plot.h"
#include "../SideView.h"

BEGIN_MESSAGE_MAP(SideSectionParams, SideSection)
	ON_WM_CREATE()
END_MESSAGE_MAP()

enum
{
	ID_header = 2000
};

SideSectionParams::~SideSectionParams()
{
	for (auto *d : params) delete d;
}

void SideSectionParams::OnEdit(Parameter *p)
{
	ParameterController pc(*this, p);
	if (pc.DoModal() == IDOK)
	{
		Redraw();
		parent().Update(true);
	}
}

int SideSectionParams::OnCreate(LPCREATESTRUCT cs)
{
	if (SideSection::OnCreate(cs) < 0) return -1;

	header.SetCanAdd(HeaderControl::Active);

	return 0;
}

void SideSectionParams::Animate(double t)
{
	for (auto *p : params) p->Animate(t);
}

void SideSectionParams::Update(bool full)
{
	SideSection::Update(full);
	CRect bounds; GetWindowRect(bounds);

	const Plot &plot = GetPlot();
	
	if (!full)
	{
		for (auto *q : params)
		{
			q->Update(false);
		}

	}
	
	DS0;

	const int W = bounds.Width();
	const int SPC = DS(5); // amount of spacing
	const int y0 = DS(22);
	int       y = y0;// y for next control
	const int x0 = SPC;  // row x start / amount of space on the left

	const int h_section = DS(20), h_label = DS(14), h_combo = DS(21), h_check = DS(20),
		h_edit = DS(20), h_color = DS(20), h_slider = DS(20), h_delta = h_slider,
		w_slider = h_slider, h_button = h_check, h_row = DS(24);


	//----------------------------------------------------------------------------------
	if (!header.GetCheck())
	{
		for (auto *p : params) HIDE(*p);
	}
	else
	{
		// add new parameters, remove deleted ones, sort by name
		std::map<Parameter*, ParameterView*, std::function<bool(const Parameter*, const Parameter*)>>
			m([](const Parameter *a, const Parameter *b) { return a->name() < b->name(); });
		for (auto *q : params)
		{
			auto *p = q->parameter();
			if (!p)
			{
				delete q;
			}
			else
			{
				m.insert(std::make_pair(p, q));
			}
		}
		for (Parameter *p : plot.ns.all_parameters(true))
		{
			if (m.count(p)) continue;
			ParameterView *q = new ParameterView(*this, *p);
			m.insert(std::make_pair(p, q));
			q->Create(CRect(0, 0, 20, 20), this, 2000 + (UINT)p->oid());
		}
		params.clear(); params.reserve(m.size());
		for (auto i : m) params.push_back(i.second);

		// place the controls
		for (auto *q : params)
		{
			int hq = q->height(W);
			MOVE(*q, 0, W, y, hq, hq);
			q->Update(true);
			y += hq;
		}
		if (!params.empty()) y += SPC;
	}

	if (full) MoveWindow(0, 0, W, y);
}
