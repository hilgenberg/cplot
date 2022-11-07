#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include "PlotWindow.h"

void GUI::axis_menu()
{
	const Plot &plot = w.plot;
	if (!ImGui::BeginMenu("Axis", plot.axis_type() != Axis::Invalid)) return;
	const Graph *g = plot.current_graph();
	const Axis   &ax = plot.axis;
	const Camera &cam = plot.camera;
	const bool in3d = (plot.axis_type() != Axis::Rect);

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	static const char *labels[] = {"x", "y", "z"};
	for (int i = 0; i < 2+in3d; ++i)
	{
		ImGui::MenuItem(format("%s center: %g", labels[i], ax.center(i)).c_str());
		ImGui::MenuItem(format("%s range:  %g", labels[i], ax.range(i)*2.0).c_str());
	}

	const int nin = g ? g->inRangeDimension() : -1;
	if (nin > 0) ImGui::Separator();
	static const char *in_labels[] = {"u", "v"};
	for (int i = 0; i < nin; ++i)
	{
		ImGui::MenuItem(format("%s center: %g", in_labels[i], ax.in_center(i)).c_str());
		ImGui::MenuItem(format("%s range:  %g", in_labels[i], ax.in_range(i)*2.0).c_str());
	}

	if (in3d)
	{
		ImGui::Separator();
		ImGui::MenuItem(format("phi:   %g", cam.phi()).c_str());
		ImGui::MenuItem(format("psi:   %g", cam.psi()).c_str());
		ImGui::MenuItem(format("theta: %g", cam.theta()).c_str());
		ImGui::MenuItem(format("Zoom:  %g", 1.0 / cam.zoom()).c_str());
	}

	ImGui::Separator();
	bool shift = SDL_GetModState() & (SDLK_LSHIFT|SDLK_RSHIFT);
	if (ImGui::MenuItem("Center")) {}
	if (ImGui::MenuItem("Upright")) {}
	if (ImGui::MenuItem(shift ? "Front" : "Back")) {}
#if 0
	xCenter.OnChange = [this] { OnAxisCenter(0, xCenter); };
	yCenter.OnChange = [this] { OnAxisCenter(1, yCenter); };
	zCenter.OnChange = [this] { OnAxisCenter(2, zCenter); };
	uCenter.OnChange = [this] { OnAxisCenter(-1, uCenter); };
	vCenter.OnChange = [this] { OnAxisCenter(-2, vCenter); };

	xRange.OnChange = [this] { OnAxisRange(0, xRange); };
	yRange.OnChange = [this] { OnAxisRange(1, yRange); };
	zRange.OnChange = [this] { OnAxisRange(2, zRange); };
	uRange.OnChange = [this] { OnAxisRange(-1, uRange); };
	vRange.OnChange = [this] { OnAxisRange(-2, vRange); };

	phi.OnChange = [this] { OnAxisAngle(0, phi); };
	psi.OnChange = [this] { OnAxisAngle(1, psi); };
	theta.OnChange = [this] { OnAxisAngle(2, theta); };

	dist.OnChange = [this] { OnDistance(); };
#endif

	ImGui::EndMenu();
}

#if 0
	if (ImGui::BeginTable("##AxisLayout", 3)) {
		/*
		ImGui::TableSetupColumn("##label");
		ImGui::TableSetupColumn("Center");
		ImGui::TableSetupColumn("Range");
		ImGui::TableHeadersRow();*/
		static const char *labels[] = {"x", "y", "z"};
		for (int i = 0; i < 2+in3d; ++i)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text(labels[i]);
			ImGui::TableNextColumn(); ImGui::Text("%g", ax.center(i));
			ImGui::TableNextColumn(); ImGui::Text("%g", ax.range(i)*2.0);
		}

		const int nin = g ? g->inRangeDimension() : -1;
		static const char *in_labels[] = {"u", "v"};
		for (int i = 0; i < nin; ++i)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text(in_labels[i]);
			ImGui::TableNextColumn(); ImGui::Text("%g", ax.in_center(i));
			ImGui::TableNextColumn(); ImGui::Text("%g", ax.in_range(i)*2.0);
		}

		ImGui::EndTable();
	}
//---------------------------------------------------------------------------------------------
// Check Boxes & Buttons
//---------------------------------------------------------------------------------------------

void OnTopView() { ChangeView(P3d(0.0, 90.0, 0.0)); }
void OnFrontView() { ChangeView(P3d(0.0, 0.3, 0.0)); }
void OnLeftView() { ChangeView(P3d(90.0, 0.3, 0.0)); }
void OnRightView() { ChangeView(P3d(-90.0, 0.3, 0.0)); }
void OnBackView() { ChangeView(P3d(180.0, 0.3, 0.0)); }
void OnBottomView() { ChangeView(P3d(0.0, -90.0, 0.0)); }

static inline void toggle(bool &b) { b = !b; }

void SideSectionAxis::OnCenterAxis()
{
	Plot &plot = document().plot;
	plot.axis.reset_center();
	Update(false);
	Recalc(plot);
}

void SideSectionAxis::OnEqualRanges()
{
	Plot &plot = document().plot;
	plot.axis.equal_ranges();
	Update(false);
	Recalc(plot);
}

void SideSectionAxis::ChangeView(const P3d &v)
{
	Plot &plot = document().plot;
	Camera &camera = plot.camera;
	camera.set_angles(v.x, v.y, v.z);
	Update(false);
	Redraw();
}

//---------------------------------------------------------------------------------------------
// Edit Fields
//---------------------------------------------------------------------------------------------

void SideSectionAxis::OnAxisRange(int i, NumericEdit &e)
{
	Plot &plot = document().plot;
	double x = e.GetDouble() * 0.5;
	if (!defined(x)) return;
	if (i < 0)
	{
		i = -(i + 1);
		plot.axis.in_range(i, x);
	}
	else
	{
		plot.axis.range(i, x);
	}
	Update(false);
	Recalc(plot);
}

void SideSectionAxis::OnAxisCenter(int i, NumericEdit &e)
{
	Plot &plot = document().plot;
	double x = e.GetDouble();
	if (!defined(x)) return;
	if (i < 0)
	{
		i = -(i + 1);
		plot.axis.in_center(i, x);
	}
	else
	{
		plot.axis.center(i, x);
	}
	Update(false);
	Recalc(plot);
}

void SideSectionAxis::OnAxisAngle(int i, NumericEdit &e)
{
	Camera &cam = document().plot.camera;
	double x = e.GetDouble();
	if (!defined(x)) return;
	switch (i)
	{
		case 0: cam.set_phi(x); break;
		case 1: cam.set_psi(x); break;
		case 2: cam.set_theta(x); break;
		default: assert(false); break;
	}
	Update(false);
	Redraw();
}

void SideSectionAxis::OnDistance()
{
	Camera &cam = document().plot.camera;
	double x = dist.GetDouble();
	if (!defined(x) || x <= 0.0) return;
	cam.set_zoom(1.0 / x);
	Update(false);
	Redraw();
}


#endif