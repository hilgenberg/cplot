#include "stdafx.h"
#include "CPlotApp.h"
#include "Document.h"
#include "../Persistence/Serializer.h"
#include <propkey.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(Document, CDocument)

Document::Document()
: plot(rns)
{
}

Document::~Document()
{
}

BOOL Document::OnNewDocument()
{
	if (!CDocument::OnNewDocument()) return FALSE;

	plot.clear();
	rns.clear();

	return TRUE;
}

void Document::Serialize(CArchive &ar)
{
	if (ar.IsStoring())
	{
		FileWriter fw(ar.GetFile());
		Serializer  w(fw);
		rns.save(w);
		plot.save(w);
		if (w.version() >= FILE_VERSION_1_8)
		{
			uint32_t box_state = 0; // from mac version
			w.uint32_(box_state);
		}
		w.marker_("EOF.");
	}
	else
	{
		FileReader   fr(ar.GetFile());
		Deserializer s(fr);
		plot.clear(); // TODO: don't modify on fail
		rns.clear();
		rns.load(s);
		plot.load(s);
		uint32_t box_state;
		if (s.version() >= FILE_VERSION_1_8) s.uint32_(box_state);
		s.marker_("EOF.");
		assert(s.done());
	}
}
