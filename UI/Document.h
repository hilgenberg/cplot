#pragma once
#include "../Engine/Namespace/RootNamespace.h"
#include "../Graphs/Plot.h"
#include <string>

struct Document
{
	Document() : plot(rns){ }

	virtual void load(const std::string &path);
	void save() const{ saveAs(path); }
	void saveAs(const std::string &path) const;

	RootNamespace       rns;
	Plot                plot;
	mutable std::string path; // empty for new documents
};

