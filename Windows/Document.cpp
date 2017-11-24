#include "stdafx.h"
#include "CPlotApp.h"
#include "Document.h"
#include "../Persistence/Serializer.h"
#include "MainWindow.h"
#include "SideView.h"
#include <propkey.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(Document, CDocument)

BOOL Document::OnNewDocument()
{
	if (!CDocument::OnNewDocument()) return FALSE;

	plot.clear();
	rns.clear();
	plot.add_graph();

	MainWindow* w = (MainWindow*)AfxGetMainWnd();
	SideView *sv = w ? &w->GetSideView() : NULL;
	if (sv)
	{
		sv->UpdateAll();
		sv->Redraw();
		w->OnFocusEdit();
	}

	return TRUE;
}

void Document::Serialize(CArchive &ar)
{
	MainWindow* w = (MainWindow*)AfxGetMainWnd();
	SideView *sv = w ? &w->GetSideView() : NULL;

	try
	{
		if (ar.IsStoring())
		{
			FileWriter fw(ar.GetFile());
			Serializer  w(fw);
			rns.save(w);
			plot.save(w);
			BoxState b; b.all = -1;
			if (sv) b = sv->GetBoxState();
			if (w.version() >= FILE_VERSION_1_8) w.uint32_(b.all);
			w.marker_("EOF.");
		}
		else
		{
			if (w) w->OnFocusGraph();
			FileReader   fr(ar.GetFile());
			Deserializer s(fr);
			plot.clear(); // TODO: don't modify on fail
			rns.clear();
			rns.load(s);
			plot.load(s);
			BoxState b; b.all = -1;
			if (s.version() >= FILE_VERSION_1_8) s.uint32_(b.all);
			s.marker_("EOF.");
			assert(s.done());
			if (sv)
			{
				sv->SetBoxState(b);
				sv->UpdateAll();
				sv->Redraw();
			}
		}
	}
	catch (const std::exception &ex)
	{
		MessageBox(NULL, CString(ex.what()), _T("Error"), MB_OK | MB_ICONERROR);
		THROW(new CArchiveException);
	}
	catch (...)
	{
		MessageBox(NULL, _T("Unexpected exception"), _T("Error"), MB_OK | MB_ICONERROR);
		THROW(new CArchiveException);
	}
}
