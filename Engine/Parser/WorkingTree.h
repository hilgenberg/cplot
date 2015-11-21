#pragma once
#include "RetainTree.h"
#include "../Namespace/Operator.h"
//#include "Function.h"
//#include "Element.h"
#include "../Namespace/RootNamespace.h"
#include <string>
#include <set>
#include <map>
#include <algorithm>

#ifdef verify
#undef verify
#endif

class Element;
class Variable;
class Function;
class Constant;
class Parameter;

class UserFunction;
class Expression;
class ParsingTree;

class WorkingTree
{
public:
	friend class OptimizingTree;
	friend class Pattern;
	friend class Rule;
	
	//------------------------------------------------------------------------------------------------------------------
	// data and info
	//------------------------------------------------------------------------------------------------------------------
	
	enum Type
	{
		TT_Root,
		TT_Constant,
		TT_Number,
		TT_Variable,
		TT_Parameter,
		TT_Function,
		TT_Operator,
		TT_Sum,
		TT_Product
	};
	
	Type type;
	
	struct CNP{ const RootNamespace *rns; cnum z; };
	
	union
	{
		const Element       *element;
		const Variable      *variable;
		const Function      *function;
		const Operator      *operator_;
		const Constant      *constant;
		const Parameter     *parameter;
		const RootNamespace *rns; // for root
		CNP                 *num;
	};
	
private: std::vector<WorkingTree> children; public:
	
	~WorkingTree(){ if (type == TT_Number) delete num; }
	
	inline int  num_children() const{ return (int)children.size(); }
	
	inline WorkingTree       &child(int i)       { assert(i >= 0 && (size_t)i < children.size()); return children[i]; }
	inline const WorkingTree &child(int i) const { assert(i >= 0 && (size_t)i < children.size()); return children[i]; }
	const Function *head() const; // top-level function or NULL
	
	typedef typename std::vector<WorkingTree>::const_iterator const_iterator;
	typedef typename std::vector<WorkingTree>::iterator       iterator;
	inline const_iterator begin() const{ return children.begin(); }
	inline iterator       begin()      { return children.begin(); }
	inline const_iterator   end() const{ return children.end();   }
	inline iterator         end()      { return children.end();   }
	
	const RootNamespace &ns() const;
	
	explicit operator cnum() const{ assert(type == TT_Number); return num->z; }
	
	bool operator== (const WorkingTree &t) const;
	bool operator!= (const WorkingTree &t) const{ return !(*this == t); }
	bool operator== (const cnum &z){ return type == TT_Number && eq(num->z, z); }
	
	//------------------------------------------------------------------------------------------------------------------
	// constructors, assignment
	//------------------------------------------------------------------------------------------------------------------
	
	WorkingTree(const ParsingTree &pt, const RootNamespace &ns);
	
	WorkingTree(const WorkingTree &t) : type(t.type), element(t.element), children(t.children)
	{
		if (type == TT_Number) num = new CNP(*num);
		assert(verify());
	}
	WorkingTree(WorkingTree &&t) : type(t.type), element(t.element), children(std::move(t.children))
	{
		t.num = NULL;
		assert((t.type = (Type)-15)); // so verify will not fail assertions if some container copies the remains
	}
	
	WorkingTree(const cnum &z, const RootNamespace &rns) : num(new CNP({&rns, z})), type(TT_Number){ assert(verify()); }
	
	explicit WorkingTree(const Variable *v) : variable(v), type(TT_Variable){ assert(verify()); }
	explicit WorkingTree(const Constant *c) : constant(c), type(TT_Constant){ assert(verify()); }
	
	explicit WorkingTree(const RootNamespace &rns) : rns(&rns), type(TT_Root){ assert(verify()); }
	void add_subtree(WorkingTree &&c)
	{
		assert(type == TT_Root);
		add_child(std::move(c));
	}
	
