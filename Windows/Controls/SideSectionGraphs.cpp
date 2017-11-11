#include "../stdafx.h"
#include "SideSectionGraphs.h"
#include "ViewUtil.h"
#include "../res/resource.h"
#include "../DefinitionController.h"
#include "DefinitionView.h"
#include "../../Graphs/Plot.h"
#include "../SideView.h"
#include "../Document.h"
#include "../MainWindow.h"
#include "../MainView.h"
#include "GraphView.h"

BEGIN_MESSAGE_MAP(SideSectionGraphs, SideSection)
	ON_WM_CREATE()
END_MESSAGE_MAP()

enum
{
	ID_header = 2000
};

void SideSectionGraphs::OnAdd()
{
	Plot &plot = document().plot;
	plot.add_graph();
	plot.update_axis();
	parent().Update(true);
	Redraw();
	MainWindow *w = (MainWindow*)GetParentFrame();
	w->GetMainView().Update();
}

void SideSectionGraphs::OnRemove()
{
	Plot &plot = document().plot;
	Graph *g = plot.current_graph();
	if (!g) return;
	if (IDOK == MessageBox(_T("Really delete graph?"), _T("Confirmation"), MB_ICONQUESTION | MB_OKCANCEL))
	{
		plot.delete_graph(g);
		plot.update_axis();
		parent().Update(true);
		Redraw();

		MainWindow *w = (MainWindow*)GetParentFrame();
		w->GetMainView().Update();
	}
}

int SideSectionGraphs::OnCreate(LPCREATESTRUCT cs)
{
	if (SideSection::OnCreate(cs) < 0) return -1;

	header.SetCanAdd(HeaderControl::Active);
	header.SetCanRemove(HeaderControl::Inactive);

	return 0;
}

void SideSectionGraphs::Update(bool full)
{
	const Plot &plot = GetPlot();
	const Graph *g = GetGraph();
	header.SetCanRemove(g ? HeaderControl::Active : HeaderControl::Inactive);

	SideSection::Update(full);

	if (!full)
	{
		for (auto *q : defs)
		{
			q->Update(false);
		}
		return;
	}
	
	DS0;

	CRect bounds; GetWindowRect(bounds);
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
		// add new graphs, remove deleted ones, sort by name
		std::map<Graph*, GraphView*> m;
		for (auto *q : defs)
		{
			auto *f = q->graph();
			if (!f)
			{
				delete q;
			}
			else
			{
				m.insert(std::make_pair(f, q));
			}
		}
		for (int i = 0, n = plot.number_of_graphs(); i < n; ++i)
		{
			Graph *f = plot.graph(i);
			if (m.count(f)) continue;
			GraphView *q = new GraphView(*this, *f);
			m.insert(std::make_pair(f, q));
			q->Create(CRect(0, 0, 20, 20), this, 4000 + (UINT)f->oid());
		}
		defs.clear(); defs.reserve(m.size());
		for (int i = 0, n = plot.number_of_graphs(); i < n; ++i)
		{
			Graph *f = plot.graph(i);
			auto it = m.find(f);
			if (it == m.end()) { assert(false); continue; }
			defs.push_back(it->second);
		}

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
