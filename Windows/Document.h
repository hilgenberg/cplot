#pragma once
#include "../Engine/Namespace/RootNamespace.h"
#include "../Graphs/Plot.h"

class Document : public CDocument
{
public:
	Document();
	~Document();

	BOOL OnNewDocument() override;
	void Serialize(CArchive& ar) override;

	RootNamespace       rns;
	Plot                plot;

	union
	{
		struct
		{
			bool params : 1;
			bool defs : 1;
			bool graphs : 1;
			bool settings : 1;
			bool axis : 1;
		};
		uint32_t all;
	} box_state;

	DECLARE_DYNCREATE(Document)
};