	WorkingTree(const UnaryOperator *o, WorkingTree &&x) : operator_(o), type(TT_Operator)
	{
		children.push_back(x);
		assert(verify());
	}
	WorkingTree(const BinaryOperator *o, WorkingTree &&x1, WorkingTree &&x2) : operator_(o), type(TT_Operator)
	{
		children.reserve(2);
		children.push_back(x1);
		children.push_back(x2);
		assert(verify());
	}
	WorkingTree(const Function *f, WorkingTree &&x1)
	: function(f), type(TT_Function)
	{
		assert(f->arity() == 1);
		children.push_back(x1);
		assert(verify());
	}
	WorkingTree(const Function *f, WorkingTree &&x1, WorkingTree &&x2)
	: function(f), type(TT_Function)
	{
		assert(f->arity() == 2);
		children.reserve(2);
		children.push_back(x1);
		children.push_back(x2);
		assert(verify());
	}
	WorkingTree(const Function *f, WorkingTree &&x1, WorkingTree &&x2, WorkingTree &&x3)
	: function(f), type(TT_Function)
	{
		assert(f->arity() == 3);
		children.reserve(3);
		children.push_back(x1);
		children.push_back(x2);
		children.push_back(x3);
		assert(verify());
	}
	
private:
	explicit WorkingTree(const Function *f)
	: function(f), type(f->isOperator() ? TT_Operator : TT_Function)
	{
	}

	private: struct private_key{ }; // get emplace_back to work...
	private: WorkingTree(const Element *e, Type t) : element(e), type(t){ assert(type != TT_Number || !e); }
	public:  WorkingTree(const Element *e, Type t, const private_key &) : WorkingTree(e,t){}
	
	//------------------------------------------------------------------------------------------------------------------
	
	WorkingTree &operator=(const WorkingTree &t)
	{
		if (this == &t) return *this;
		type = t.type; element = t.element;
		if (type == TT_Number) num = new CNP(*num);
		children = t.children;
		assert(verify());
		return *this;
	}
	
	WorkingTree &operator=(WorkingTree &&t)
	{
		type = t.type; element = t.element; t.num = NULL;
		children = std::move(t.children);
		assert(verify());
		return *this;
	}
	
