#include "WorkingTree.h"
#include "ParsingTree.h"
#include "../Namespace/BaseFunction.h"
#include "../Namespace/UserFunction.h"
#include "../Namespace/Operator.h"
#include "../Namespace/Function.h"
#include "../Namespace/Constant.h"
#include "../Namespace/Variable.h"
#include "../Namespace/Parameter.h"
#include "../Namespace/Expression.h"
#include "../Namespace/RootNamespace.h"
#include "../Namespace/AliasVariable.h"
#include <ostream>
#include <iostream>
#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// Printing
//----------------------------------------------------------------------------------------------------------------------

static inline void list_format(PrintingStyle ds, const char *&s0, const char *&sep, const char *&s1)
{
	switch (ds)
	{
		case PS_Debug:       s0 = "LIST{";   sep = ", ";    s1 = "}";  break;
		case PS_Function:    s0 = "{";       sep = ", ";    s1 = "}";  break;
		case PS_Input:       s0 = "{ ";      sep = " ; ";   s1 = " }"; break;
		case PS_Console:     s0 = "{";       sep = "; ";    s1 = "}";   break;
		case PS_HTML:        s0 = "";        sep = "<br/>"; s1 = "";   break;
		case PS_LaTeX:       s0 = "\\left{"; sep = ", ";    s1 = "\\right}"; break;
		case PS_Mathematica: s0 = "{";       sep = ", ";    s1 = "}";  break;
		case PS_Head:        s0 =            sep =          s1 = "";   break;
	}
}
static inline void function_format(PrintingStyle ds, const char *&s0, const char *&sep, const char *&s1)
{
	switch (ds)
	{
		case PS_Debug:
		case PS_Function:
		case PS_Console:
		case PS_Input:
		case PS_HTML:
		case PS_LaTeX:       s0 = "("; sep = ", "; s1 = ")"; break;
		case PS_Mathematica: s0 = "["; sep = ", "; s1 = "]"; break;
		case PS_Head:        s0 =      sep =       s1 = "";  break;
	}
}
static inline void grouping_format(PrintingStyle ds, const char *&s0, const char *&s1)
{
	switch (ds)
	{
		case PS_LaTeX: s0 = "\\left("; s1 = "\\right)"; break;
			
		case PS_Debug:
		case PS_Function:
		case PS_Console:
		case PS_Input:
		case PS_HTML:
		case PS_Mathematica: s0 = "("; s1 = ")"; break;
		case PS_Head:        s0 =      s1 = "";  break;
	}
}

