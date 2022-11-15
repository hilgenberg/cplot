#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "PlotWindow.h"
#include "../Engine/Namespace/Expression.h"

void GUI::params_menu()
{
	if (!ImGui::BeginMenu("Parameters")) return;

	std::set<Parameter*> aps(w.rns.all_parameters(true));
	std::vector<Parameter*> ps(aps.begin(), aps.end());
	std::sort(ps.begin(), ps.end(), [&](Parameter *a, Parameter *b)->bool{ return a->name() < b->name(); });
	for (Parameter *p : ps)
	{
		if (!ImGui::BeginMenu((
			p->name() +
			std::string(" = ") +
			to_string(p->value(), w.rns) + "###param_" + p->name()
			).c_str())) continue;
		ImGui::PushID(p);
		if (ImGui::MenuItem(p->anim ? "Stop" : "Animate"))
		{
			if (p->anim) p->anim_stop();
			else if (p->anim_start()) { w.start_animations(); }
		}
		if (ImGui::MenuItem("Edit...", NULL, false, !param_edit))
		{
			param_edit = true; param_orig = p->oid();
			param_tmp_type = p->type();
			param_tmp[0] = p->name();
			param_tmp[1] = format_complex(p->value());
			param_tmp[2] = format_double(p->min());
			param_tmp[3] = format_double(p->max());
			param_tmp[4] = format_double(p->imin());
			param_tmp[5] = format_double(p->imax());
			param_tmp[6] = format_double(p->rmax());
			param_tmp_rad = p->angle_in_radians();
		}
		if (ImGui::MenuItem("Delete"))
			w.deleteParam(p->oid());
		ImGui::PopID();
		ImGui::EndMenu();
	}
	if (!ps.empty()) ImGui::Separator();
	if (ImGui::MenuItem("New Parameter", NULL, false, !param_edit))
	{
		param_edit = true; param_orig = 0;
		for (int i = 0; i < 7; ++i) param_tmp[i].clear();
		param_tmp_type = Real;
		param_tmp[1] = "0";
		param_tmp_rad = true;
	}
	ImGui::EndMenu();
}

std::string GUI::format_double(double x) const
{
	if (!defined(x)) return std::string();
	return to_string(x, w.rns);
}
double GUI::parse(const std::string &s, const char *desc)
{
	if (s.empty() || s == " ") return UNDEFINED;

	ParsingResult result;
	cnum v = Expression::parse(s, &w.rns, result);
	if (!result.ok)
	{
		error(format("%s: %s", desc, result.info.c_str()));
		throw std::runtime_error("parsing error");
	}
	if (!is_real(v))
	{
		error(format("%s is not real", desc));
		throw std::runtime_error("parsing error");
	}

	return v.real();
}
std::string GUI::format_complex(cnum x) const
{
	if (!defined(x)) return std::string();
	return to_string(x, w.rns);
}
cnum GUI::cparse(const std::string &s, const char *desc)
{
	if (s.empty() || s == " ") return UNDEFINED;

	ParsingResult result;
	cnum v = Expression::parse(s, &w.rns, result);
	if (!result.ok)
	{
		error(format("%s: %s", desc, result.info.c_str()));
		throw std::runtime_error("parsing error");
	}
	return v;
}

void GUI::param_editor()
{
	if (!param_edit) return;

	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::Begin(param_orig ? "Parameter Settings" : "New Parameter", NULL, window_flags);

	ImGui::InputText("Name", &param_tmp[0]);
	ImGui::InputText("Value", &param_tmp[1]);

	static const char *types[] = {"Real", "Complex", "Angle", "ComplexAngle", "Integer"};
	ImGui::Combo("Type", &param_tmp_type, types, IM_ARRAYSIZE(types));

	assert(enabled);
	ParameterType t = (ParameterType)param_tmp_type;
	enable(t == ParameterType::Angle);
	ImGui::Checkbox("Radians", &param_tmp_rad);

	enable(t == ParameterType::Real || t == ParameterType::Integer || t == ParameterType::Complex);
	ImGui::InputText("re min", &param_tmp[2]);
	ImGui::InputText("re max", &param_tmp[3]);

	enable(t == ParameterType::Complex);
	ImGui::InputText("im min", &param_tmp[4]);
	ImGui::InputText("im max", &param_tmp[5]);
	ImGui::InputText("abs max", &param_tmp[6]);

	enable();

	bool apply = false, close = false;
	if (ImGui::Button("OK")) apply = close = true;
	ImGui::SameLine();
	if (ImGui::Button("Apply")) apply = true;
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) close = true;

	if (apply)
	{
		std::unique_ptr<Parameter> tmp;
		Parameter *p = NULL;
		if (!param_orig)
		{
			tmp.reset(new Parameter);
			p = tmp.get();
		}
		else
		{
			p = (Parameter*)IDCarrier::find(param_orig);
			assert(p);
		}
		if (!p)
		{
			error("Parameter not found!");
			ImGui::End();
			return;
		}

		Namespace &ns = w.rns;
		Plot &plot = w.plot;
		std::string &name = param_tmp[0];

		if (!ns.valid_name(name))
		{
			error("Error: Invalid name");
			ImGui::End();
			return;
		}
		else if (!param_orig || name != p->name()) for (auto *q : ns.all_parameters(true))
		{
			if (name != q->name()) continue;
			error("Error: Name already exists");
			ImGui::End();
			return;
		}

		cnum value;
		double r0 = 0.0, r1 = 0.0, i0 = 0.0, i1 = 0.0, a1 = 0.0;
		try
		{
			value = cparse(param_tmp[1], "Value");
			if (t == ParameterType::Real || t == ParameterType::Integer || t == ParameterType::Complex)
			{
				r0 = parse(param_tmp[2], "Re Min");
				r1 = parse(param_tmp[3], "Re Max");
			}
			if (t == ParameterType::Complex)
			{
				i0 = parse(param_tmp[4], "Im Min");
				i1 = parse(param_tmp[5], "Im Max");
				a1 = parse(param_tmp[6], "Abs Max");
			}
		}
		catch (...)
		{
			assert(!error_msg.empty());
			ImGui::End();
			return;
		}

		if (param_orig)
		{
			std::vector<char> data2;
			ArrayWriter ww(data2);
			Serializer s(ww);
			p->save(s);
			auto p_ = param_orig;
			w.ut.reg("Modify Parameter", [this,data2,p_]{ w.modifyParam(data2, p_); });
		}

		p->type(t);

		if (t == ParameterType::Real || t == ParameterType::Integer || t == ParameterType::Complex)
		{
			p->min(r0);
			p->max(r1);
		}
		if (t == ParameterType::Complex)
		{
			p->imin(i0);
			p->imax(i1);
			p->rmax(a1);
		}

		if (t == Angle) p->angle_in_radians(param_tmp_rad);

		p->value(value);

		if (!param_orig)
		{
			p->rename(name);
			ns.add(p); tmp.release();
			param_orig = p->oid();
			auto oid = param_orig;
			w.ut.reg("Add Parameter", [this,oid]{ w.deleteParam(oid); });
		}
		else
		{
			plot.reparse(p->name()); // reparse for old name
			p->rename(name);
		}

		plot.reparse(p->name()); // reparse for new name
		w.recalc(plot);
	}
	if (close)
	{
		param_edit = false;
	}
	ImGui::End();
}
