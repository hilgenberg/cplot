#include "GUI.h"
#include "imgui/imgui.h"
#include "ImFileDialog/ImFileDialog.h"
#include "PlotWindow.h"

static void init_ImFileDialog()
{
	static bool done = false;
	if (done) return;
	done = true;

	// ImFileDialog requires you to set the CreateTexture and DeleteTexture
	ifd::FileDialog::Instance().CreateTexture = [](uint8_t* data, int w, int h, char fmt) -> void* {
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		return (void*)(uintptr_t)tex;
	};
	ifd::FileDialog::Instance().DeleteTexture = [](void* tex) {
		GLuint texID = (GLuint)(uintptr_t)tex;
		glDeleteTextures(1, &texID);
	};
}

void GUI::file_menu()
{
	init_ImFileDialog();
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New"))
		{
			confirm(w.ut.have_changes(), "Lose all changes?", 
				[this]{ w.clear(); redraw(); });
		}
		if (ImGui::MenuItem("Open...", "Ctrl+O"))
			ifd::FileDialog::Instance().Open("OpenDialog", "Open File", "CPlot files (*.cplot){.cplot},.*", false);

		if (ImGui::MenuItem("Save", "Ctrl+S"))
		{
			try
			{
				w.save();
			}
			catch (std::exception &e)
			{
				error(e.what());
			}
		}

		if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
			ifd::FileDialog::Instance().Save("SaveDialog", "Save File", "*.cplot {.cplot}");
		ImGui::Separator();
		if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
			SDL_Event e; e.type = SDL_QUIT;
			SDL_PushEvent(&e);
		}
		ImGui::EndMenu();
	}
	
	if (ifd::FileDialog::Instance().IsDone("OpenDialog"))
	{
		if (ifd::FileDialog::Instance().HasResult())
		{
			std::string r = ifd::FileDialog::Instance().GetResult().u8string();
			confirm(w.ut.have_changes(), "Document has unsaved changes. Continue?", 
				[this,r]{ w.load(r); redraw(); });
		}
		ifd::FileDialog::Instance().Close();
	}
	if (ifd::FileDialog::Instance().IsDone("SaveDialog"))
	{
		if (ifd::FileDialog::Instance().HasResult())
		{
			std::string r = ifd::FileDialog::Instance().GetResult().u8string();
			try
			{
				w.saveAs(r);
			}
			catch (std::exception &e)
			{
				error(e.what());
			}
		}
		ifd::FileDialog::Instance().Close();
	}
}
