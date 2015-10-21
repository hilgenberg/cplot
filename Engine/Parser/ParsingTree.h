#pragma once
#include "RetainTree.h"
#include "ParsingResult.h"
#include "PreToken.h"
#include "Token.h"
#include "../cnum.h"
#ifdef verify
#undef verify
#endif

class Token;

class ParsingTree : public RetainTree<ParsingTree>
{
public:
	typedef RetainTree<ParsingTree> Super;
	
	static ParsingTree *parse(const std::string &s, const Namespace &ns, ParsingResult &info);
	
	ParsingTree() : head(NULL){ } // creates a union/root node
	explicit ParsingTree(Token *head) : head(head){ assert(head); }
	~ParsingTree(){ delete head; }
	
	virtual void reset() // delete head, clear children
	{
		delete head; head = NULL;
		Super::reset();
	}
	
	Token *head;
	
	bool verify(ParsingResult &info); // catches internal screwups, wrong number of arguments
	
	ParsingTree *extract_child(int i = 0)
	{
		assert(retainCount() <= 1);
		assert(i >= 0 && i < num_children());
		ParsingTree *c = child(i);
		children.erase(children.begin()+i);
		c->release_dont_delete();
		return c;
	}
	
private:
	static ParsingTree *parse(Token *token, ParsingResult &info, const RootNamespace &rns);
	static bool parenthesize(Token *&tokens, Token *begin, Token *end, ParsingResult &info, const RootNamespace &rns);

	static ParsingTree *parse(const std::string &str, PreTokens &T0, ParsingResult &info, const Namespace &ns);
	// Parses a token list and returns the root of the parsing tree or NULL on error
	// like this: sin((x+y)*z)  -->  sin
	//                                |
	//                               mul
	//                              /   \        .
	//                            add    z       .
	//                            / \            .
	//                           x   y
	//
	// All PreToken::TT_List must already have been converted to TT_Tree!
};

std::ostream &operator<<(std::ostream &out, const ParsingTree &tree);
