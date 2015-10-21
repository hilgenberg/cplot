#include "ParsingTree.h"
#include "Waypoint.h"
#include "../Namespace/RootNamespace.h"
#include "../Namespace/Operator.h"
#include <ostream>
#include <iostream>
#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// Printing
//----------------------------------------------------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &out, const ParsingTree &tree)
{
	int nc = tree.num_children();
	
	if(!tree.head)
	{
		out << "{";
		for (int i = 0; i < nc; ++i)
		{
			if (i > 0) out << ", ";
			out << *tree.child(i);
		}
		out << "}";
		return out;
	}
	
	Token &h = *tree.head;
	
	if (h.type == Token::TT_Function)
	{
		out << h << "(";
		for (int i = 0; i < nc; ++i)
		{
			if (i > 0) out << ", ";
			out << *tree.child(i);
		}
		out << ")";
	}
	else if (h.type == Token::TT_Operator)
	{
		if (h.data.operator_->prefix())
		{
			out << "(" << h << *tree.child(0) << ")";
		}
		else if (h.data.operator_->postfix())
		{
			out << "(" << *tree.child(0) << h << ")";
		}
		else
		{
			out << "(" << *tree.child(0) << " " << h << " " << *tree.child(1) << ")";
		}
	}
	else
	{
		assert(nc == 0);
		out << h;
	}
	return out;
}

//----------------------------------------------------------------------------------------------------------------------
// ParsingTree::parse: Translate a string into a ParsingTree.
//----------------------------------------------------------------------------------------------------------------------
ParsingTree *ParsingTree::parse(const std::string &s, const Namespace &ns, ParsingResult &info)
{
	info.reset();
	
	if (s.length() == 0)
	{
		info.error("Empty expression", 0, 0);
		return NULL;
	}
	
	#ifdef PARSER_DEBUG
	std::cerr << endl << "Parsing function: " << s << endl;
	#endif
	
	// check parens and bars, remove parens and split on whitespace, convert as much as possible
	PreTokens T0;
	if (!PreToken::parse(T0, s, info, ns))
	{
		assert(!info.ok);
		return NULL;
	}
	assert(info.ok);
	#ifdef PARSER_DEBUG
	std::cerr << "After pretokenizing: ";
	print(std::cerr, T0, s);
	std::cerr << endl;
	#endif
	
	std::stack<PreToken*>  todo;  // TT_Lists to convert
	std::stack<PreTokens*> todo0; // PreTokens to scan for lists
	todo0.push(&T0);
	while (!todo0.empty())
	{
		PreTokens &A = *todo0.top(); todo0.pop();
		for (PreToken &a : A)
		{
			if (a.type == PreToken::TT_List)
			{
				todo.push(&a);
				for (PreTokens &B : a.children) todo0.push(&B);
			}
		}
	}
	
	// convert TT_List to TT_Tree
	while (!todo.empty())
	{
		PreToken &a = *todo.top(); todo.pop();
		a.pt   = (new ParsingTree)->retain();
		a.type = PreToken::TT_Tree;
		for (PreTokens &B : a.children)
		{
			ParsingTree *c = ParsingTree::parse(s, B, info, ns);
			if (!c)
			{
				assert(!info.ok);
				return NULL;
			}
			a.pt->add_child(c);
		}
	}
	assert(info.ok);
	
	// convert root
	ParsingTree *c = ParsingTree::parse(s, T0, info, ns);
	if (!c)
	{
		assert(!info.ok);
		return NULL;
	}
	while (!c->head)
	{
		assert(false);
		if (c->num_children() != 1)
		{
			assert(false);
			delete c;
			info.error("Internal error (not expecting list) - Please file a bug report for this expression", 0, 0);
			return NULL;
		}
		ParsingTree *c0 = c;
		c = c0->extract_child(0);
		delete c0;
	}
	assert(info.ok);
	
	#ifdef DEBUG
	for (auto *cc : *c) assert(cc->check_retained(false));
	#endif
	
	return c;
}

