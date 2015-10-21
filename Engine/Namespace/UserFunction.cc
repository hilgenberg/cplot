#include "UserFunction.h"
#include "Expression.h"
#include "Variable.h"
#include "../Parser/WorkingTree.h"
#include "../Parser/Token.h"

#include <iostream>
#include <cassert>
#include <map>
#include <set>
#include <string>
#include <queue>
#include <cstring>

void UserFunction::save(Serializer &s) const
{
	Function::save(s);
	s._string(m_formula);
}
void UserFunction::load(Deserializer &s)
{
	Function::load(s);
	std::string f;
	s._string(f);
	formula(f);
}

UserFunction &UserFunction::operator=(const UserFunction &f)
{
	formula(f.formula());
	return *this;
}

UserFunction::UserFunction() : Function(""), nameLen(0), dirty(false), fx(NULL), m_arity(0)
{
}

Element *UserFunction::copy() const
{
	UserFunction *f = new UserFunction;
	f->formula(formula());
	return f;
}

UserFunction::~UserFunction()
{
	if (ins.container()) ins.container()->remove(&ins);
	// fx lives in ins
}

bool UserFunction::valid() const
{
	if (!nameLen) return false;
	if (dirty) parse();
	return fx && fx->valid() && !recursive();
}

bool UserFunction::is_real(bool real_params) const
{
	if (dirty) parse();
	if (!valid()) return false;
	
	const WorkingTree *t = fx->wt0(); assert(t);
	return real_params ? t->is_real(std::set<Variable*>(args.begin(), args.end())) : t->is_real();
}
Range UserFunction::range(Range input) const
{
	if (dirty) parse();
	if (!valid()) return false;
	
	const WorkingTree *t = fx->wt0(); assert(t);
	if (input == R_Complex) return t->range();
	
	std::map<Variable*, Range> vars;
	for (Variable *v : args) vars.insert(std::make_pair(v, input));
	return t->range(vars);
}

WorkingTree *UserFunction::apply(const std::vector<WorkingTree*> &xargs) const
{
	if (dirty) parse();
	if (!valid()) return NULL;
	assert(fx->wt());
	if (xargs.size() != args.size()) return NULL;
	size_t n = xargs.size();
	
	#ifdef UF_DEBUG
	std::cout << "UserFunction: applying " << *fx->wt() << std::endl << "to (" ;
	for (size_t i = 0; i < n; ++i) std::cout << (i == 0 ? "" : ", ") << *xargs[i];
	std::cout << ")" << std::endl;
	#endif
	
	std::map<const Variable *, const WorkingTree *> values;
	for (size_t i = 0; i < n; ++i) values[args[i]] = xargs[i];
	WorkingTree ret(fx->wt()->evaluate(values));
	
	#ifdef UF_DEBUG
	std::cout << "result" << (ret.type != WorkingTree::TT_Root ? "" : "(list)") << ": " << ret << std::endl << std::endl;
	#endif
	
	return new WorkingTree(std::move(ret));
}
WorkingTree *UserFunction::apply(const std::vector<WorkingTree> &xargs) const
{
	if (dirty) parse();
	if (!valid()) return NULL;
	assert(fx->wt());
	if (xargs.size() != args.size()) return NULL;
	size_t n = xargs.size();
	
	#ifdef UF_DEBUG
	std::cout << "UserFunction: applying " << *fx->wt() << std::endl << "to (" ;
	for (size_t i = 0; i < n; ++i) std::cout << (i == 0 ? "" : ", ") << xargs[i];
	std::cout << ")" << std::endl;
	#endif
	
	std::map<const Variable *, const WorkingTree *> values;
	for (size_t i = 0; i < n; ++i) values[args[i]] = &xargs[i];
	WorkingTree ret(fx->wt()->evaluate(values));
	
	#ifdef UF_DEBUG
	std::cout << "result" << (ret.type != WorkingTree::TT_Root ? "" : "(list)") << ": " << ret << std::endl << std::endl;
	#endif
	
	return new WorkingTree(std::move(ret));
}

bool UserFunction::recursive() const
{
	std::set<const UserFunction*> functions;
	collect(functions);
	return functions.count(this);
}

void UserFunction::collect(std::set<const UserFunction*> &functions) const
{
	if (dirty) parse();
	if (!fx || !fx->valid()) return;

	std::queue<const WorkingTree*> todo;
	todo.push(fx->wt());
	while (!todo.empty())
	{
		const WorkingTree *t = todo.front();
		todo.pop();

		if (t->type == WorkingTree::TT_Function && !t->function->base())
		{
			const UserFunction *f = (const UserFunction*)t->function;
			if (!functions.count(f))
			{
				functions.insert(f);
				f->collect(functions);
			}
		}
		for (auto &c : *t) todo.push(&c);
	}
}


