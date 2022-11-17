#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "PlotWindow.h"
#include "../Engine/Namespace/UserFunction.h"
#include "../Engine/Namespace/Expression.h"

struct GUI_DefsMenu;

struct DefEditor : public GUI_Panel
{
	DefEditor(GUI_DefsMenu &parent, UserFunction *p);
	void operator()() override;
	bool handle(const SDL_Event &event) override;

	GUI_DefsMenu &parent;
	IDCarrier::OID orig = 0;
	std::string tmp;
	int  tmp_type;
	bool tmp_rad;

	std::string format_double(double x) const;
	double parse(const std::string &s, const char *desc);
	std::string format_complex(cnum x) const;
	cnum cparse(const std::string &s, const char *desc);
};

struct GUI_DefsMenu : public GUI_Menu
{
	GUI_DefsMenu(GUI &gui) : GUI_Menu(gui) {}

	std::unique_ptr<DefEditor> ed;
	void close_editor() { ed = nullptr; gui.redraw(); }

	bool handle(const SDL_Event &event)
	{
		if (!ed) return false;
		return ed->handle(event);
	}

	void operator()()
	{
		if (ImGui::BeginMenu("Definitions"))
		{
			auto &w = gui.w;
			std::set<UserFunction*> afs(w.rns.all_functions(true));
			std::vector<UserFunction*> fs(afs.begin(), afs.end());
			std::sort(fs.begin(), fs.end(), [&](const UserFunction *a, const UserFunction *b)->bool{ return a->formula() < b->formula(); });

			for (UserFunction *f : fs)
			{
				ImGui::PushID(f);
				if (ImGui::BeginMenu(f->formula().c_str()))
				{
					if (ImGui::MenuItem("Edit...", NULL, false, !ed))
					{
						ed.reset(new DefEditor(*this, f));
						gui.redraw();
					}
					if (ImGui::MenuItem("Delete"))
						w.deleteDef(f->oid());
					ImGui::EndMenu();
				}
				ImGui::PopID();
			}
			if (!fs.empty()) ImGui::Separator();

			if (ImGui::MenuItem("New Definition", NULL, false, !ed))
			{
				ed.reset(new DefEditor(*this, NULL));
				gui.redraw();
			}
			ImGui::EndMenu();
		}

		if (ed) (*ed)();
	}
};
GUI_Menu *new_defs_menu(GUI &gui) { return new GUI_DefsMenu(gui); }

//----------------------------------------------------------------------------
// ParamEditor implementation
//----------------------------------------------------------------------------

DefEditor::DefEditor(GUI_DefsMenu &parent, UserFunction *f)
: parent(parent), GUI_Panel(parent.gui)
{
	if (f)
	{
		orig = f->oid();
		tmp = f->formula();
	}
	else
	{
		orig = 0;
		tmp = "f(x) = sinx / x";
	}
}

void DefEditor::operator()()
{
	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	//window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::Begin(orig ? "Edit Definition" : "New Definition", NULL, window_flags);

	ImGui::InputTextMultiline("##Formula", &tmp, 
		ImVec2(ImGui::GetContentRegionAvail().x, 
		       ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()));

	bool apply = false, close = false;
	if (ImGui::Button("OK", ImVec2(ImGui::GetContentRegionAvail().x*0.33,0))) apply = close = true;
	ImGui::SameLine();
	if (ImGui::Button("Apply", ImVec2(ImGui::GetContentRegionAvail().x*0.5,0))) apply = true;
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvail().x,0))) close = true;

	if (apply)
	{
		Namespace &ns = gui.w.rns;
		Plot &plot = gui.w.plot;
		std::string &formula = tmp;

		if (formula.empty() || formula == " ")
		{
			gui.error("Error: Empty definition");
			ImGui::End();
			return;
		}

		// testrun first
		Namespace *tns = new Namespace; // to ignore name clash when modifying existing function
		ns.add(tns);
		try
		{
			UserFunction *f = new UserFunction;
			f->formula(formula);
			tns->add(f);
			Expression *ex = f->expression();
			if (!ex)
			{
				gui.error("Syntax Error : Expected <name>(p₁, …, pᵣ) = <definition>");
				ns.remove(tns);
				return;
			}
			else if (!ex->result().ok)
			{
				gui.error(ex->result().info);
				ns.remove(tns);
				return;
			}
		}
		catch (...)
		{
			assert(false);
			gui.error("Unexpected exception");
			ns.remove(tns);
			return;
		}
		ns.remove(tns); tns = NULL;

		// parsing was ok - add it
		std::unique_ptr<UserFunction> ftmp;
		UserFunction *f = NULL;
		if (!orig)
		{
			ftmp.reset(new UserFunction);
			f = ftmp.get();
		}
		else
		{
			f = (UserFunction*)IDCarrier::find(orig);
			assert(f);
		}
		if (!f)
		{
			gui.error("Definition not found!");
			ImGui::End();
			return;
		}

		if (orig)
		{
			std::vector<char> data2;
			ArrayWriter ww(data2);
			Serializer s(ww);
			f->save(s);
			auto p_ = orig;
			gui.w.ut.reg("Modify Definition", [this,data2,p_]{ gui.w.modifyDef(data2, p_); });
			plot.reparse(f->name()); // reparse for old name
		}

		f->formula(formula);

		if (!orig)
		{
			ns.add(f); ftmp.release();
			orig = f->oid();
			auto oid = orig;
			gui.w.ut.reg("Add Definition", [this,oid]{ gui.w.deleteDef(oid); });
		}

		plot.reparse(f->name()); // reparse for new name
		gui.w.recalc(plot);
	}
	if (close) parent.close_editor();
	ImGui::End();
}

bool DefEditor::handle(const SDL_Event &event)
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
