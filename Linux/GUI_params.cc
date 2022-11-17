#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "PlotWindow.h"
#include "../Engine/Namespace/Expression.h"
struct GUI_ParamsMenu;

struct ParamEditor : public GUI_Panel
{
	ParamEditor(GUI_ParamsMenu &parent, Parameter *p);
	void operator()() override;
	bool handle(const SDL_Event &event) override;

	GUI_ParamsMenu &parent;
	IDCarrier::OID orig = 0;
	std::string tmp[7];
	int  tmp_type;
	bool tmp_rad;

	std::string format_double(double x) const;
	double parse(const std::string &s, const char *desc);
	std::string format_complex(cnum x) const;
	cnum cparse(const std::string &s, const char *desc);
};

struct GUI_ParamsMenu : public GUI_Menu
{
	GUI_ParamsMenu(GUI &gui) : GUI_Menu(gui) {}

	std::unique_ptr<ParamEditor> ed;
	void close_editor() { ed = nullptr; gui.redraw(); }

	bool handle(const SDL_Event &event)
	{
		if (!ed) return false;
		return ed->handle(event);
	}

	void operator()()
	{
		if (ImGui::BeginMenu("Parameters"))
		{
			auto &w = gui.w;

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
				if (ImGui::MenuItem("Edit...", NULL, false, !ed))
				{
					ed.reset(new ParamEditor(*this, p));
					gui.redraw();
				}
				if (ImGui::MenuItem("Delete"))
					gui.w.deleteParam(p->oid());
				ImGui::PopID();
				ImGui::EndMenu();
			}
			if (!ps.empty()) ImGui::Separator();
			if (ImGui::MenuItem("New Parameter", NULL, false, !ed))
			{
				ed.reset(new ParamEditor(*this, NULL));
				gui.redraw();
			}
			ImGui::EndMenu();
		}

		if (ed) (*ed)();
	}
};
GUI_Menu *new_params_menu(GUI &gui) { return new GUI_ParamsMenu(gui); }

//----------------------------------------------------------------------------
// ParamEditor implementation
//----------------------------------------------------------------------------

ParamEditor::ParamEditor(GUI_ParamsMenu &parent, Parameter *p)
: parent(parent), GUI_Panel(parent.gui)
{
	if (p)
	{
		orig = p->oid();
		tmp_type = p->type();
		tmp[0] = p->name();
		tmp[1] = format_complex(p->value());
		tmp[2] = format_double(p->min());
		tmp[3] = format_double(p->max());
		tmp[4] = format_double(p->imin());
		tmp[5] = format_double(p->imax());
		tmp[6] = format_double(p->rmax());
		tmp_rad = p->angle_in_radians();
	}
	else
	{
		orig = 0;
		for (int i = 0; i < 7; ++i) tmp[i].clear();
		tmp[1] = "0";
		tmp_type = Real;
		tmp_rad = true;

	}
	
}

std::string ParamEditor::format_double(double x) const
{
	if (!defined(x)) return std::string();
	return to_string(x, gui.w.rns);
}
double ParamEditor::parse(const std::string &s, const char *desc)
{
	if (s.empty() || s == " ") return UNDEFINED;

	ParsingResult result;
	cnum v = Expression::parse(s, &gui.w.rns, result);
	if (!result.ok)
	{
		gui.error(format("%s: %s", desc, result.info.c_str()));
		throw std::runtime_error("parsing error");
	}
	if (!is_real(v))
	{
		gui.error(format("%s is not real", desc));
		throw std::runtime_error("parsing error");
	}

	return v.real();
}
std::string ParamEditor::format_complex(cnum x) const
{
	if (!defined(x)) return std::string();
	return to_string(x, gui.w.rns);
}
cnum ParamEditor::cparse(const std::string &s, const char *desc)
{
	if (s.empty() || s == " ") return UNDEFINED;

	ParsingResult result;
	cnum v = Expression::parse(s, &gui.w.rns, result);
	if (!result.ok)
	{
		gui.error(format("%s: %s", desc, result.info.c_str()));
		throw std::runtime_error("parsing error");
	}
	return v;
}

