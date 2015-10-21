#include "GL_StringCache.h"

void GL_StringCache::clear()
{
	for (auto &i : cache) delete i.second;
	cache.clear();
	unused.clear();
}

void GL_StringCache::fonts(std::vector<GL_Font> &&F)
{
	if (F == _fonts) return;
	clear();
	_fonts = F;
	assert(!_fonts.empty());
	cf = 0;
}
void GL_StringCache::font(const GL_Font &F)
{
	if (_fonts.size() == 1 && _fonts[0] == F) return;
	clear();
	_fonts.clear();
	_fonts.push_back(F);
	cf = 0;
}

void GL_StringCache::start()
{
	for (auto &i : cache) unused.insert(i.first);
}
GL_String *GL_StringCache::get(const std::string &s)
{
	if (cf < 0 || (size_t)cf >= _fonts.size()){ assert(false); return NULL; }
	Key k = std::make_pair(s, cf);
	if (cache.count(k))
	{
		unused.erase(k);
		return cache[k];
	}
	GL_String *gls = new GL_String(s, _fonts[cf]);
	cache[k] = gls;
	return gls;
}
void GL_StringCache::finish()
{
	for (auto &k : unused)
	{
		delete cache[k];
		cache.erase(k);
	}
	unused.clear();
}
GL_StringCache::~GL_StringCache()
{
	for (auto &i : cache) delete i.second;
}

