#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "PlotWindow.h"
#include "../Engine/Namespace/UserFunction.h"
#include "../Engine/Namespace/Expression.h"

void GUI::defs_menu()
{
	if (!ImGui::BeginMenu("Definitions")) return;

	std::set<UserFunction*> afs(w.rns.all_functions(true));
	std::vector<UserFunction*> fs(afs.begin(), afs.end());
	std::sort(fs.begin(), fs.end(), [&](const UserFunction *a, const UserFunction *b)->bool{ return a->formula() < b->formula(); });

	for (UserFunction *f : fs)
	{
		ImGui::PushID(f);
		if (ImGui::BeginMenu(f->formula().c_str()))
		{
			if (ImGui::MenuItem("Edit...", NULL, false, !def_edit))
			{
				def_edit = true; def_orig = f->oid();
				def_tmp = f->formula();
			}
			if (ImGui::MenuItem("Delete"))
				w.deleteDef(f->oid());
			ImGui::EndMenu();
		}
		ImGui::PopID();
	}
	if (!fs.empty()) ImGui::Separator();

	if (ImGui::MenuItem("New Definition"))
	{
		def_edit = true; def_orig = 0;
		def_tmp = "f(x) = sinx / x";
	}
	ImGui::EndMenu();
}

void GUI::def_editor()
{
	if (!def_edit) return;

	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	//window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::Begin(param_orig ? "Edit Definition" : "New Definition", NULL, window_flags);

	ImGui::InputTextMultiline("##Formula", &def_tmp, 
		ImVec2(ImGui::GetContentRegionAvail().x, 
		       ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()));
	ImGui::PopTextWrapPos();

	bool apply = false, close = false;
	if (ImGui::Button("OK", ImVec2(ImGui::GetContentRegionAvail().x*0.33,0))) apply = close = true;
	ImGui::SameLine();
	if (ImGui::Button("Apply", ImVec2(ImGui::GetContentRegionAvail().x*0.5,0))) apply = true;
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvail().x,0))) close = true;

	if (apply)
	{
		Namespace &ns = w.rns;
		Plot &plot = w.plot;
		std::string &formula = def_tmp;

		if (formula.empty() || formula == " ")
		{
			error("Error: Empty definition");
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
				error("Syntax Error : Expected <name>(p₁, …, pᵣ) = <definition>");
				ns.remove(tns);
				return;
			}
			else if (!ex->result().ok)
			{
				error(ex->result().info);
				ns.remove(tns);
				return;
			}
		}
		catch (...)
		{
			assert(false);
			error("Unexpected exception");
			ns.remove(tns);
			return;
		}
		ns.remove(tns); tns = NULL;

		// parsing was ok - add it
		std::unique_ptr<UserFunction> tmp;
		UserFunction *f = NULL;
		if (!def_orig)
		{
			tmp.reset(new UserFunction);
			f = tmp.get();
		}
		else
		{
			f = (UserFunction*)IDCarrier::find(def_orig);
			assert(f);
		}
		if (!f)
		{
			error("Definition not found!");
			ImGui::End();
			return;
		}

		if (def_orig)
		{
			std::vector<char> data2;
			ArrayWriter ww(data2);
			Serializer s(ww);
			f->save(s);
			auto p_ = def_orig;
			w.ut.reg("Modify Definition", [this,data2,p_]{ w.modifyDef(data2, p_); });
			plot.reparse(f->name()); // reparse for old name
		}

		f->formula(formula);

		if (!def_orig)
		{
			ns.add(f); tmp.release();
			def_orig = f->oid();
			auto oid = def_orig;
			w.ut.reg("Add Definition", [this,oid]{ w.deleteDef(oid); });
		}

		plot.reparse(f->name()); // reparse for new name
		w.recalc(plot);
	}
	if (close) def_edit = false;
	ImGui::End();
}

