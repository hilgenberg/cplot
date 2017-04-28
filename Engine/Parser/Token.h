#pragma once
#include "../cnum.h"
#include "PreToken.h"
#include <string>
#include <cassert>

struct ParsingResult;
class Namespace;
class RootNamespace;
class Function;
class Constant;
class Operator;
class Variable;
class Parameter;
class AliasVariable;
struct WayPoint;
class ParsingTree;
	
class Token
{
public:
	// Parsing breaks the remaining strings into tokens.
	// caller must delete only the return value of the last call (wp contains pointers to the previous result)
	static Token* parse(const std::string &str, const PreTokens &pt, ParsingResult &info, const Namespace &ns, WayPoint &wp);
	
	Token *copy(); // copy the entire string of Tokens, caller must delete the result
	
	enum TokenType
	{
		TT_Space,
		TT_Function,
		TT_Constant,
		TT_Operator,
		TT_Pow,      // unicode exponent, treat as postfix operator
		TT_Variable,
		TT_Parameter,
		TT_Number,
		TT_Alias,    // AliasVariable
		TT_OBar,     // opening |
		TT_CBar,     // closing |
		TT_Args
	};
	
	Token(TokenType type, void *data, size_t pos, size_t len);
	Token(TokenType type, const PreToken &T, void *data_ = NULL);
	explicit Token(const Token &t);
	explicit Token(const cnum  &z) : prev(NULL), next(NULL), type(TT_Number), pos(0), len(0), num(z){ data.args = NULL; }
	~Token(); // also deletes next, next->next, ... and clears prev->next
	
	Token *prev, *next; // doubly linked list
	size_t pos, len;    // origin in parsed string
	
	TokenType type;
	
	union
	{
		const Element       *element;
		const Variable      *variable;
		const Function      *function;
		const Operator      *operator_;
		const Constant      *constant;
		const Parameter     *parameter;
		const AliasVariable *alias;
		ParsingTree         *args;
	}
	data;
	
	cnum num; // for TT_Number and TT_Pow
	
	inline void eject(bool and_delete = true)
	{
		// remove from Token chain, linking prev and next together
		if (next) next->prev = prev;
		if (prev) prev->next = next;
		prev = next = NULL;
		if(and_delete) delete this;
	}
	inline void cut(bool and_delete = true)
	{
		// remove from Token chain, not linking prev and next together
		if (next) next->prev = NULL;
		if (prev) prev->next = NULL;
		prev = next = NULL;
		if(and_delete) delete this;
	}
	inline bool connected(Token *token)
	{
		for (Token *t = this; t; t = t->next) if (t == token) return true;
		return false;
	}
	inline Token *cut_and_splice(Token *end)
	{
		// eject the Tokens from this up to end, linking prev and end->next together
		assert(end && connected(end));
		if (prev) prev->next = end->next;
		if (end->next) end->next->prev = prev;
		prev = end->next = NULL;
		return this;
	}
	
	inline Token *prev_nonspace(){ Token *t = prev; while(t && t->type == TT_Space) t = t->prev; return t; }
	inline Token *next_nonspace(){ Token *t = next; while(t && t->type == TT_Space) t = t->next; return t; }
	inline Token *first(){ Token *t = this; while(t->prev) t = t->prev; return t; }
	inline Token *last (){ Token *t = this; while(t->next) t = t->next; return t; }
	inline const Token *first() const{ const Token *t = this; while(t->prev) t = t->prev; return t; }
	inline const Token *last () const{ const Token *t = this; while(t->next) t = t->next; return t; }
	
	int  arity(bool left = false) const;
	inline bool is_space() const{ return type == TT_Space; }
	bool is_valid_binary() const;
	bool is_valid_operand(bool left) const;
	bool is_valid_func_or_prefix() const;
	bool is_valid_postfix() const;
	bool is_valid_void_func() const;
	int  num_args() const;
	
	void  append(Token *token);
	void prepend(Token *token);
	void  insert(Token *token, bool before = false);
};

std::ostream &operator<<(std::ostream &out, const Token &token);