void WorkingTree::print(std::ostream &o, PrintingStyle ds, const Operator *outside, bool left_child) const
{
	int nc = num_children();
	switch (type)
	{
		case TT_Root:
			assert(ds != PS_Input);
			if (ds == PS_Head)
			{
				o << "<root>";
			}
			else
			{
				const char *s0, *sep, *s1;
				list_format(ds, s0, sep, s1);
				o << s0;
				for (int i = 0; i < nc; ++i)
				{
					if (i > 0) o << sep;
					child(i).print(o, ds, NULL);
				}
				o << s1;
			}
			break;
			
			
		case TT_Function:
		{
			if (ds == PS_Head)
			{
				std::string s = function->displayName(ds);
				if (s == "√") s = "√•";
				o << s;
				break;
			}
			
			if (ds == PS_Input)
			{
				const RootNamespace &rns = ns();
				if (function == rns.Identity)
				{
					return child(0).print(o, ds, outside, left_child);
				}
				if (function == rns.Combine)
				{
					const char *g0, *g1;
					grouping_format(ds, g0, g1);
					BinaryOperator &op = *rns.Plus;
					bool grp;
					if (!outside) grp = false;
					else if (outside->unary()) grp = true;
					else
					{
						grp = true;
						BinaryOperator &o0 = *(BinaryOperator*)outside;
						if (o0.precedence() < op.precedence() ||
							(&op == &o0 && (op.associative() || op.rightbinding() == !left_child)) ||
							(o0.precedence() == op.precedence() && o0.rightbinding() == !left_child)) grp = false;
					}
					if (grp) o << g0;
					child(0).print(o, ds, &op, true);
					o << "+i*";
					child(1).print(o, ds, rns.Mul, false);
					if (grp) o << g1;
					return;
				}
			}
			
			const char *s0, *sep, *s1;
			function_format(ds, s0, sep, s1);
			
			o << function->displayName(ds);
			if (nc == 0 && !ns().find(function->name(), -1)) break;
			o << s0;
			for (int i = 0; i < nc; ++i)
			{
				if (i > 0) o << sep;
				child(i).print(o, ds, NULL);
			}
			o << s1;
			break;
		}
			
		case TT_Operator:
		{
			switch (ds)
			{
				case PS_Head:
				{
					std::string s = operator_->displayName(ds);
					if (operator_->postfix()) o << "•";
					o << s;
					break;
				}
					
				case PS_Debug:       // +((*)(x,y), sin(z))
				case PS_Function:
				{
					const char *s0, *sep, *s1;
					function_format(ds, s0, sep, s1);
					o << operator_->displayName(ds) << s0;
					for (int i = 0; i < nc; ++i)
					{
						if (i > 0) o << sep;
						child(i).print(o, ds, NULL);
					}
					o << s1;
					break;
				}
					
				case PS_Console:
				case PS_Input:
				case PS_HTML:
				case PS_LaTeX:
				case PS_Mathematica:
				{
					const char *s0, *s1;
					grouping_format(ds, s0, s1);
					const RootNamespace &rns = ns();
					
					if (operator_->prefix())
					{
						// postfix binds stronger than prefix (and infix)
						bool grp = (outside && outside->postfix());
						if (grp) o << s0;
						o << operator_->displayName(ds);
						child(0).print(o, ds, operator_);
						if (grp) o << s1;
					}
					else if (operator_->postfix())
					{
						// postfix binds stronger than prefix and infix
						
						if (operator_ == rns.Invert)
						{
							BinaryOperator &op = *rns.Div;
							bool grp;
							if (!outside) grp = false;
							else if (outside->unary()) grp = true;
							else
							{
								grp = true;
								BinaryOperator &o0 = *(BinaryOperator*)outside;
								if (o0.precedence() < op.precedence() ||
									(&op == &o0 && (op.associative() || op.rightbinding() == !left_child)) ||
									(o0.precedence() == op.precedence() && o0.rightbinding() == !left_child)) grp = false;
							}
							if (grp) o << s0;
							o << "1" << op.displayName(ds);
							child(0).print(o, ds, &op, false);
							if (grp) o << s1;
							return;
						}
						
						// make sure x^2^2 does not print as x^22
						bool grp = outside && outside->postfix() && operator_->is_power() && outside->is_power();
						
						if (grp) o << s0;
						child(0).print(o, ds, operator_);
						o << operator_->displayName(ds);
						if (grp) o << s1;
					}
					else
					{
						BinaryOperator &op = *(BinaryOperator*)operator_;
						bool grp;
						if (!outside) grp = false;
						else if (outside->unary())
						{
							if (outside == rns.UMinus && (operator_ == rns.Mul || operator_ == rns.Div))
							{
								grp = false;
							}
							else
							{
								grp = true;
							}
						}
						else
						{
							grp = true;
							BinaryOperator &o0 = *(BinaryOperator*)outside;
							if (o0.precedence() < op.precedence() ||
								(&op == &o0 && (op.associative() || op.rightbinding() == !left_child)) ||
								(o0.precedence() == op.precedence() && o0.rightbinding() == !left_child)) grp = false;
						}
						if (grp) o << s0;
						child(0).print(o, ds, operator_, true);
						o << operator_->displayName(ds);
						child(1).print(o, ds, operator_, false);
						if (grp) o << s1;
					}
					break;
				}
			}
			break; // end of TT_Operator case
		}
			
			
		case TT_Sum:
		case TT_Product:
		{
			BinaryOperator &add = *(type == TT_Sum ? rns->Plus   : rns->Mul);
			BinaryOperator &sub = *(type == TT_Sum ? rns->Minus  : rns->Div);
			UnaryOperator  &neg = *(type == TT_Sum ? rns->UMinus : rns->Invert);
			
			switch (ds)
			{
				case PS_Head:
					o << add.displayName(ds);
					break;
					
				case PS_Debug:       // +((*)(x,y), sin(z))
				case PS_Function:
				{
					const char *s0, *sep, *s1;
					function_format(ds, s0, sep, s1);
					o << add.displayName(ds) << s0;
					for (int i = 0; i < nc; ++i)
					{
						if (i > 0) o << sep;
						child(i).print(o, ds, NULL);
					}
					o << s1;
					break;
				}
					
				case PS_Console:
				case PS_Input:
				case PS_HTML:
				case PS_LaTeX:
				case PS_Mathematica:
				{
					if (children.empty())
					{
						o << (type == TT_Sum ? "0" : "1");
						break;
					}
					
					const char *s0, *s1;
					grouping_format(ds, s0, s1);
					
					bool grp;
					if (!outside) grp = false;
					else if (outside->unary())
					{
						if (type == TT_Product && outside == rns->UMinus)
						{
							grp = false;
						}
						else
						{
							grp = true;
						}
					}
					else
					{
						grp = true;
						BinaryOperator &o0 = *(BinaryOperator*)outside;
						if (o0.precedence() < add.precedence() ||
							&o0 == &add ||
							(o0.precedence() == add.precedence() && o0.rightbinding() == !left_child)) grp = false;
					}
					if (grp) o << s0;
					child(0).print(o, ds, &add, true);
					for (int i = 1, n = num_children(); i < n; ++i)
					{
						auto &c = child(i);
						if (i > 0 && c.type == TT_Operator && c.operator_ == &neg)
						{
							o << sub.displayName(ds);
							c.child(0).print(o, ds, &sub, false);
						}
						else
						{
							o << add.displayName(ds);
							c.print(o, ds, &add, false);
						}
						
					}
					if (grp) o << s1;
					break;
				}
			}
			break;
		}
			
		case WorkingTree::TT_Number:
		{
			const char *s0, *s1;
			grouping_format(ds, s0, s1);
			const cnum &z = num->z;
			
			if (prints_sum(z))
			{
				BinaryOperator &op = *(BinaryOperator*)ns().Plus; // same precedence for Minus
				bool grp;
				if (!outside) grp = false;
				else if (outside->unary()) grp = true;
				else
				{
					grp = true;
					BinaryOperator &o0 = *(BinaryOperator*)outside;
					if (o0.precedence() < op.precedence() ||
						(&op == &o0 && (op.associative() || op.rightbinding() == !left_child)) ||
						(o0.precedence() == op.precedence() && o0.rightbinding() == !left_child)) grp = false;
				}
				if (grp) o << s0;
				o << ::to_string(z, ds);
				if (grp) o << s1;
			}
			else if (prints_sign(z))
			{
				bool grp = (outside && outside->postfix());
				if (grp) o << s0;
				o << ::to_string(z, ds);
				if (grp) o << s1;
			}
			else
			{
				o << ::to_string(z, ds);
			}
			break;
		}
		case WorkingTree::TT_Constant:
		case WorkingTree::TT_Variable:
		case WorkingTree::TT_Parameter: o << element->displayName(ds); break;
	}
}

std::ostream &operator<<(std::ostream &o, const WorkingTree &tree)
{
	tree.print(o);
	return o;
}

std::string WorkingTree::to_string(PrintingStyle ds) const
{
	std::ostringstream o;
	print(o, ds);
	return o.str();
}
