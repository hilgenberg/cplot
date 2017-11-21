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

SideSectionGraphs::~SideSectionGraphs()
{
	for (auto *d : defs) delete d;
}

void SideSectionGraphs::OnAdd()
{
	Show();
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
	Show();
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
	
	Layout layout(*this, 22); SET(-1);

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
		DS0;
		for (int i = 0, n = plot.number_of_graphs(); i < n; ++i)
		{
			Graph *f = plot.graph(i);
			if (m.count(f)) continue;
			GraphView *q = new GraphView(*this, *f);
			m.insert(std::make_pair(f, q));
			q->Create(CRect(0, 0, DS(20), DS(22)), this, 4000 + (UINT)f->oid());
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
			USE(q);
			q->Update(true);
		}
		if (!defs.empty()) layout.skip();
	}

	if (full) MoveWindow(0, 0, layout.W, layout.y);
}
