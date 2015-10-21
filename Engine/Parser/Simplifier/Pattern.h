#pragma once
#include "../RetainTree.h"
#include "../../cnum.h"
#include <map>
#include <set>
#include <string>
class Element;
class Function;
class Constant;
class Variable;
class WorkingTree;
class RootNamespace;

class Rule
{
	struct Pattern : public RetainTree<Pattern>
	{
		enum Type
		{
			TT_Constant,
			TT_Number,
			TT_Wildcard,
			TT_Function,
			TT_Sum,
			TT_Product
		};
		
		Type type;
		
		union
		{
			const Function *function;
			const Constant *constant;
			cnum            number;
			Range           range; // for TT_Wildcard
		};

		Pattern(Type t) : type(t){}

		typedef std::map<const Pattern*, const WorkingTree*> Bindings;
		bool matches(const WorkingTree &t, Bindings &bindings) const;
		
		WorkingTree apply(const Bindings &bindings, const RootNamespace &rns) const;
		
		void collect_elements(std::set<Element*> &e, const RootNamespace &rns) const;
		Function *head(const RootNamespace &rns) const;
		
		static Pattern *load(bool replacement, const WorkingTree &t,
							 const std::map<const Variable *, Range> &types,
							 std::map<const Variable*, Pattern*> &vars);
	};

	const RootNamespace &rns;
	Pattern *pattern, *replacement;
	
public:
	~Rule(){ if (pattern) pattern->release(); if (replacement) replacement->release(); }
	
	Rule(RootNamespace &rns, const std::string &s);
	Rule(Rule &&r) : pattern(r.pattern), replacement(r.replacement), rns(r.rns)
	{
		r.pattern = r.replacement = NULL;
	}
	Rule(const Rule &r) : pattern(r.pattern), replacement(r.replacement), rns(r.rns)
	{
		if (pattern) pattern->retain();
		if (replacement) replacement->retain();
	}
	
	bool transform(WorkingTree &t) const;
	
	inline Function *head() const{ return pattern->head(rns); }
	
	void collect_elements(std::set<Element*> &e, const RootNamespace &rns) const;

	inline std::set<Element*> elements() const
	{
		return es;
	}

private:
	std::set<Element*> es;
};

void load(const std::string &path, std::vector<Rule> &rules, RootNamespace &rns);
