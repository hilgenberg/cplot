#include "GUI.h"

void GUI::error_panel()
{
	static bool showing = false;
	if (error_msg.empty()) return;
	if (!showing)
	{
		visible = true;
		showing = true;
		ImGui::OpenPopup("Error");
		need_redraw = 20; // imgui wants to animate dimming the background
	}
	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (!ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) return;
	ImGui::TextWrapped(error_msg.c_str());
	ImGui::Separator();
	bool close = false;
	if (ImGui::Button("OK", ImVec2(120, 0)))
		close = true;
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape))
		close = true;
	if (close)
	{
		error_msg.clear();
		ImGui::CloseCurrentPopup();
		showing = false;
	}
	ImGui::EndPopup();
}

void GUI::confirmation_panel()
{
	static bool showing = false;
	if (!confirm_action) return;
	if (!showing)
	{
		visible = true;
		showing = true;
		ImGui::OpenPopup("Confirmation");
		need_redraw = 20; // imgui wants to animate dimming the background
	}
	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (!ImGui::BeginPopupModal("Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) return;
	ImGui::TextWrapped(confirm_msg.c_str());
	ImGui::Separator();
	bool close = false;
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape))
		close = true;
	if (ImGui::Button("Cancel", ImVec2(120, 0))) close = true;
	ImGui::SameLine();
	if (ImGui::Button("OK", ImVec2(120, 0)))
	{
		try
		{
			confirm_action();
		}
		catch (std::exception &e)
		{
			error(e.what());
		}
		close = true;
	}
	if (close)
	{
		confirm_msg.clear();
		confirm_action = nullptr;
		ImGui::CloseCurrentPopup();
		showing = false;
	}
	ImGui::EndPopup();
}
