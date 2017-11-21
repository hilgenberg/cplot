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
	
	if (!full)
	{
		for (auto *q : defs)
		{
			q->Update(false);
		}
	}
	
	Layout layout(*this, 22, 22); SET(-1);
	const Plot &plot = GetPlot();

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
		DS0;
		for (UserFunction *f : plot.ns.all_functions(true))
		{
			if (m.count(f)) continue;
			DefinitionView *q = new DefinitionView(*this, *f);
			m.insert(std::make_pair(f, q));
			q->Create(CRect(0, 0, DS(20), DS(22)), this, 3000 + (UINT)f->oid());
		}
		defs.clear(); defs.reserve(m.size());
		for (auto i : m) defs.push_back(i.second);

		// place the controls
		for (auto *q : defs)
		{
			USE(q);
			q->Update(true);
		}
		if (!defs.empty()) layout.skip();
	}

	if (full) MoveWindow(0, 0, layout.W, layout.y);
}
