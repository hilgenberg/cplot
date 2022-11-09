#include "Preferences.h"
#include "System.h"
#include "StringFormatting.h"
#include <pwd.h>
using namespace std::filesystem;
static bool load();
static bool save();
static bool have_changes = false; // were any defaults changed?

static bool normals_   = false;
static bool dynamic_   = true;
static bool depthSort_ = true;
static int  threads_   = -1;

namespace Preferences
{
	path directory()
	{
		static path cfg;
		if (cfg.empty())
		{
			const char *home = getenv("HOME");
			if (!home) {
				passwd *pw = getpwuid(geteuid());
				if (pw) home = pw->pw_dir;
			}
			if (!home) return cfg;

			path h = weakly_canonical(home);
			auto cfg1 = h / ".config/cplot", cfg2 = h / ".cplot";
			cfg = is_directory(cfg1) ? cfg1 : is_directory(cfg2) ? cfg2 : 
				is_directory(h / ".config") ? cfg1 : cfg2;
			create_directories(cfg);
		}
		return cfg;
	}

	bool reset()
	{
		normals_   = false;
		dynamic_   = true;
		depthSort_ = true;
		threads_   = -1;
		load(); have_changes = false;
		return true;
	}
	bool flush() { return !have_changes || save(); }

	#define SET(x) do{ if ((x)==value) return; (x) = value; have_changes = true; }while(0)

	bool dynamic() { return dynamic_; }
	void dynamic(bool value) { SET(dynamic_); }

	bool drawNormals() { return normals_; }
	void drawNormals(bool value) { SET(normals_); }

	bool depthSort() { return depthSort_; }
	void depthSort(bool value) { SET(depthSort_); }

	int  threads() { return threads_ <= 0 ? n_cores : threads_; }
	void threads(int value) { SET(threads_); }
};


static path config_file()
{
	static path cfg = Preferences::directory();
	if (cfg.empty()) return cfg;
	return cfg / "cplot.ini";
}

static bool parse(const char *v, bool &o)
{
	if      (!strcasecmp(v, "yes")) o = true;
	else if (!strcasecmp(v, "no" )) o = false;
	else if (!strcasecmp(v, "1"  )) o = true;
	else if (!strcasecmp(v, "0"  )) o = false;
	else if (!strcasecmp(v, "on" )) o = true;
	else if (!strcasecmp(v, "off")) o = false;
	else
	{
		fprintf(stderr, "Invalid boolean: %s\n", v);
		return false;
	}
	return true;
}
static bool parse(const char *v, int &o)
{
	if (is_int(v, o)) return true;
	fprintf(stderr, "Invalid integer: %s\n", v);
	return false;
}
static bool load()
{
	path cf = config_file(); if (cf.empty()) return false;
	if (!is_regular_file(cf)) return false;
	FILE *file = fopen(cf.c_str(), "r");
	if (!file)
	{
		fprintf(stderr, "cannot read from config file %s!\n", cf.c_str());
		return false;
	}
	int line = 0;
	while (true)
	{
		int c = fgetc(file); if (c == EOF) break;
		++line;
		
		// skip initial whitespace and comment lines
		while (isspace(c)) c = fgetc(file); if (c == EOF) break;
		if (c == '#')
		{
			do c = fgetc(file); while (c != EOF && c != '\n');
			continue;
		}

		// read this option's key
		std::string key, value;
		while (!isspace(c) && c != EOF && c != '=') { key += c; c = fgetc(file); }

		// skip space, equal sign, space
		while (isspace(c)) c = fgetc(file); if (c == EOF) break;
		if (c != '=')
		{
			fprintf(stderr, "garbage in config file on line %d\n", line);
			do c = fgetc(file); while (c != EOF && c != '\n');
			continue;
		}
		c = fgetc(file);
		while (isspace(c) && c != EOF) c = fgetc(file);

		// read this option's value
		if (c == '\'' || c == '"')
		{
			int q = c; c = fgetc(file);
			while (c != EOF && c != '\n' && c != q)
			{
				value += c;
				c = fgetc(file);
			}
			if (c != q) value.insert(0, 1, (char)q);
			else while (c != '\n' && c != EOF) c = fgetc(file);
		}
		else
		{
			while (c != '#' && c != '\n' && c != EOF) { value += c; c = fgetc(file); }
			while (!value.empty() && isspace(value.back())) value.pop_back();
			if (c == '#') while (c != '\n' && c != EOF) c = fgetc(file);
		}

		const char *v = value.c_str();
		if      (key == "normals"  ) parse(v, normals_);
		else if (key == "dynamic"  ) parse(v, dynamic_);
		else if (key == "depthSort") parse(v, depthSort_);
		else if (key == "threads"  ) parse(v, threads_);
		else
		{
			fprintf(stderr, "Invalid key: %s\n", key.c_str());
		}
	}
	fclose (file);
	have_changes = false;
	return true;
}

static bool save()
{
	path cf = config_file(); if (cf.empty()) return false;
	FILE *file = fopen(cf.c_str(), "w");
	if (!file)
	{
		fprintf(stderr, "cannot write to config file %s!\n", cf.c_str());
		return false;
	}

	fprintf(file, "# auto-generated - file will be overwritten by preference dialog!\n");
	fprintf(file, "normals=%s\n", normals_ ? "on" : "off");
	fprintf(file, "dynamic=%s\n", dynamic_ ? "on" : "off");
	fprintf(file, "depthSort=%s\n", depthSort_ ? "on" : "off");
	fprintf(file, "threads=%d\n", threads_);
	fclose (file);
	have_changes = false;
	return true;
}

