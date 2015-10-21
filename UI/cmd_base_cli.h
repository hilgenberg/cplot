#pragma once
#include <string>
#include <cstring>
#include <functional>
#include <string> 
#include <vector>
#include "Command.h"

bool is_int(const std::string &s, int &v_);
bool has_prefix(const std::string &s, const char *p, size_t &l);

struct CommandInfo
{
	CommandInfo(const char *n, const char *n2, CID cid,
		std::function<bool(const std::vector<std::string>&)> f,
		const char *u, const char *d = NULL)
	: name(n), name2(n2), cid(cid), usage(u), desc(d), run(f)
	{}
	CommandInfo(const CommandInfo &) = delete;

	const char * const name, * const name2;
	const char * const usage, *const desc;
	const CID cid;

	const std::function<bool(const std::vector<std::string>&)> run;
	// returns false on parsing errors

	inline operator bool() const{ return name; }

	const char *matches(const char *s, bool partial) const;
	// returns the name on match
};

extern CommandInfo *cli_commands[];

CommandInfo *find_command(const char *line, const char **args = NULL, int cursor = 0, int *cursor_word_index = NULL);

