#include "Token.h"
#include "Waypoint.h"
#include "ParsingResult.h"
#include "../Namespace/RootNamespace.h"
#include "../Namespace/Operator.h"
#include "readnum.h"
#include "readconst.h"
#include "utf8/utf8.h"
#include "ParsingTree.h"

#include <cmath>
#include <ostream>
#include <iostream>
#include <cassert>
#include <algorithm>

//----------------------------------------------------------------------------------------------------------------------
// Printing
//----------------------------------------------------------------------------------------------------------------------

std::ostream & operator<< (std::ostream &out, const Token &token)
{
	switch(token.type)
	{
		case Token::TT_Variable:
		case Token::TT_Alias:
		case Token::TT_Function:
		case Token::TT_Operator:
		case Token::TT_Constant:
		case Token::TT_Parameter:out << token.data.element->name(); break;
			
		case Token::TT_Space:    out << "⌴"; break;
		case Token::TT_Number:   out << to_string(token.num); break;
		case Token::TT_Pow:      out << "^[" << to_string(token.num) << "]"; break;
		case Token::TT_OBar:     out << "["; break;
		case Token::TT_CBar:     out << "]"; break;
		case Token::TT_Args: token.data.args ? out << *token.data.args : out << "{}"; break;
		default: out << "???"; break;
	}
	return out;
}

//----------------------------------------------------------------------------------------------------------------------
// c'tor, d'tor, comparison
//----------------------------------------------------------------------------------------------------------------------

Token::Token(TokenType _type, void *data_, size_t _pos, size_t _len)
: next(NULL), prev(NULL)
, pos(_pos), len(_len)
, type(_type)
{
	switch(type)
	{
		case TT_Variable:  data.variable  =  (Variable     *)data_; break;
		case TT_Function:  data.operator_ =  (Operator     *)data_; break;
		case TT_Operator:  data.function  =  (Function     *)data_; break;
		case TT_Constant:  data.constant  =  (Constant     *)data_; break;
		case TT_Parameter: data.parameter =  (Parameter    *)data_; break;
		case TT_Args: if ((data.args      =  (ParsingTree  *)data_)) data.args->retain(); break;
		case TT_Alias:     data.alias     =  (AliasVariable*)data_; break;
		case TT_Pow:
		case TT_Number:    if (data_) num = *(const cnum *)data_; // fallthrough
		default:           data.function  = NULL;                   break;
	}
}
Token::Token(TokenType type, const PreToken &T, void *data_) : Token(type, data_, T.i, T.n){ }

Token::Token(const Token &t)
: prev(NULL), next(NULL), pos(t.pos), len(t.len), type(t.type), data(t.data), num(t.num)
{
	if (type == TT_Args && data.args) data.args->retain();
}

Token::~Token()
{
	if (type == TT_Args && data.args) data.args->release();
	
	if (prev) prev->next = NULL;
	
	Token *t = next;
	while (t)
	{
		Token *s = t->next;
		t->prev = t->next = NULL;
		delete t;
		t = s;
	}
}

Token *Token::copy()
{
	Token *ret = new Token(*this);
	try
	{
		for (Token *n = next, *t = ret; n; n = n->next, t = t->next){ assert(t); t->append (new Token(*n)); }
		for (Token *p = prev, *t = ret; p; p = p->prev, t = t->prev){ assert(t); t->prepend(new Token(*p)); }
	}
	catch(...)
	{
		delete ret->first();
		throw;
	}
	return ret;
}

//----------------------------------------------------------------------------------------------------------------------
// linked list and traversal
//----------------------------------------------------------------------------------------------------------------------

void Token::append(Token *token)
{
	assert(token);
	Token *t = this;
	while(t->next) t = t->next;
	while(token->prev) token = token->prev;
	t->next = token;
	token->prev = t;
}

void Token::prepend(Token *token)
{
	assert(token);
	token->append(this);
}

void Token::insert(Token *token, bool before)
{
	assert(token);
	assert(!token->next);
	assert(!token->prev);
	if (before)
	{
		token->next = this;
		if (prev)
		{
			token->prev = prev;
			prev->next = token;
		}
		prev = token;
	}
	else
	{
		token->prev = this;
		if (next)
		{
			token->next = next;
			next->prev = token;
		}
		next = token;
	}
}

//----------------------------------------------------------------------------------------------------------------------
// informational
//----------------------------------------------------------------------------------------------------------------------