void ParamEditor::operator()()
{
	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::Begin(orig ? "Parameter Settings" : "New Parameter", NULL, window_flags);

	ImGui::InputText("Name", &tmp[0]);
	ImGui::InputText("Value", &tmp[1]);

	static const char *types[] = {"Real", "Complex", "Angle", "ComplexAngle", "Integer"};
	ImGui::Combo("Type", &tmp_type, types, IM_ARRAYSIZE(types));

	ParameterType t = (ParameterType)tmp_type;
	{
		EnableGuard on;
		on(t == ParameterType::Angle);
		ImGui::Checkbox("Radians", &tmp_rad);

		on(t == ParameterType::Real || t == ParameterType::Integer || t == ParameterType::Complex);
		ImGui::InputText("re min", &tmp[2]);
		ImGui::InputText("re max", &tmp[3]);

		on(t == ParameterType::Complex);
		ImGui::InputText("im min", &tmp[4]);
		ImGui::InputText("im max", &tmp[5]);
		ImGui::InputText("abs max", &tmp[6]);
	}

	bool apply = false, close = false;
	if (ImGui::Button("OK", ImVec2(ImGui::GetContentRegionAvail().x*0.33,0))) apply = close = true;
	ImGui::SameLine();
	if (ImGui::Button("Apply", ImVec2(ImGui::GetContentRegionAvail().x*0.5,0))) apply = true;
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvail().x,0))) close = true;

	if (apply)
	{
		std::unique_ptr<Parameter> ptmp;
		Parameter *p = NULL;
		if (!orig)
		{
			ptmp.reset(new Parameter);
			p = ptmp.get();
		}
		else
		{
			p = (Parameter*)IDCarrier::find(orig);
			assert(p);
		}
		if (!p)
		{
			gui.error("Parameter not found!");
			ImGui::End();
			return;
		}

		Namespace &ns = gui.w.rns;
		Plot &plot = gui.w.plot;
		std::string &name = tmp[0];

		if (!ns.valid_name(name))
		{
			gui.error("Error: Invalid name");
			ImGui::End();
			return;
		}
		else if (!orig || name != p->name()) for (auto *q : ns.all_parameters(true))
		{
			if (name != q->name()) continue;
			gui.error("Error: Name already exists");
			ImGui::End();
			return;
		}

		cnum value;
		double r0 = 0.0, r1 = 0.0, i0 = 0.0, i1 = 0.0, a1 = 0.0;
		try
		{
			value = cparse(tmp[1], "Value");
			if (t == ParameterType::Real || t == ParameterType::Integer || t == ParameterType::Complex)
			{
				r0 = parse(tmp[2], "Re Min");
				r1 = parse(tmp[3], "Re Max");
			}
			if (t == ParameterType::Complex)
			{
				i0 = parse(tmp[4], "Im Min");
				i1 = parse(tmp[5], "Im Max");
				a1 = parse(tmp[6], "Abs Max");
			}
		}
		catch (...)
		{
			// the parse methods already call gui.error(...)
			ImGui::End();
			return;
		}

		if (orig)
		{
			std::vector<char> data2;
			ArrayWriter ww(data2);
			Serializer s(ww);
			p->save(s);
			auto p_ = orig;
			gui.w.ut.reg("Modify Parameter", [this,data2,p_]{ gui.w.modifyParam(data2, p_); });
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

		if (t == Angle) p->angle_in_radians(tmp_rad);

		p->value(value);

		if (!orig)
		{
			p->rename(name);
			ns.add(p); ptmp.release();
			orig = p->oid();
			auto oid = orig;
			gui.w.ut.reg("Add Parameter", [this,oid]{ gui.w.deleteParam(oid); });
		}
		else
		{
			plot.reparse(p->name()); // reparse for old name
			p->rename(name);
		}

		plot.reparse(p->name()); // reparse for new name
		gui.w.recalc(plot);
	}
	if (close)
	{
		parent.close_editor();
	}
	ImGui::End();
}

bool ParamEditor::handle(const SDL_Event &event)
{
	if (event.type != SDL_KEYDOWN) return false;
	auto key = event.key.keysym.sym;
	if (key == SDLK_ESCAPE)
	{
		parent.close_editor();
		return true;
	}
	return false;
}

