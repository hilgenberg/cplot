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

	DECLARE_DYNCREATE(Document)
};