bool Token::is_valid_operand(bool left) const
{
	const Token *t = this;
	if (left)
	{
		while(t && t->type == TT_Space) t = t->prev; if(!t) return false;
		switch(t->type)
		{
			case TT_Function: return t->data.function->arity() == 0;
			case TT_Operator: return t->data.operator_->postfix();
			default: return true;
		}
	}
	else
	{
		while(t && t->type == TT_Space) t = t->next; if(!t) return false;
		switch(t->type)
		{
			case TT_Operator: return t->data.operator_->prefix();
			case TT_Pow:      return false;
			default: return true;
		}
	}
}

bool Token::is_valid_binary() const
{
	return type == TT_Operator && data.operator_->binary() && next && prev &&
	prev->is_valid_operand(true) && next->is_valid_operand(false);
}

bool Token::is_valid_func_or_prefix() const
{
	if (type == TT_Function) return data.function->arity() == 0 || next && next->is_valid_operand(false);
	if (type == TT_Operator && data.operator_->postfix() || type == TT_Pow) return next && next->is_valid_operand(false);
	return false;
}

bool Token::is_valid_postfix() const
{
	return (type == TT_Pow || type == TT_Operator && data.operator_->postfix()) && prev && prev->is_valid_operand(true);
}

bool Token::is_valid_void_func() const
{
	return type == TT_Function && data.function->arity() == 0;
}

int Token::arity(bool left) const
{
	switch (type)
	{
		case TT_Space:
		case TT_Args:
		case TT_Constant:
		case TT_Number:
		case TT_Variable:
		case TT_Alias:
		case TT_Parameter: return 0;
			
		case TT_OBar: return left ? 0 : 1;
		case TT_CBar: return left ? 1 : 0;
			
		case TT_Function: return left ? 0 : data.function->arity();
		case TT_Operator:
			return data.operator_->binary() ? 1 :
			data.operator_->prefix() ? (left ? 0 : 1) : (left ? 1 : 0);
		case TT_Pow: return left ? 1 : 0;
	}
	assert(false); throw std::logic_error("can't happen");
}

int Token::num_args() const
{
	return (type == TT_Args && data.args) ? (int)data.args->num_children() : 0;
}

//----------------------------------------------------------------------------------------------------------------------
// Parsing
//----------------------------------------------------------------------------------------------------------------------

static Token::TokenType typeFor(const Element *e)
{
	if      (e->isVariable ()) return Token::TT_Variable;
	else if (e->isAlias    ()) return Token::TT_Alias;
	else if (e->isConstant ()) return Token::TT_Constant;
	else if (e->isOperator ()) return Token::TT_Operator;
	else if (e->isFunction ()) return Token::TT_Function;
	else if (e->isParameter()) return Token::TT_Parameter;
	else INCONSISTENCY("Invalid element type");
}

static void arities(const PreTokens &T0, int i, int &a_min, int &a_max)
{
	// sets the possible range of arities for the Token at i, depending on what i+1 is
	int n = (int)T0.size();
	const PreToken *t;
	do t = (++i < n ? &T0[i] : NULL); while (t && t->type == PreToken::TT_Space);
	if (!t){ a_min = -1; a_max = 0; return; }
	switch (t->type)
	{
		case PreToken::TT_Space:  assert(false); a_min = a_max = -1; break;
		case PreToken::TT_CBar:   a_min = -1; a_max = 0; break;
		case PreToken::TT_List:   a_min = a_max = (int)t->children.size(); break;
		case PreToken::TT_Tree:   a_min = a_max = (t->pt ? (int)t->pt->num_children() : 0); break;
		case PreToken::TT_OBar:
		case PreToken::TT_Bar:    // union of OBar/CBar ranges
		case PreToken::TT_Number:
		case PreToken::TT_String: a_min = -1; a_max = 1; break;
		case PreToken::TT_Element:
		{
			switch (typeFor(t->element))
			{
				case Token::TT_Function:
				case Token::TT_Constant:
				case Token::TT_Variable:
				case Token::TT_Parameter:
				case Token::TT_Number:
				case Token::TT_Alias: a_min = -1; a_max = 1; break;
					
				case Token::TT_Operator:
					a_min = -1;
					a_max = (((Operator*)t->element)->prefix() ? 1 : 0);
					break;
					
				case Token::TT_Pow: a_min = -1; a_max = 0; break; // same as postfix operator
					
				default: assert(false); a_min = a_max = -1; break;
			}
			break;
		}
		default: assert(false); a_min = a_max = -1; break;
	}
}

