#include "../stdafx.h"
#include "SideSectionDefs.h"
#include "ViewUtil.h"
#include "../res/resource.h"
#include "../DefinitionController.h"
#include "DefinitionView.h"
#include "../../Graphs/Plot.h"
#include "../SideView.h"

BEGIN_MESSAGE_MAP(SideSectionDefs, SideSection)
	ON_WM_CREATE()
END_MESSAGE_MAP()

enum
{
	ID_header = 2000
};

SideSectionDefs::~SideSectionDefs()
{
	for (auto *d : defs) delete d;
}

void SideSectionDefs::OnEdit(UserFunction *f)
{
	DefinitionController pc(*this, f);
	if (pc.DoModal() == IDOK)
	{
		Redraw();
		parent().Update(true);
	}
}

int SideSectionDefs::OnCreate(LPCREATESTRUCT cs)
{
	if (SideSection::OnCreate(cs) < 0) return -1;

	header.SetCanAdd(HeaderControl::Active);

	return 0;
}

void SideSectionDefs::Update(bool full)
{
	SideSection::Update(full);
	CRect bounds; GetWindowRect(bounds);

	const Plot &plot = GetPlot();
	
	if (!full)
	{
		for (auto *q : defs)
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
		for (auto *d : defs) HIDE(*d);
	}
	else
	{
		// add new parameters, remove deleted ones, sort by name
		std::map<UserFunction*, DefinitionView*, std::function<bool(const UserFunction*, const UserFunction*)>>
			m([](const UserFunction *a, const UserFunction *b) { return a->formula() < b->formula(); });
		for (auto *q : defs)
		{
			auto *f = q->function();
			if (!f)
			{
				delete q;
			}
			else
			{
				m.insert(std::make_pair(f, q));
			}
		}
		for (UserFunction *f : plot.ns.all_functions(true))
		{
			if (m.count(f)) continue;
			DefinitionView *q = new DefinitionView(*this, *f);
			m.insert(std::make_pair(f, q));
			q->Create(CRect(0, 0, 20, 20), this, 3000 + (UINT)f->oid());
		}
		defs.clear(); defs.reserve(m.size());
		for (auto i : m) defs.push_back(i.second);

		// place the controls
		for (auto *q : defs)
		{
			int hq = q->height(W);
			MOVE(*q, 0, W, y, hq, hq);
			q->Update(true);
			y += hq;
		}
		if (!defs.empty()) y += SPC;
	}

	if (full) MoveWindow(0, 0, W, y);
}
