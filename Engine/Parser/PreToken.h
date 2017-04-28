#pragma once
#include <string>
#include <vector>
#include <stack>
#include <cassert>

struct ParsingResult;
class Namespace;
class Element;
struct PreToken;
class  ParsingTree;

typedef std::vector<PreToken> PreTokens;

struct PreToken
{
	~PreToken();
	
	enum Type
	{
		TT_Space,
		TT_String,  // substring with ambiguities that Token will figure out with backtracking
		TT_List,    // parenthesized subexpression
		TT_Bar,     // unknown if opening or closing
		TT_OBar,
		TT_CBar,
		TT_Element, // former TT_String part that had only one candidate
		TT_Number,
		TT_Tree     // ParsingTree, former TT_List
	};
	
	Type   type;
	size_t i, n; // source position in string (for error reporting) and contents for TT_String
	std::vector<PreTokens> children; // for TT_List
	union
	{
		Element     *element; // for TT_Element
		double       number;  // for TT_Number
		ParsingTree *pt;      // for TT_Tree
	};
	
	// returns empty TT_List and sets info on error
	static bool parse(PreTokens &dest, const std::string &str, ParsingResult &info, const Namespace &ns);
	
	
	private: struct private_key{ }; public: // get emplace_back to work...
	PreToken(Type t, const std::string &s, std::string::const_iterator b, std::string::const_iterator e, const private_key &) : PreToken(t, s, b, e){ }
	PreToken(Type t, size_t pos, size_t n, const private_key &) : PreToken(t, pos, n){ }
	PreToken(PreToken &t, bool move, const private_key &) : PreToken(t, move){ }
	
private:
	PreToken(Type t, const std::string &s, std::string::const_iterator b, std::string::const_iterator e)
	: type(t), i(b-s.begin()), n(e-b)
	{
		assert(b - s.begin() <= e - s.begin());
	}
	
	PreToken(Type t, size_t pos, size_t n) : type(t), i(pos), n(n){ }
	
	PreToken(PreToken &t, bool move) : type(t.type), i(t.i), n(t.n)
	{
		switch (type)
		{
			case TT_Element: element = t.element; break;
			case TT_Number:  number  = t.number;  break;
			case TT_List:
				if (move) std::swap(children, t.children); else children.reserve(t.children.size());
				break;
			case TT_Tree:
				if (move){ pt = t.pt; t.pt = NULL; }else{ assert(false); pt = NULL; }
				break;
			default: break;
		}
	}
};

void print(std::ostream &out, const PreTokens &t, const std::string &s);

// depth-first over TT_List/children (for converting TT_List to TT_Tree)
void depth_first(PreTokens &root, std::stack<PreTokens*> &stack);
void depth_first(PreTokens &root, std::vector<PreTokens*> &stack);
