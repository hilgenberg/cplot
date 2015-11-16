#pragma once

#include <string>
class Expression;

struct ParsingResult
{
	ParsingResult() : ok(true), index(0), pos(0), len(0) { }
	
	bool        ok;        ///< if ok is false, the other fields will be set
	std::string info;      ///< displayable error message
	size_t      index;     ///< index of the input string in the list
	size_t      pos, len;  ///< position of error in the input string

	void error(const std::string &info_, size_t pos_, size_t len_)
	{
		ok   = false;
		info = info_;
		pos  = pos_;
		len  = len_;
	}
	
	void reset(bool reset_index = false)
	{
		ok = true;
		info.clear();
		pos = len = 0;
		if (reset_index) index = 0;
	}

	void print(const Expression &ex) const;
};