//----------------------------------------------------------------------------------------------------------------------
// Parsing
//----------------------------------------------------------------------------------------------------------------------

static inline bool nameChar(int c, bool first = false)
{
	if (!isascii(c)) return true; // don't break on weird unicode chars
	if (first && isdigit(c)) return false;
	return !strchr("()[]{}=,;:.+-*/^<>!\"°%&|\\?", c) && c != 0 && !(isspace(c) || iscntrl(c));
}

static inline void skipspace(const std::string &s, size_t &i, size_t n)
{
	while (i < n && isspace(s[i])) ++i;
}

static inline size_t readname(const std::string &s, size_t &i, size_t n)
{
	if (i >= n || !nameChar(s[i], true)) return 0;
	size_t namelen = 1;
	++i;
	while (i < n && nameChar(s[i])){ ++i; ++namelen; }
	return namelen;
}

void UserFunction::redefinition(const std::set<std::string> &affected_names)
{
	if (dirty) return;

	for (auto &s : affected_names)
	{
		if (s == name()) continue;

		if (m_formula.find(s))
		{
			if (nameLen)
			{
				dirty = true;
				redefine();
			}
			else
			{
				std::string tmp = m_formula;
				m_formula = "";
				formula(tmp);
			}
			break;
		}
	}
}

void UserFunction::added_to_namespace()
{
	assert(dirty);
	assert(container());
	std::string tmp = m_formula;
	m_formula = "";
	formula(tmp);
}

void UserFunction::formula(const std::string &f)
{
	if (m_formula == f) return;

	m_formula = f;
	dirty = true;

	Namespace *ns = container();
	if (!ns) return;
	
	int old_arity = m_arity;
	nameIndex =  0;
	exprIndex =  0;
	nameLen   =  0;
	m_arity   =  0;
	m_deterministic = true;
	args.clear();
	fx = NULL;
	ins.clear();

	std::vector<std::string> argnames;
	std::string newname;
	
	// extract the pieces: name
	size_t n = m_formula.length(), i = 0;
	skipspace(f, i, n);
	nameIndex = i;
	nameLen   = readname(f, i, n);
	if (!nameLen) goto SYNTAX_ERROR;
	newname = f.substr(nameIndex, nameLen);
	if (!Namespace::valid_name(newname)) goto SYNTAX_ERROR;

	// arguments
	skipspace(f, i, n);
	if (i < n && f[i]=='(')
	{
		do
		{
			++i;
			skipspace(f,i,n);
			size_t start = i;
			size_t plen  = readname(f,i,n);
			if (!plen) goto SYNTAX_ERROR;
			argnames.push_back(f.substr(start, plen));
			skipspace(f,i,n);
		}
		while(i < n && f[i] == ',');
		
		skipspace(f,i,n);
		if (i >= n || f[i] != ')') goto SYNTAX_ERROR;
		++i;
	}
	else if(i < n && nameChar(f[i], true))
	{
		size_t start = i;
		size_t plen  = readname(f,i,n);
		if (!plen) goto SYNTAX_ERROR;
		argnames.push_back(f.substr(start, plen));
	}

	// = or :=
	skipspace(f,i,n);
	if (!(i < n && f[i] == '=' || i+1 < n && f[i] == ':' && f[i+1] == '=')) goto SYNTAX_ERROR;
	i += (f[i] == ':' ? 2 : 1);
	skipspace(f,i,n);

	if (i >= n) goto SYNTAX_ERROR;
	exprIndex = i;
	
	// sanity check the args
	for (size_t k = 0; k < argnames.size(); ++k)
	{
		if (!Namespace::valid_name(argnames[k])) goto SYNTAX_ERROR;
		for (size_t l = k+1; l < argnames.size(); ++l)
		{
			if (argnames[k] == argnames[l]) goto SYNTAX_ERROR;
		}
	}
	
	// add vars for the arguments
	for (auto &p : argnames)
	{
		Variable *v = new Variable(p, false);
		ins.add(v);
		args.push_back(v);
	}
	m_arity = (int)argnames.size();
	
	// update our name
	if (ns)
	{
		if (newname == name() && old_arity == m_arity)
		{
			redefine();
		}
		else if (!ns->rename(this, old_arity, newname))
		{
			goto SYNTAX_ERROR;
		}
	}

	return;

SYNTAX_ERROR:
	nameLen = 0;
	dirty = false;
	if (ns) ns->rename(this, old_arity, "");
}

void UserFunction::parse() const
{
	if (!dirty || !nameLen) return;
	dirty = false;
	
	if (ins.container() != container())
	{
		if (ins.container()) ins.container()->remove(&ins);
		ins.link(container());
	}
	
	if (!fx)
	{
		fx = new Expression;
		ins.add(fx);
	}
	fx->strings(m_formula.substr(exprIndex));
}
