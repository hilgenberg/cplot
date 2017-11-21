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

void SideSectionParams::Change(int i, cnum delta)
{
	if (i < 0 || (size_t)i >= params.size()) return;
	params[i]->Change(delta);
}

void SideSectionParams::ToggleAnimation(int i)
{
	if (i < 0 || (size_t)i >= params.size()) return;
	params[i]->OnAnimate();
}
void SideSectionParams::StopAllAnimation()
{
	for (auto *p : params) p->StopAnimation();
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

	Layout layout(*this, 22, 44); SET(-1);

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
		DS0;
		for (Parameter *p : plot.ns.all_parameters(true))
		{
			if (m.count(p)) continue;
			ParameterView *q = new ParameterView(*this, *p);
			m.insert(std::make_pair(p, q));
			q->Create(CRect(0, 0, DS(20), DS(44)), this, 2000 + (UINT)p->oid());
		}
		params.clear(); params.reserve(m.size());
		for (auto i : m) params.push_back(i.second);

		// place the controls
		for (auto *q : params)
		{
			USE(q);
			q->Update(true);
		}
		if (!params.empty()) layout.skip();
	}

	if (full) MoveWindow(0, 0, layout.W, layout.y);
}