//----------------------------------------------------------------------------------------------------------------------
// ParsingTree::parse: Translate a string of PreTokens into Tokens and then into a ParsingTree.
// Returns NULL on error and fills info with details (and deletes token)
// Returns a new ParsingTree on success (caller has to delete it)
//----------------------------------------------------------------------------------------------------------------------

ParsingTree *ParsingTree::parse(const std::string &str, PreTokens &T0, ParsingResult &info, const Namespace &ns)
{
	#ifdef DEBUG
	for (PreToken &t : T0) assert(t.type != PreToken::TT_List && !(t.type == PreToken::TT_Tree && t.pt && t.pt->head));
	#endif
	
	RootNamespace *rns = ns.root_container();
	assert(rns);
	
	WayPoint wp;
	
	// (1) parse the strings into tokens (with backtracking)
	while (Token *t0 = Token::parse(str, T0, info, ns, wp))
	{
		assert(info.ok);
		
		Token *tokens = t0->copy(); // working copy (we still need t0 for backtracking)
		
		#ifdef PARSER_DEBUG
		std::cerr << "After tokenizing: ";
		for(Token *t = tokens; t; t=t->next) std::cerr << *t << ' ';
		std::cerr << std::endl;
		#endif
		
		// (2) assemble the tokens into trees
		assert(info.ok);
		ParsingTree *pt = ParsingTree::parse(tokens, info, *rns);
		assert(!pt || info.ok);
		if(!pt || !pt->verify(info)){ delete pt; continue; }
		assert(info.ok);
		
		#ifdef PARSER_DEBUG
		std::cerr << "After tree building: " << *pt << std::endl << std::endl;
		#endif
		
		delete t0;
		return pt;
	}
	
	assert(!info.ok);
	return NULL;
}

//----------------------------------------------------------------------------------------------------------------------
// ParsingTree::parse: Translate a string of tokens into a ParsingTree.
// Input tokens will always be completely deleted before the function returns
// Returns NULL on error and fills info with details (and deletes token)
// Returns a new ParsingTree on success (caller has to delete it)
// RootNamespace is needed to get some operators (namely implicit multiplication)
//----------------------------------------------------------------------------------------------------------------------

bool ParsingTree::parenthesize(Token *&tokens, Token *begin, Token *end, ParsingResult &info, const RootNamespace &rns)
{
	assert(begin && end);
	Token *prev = begin->prev, *next = end->next;
	Token *argt = begin->cut_and_splice(end);
	if (!prev) tokens = next; // can be NULL now
	
	size_t pos = begin->pos, len = end->pos+end->len-begin->pos;
	ParsingTree *arg = ParsingTree::parse(argt, info, rns);
	if (!arg)
	{
		assert(!info.ok);
		return false;
	}
	Token *t = new Token(Token::TT_Args, new ParsingTree, pos, len);
	t->data.args->add_child(arg);
	
	if (prev) prev->insert(t, false); else if (next){ next->insert(t, true); tokens = t->first(); } else tokens = t;
	return true;
}

