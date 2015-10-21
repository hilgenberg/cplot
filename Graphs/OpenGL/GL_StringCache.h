#pragma once
#include "GL_String.h"
#include <map>
#include <set>
#include <vector>

class GL_StringCache
{
public:
	GL_StringCache() : cf(-1){ }
	~GL_StringCache();
	
	GL_StringCache(const GL_StringCache &) = delete;
	GL_StringCache &operator=(const GL_StringCache &) = delete;

	void start();
	void finish();
	GL_String *get(const std::string &s);

	void font(int i) { assert(i >= 0 && (size_t)i < _fonts.size()); cf = i; }
	void fonts(std::vector<GL_Font> &&F);
	void font(const GL_Font &F);

private:
	typedef std::pair<std::string, int> Key;
	std::map<Key, GL_String*> cache;
	std::set<Key> unused;
	int cf; // current_font
	std::vector<GL_Font> _fonts;

	void clear();
};