	WorkingTree &operator=(const cnum &z)
	{
		if (type == TT_Number)
		{
			num->z = z;
		}
		else
		{
			auto *rns = &ns();
			num = new CNP({rns, z});
			type = TT_Number;
			children.clear();
		}
		assert(verify());
		return *this;
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// computation and information
	//------------------------------------------------------------------------------------------------------------------
	
	bool is_constant(bool strong = true) const; // strong: mathematical constness, weak: can be evaluated to cnum
	bool is_deterministic() const;
	
	cnum        evaluate(const std::map<const Variable*, cnum> &values) const;
	inline cnum evaluate() const{ std::map<const Variable*, cnum> dummy; return evaluate(dummy); }
	WorkingTree evaluate(const std::map<const Variable*, const WorkingTree *> &values) const;
	void collect_parameters(std::set<Parameter*> &dst) const;
	void collect_variables(std::set<Variable*> &dst) const;
	
	WorkingTree *derivative(const Variable &x, std::string &error) const;
	WorkingTree *split_var(const Variable &z, const Variable &x, const Variable &y) const; // f(z) --> f(x+iy)
	
	void simplify(bool full);
	
	inline bool is_zero     () const{ return type == TT_Number && ::isz        (num->z); }
	inline bool is_one      () const{ return type == TT_Number && ::is_one     (num->z); }
	inline bool is_minus_one() const{ return type == TT_Number && ::is_minusone(num->z); }
	inline bool is_real(double &value) const
	{
		if (type != TT_Number || !::is_real(num->z)) return false;
		value = num->z.real();
		return true;
	}
	
	bool is_real() const; // handles functions and operators
	bool is_real(const std::set<Variable*> &real_vars) const; // assume that real_vars are real too
	Range range() const;
	Range range(const std::map<Variable*, Range> &input) const;

	inline bool is_function()  const{ return type == TT_Function || type == TT_Operator; }
	inline bool is_function(const Function *f)  const{ return function == f && is_function(); }
	inline bool is_operator(const Operator *op) const{ return is_function(op); }
	
	inline bool is_negation ()   const{ return is_operator(ns().UMinus); }
	inline bool is_inversion()   const{ return is_operator(ns().Invert); }
	inline bool is_conjugation() const{ return is_operator(ns().ConjOp) || is_function(ns().Conj); }
	
	//------------------------------------------------------------------------------------------------------------------
	// printing
	//------------------------------------------------------------------------------------------------------------------
	
	void print(std::ostream &o, PrintingStyle ds = PS_Console) const{ print(o, ds, NULL); }
	std::string to_string(PrintingStyle ds = PS_Console) const;
private:
	void print(std::ostream &o, PrintingStyle ds, const Operator *outside, bool left_child = true) const;
	
	//------------------------------------------------------------------------------------------------------------------
	// informationals for simplify()
	//------------------------------------------------------------------------------------------------------------------
	
	bool verify() const; // catches wrong number of arguments, NULL elements
	
	int  uglyness() const;
	
	inline bool has_real_children() const
	{
		for (auto &c : children) if (!c.is_real()) return false;
		return true;
	}
	
	size_t  size() const { size_t n = 0; for (auto &c : children) n += c.size();               return ++n; }
	size_t depth() const { size_t d = 0; for (auto &c : children) d  = std::max(d, c.depth()); return ++d; }
	
	//------------------------------------------------------------------------------------------------------------------
	// rearranging methods for simplify() and others
	//------------------------------------------------------------------------------------------------------------------
	
	static WorkingTree add(WorkingTree &&a, WorkingTree &&b);
	static WorkingTree mul(WorkingTree &&a, WorkingTree &&b);
	
	void add_child(WorkingTree &&c){ children.push_back(c); }
	
	void pull(int i) // replace this by child(i)
	{
		WorkingTree &t = children[i];
		type = t.type;
		element = t.element;
		t.num = NULL;
		
		std::vector<WorkingTree> tmp(std::move(t.children));
		children = std::move(tmp);
		assert(verify());
	}
	inline void pull(int i0, int i1) // replace this by child(i0).child(i1)
	{
		pull(i0);
		pull(i1);
		// same as child(i0).pull(i1); pull(i0);
	}
	
	// move child(ichild).child(igrandchild) into position children[idst] and
	// change element of child(ichild) to new_child_func (some unary operator probably)
	void move_grandchild(int ichild, int igrandchild, const Element *new_child_func, int idst)
	{
		auto &c = children[ichild];
		c.element = new_child_func;
		children.insert(children.begin()+idst, std::move(c.child(igrandchild)));
		if (idst <= ichild) ++ichild;
		child(ichild).children.erase(child(ichild).children.begin()+igrandchild);
		assert(child(ichild + (idst <= ichild ? 1 : 0)).verify());
		assert(verify()); // should only be done to sums and products
	}
	
	// replace child(i) by child(i).children
	void unpack_child(int i)
	{
		assert(verify());
		int n = child(i).num_children();
		if (!n) return remove_child(i);
		if (n > 1)
		{
			auto &cc = child(i).children;
			children.insert(children.begin()+i+1, std::make_move_iterator(cc.begin()+1),
							std::make_move_iterator(cc.end()));
		}
		child(i).pull(0);
		assert(verify());
	}
	
	// put this into a new node (stuff like f(x,y) --> -f(x,y))
	void pack(const Element *e, bool transit = false)
	{
		assert(transit || e->arity() == 1);
		std::vector<WorkingTree> tmp;
		std::swap(tmp, children);
		children.emplace_back(nullptr, type, private_key()); // don't copy number
		children[0].element = element;
		element = e;
		type    = e->isOperator() ? TT_Operator : TT_Function;
		std::swap(tmp, children[0].children);
		assert(transit || verify());
	}
	void pack(Type t, const RootNamespace &ns)
	{
		assert(t == TT_Sum || t == TT_Product);
		std::vector<WorkingTree> tmp;
		std::swap(tmp, children);
		children.emplace_back(nullptr, type, private_key()); // don't copy number
		children[0].element = element;
		rns = &ns;
		type = t;
		std::swap(tmp, children[0].children);
		assert(verify());
	}
	
	// move child(i0) into position i
	void move_child(int i0, int i)
	{
		if (i0 == i) return;
		if (i == i0+1 || i == i0-1) std::swap(children[i], children[i0]);
		else if (i0 < i)
		{
			auto r0 = children.begin()+i0;
			auto r1 = children.begin()+(i+1);
			std::rotate(r0, r0+1, r1);
		}
		else
		{
			auto r0 = children.begin()+i;
			auto r1 = children.begin()+(i0+1);
			std::rotate(r0, r1-1, r1);
		}
		assert(verify());
	}
	
	// remove child(i)
	inline void remove_child(int i)
	{
		children.erase(children.begin()+i);
		assert(verify()); // should only be done on TT_Sum and TT_Product
	}
	
	inline void remove_children(int i, int j)
	{
		if (i < j)
		{
			children.erase(children.begin()+j);
			children.erase(children.begin()+i);
		}
		else if (j < i)
		{
			children.erase(children.begin()+i);
			children.erase(children.begin()+j);
		}
		else
		{
			assert(false);
			children.erase(children.begin()+i);
		}
		assert(verify()); // should only be done on TT_Sum and TT_Product
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// algebra - for use during simplification: must keep trees normalized
	//------------------------------------------------------------------------------------------------------------------
	
	// -x -> x, x -> -x
	void flip_involution(const Function *f)
	{
		assert(f->involution());
		if (type == TT_Number && f == ns().UMinus){ num->z = -num->z; return; }
		if ((type == TT_Function || type == TT_Operator) && function == f) pull(0); else pack(f);
		assert(verify());
	}
	
	// add a number (from the right)
	void operator+= (const cnum &z)
	{
		if (type == TT_Number){ num->z += z; return; }
		if (type != TT_Sum) pack(TT_Sum, ns());
		children.emplace_back(z, ns());
		assert(verify());
	}
	// add an expression (from the right)
	void operator+= (WorkingTree &&z)
	{
		if (type == TT_Number && z.type == TT_Number){ num->z += z.num->z; return; }
		if (type != TT_Sum) pack(TT_Sum, ns());
		children.push_back(std::move(z));
		assert(verify());
	}
	
	// subtract a number (from the right)
	void operator-= (const cnum &z)
	{
		if (type == TT_Number){ num->z -= z; return; }
		if (type != TT_Sum) pack(TT_Sum, ns());
		children.emplace_back(-z, ns());
		assert(verify());
	}
	// subtract an expression (from the right)
	void operator-= (WorkingTree &&z)
	{
		if (type == TT_Number && z.type == TT_Number){ num->z -= z.num->z; return; }
		if (type != TT_Sum) pack(TT_Sum, ns());
		z.flip_involution(ns().UMinus);
		children.push_back(std::move(z));
		assert(verify());
	}
	
	// multiply by a number (from the left)
	void operator*= (const cnum &z)
	{
		if (type == TT_Number){ num->z *= z; return; }
		if (type != TT_Product) pack(TT_Product, ns());
		children.emplace(children.begin(), z, ns());
		assert(verify());
	}
	// multiply by an expression (from the left)
	void operator*= (WorkingTree &&z)
	{
		if (type == TT_Number && z.type == TT_Number){ num->z *= z.num->z; return; }
		if (type != TT_Product) pack(TT_Product, ns());
		children.push_back(std::move(z));
		assert(verify());
	}
	
	// raise to some power
	void raise(const cnum &z)
	{
		pack(ns().Pow, true);
		children.emplace_back(z, ns());
		assert(verify());
	}
	void raise(WorkingTree &&z)
	{
		pack(ns().Pow, true);
		children.push_back(std::move(z));
		assert(verify());
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// simplify
	//------------------------------------------------------------------------------------------------------------------
	
	void normalize();
	void flatten(bool &change);
	void denormalize();
	void apply_rules(bool &change);
	bool can_match(std::set<Element*> &es) const;
	
	void simplify_unary(bool &change);
	void simplify_powers(bool &change);
	void simplify_products(bool &change);
	void simplify_sums(bool &change);
	void simplify_functions(bool &change);
	
	void simplify_pairs(bool &change);
	int combine_factors(int i, int j, WorkingTree &A, WorkingTree &B, bool a_conj, bool b_conj, bool a_inv, bool b_inv, bool &change);
	
	// check if A = a_coeff * X and B = b_coeff * X (so A+B can become (a_coeff+b_coeff) * X)
	static bool sum_compatible(const WorkingTree &A, const WorkingTree &B,
							   cnum &a_coeff, cnum &b_coeff, int &a_coeff_i, int &b_coeff_i);
	
	// simplify child(i) + child(j) if possible and return the number of children removed
	// if it returns 1, child(j) was removed (and possibly swapped with child(i) first)
	int combine_sum(int i, int j);
	
	// simplify child(i) * child(j) if possible and return the number of children removed
	// checks if A = X^a and B = X^b (so A*B can become X^(a+b))
	// if it returns 1, child(j) was removed (and possibly swapped with child(i) first)
	int combine_product(int i, int j);
	
	// X^k --> X^(k+p)
	void add_power(double p);
	void add_power(WorkingTree &&p);
	
	static bool combine_function_product(WorkingTree &F, WorkingTree &G, bool a_inv, bool b_inv, bool a_conj, bool b_conj);
	
};

std::ostream &operator<<(std::ostream &out, const WorkingTree &tree);