Token *Token::parse(const std::string &str, const PreTokens &T0, ParsingResult &info, const Namespace &ns, WayPoint &wp)
{
	assert(!T0.empty());
	const size_t N = T0.size();
	
	while (true) // backtrack until we find a valid parsing or run out of options
	{
		if (wp.empty() && !wp.fresh)
		{
			delete wp.base;
			return NULL;
		}
		wp.fresh = false;
		
		info.reset();
		
		size_t i = 0, j = 0;
		int    bar_level = 0; // bar nesting level up to i / current token
		Token *last = NULL, *lastNS = NULL; // last, last non-space tokens
		
		//--------------------------------------------------------------------------------------------------------------
		// update state to next waypoint
		//--------------------------------------------------------------------------------------------------------------
		
		if (!wp.empty())
		{
			Element *e;
			wp.pop(i, j, last, e);
			if (e)
			{
				assert(T0[i].type == PreToken::TT_String);
				last->type = typeFor(e);
				last->data.function = (Function*)e; // any pointer works
				last->len = e->namelen();
				j += last->len;
				if (j >= T0[i].n){ j = 0; ++i; }
			}
			else
			{
				assert(T0[i].type == PreToken::TT_Bar);
				assert(last->type == TT_CBar);
				last->type = TT_OBar;
				last->len = 1;
				assert(T0[i].n == 1 && j == 0);
				++i;
			}
			delete last->next; assert(!last->next);
			lastNS = last;
			if (lastNS->is_space()) lastNS = lastNS->prev_nonspace();
			
			for (Token *t = last->first(); t; t = t->next)
			{
				switch (t->type)
				{
					case TT_OBar: assert(bar_level >= 0); ++bar_level; break;
					case TT_CBar: assert(bar_level >  0); --bar_level; break;
					default: break;
				}
			}
		}
		
		// check that the bars can work out
		int nbar = 0, nobar = 0, ncbar = 0; // number of bar types in the remaining PreTokens
		for (size_t k = i; k < N; ++k)
		{
			switch (T0[k].type)
			{
				case PreToken::TT_Bar:  ++nbar;  break;
				case PreToken::TT_OBar: ++nobar; break;
				case PreToken::TT_CBar: ++ncbar; break;
				default: break;
			}
		}
		if (bar_level < 0 || bar_level+nobar-(ncbar+nbar) > 0 || bar_level+(nobar+nbar)-ncbar < 0)
		{
			info.error("Mismatched |...| bars", last->pos, 1);
			continue;
		}
		
		//--------------------------------------------------------------------------------------------------------------
		// parse the remaining PreTokens
		//--------------------------------------------------------------------------------------------------------------
		
		while(i < N)
		{
			const PreToken &T = T0[i]; // current PreToken to convert
			Token    *t = NULL;  // the next (or first) converted Token
			switch (T.type)
			{
				case PreToken::TT_Space: ++i; j = 0; t = new Token(TT_Space, T); break;
				case PreToken::TT_OBar:  ++i; j = 0; t = new Token(TT_OBar,  T); --nobar; ++bar_level; break;
				case PreToken::TT_CBar:  ++i; j = 0; t = new Token(TT_CBar,  T); --ncbar; --bar_level; break;
				case PreToken::TT_List: assert(false); info.error("Internal error", T.i, T.n); return NULL;
					
				case PreToken::TT_Bar:
					if (bar_level == 0 || bar_level+(nobar+nbar)-ncbar == 0 || lastNS && lastNS->arity(false) > 0)
					{
						t = new Token(TT_OBar, T);
						--nbar; ++bar_level;
					}
					else if (bar_level > 0 && bar_level+nobar-(ncbar+nbar) == 0)
					{
						t = new Token(TT_CBar, T);
						--nbar; --bar_level;
					}
					else
					{
						t = new Token(TT_CBar, T);
						wp.push(i, j, t, std::vector<Element *>(1, nullptr));
						--nbar; --bar_level;
					}
					++i; j = 0;
					break;
					
				case PreToken::TT_Number: ++i; j = 0; t = new Token(TT_Number, T); t->num = T.number; break;
				case PreToken::TT_Element: ++i; j = 0; t = new Token(typeFor(T.element), T, T.element); break;
					
				case PreToken::TT_Tree: ++i; j = 0; t = new Token(TT_Args, T, T.pt); break;
					
				case PreToken::TT_String:
				{
					int c = str[T.i+j];
					assert(!isspace(c));
					if (isdigit(c) || c == '.' || c == 'c')
					{
						double x;
						int tlen = (int)(c == 'c' ? readconst : readnum)(str, T.i+j, T.i+T.n, x);
						if (tlen)
						{
							t = new Token(TT_Number, NULL, T.i+j, tlen);
							t->num = x;
							j += tlen;
						}
					}
					else if (last)
					{
						int re, im, tlen = (int)readexp(str, T.i+j, T.i+T.n, re, im);
						if (tlen)
						{
							t = new Token(TT_Pow, NULL, T.i+j, tlen);
							t->num.real(re);
							t->num.imag(im);
							j += tlen;
						}
					}
					
					//--------------------------------------------------------------------------------------------------
					// The hard part
					//--------------------------------------------------------------------------------------------------
					
					if (!t)
					{
						std::vector<Element*> matches = ns.candidates(str, T.i+j);
						assert(std::set<Element*>(matches.begin(), matches.end()).size() == matches.size());
						
						// minimize the number of matches
						int a_min, a_max; arities(T0, (int)i, a_min, a_max);
						bool postfix_ok = (lastNS && lastNS->is_valid_operand(true));
						for (int m = (int)matches.size() - 1; m >= 0; --m)
						{
							const Element &match = *matches[m];
							bool good = true;
							size_t ml = match.namelen();
							int a0 = a_min, a1 = a_max;
							if (j+ml < T.n){ a0 = -1; a1 = 1; } // next is still this string
							if (match.isFunction())
							{
								int a = ((const Function &)match).arity();
								if (a < a0 || a > a1) good = false;
							}
							else if (match.isOperator())
							{
								const Operator &op = (Operator &) match;
								int a = (op.binary() || op.prefix() ? 1 : 0);
								if ((a < a0 || a > a1) || !postfix_ok && (op.binary() || op.postfix()))
								{
									good = false;
								}
							}
							else
							{
								assert(match.arity() == -1);
								if (-1 < a0 || -1 > a1) good = false;
							}
							if (good && j+ml < T.n)
							{
								// see if we would have any matches for what remains
								int c = str[T.i+j+ml];
								
								if (isalpha(c) && c != 'c') // this could be more precise...
								{
									if (ns.candidates(str, T.i+j+ml).empty()) good = false;
								}
							}
							
							if (!good) matches.erase(matches.begin()+m);
						}
						
						// sort matches in ascending preference
						bool prefer_binary = last && (!last->is_space() || j+1 == T.n && i+1 < N && T0[i+1].type == PreToken::TT_Space);
						
						std::sort(matches.begin(), matches.end(), [&](Element *a, Element *b)->bool
						{
							if (a == b) return false;
							
							// prefer longer matches (first matches go to the back, so we're somewhat inverted)
							size_t la = a->namelen(), lb = b->namelen();
							if (la != lb) return la < lb;
							
							// match binary before prefix operators according to surrounding spaces
							if (a->isOperator() && b->isOperator())
							{
								Operator *ao = (Operator*)a, *bo = (Operator*)b;
								if (ao->prefix() && bo->binary()) return prefer_binary;
								if (bo->prefix() && ao->binary()) return !prefer_binary;
							}
							
							// prefer functions with fewer arguments (especially -1 before 0)
							if (a->arity() != b->arity()) return a->arity() > b->arity();
							
							// sort by type: variable/alias, parameter, userfunction, basefunction, constant
							int ta = (a->isVariable() || a->isAlias()) ? 0 : a->isParameter() ? 1 : a->isFunction() ? ((Function*)a)->base() ? 2 : 3 : a->isConstant() ? 4 : 5;
							int tb = (b->isVariable() || b->isAlias()) ? 0 : b->isParameter() ? 1 : b->isFunction() ? ((Function*)b)->base() ? 2 : 3 : b->isConstant() ? 4 : 5;
							assert(ta < 5 && tb < 5 && ta != tb); // if this fails, make it deterministic again
							return ta > tb;
						});
						
						// pick the first match and put the rest into the waypoint
						if (matches.empty()) break;
						
						Element *best = matches.back();
						matches.pop_back();
						t = new Token(typeFor(best), best, T.i+j, best->namelen());
						if (!matches.empty()) wp.push(i, j, t, matches);
						j += best->namelen();
					}
					break;
				} // end of TT_String case
			} // end of switch
			
			if (!t)
			{
				info.error("Unrecognized token", T.i, T.n);
				break;
			}
			
			if (last) last->append(t);
			last = t->last(); if (last->type != TT_Space) lastNS = last;
			
			if (j >= T.n){ assert(j == T.n); j = 0; ++i; }
			
			if (!wp.base) wp.base = last->first();
			
			if (bar_level < 0 || bar_level+nobar-ncbar-nbar > 0 || bar_level+nobar-ncbar+nbar < 0)
			{
				info.error("Mismatched |...| bars", last->pos, 1);
				break;
			}
		}// end of while over string
		
		if (!last)
		{
			assert(!info.ok);
			return NULL; // failed on the very first token
		}
		if (info.ok && bar_level != 0)
		{
			assert(false);
			info.error("Missing closing |...| bars", 0, str.length());
			continue;
		}
		if (info.ok)
		{
			assert(wp.base == last->first());
			return wp.base;
		}
	} // end of while(true)
}