ParsingTree *ParsingTree::parse(Token *token, ParsingResult &info, const RootNamespace &rns)
{
	assert(info.ok);
	assert(token && !token->prev);
	
	//------------------------------------------------------------------------------------------------------------------
	// remove bars: |x| --> abs(x)
	//------------------------------------------------------------------------------------------------------------------
	
	#ifdef DEBUG
	int bars = 0;
	for (Token *t = token->first(); t; t = t->next)
	{
		if (t->type == Token::TT_CBar){ --bars; assert(bars >= 0); }
		if (t->type == Token::TT_OBar){ ++bars; }
	}
	assert(bars == 0);
	#endif
	
	for (bool change = true; change; )
	{
		change = false;
		Token *o = NULL;
		for (Token *t = token->first(); t; t = t->next)
		{
			if (t->type == Token::TT_OBar) o = t;
			if (o && t->type == Token::TT_CBar)
			{
				if (t == o->next)
				{
					info.error("Bars without argument", o->pos, o->len);
					delete token; return NULL;
				}
				Token *argt = o->next->cut_and_splice(t->prev);
				assert(o->next == t && t->prev == o);
				ParsingTree *arg = parse(argt, info, rns);
				if (!arg)
				{
					assert(!info.ok);
					delete token; return NULL;
				}
				t->data.args = (new ParsingTree)->retain();
				t->type = Token::TT_Args;
				t->data.args->add_child(arg);
				
				o->type = Token::TT_Function;
				o->data.function = rns.Abs;
				change = true;
			}
		}
	}
	#ifdef PARSER_DEBUG
	std::cerr << "After bar removal: ";
	for(Token *t = token; t; t = t->next) std::cerr << *t << ' ';
	std::cerr << std::endl;
	#endif
	
	//------------------------------------------------------------------------------------------------------------------
	// remove spaces
	//------------------------------------------------------------------------------------------------------------------
	
	// remove spaces at the beginning
	if (token->is_space())
	{
		if(!token->next){ info.error("Empty subexpression", 0, 0); delete token; return NULL; }
		token = token->next;
		token->prev->eject();
		assert(!token->is_space());
	}
	
	// remove trailing spaces
	if (token->last()->is_space())
	{
		if(!token->next){ info.error("Empty subexpression", 0, 0); delete token; return NULL; }
		token->last()->eject();
	}
	
	// remove all spaces like these: f (x,y)
	for (Token *t = token; t; t = t->next)
	{
		if (t->type != Token::TT_Function || !t->next || !t->next->is_space()) continue;
		assert(t->next->next);
		const Function *f = t->data.function;
		if (f->arity() < 2 || t->next->next->type != Token::TT_Args) continue;
		assert(false); // should already have been done
		t->next->eject();
	}
	
	// put parens around any block where it makes sense
	for (Token *t = token; t; t = t->next)
	{
		if (!t->is_space()) continue;
		assert(t->next && t->prev);
		
		// basic checks (would it be valid, would it be obviously redundant?)
		bool pbefore = true, pafter = true; // can we put an ) before or a ( after the space?
		switch(t->prev->type)
		{
			case Token::TT_Function: pbefore = false; break;
			case Token::TT_Operator:
			{
				const Operator *op = t->prev->data.operator_;
				if (op->binary() || op->prefix()){ pbefore = false; break; }
				// fallthrough
			}
			default:
			{
				// check if it would be useless
				Token *tpp = t->prev->prev;
				if (!tpp || tpp->type == Token::TT_Space) pbefore = false;
				break;
			}
		}
		switch (t->next->type)
		{
			case Token::TT_Pow: pafter = false; break;
			case Token::TT_Operator:
			{
				const Operator *op = t->next->data.operator_;
				if (op->binary() || op->postfix()){ pafter = false; break; }
				// fallthrough
			}
			default:
			{
				Token *tnn = t->next->next;
				if (!tnn || tnn->is_space()) pafter = false;
				break;
			}
		}
		
		if (!pbefore && !pafter) continue;
		
		// figure out where to put the opening paren
		if (pbefore)
		{
			Token *tp = t->prev;
			while((tp = tp->prev))
			{
				if (tp->is_space())
				{
					if (tp->next->type == Token::TT_Operator)
					{
						const Operator *op = tp->next->data.operator_;
						if (op->prefix()) break; // otherwise we can't put an OParen there
					}
					else if (!(tp->prev && tp->prev->type == Token::TT_Function))
					{
						break;
					}
				}
			}
			if (!parenthesize(token, tp ? tp->next : token, t->prev, info, rns))
			{
				assert(!info.ok);
				delete token;
				return NULL;
			}
		}
		if (pafter)
		{
			Token *tn = t->next;
			while((tn = tn->next))
			{
				if (tn->is_space())
				{
					if (tn->prev->type != Token::TT_Operator)
					{
						if (tn->prev->type != Token::TT_Function) break;
						if (tn->prev->data.function->arity() == 0) break;
					}
					else
					{
						const Operator *op = tn->prev->data.operator_;
						if (op->postfix()) break; // otherwise we can't put a CParen there
					}
				}
			}
			if (!parenthesize(token, t->next, tn ? tn->prev : t->last(), info, rns))
			{
				assert(!info.ok);
				delete token;
				return NULL;
			}
		}
		
		// cases: x + y, x+ y, x +y, sin x+y, sin x+y !
		//       (x)+ y,   x+y / z+w
		// x ^ sin foo !
		// x ^ foo !
		// f x+y !   sinx+ y ! x + a*b ! x+sin y+z +
		// f (x , y)
		
		// sin (x+y)*z
	}
	
	// remove all spaces
	token = token->first();
	assert(!token->is_space());
	for (Token *t = token->next; t; t = t->next) if (t->is_space()){ t = t->prev; t->next->eject(); }
	
	#ifdef PARSER_DEBUG
	std::cerr << "After space removal: ";
	for(Token *t = token; t; t = t->next) std::cerr << *t << ' ';
	std::cerr << std::endl;
	#endif
	
	//------------------------------------------------------------------------------------------------------------------
	// Normalizing: f() --> f
	//------------------------------------------------------------------------------------------------------------------
	
	for (Token *t = token; t; t = t->next)
	{
		if (t->type != Token::TT_Function || t->data.function->arity() != 0) continue;
		if (!t->next || t->next->type != Token::TT_Args || !t->next->data.args->children.empty()) continue;
		t->next->eject();
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// single token, (x) --> x
	//------------------------------------------------------------------------------------------------------------------
	
	if (!token->next)
	{
		if (token->type == Token::TT_Args)
		{
			if (token->num_args() == 1)
			{
				ParsingTree *pt = token->data.args->children[0]->retain();
				delete token;
				if (pt){ assert(pt->retain_count >= 1); --pt->retain_count; }
				return pt;
			}
			info.error("Unexpected list", token->pos, token->len);
			delete token; return NULL;
		}
		if (token->arity() > 0)
		{
			info.error("Missing parameters", token->pos, token->len);
			delete token; return NULL;
		}
		return new ParsingTree(token);
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// find an operator with lowest precedence
	//------------------------------------------------------------------------------------------------------------------
	
	Token *lowOpToken = NULL; int prec; const Operator *lowOp = NULL; bool implicit = false;
	for (Token *t = token; t; t = t->next)
	{
		int p; const BinaryOperator *op = NULL; bool imp;
		if (t->is_valid_binary())
		{
			op  = (const BinaryOperator*) t->data.operator_;
			p = op->precedence();
			imp = false;
		}
		else if (t->next && t->is_valid_operand(true) && t->next->is_valid_operand(false))
		{
			op  = rns.Mul;
			p   = rns.ImplicitMultiplicationPrecedence();
			imp = true;
		}
		else if (t->type == Token::TT_Operator && t->data.operator_->binary())
		{
			info.error("Binary operator with missing operands", t->pos, t->len);
			delete token; return NULL;
		}
		
		if(!op) continue;
		assert(!lowOp || p != prec || op->rightbinding() == ((BinaryOperator*)lowOp)->rightbinding());
		if (lowOp && (prec < p || p == prec && op->rightbinding())) continue;
		
		prec = p;
		lowOp = op;
		lowOpToken = t; // for implicit multiplication, this will be the left token
		implicit = imp;
	}
	if (lowOp)
	{
		Token *a = token, *b = lowOpToken->next;
		ParsingTree *ret;
		if (!implicit)
		{
			lowOpToken->cut(false);
			ret = new ParsingTree(lowOpToken);
		}
		else
		{
			b->prev->next = NULL;
			b->prev = NULL;
			Token *head = new Token(Token::TT_Operator, (void*)lowOp, b->pos, 0);
			ret = new ParsingTree(head);
		}
		ParsingTree *A = parse(a, info, rns);
		if (!A){ delete b; delete ret; return NULL; }
		ParsingTree *B = parse(b, info, rns);
		if (!B){ delete A; delete ret; return NULL; }
		ret->add_child(A);
		ret->add_child(B);
		return ret;
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// find functions and unary operators
	//------------------------------------------------------------------------------------------------------------------
	
	if (token->type == Token::TT_Function || token->type == Token::TT_Operator)
	{
		if (token->type == Token::TT_Operator && token->data.operator_->postfix())
		{
			info.error("Postfix operator without operand", token->pos, token->len);
			delete token; return NULL;
		}
		if (!token->next || !token->next->is_valid_operand(false))
		{
			info.error(token->type == Token::TT_Function ? "Function without argument" :
					   "Unary operator without operand", token->pos, token->len);
			delete token; return NULL;
		}
		
		Token *head = token;
		token = token->next;
		head->cut(false);
		ParsingTree *ret = new ParsingTree(head);
		if (head->type == Token::TT_Function && token->type == Token::TT_Args && !token->next)
		{
			// add the parameters
			if (token->data.args)
			{
				for (ParsingTree *c : token->data.args->children)
				{
					ret->add_child(c);
				}
			}
			delete token;
		}
		else
		{
			ParsingTree *c = parse(token, info, rns);
			if(!c){ delete ret; return NULL; }
			ret->add_child(c);
		}
		#ifdef DEBUG
		for (ParsingTree *c : ret->children) assert(c->check_retained(false));
		#endif
		
		return ret;
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// find postfix operators
	//------------------------------------------------------------------------------------------------------------------
	
	Token *last = token->last();
	
	
	if (last->type == Token::TT_Operator)
	{
		if (!last->data.operator_->postfix())
		{
			info.error("Unary operator without operand", last->pos, last->len);
			delete token; return NULL;
		}
		if (!last->prev || !last->prev->is_valid_operand(true))
		{
			info.error("Postfix operator without operand", last->pos, last->len);
			delete token; return NULL;
		}
		last->cut(false);
		ParsingTree *ret = new ParsingTree(last);
		ParsingTree *A = parse(token, info, rns);
		if (!A){ delete ret; return NULL; }
		ret->add_child(A);
		return ret;
	}
	if (last->type == Token::TT_Pow)
	{
		if (!last->prev || !last->prev->is_valid_operand(true))
		{
			info.error("Postfix exponent without operand", last->pos, last->len);
			delete token; return NULL;
		}
		last->cut(false);
		last->type = Token::TT_Number;
		ParsingTree *ret = new ParsingTree(new Token(Token::TT_Operator, rns.Pow, last->pos, last->len));
		ParsingTree *A = parse(token, info, rns);
		if (!A){ delete ret; delete last; return NULL; }
		ret->add_child(A);
		ret->add_child(new ParsingTree(last));
		return ret;
	}
	
	if (token->type == Token::TT_Pow)
	{
		info.error("Postfix exponent without operand", token->pos, token->len);
		delete token; return NULL;
	}
	
	//--- give up ------------------------------------------------------------------------------------------------------
	info.error("Cannot parse", token->pos, token->last()->pos + token->last()->len - token->pos);
	delete token; return NULL;
}

//----------------------------------------------------------------------------------------------------------------------
// Check number of parameters and sanity
//----------------------------------------------------------------------------------------------------------------------

bool ParsingTree::verify(ParsingResult &info)
{
	switch(head->type)
	{
		case Token::TT_Space:
		case Token::TT_OBar:
		case Token::TT_CBar:
		case Token::TT_Pow:
		case Token::TT_Args:
			info.error("Internal error: Structure token survived the tree building", head->pos, head->len);
			return false;
		case Token::TT_Alias:
		case Token::TT_Constant:
		case Token::TT_Parameter:
		case Token::TT_Variable:
		case Token::TT_Number:
			if(children.size() > 0)
			{
				info.error("Internal error: Constant with parameters", head->pos, head->len);
				return false;
			}
			return true;
		case Token::TT_Function:
		case Token::TT_Operator:
		{
			const Function *f = head->data.function;
			size_t nc = children.size();
			if((int)nc != f->arity())
			{
				info.error("Function with wrong number of arguments", head->pos, head->len);
				return false;
			}
			for(size_t i = 0; i < nc; ++i)
			{
				if(!children[i]->verify(info)) return false;
			}
			return true;
		}
		default:
			info.error("Internal error: Unknown token type", head->pos, head->len);
			return false;
	}
}
