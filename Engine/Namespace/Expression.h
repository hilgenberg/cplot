#pragma once

#include "../Parser/ParsingResult.h"
#include "Namespace.h"

#include <vector>
#include <map>

class Evaluator;
class EvalContext;
class Variable;
class Parameter;
class WorkingTree;
class OptimizingTree;

/**
 * @addtogroup Namespace
 * @{
 */

/**
 * Expressions turn a list of strings into a parsing tree and can evaluate that by replacing variables with values.
 */

class Expression : public Element
{
public:
	Expression() : Element(""), m_ot(NULL), m_ev(NULL), m_wt(NULL), dirty(false){ }
	~Expression();

	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	virtual TypeCode type_code() const{ return TypeCode::INVALID; }
	virtual bool builtin() const{ return false; }
	virtual int  arity() const{ return -1; }

	std::vector<std::string> strings() const{ return m_strings; }
	void strings(const std::vector<std::string> &);
	void strings(const std::string &s){ strings(std::vector<std::string>(1, s)); }
	virtual void redefinition(const std::set<std::string> &affected_names);

	bool valid() const{ if (dirty) parse(); return m_wt && m_result.ok; }
	bool constant(bool strong = true) const; // strong -> independent of time (i.e. Parameters are not strong const, 2.3 is)
	bool deterministic() const;
	std::set<Parameter*> usedParameters() const;
	bool uses_object(const std::string &name) const;

	const ParsingResult &result() const{ if (dirty) parse(); return m_result; }
	WorkingTree  *wt() const{ if (dirty) parse(); return m_wt; }
	WorkingTree  *wt0() const; ///< Returns the first (and only, which is asserted) child of wt()

	cnum evaluate(const std::map<const Variable*, cnum> &values) const;
	Evaluator *evaluator(const std::vector<const Variable *> &var_order); ///< For fast multi-evaluations

	static cnum parse(const std::string &s, const Namespace *ns, ParsingResult &result); ///< Convenience function

	WorkingTree *derivative(const Variable &x, std::string &error) const;
	
	#ifdef DEBUG
	virtual void dump(std::ostream &o) const{ o << "Expression "; for (auto &i : m_strings) o << i << " "; }
	#endif

protected:
	virtual Element *copy() const{ return NULL; }

private:
	std::vector<std::string>    m_strings;
	mutable ParsingResult       m_result;
	mutable WorkingTree        *m_wt;
	mutable OptimizingTree     *m_ot;
	mutable Evaluator          *m_ev;

	mutable bool dirty;
	void parse() const; ///< Create or update m_pt, etc and clear dirty flag.
};

/** @} */

