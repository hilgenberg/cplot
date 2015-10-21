#include "Document.h"
#include "../Persistence/Serializer.h"

void Document::saveAs(const std::string &p) const
{
	FILE *F = fopen(p.c_str(), "w");
	if (!F) throw std::runtime_error(std::string("can't open file for writing: ") + p);
	try
	{
		FileWriter fw(F);
		Serializer  w(fw);
		rns.save(w);
		plot.save(w);
		if (w.version() >= FILE_VERSION_1_8)
		{
			uint32_t box_state = 0; // from mac version
			w._uint32(box_state);
		}
		w._marker("EOF.");
	}
	catch(...)
	{
		fclose(F);
		throw;
	}
	fclose(F);
	path = p;
}

void Document::load(const std::string &p)
{
	FILE *F = fopen(p.c_str(), "r");
	if (!F) throw std::runtime_error(std::string("can't open file for reading: ") + p);
	try
	{
		FileReader   fr(F);
		Deserializer s(fr);
		plot.clear(); // TODO: don't modify on fail
		rns.clear();
		rns.load(s);
		plot.load(s);
		uint32_t box_state;
		if (s.version() >= FILE_VERSION_1_8) s._uint32(box_state);
		s._marker("EOF.");
		assert(s.done());
	}
	catch(...)
	{
		fclose(F);
		throw;
	}
	fclose(F);
	path = p;
}

