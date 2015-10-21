#include "PreToken.h"
#include "ParsingResult.h"
#include "utf8/utf8.h"
#include "../../Utility/StringFormatting.h"
#include "../Namespace/BaseFunction.h"
#include "../Namespace/RootNamespace.h"
#include "readnum.h"
#include "readconst.h"
#include "ParsingTree.h"
#include <stack>
#include <iostream>
#include <cstring>

PreToken::~PreToken()
{
	if (type == TT_Tree && pt) pt->release();
}

void depth_first(PreTokens &root, std::stack<PreTokens*> &stack)
{
	assert(stack.empty());
	std::stack<PreTokens*> todo;
	todo.push(&root); // things with children to collect
	while (!todo.empty())
	{
		PreTokens *A = todo.top(); todo.pop();
		stack.push(A);
		for (PreToken &a : *A)
		{
			if (a.type == PreToken::TT_List) for (PreTokens &B : a.children) todo.push(&B);
		}
	}
}
void depth_first(PreTokens &root, std::vector<PreTokens*> &stack)
{
	assert(stack.empty());
	std::stack<PreTokens*> todo;
	todo.push(&root); // things with children to collect
	while (!todo.empty())
	{
		PreTokens *A = todo.top(); todo.pop();
		stack.push_back(A);
		for (PreToken &a : *A)
		{
			if (a.type == PreToken::TT_List) for (PreTokens &B : a.children) todo.push(&B);
		}
	}
}

typedef std::string::const_iterator SI;

#define ERROR0(err, i, n) do{\
	info.error(err, i, n);\
	root.clear();\
	return false; }while(0)

#define ERROR(err, i0, i1) do{\
	assert(i0 - s.begin() <= i1 - s.begin());\
	ERROR0(err, i0-s.begin(), i1-i0); }while(0)
	
#define CHECK(x) do{ assert(x); if (!(x)) ERROR("Unknown parsing error - please file a bug report for this expression", s.begin(), s.end()); }while(0)

