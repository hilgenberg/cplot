#pragma once
#include "Token.h"
#include <stack>
#include <vector>
#include <set>
#include <cassert>

class Element;

struct WayPoint // for backtracking
{
	WayPoint() : fresh(true), base(NULL){ }
	
	bool  empty() const{ return points.empty(); }
	
	bool  fresh; // parsing stops on an empty, non-fresh waypoint
	Token *base; // start of waypointed token chain - must not be changed before the last pushed token
	
	void push(size_t i, size_t j, Token *t, const std::vector<Element*> &c)
	{
		assert(t != NULL);
		
		if (fresh)
		{
			fresh = false;
			base = t->first();
		}
		
		assert(!t->prev || base == t->first());
		assert(points.empty() || i > points.top().i || i == points.top().i && j > points.top().j);
		assert(std::set<Element*>(c.begin(), c.end()).size() == c.size());
		
		// when bars get pushed, a closing bar will be tried as an opening one next
		// in that case c should contain a single NULL
		assert(!c.empty());
		
		points.emplace(i, j, t, c);
	}
	
	inline void pop(size_t &i, size_t &j, Token *&t, Element *&e)
	{
		assert(!empty());
		auto &p = points.top();
		assert(!p.candidates.empty());
		i = p.i;
		j = p.j;
		t = p.token;
		e = p.candidates.back();
		assert(base == t->first());
		
		p.candidates.pop_back();
		if (p.candidates.empty()) points.pop();
	}
	
private:
	struct InsertionPoint
	{
		InsertionPoint(size_t i, size_t j, Token *t, const std::vector<Element*> &c)
		: i(i), j(j), token(t), candidates(c){ }
		
		size_t i;     // PreToken index
		size_t j;     // position in substring (preTokens[i].i + j is the position in the parsed string)
		Token *token; // current token at (i,pos), to be modified after pop(...)
		std::vector<Element*> candidates; // last element will be tried next, then removed
	};
	
	std::stack<InsertionPoint> points;
};
