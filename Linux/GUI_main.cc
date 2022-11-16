#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "PlotWindow.h"
#include "../Engine/Namespace/Expression.h"

void GUI::main_panel()
{
	const Plot &plot = w.plot;
	const Graph *g = plot.current_graph();
	if (!g) return;
	assert(plot.axis.type() == plot.axis_type());

	ImGuiViewport &screen = *ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(screen.WorkPos);
	ImGui::SetNextWindowSize(ImVec2(screen.WorkSize.x, 0.0f));
	ImGui::SetNextWindowBgAlpha(0.5f);

	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |= ImGuiWindowFlags_NoScrollbar;
	window_flags |= ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoSavedSettings;

	ImGui::Begin("Graph Definition", &show_main_panel, window_flags);

	if (ImGui::BeginTable("##Layout", 2))
	{
		ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("##data", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGuiComboFlags flags = 0
			//| ImGuiComboFlags_PopupAlignLeft
			| ImGuiComboFlags_NoArrowButton
		;

		ImGui::SetNextItemWidth(30.0f); // TODO

		const int n = plot.number_of_graphs(), i0 = plot.current_graph_index();
		if (n > 1 && ImGui::BeginCombo("##graph_sel", format("%d:", i0+1).c_str(), flags))
		{
			for (int i = 0; i < n; ++i)
			{
				if (ImGui::Selectable(format("%d", i+1).c_str(), i == i0))
					w.selectGraph(i);
				if (i == i0) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::TableNextColumn();

		//----------------------------------------------------------------------------------
		// domain
		//----------------------------------------------------------------------------------
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 2.0f / 9.0f);
		constexpr int N = 12;
		static const char *dom_asc[] = {
			"R >> R", "R² >> R", "C >> C",
			"R >> R²", "R >> R³", "S¹ >> R²", "S¹ >> R³",
			"R² >> R³", "S² >> R³",
			"R³ >> R", "R² >> R²", "R³ >> R³"};
		static const char *dom_utf[] = {
			"R \u2192 R", "R\u00b2 \u2192 R", "C \u2192 C", 
			"R \u2192 R\u00b2", "R \u2192 R\u00b3",  "S\u00b9 \u2192 R\u00b2", "S\u00b9 \u2192 R\u00b3", 
			"R\u00b2 \u2192 R\u00b3", "S\u00b2 \u2192 R\u00b3", 
			"R\u00b3 \u2192 R", "R\u00b2 \u2192 R\u00b2", "R\u00b3 \u2192 R\u00b3"};
		static GraphType dom_val[] = {
			R_R, R2_R, C_C,
			R_R2, R_R3, S1_R2, S1_R3,
			R2_R3, S2_R3,
			R3_R, R2_R2, R3_R3};
		assert(IM_ARRAYSIZE(dom_asc) == N);
		assert(IM_ARRAYSIZE(dom_utf) == N);
		assert(IM_ARRAYSIZE(dom_val) == N);
		
		int orig = -1;
		for (int i = 0; i < N; ++i) if (dom_val[i] == g->type()) { orig = i; break; }
		int tmp = orig;
		ImGui::Combo("##DomainCombo", &tmp, dom_asc, N);
		if (tmp != orig) w.setDomain(dom_val[tmp]);
		ImGui::SameLine();

		//----------------------------------------------------------------------------------
		// coords
		//----------------------------------------------------------------------------------
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 3.0f / 7.0f);
		static const char *coord[] = { "Cartesian", "Polar", "Spherical", "Cylindrical" };
		if (ImGui::BeginCombo("##CoordCombo", coord[g->coords()]))
		{
			for (GraphCoords c : g->valid_coords())
			{
				const bool is_selected = (c == g->coords());
				if (ImGui::Selectable(coord[c], is_selected)) w.setCoords(c);
				if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();

		//----------------------------------------------------------------------------------
		// mode
		//----------------------------------------------------------------------------------
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

		static const char *modes[] = { 
			"Graph", "Image", "Riemann Image", "Vector Field", "Real Part", "Imaginary Part",
			"Absolute Value", "Phase", "Implicit", "Color", "Riemann Color", "Histogram" };
		if (ImGui::BeginCombo("##ModeCombo", modes[g->mode()]))
		{
			int i = 0;
			for (GraphMode m : g->valid_modes())
			{
				
				const bool is_selected = (m == g->mode());
				if (ImGui::Selectable(format("F%d: %s", ++i, modes[m]).c_str(), is_selected)) w.setMode(m);
				if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		
		//----------------------------------------------------------------------------------
		// defs
		//----------------------------------------------------------------------------------
		int nf = g->n_components(); assert(nf >= 1 && nf <= 3);
		auto comp = g->components(); assert(nf == (int)comp.size());
		for (int i = 0; i < nf; ++i)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::PushID(i);
			ImGui::Text("%s =", comp[i].c_str());
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			//ImGui::Text("%s", g->fn(i + 1).c_str());
			std::string orig = g->fn(i + 1), tmp = orig;
			constexpr int flags = 0;//ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackAlways|ImGuiInputTextFlags_CallbackEdit;
			ImGui::InputText("##F_Input", &tmp, flags);
			if (tmp != orig) w.setF(i+1, tmp);

			ImGui::PopID();
		}

		ImGui::EndTable();
	}

	//----------------------------------------------------------------------------------
	// parsing errors
	//----------------------------------------------------------------------------------
	Expression *expr = g->expression();
	if (!expr)
	{
		if (plot.number_of_visible_graphs() > 0 && !plot.axis.valid())
		{
			ImGui::Text("Error: Axis type mismatch");
		}
	}
	else
	{
		auto &result = expr->result();
		int nf = g->n_components();
		int i = (int)result.index;
		if (!result.ok && i < nf && !g->fn(i + 1).empty())
		{
			ImGui::Text("Error: %s", result.info.c_str());
			// TODO: mark the error position somehow
		}
	}

	main_panel_height = ImGui::GetWindowHeight();

	ImGui::End();
}








