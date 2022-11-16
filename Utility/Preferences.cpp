#include "Preferences.h"
#include "System.h"
#define PATH "5GL\\CPlot\\"

struct Pref
{
	Pref(HKEY key, const char *n, int v_default) : name(n), changed(false), int_default(v_default), int_value(v_default)
	{
		if (!key) return;
		DWORD type = REG_DWORD, bufsize = sizeof(DWORD);
		std::vector<BYTE> buf(bufsize);

		if (RegQueryValueEx(key, CString(name), 0, &type, buf.data(), &bufsize) == 0 && bufsize == sizeof(DWORD))
		{
			int_value = *(DWORD*)buf.data();
		}
	}

	void set(int v)
	{
		if (v == int_value) return;
		int_value = v;
		changed = true;
	}

	bool flush(HKEY key)
	{
		if (!changed) return true;
		DWORD v = (DWORD)int_value;
		bool ok = (RegSetValueEx(key, CString(name), 0, REG_DWORD, (BYTE*)&v, sizeof(DWORD)) == 0);
		if (ok) changed = false;
		return ok;
	}

	void reset(HKEY key)
	{
		DWORD type = REG_DWORD, bufsize = sizeof(DWORD);
		std::vector<BYTE> buf(bufsize);
		if (key && RegQueryValueEx(key, CString(name), 0, &type, buf.data(), &bufsize) == 0 && bufsize == sizeof(DWORD))
		{
			int_value = *(DWORD*)buf.data();
			changed = true;
		}
		else
		{
			int_value = int_default;
		}
		changed = false;
	}

	const char *name;
	bool changed; // was it changed from what's currently written in the registry?
	int int_value, int_default;
};

enum
{
	PREF_DYNAMIC = 0,
	PREF_SLIDEBACK,
	PREF_NORMALS,
	PREF_DSORT,
	PREF_THREADS
};
#define NPREFS 5

struct Prefs
{
	Prefs()
	{
		HKEY key;
		if (RegOpenKeyEx(HKEY_CURRENT_USER, CString(PATH), 0, KEY_QUERY_VALUE, &key) != 0) key = 0;
		cache.reserve(NPREFS);
		cache.emplace_back(key, "dynamic",    true);
		cache.emplace_back(key, "slideback",  true);
		cache.emplace_back(key, "normals",    false);
		cache.emplace_back(key, "depth_sort", false);
		cache.emplace_back(key, "threads",    -1);
		RegCloseKey(key);
	}

	Pref &operator[](int i) { return cache[i]; }

	bool flush()
	{
		HKEY key;
		if (RegCreateKeyEx(HKEY_CURRENT_USER, CString(PATH), 0, NULL, 0, KEY_WRITE, NULL, &key, NULL) != 0) return false;
		bool ok = true;
		for (int i = 0; i < NPREFS; ++i)
		{
			ok &= cache[i].flush(key);
			assert(ok);
		}
		RegCloseKey(key);
		ok &= (RegFlushKey(HKEY_CURRENT_USER) == 0);
		return ok;
	}

	void reset()
	{
		HKEY key;
		if (RegOpenKeyEx(HKEY_CURRENT_USER, CString(PATH), 0, KEY_QUERY_VALUE, &key) != 0) key = 0;
		for (int i = 0; i < NPREFS; ++i)
		{
			cache[i].reset(key);
		}
		RegCloseKey(key);
	}

private:
	std::vector<Pref> cache;
};

static Prefs prefs;


namespace Preferences
{
	bool dynamic() { return prefs[PREF_DYNAMIC].int_value; }
	void dynamic(bool value) { prefs[PREF_DYNAMIC].set(!!value); }

	bool slideback() { return prefs[PREF_SLIDEBACK].int_value; }
	void slideback(bool value) { prefs[PREF_SLIDEBACK].set(!!value); }

	bool drawNormals() { return prefs[PREF_NORMALS].int_value; }
	void drawNormals(bool value) { prefs[PREF_NORMALS].set(!!value); }

	bool depthSort() { return prefs[PREF_DSORT].int_value; }
	void depthSort(bool value) { prefs[PREF_DSORT].set(!!value); }
	
	int  threads(int effective)
	{
		int n = prefs[PREF_THREADS].int_value;
		if (!effective) return n;
		return (n <= 0 || n > 256) ? -1 : n;
	}
	void threads(int n) { prefs[PREF_THREADS].set(n); }

	bool flush() { return prefs.flush(); }
	bool reset() { return prefs.flush(); }
};