bool PreToken::parse(PreTokens &root, const std::string &s, ParsingResult &info, const Namespace &ns)
{
	assert(info.ok);
	assert(root.empty());
	RootNamespace *rns = ns.root_container();
	CHECK(rns);
	
	//------------------------------------------------------------------------------------------------------------------
	// Check nesting and basic sanity
	//------------------------------------------------------------------------------------------------------------------
	
	if (s.length() == 0) ERROR("Empty expression", s.begin(), s.end());
	
	{
		struct PI{ char op, cp; unsigned bars; SI i; };
		std::stack<PI> plevel;
		plevel.push({0,0,0,s.begin()}); // for counting toplevel bars
		auto i = s.begin();
		try
		{
			for (; info.ok && i != s.end(); utf8::next(i, s.end()))
			{
				switch (*i)
				{
					case '|': ++plevel.top().bars; break;
						
					case '(': plevel.push({'(', ')', 0, i}); break;
					case '{': plevel.push({'{', '}', 0, i}); break;
					case '[': plevel.push({'[', ']', 0, i}); break;
						
					case ',':
						if (!plevel.top().op) ERROR("Stray comma", i, i+1);
						if (i+1 == s.end() || strchr(")}],", i[1])) ERROR("Invalid comma", i, i+1);
						if (plevel.top().bars & 1) ERROR("Odd number of bars", plevel.top().i, i+1);
						break;
						
					case ')':
					case '}':
					case ']':
					{
						PI &t = plevel.top();
						if (t.cp != *i)
						{
							if (!t.op) ERROR(format("Extraneous %c", *i), i, i+1);
							ERROR(format("Missing '%c' or mismatched '%c'", t.cp, *i), t.i, i+1);
						}
						if (t.bars & 1) ERROR("Odd number of bars", t.i, i+1);
						
						plevel.pop();
						break;
					}
				}
			}
			if (info.ok)
			{
				if (plevel.top().op) ERROR(format("Missing '%c'", plevel.top().cp), plevel.top().i, s.end());
				if (plevel.top().bars & 1) ERROR("Odd number of bars", s.begin(), s.end());
			}
		}
		catch(utf8::exception &e)
		{
			ERROR("Invalid unicode", i, s.end());
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// Create initial PreToken list
	//------------------------------------------------------------------------------------------------------------------
	
	assert(info.ok);
	
	PreTokens *t = &root; // t is where the next PreToken should be appended
	std::stack<PreTokens*> T; // and T.top() is where a closing paren jumps back to
	
	auto i = s.begin();
	while (info.ok && i != s.end())
	{
		switch (*i)
		{
			case '|': t->emplace_back(TT_Bar, s, i, i+1, private_key()); ++i; break;
				
			case '(':
			case '{':
			case '[':
				t->emplace_back(TT_List, s, i, i+1, private_key());
				T.push(t); // push toplevel
				t->back().children.emplace_back();
				t = &t->back().children.back();
				++i; break;
				
			case ',':
				CHECK(!T.empty());
				t = T.top(); // back to toplevel
				t->back().children.emplace_back();
				t = &t->back().children.back();
				++i; break;
				
			case ')':
			case '}':
			case ']':
				CHECK(!T.empty());
				t = T.top(); T.pop(); // back to toplevel
				if (!t->empty())
				{
					assert(t->back().n == 1);
					t->back().n = i-s.begin()+1 - t->back().i;
				}
				++i; break;
				
			default:
			{
				auto i0 = i;
				if (isspace(*i))
				{
					do ++i; while (i != s.end() && isspace(*i));
					t->emplace_back(TT_Space, s, i0, i, private_key());
				}
				else
				{
					do utf8::next(i, s.end()); while (i != s.end() && !isspace(*i) && !strchr("|(){}[],", *i));
					t->emplace_back(TT_String, s, i0, i, private_key());
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// Simplify and check for empty arguments
	//------------------------------------------------------------------------------------------------------------------
	
	assert(info.ok);
	assert(!root.empty());
	
	std::stack<PreTokens*> todo;
	depth_first(root, todo);
	
	while (!todo.empty())
	{
		PreTokens &A = *todo.top(); todo.pop();
		
		//--------------------------------------------------------------------------------------------------------------
		// remove leading and trailing space
		//--------------------------------------------------------------------------------------------------------------
		
		if (!A.empty() && A.back().type == TT_Space) A.pop_back();
		if (!A.empty() && A[0]    .type == TT_Space) A.erase(A.begin());
		
		//--------------------------------------------------------------------------------------------------------------
		// simplify nested lists
		//--------------------------------------------------------------------------------------------------------------
		
		for (PreToken &a : A)
		{
			if (a.type != TT_List) continue;
			
			while (a.children.size() == 1 && a.children[0].size() == 1 && a.children[0][0].type == TT_List)
			{
				std::vector<PreTokens> tmp;
				std::swap(tmp, a.children[0][0].children);
				a.children = tmp; // ((a,b,c)) --> (a,b,c)
			}
			for (PreTokens &B : a.children)
			{
				while (B.size() == 1 && B[0].type == TT_List)
				{
					if (B[0].children.size() == 1)
					{
						PreTokens tmp;
						std::swap(tmp, B[0].children[0]);
						B = tmp; // ((a),(b),(c + d)) --> (a,b,c + d)
					}
					else
					{
						// ((a,b), c) and (a, ()) --> error
						ERROR0("Invalid parentheses nesting", a.i, a.n);
					}
				}
			}
			
			//----------------------------------------------------------------------------------------------------------
			// check empty arguments
			//----------------------------------------------------------------------------------------------------------
			
			for (PreTokens &B : a.children)
			{
				if (B.empty())
				{
					if (a.children.size() == 1)
					{
						a.children.clear();
						break;
					}
					else
					{
						ERROR0("Empty argument", a.i, a.n);
					}
				}
			}
		}
		
		//--------------------------------------------------------------------------------------------------------------
		// figure out opening and closing bars as far as possible
		//--------------------------------------------------------------------------------------------------------------
		
		// the first consecutive set of bars must be opening
		size_t n = A.size();
		size_t i = 0, no = 0, nc = 0, nb = 0;
		bool first = true;
		for (i = 0; i < n; ++i)
		{
			if (A[i].type == TT_Space) continue;
			if (A[i].type == TT_Bar){ A[i].type = TT_OBar; ++no; first = false; continue; }
			if (!first) break;
			
		}
		// the last consecutive set of bars must be closing
		first = true;
		for (i = n; i > 0; --i)
		{
			if (A[i-1].type == TT_Space) continue;
			if (A[i-1].type == TT_Bar){ A[i-1].type = TT_CBar; ++nc; first = false; continue; }
			if (!first) break;
		}
		for (; i > 0; --i) if (A[i-1].type == TT_Bar) ++nb;
		
		// check if this can work out
		if (no > nc + nb || nc > no + nb)
		{
			ERROR0("Mismatched bars", A[0].i, A.back().i - A[0].i + A.back().n);
		}
		
		// in some cases we know what every bar is (if there are only two, for instance)
		if (nb > 0 && nc == no + nb)
		{
			for (i = 0; i < n; ++i) if (A[i].type == TT_Bar) A[i].type = TT_OBar;
		}
		else if (nb > 0 && no == nc + nb)
		{
			for (i = 0; i < n; ++i) if (A[i].type == TT_Bar) A[i].type = TT_CBar;
		}
		
		//--------------------------------------------------------------------------------------------------------------
		// convert unique elements
		//--------------------------------------------------------------------------------------------------------------
		
		assert(n == A.size());
		
		for (int i = 0; i < (int)n; ++i)
		{
			PreToken &a = A[i];
			if (a.type != TT_String) continue;
			assert(a.n > 0);
			
			char c = s[a.i];
			if (isdigit(c) || c=='.' || c =='c')
			{
				size_t  l = (c == 'c' ? readconst : readnum)(s, a.i, a.i+a.n, a.number);
				assert (l <= a.n);
				if (l > 0)
				{
					auto ai = a.i, an = a.n;
					a.type = TT_Number;
					a.n = l;
					if (l < an){ A.insert(A.begin()+(i+1), PreToken(TT_String, ai+l, an-l)); ++n; }
					continue;
				}
			}
			else if (a.n > 1)
			{
				// protect unicode exponents from becoming a sequence of postfix ops
				int re, im; size_t tlen = readexp(s, a.i, a.i+a.n, re, im);
				if (tlen)
					try
				{
					size_t nc = utf8::distance(s.begin()+a.i, s.begin()+(a.i+tlen));
					if (nc > 1)
					{
						if (tlen < a.n)
						{
							auto ai = a.i, an = a.n;
							a.n = tlen;
							A.insert(A.begin()+(i+1), PreToken(TT_String, ai+tlen, an-tlen)); ++n;
						}
						continue;
					}
				}
				catch(...)
				{
					continue;
				}
			}
			
			auto cd = ns.candidates(s, a.i);
			if (cd.size() == 1)
			{
				size_t l = cd[0]->namelen();
				CHECK(l <= a.n);
				size_t ai = a.i, an = a.n;
				a.type = TT_Element; a.element = cd[0];
				a.n = l;
				if (l < an){ A.insert(A.begin()+(i+1), PreToken(TT_String, ai+l, an-l)); ++n; } // this can invalidate a
				continue;
			}
			
			// do not fail on c.empty() because we don't do the full thing here!
			if (!cd.empty())
			{
				size_t l = cd[0]->namelen();
				for (size_t j = 1, m = cd.size(); j < m; ++j)
				{
					if (cd[j]->namelen() != l){ l = 0; break; }
				}
				if (l > 0 && l < a.n)
				{
					size_t ai = a.i, an = a.n;
					a.n = l;
					A.insert(A.begin()+i+1, PreToken(TT_String, ai+l, an-l)); ++n;
					continue;
				}
			}
			
			// check from the other end
			cd = ns.rcandidates(s, a.i, a.n);
			if (cd.size() == 1)
			{
				size_t l = cd[0]->namelen();
				assert (l <= a.n);
				size_t ai = a.i, an = a.n;
				a.type = TT_Element; a.element = cd[0];
				a.i += a.n - l;
				a.n = l;
				if (l < an){ A.insert(A.begin()+i, PreToken(TT_String, ai, an-l)); ++n; --i; } // this can invalidate a
				continue;
			}
			if (!cd.empty())
			{
				size_t l = cd[0]->namelen();
				for (size_t j = 1, m = cd.size(); j < m; ++j)
				{
					if (cd[j]->namelen() != l){ l = 0; break; }
				}
				if (l > 0 && l < a.n)
				{
					size_t an = a.n;
					a.n -= l;
					A.insert(A.begin()+i+1, PreToken(TT_String, a.i+a.n, an-a.n)); ++n; --i;
					continue;
				}
			}
		}
		
		//--------------------------------------------------------------------------------------------------------------
		// remove spaces between functions and lists with size != 1
		//--------------------------------------------------------------------------------------------------------------
		
		assert(n == A.size());
		
		for (int i = (int)n - 1; i > 0; --i)
		{
			PreToken &a = A[i], &b = A[i+1];
			if (a.type != TT_Space || b.type != TT_List || b.children.size() == 1) continue;
			A.erase(A.begin()+i);
			--n;
			--i; // skip the function (or garbage non-space)
		}
		
		//--------------------------------------------------------------------------------------------------------------
		// parenthesize OBar-CBar pairs
		//--------------------------------------------------------------------------------------------------------------
		
		assert(n == A.size());
		bool change;
		do
		{
			change = false;
			int lo = -1; // last OBar
			for (int i = 0; i < (int)n; ++i)
			{
				PreToken &a = A[i];
				if      (a.type == TT_OBar) lo =  i;
				else if (a.type == TT_Bar ) lo = -1;
				else if (a.type == TT_CBar && lo >= 0)
				{
					if (i == lo+1) ERROR0("Empty |...|", A[lo].i, a.i+a.n-A[lo].i);
					
					// |...| --> (abs(...))
					
					A[lo].type = TT_List;
					A[lo].n = a.i+a.n-A[lo].i;
					A[lo].children.emplace_back();
					PreTokens &B = A[lo].children[0];
					
					B.emplace_back(TT_Element, A[lo].i, 1, private_key());
					B[0].element = rns->Abs;
					
					B.emplace_back(TT_List, B[0].i, B[0].n, private_key());
					B[1].children.emplace_back();
					PreTokens &C = B[1].children[0];
					
					C.reserve(i-lo-1);
					for (int j = lo+1; j < i; ++j)
					{
						C.emplace_back(A[j], true, private_key());
					}
					
					A.erase(A.begin()+(lo+1), A.begin()+(i+1));
					
					change = true;
					break; // restart at the beginning
				}
			}
		}
		while (change);
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// remove parens around entire expression
	//------------------------------------------------------------------------------------------------------------------
	
	while (root.size() == 1 && root[0].type == TT_List)
	{
		if (root[0].children.size() == 1)
		{
			PreTokens tmp;
			std::swap(tmp, root[0].children[0]);
			root = tmp;
		}
		else if (root[0].children.size() == 0)
		{
			ERROR0("Empty expression", 0, s.length());
		}
		else
		{
			ERROR0("Extraneous commas", 0, s.length());
		}
	}
	
	return true;
}

static void print(std::ostream &o, const PreToken &t, const std::string &s)
{
	switch (t.type)
	{
		case PreToken::TT_Space:  o << "⌴"; break;
		case PreToken::TT_String: o << '"' << s.substr(t.i, t.n) << '"'; break;
		case PreToken::TT_Bar:    o << "|"; break;
		case PreToken::TT_OBar:   o << "["; break;
		case PreToken::TT_CBar:   o << "]"; break;
		case PreToken::TT_Number: o << t.number; break;
		case PreToken::TT_List:
		{
			bool first = true;
			o << '(';
			for (const PreTokens &T : t.children)
			{
				if (!first) o << ", "; first = false;
				print(o, T, s);
			}
			o << ')';
			break;
		}
		case PreToken::TT_Element: o << t.element->name(); break;
		case PreToken::TT_Tree:    o << *t.pt; break;
	}
}

void print(std::ostream &o, const PreTokens &T, const std::string &s)
{
	for (const PreToken &t : T) print(o, t, s);
}
