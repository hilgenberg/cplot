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
		{
			ifd::FileDialog::Instance().Open("OpenDialog", "Open File", "CPlot files (*.cplot){.cplot},.*", false);
			need_redraw = 20; // imgui wants to animate dimming the background
		}

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
		{
			ifd::FileDialog::Instance().Save("SaveDialog", "Save File", "*.cplot {.cplot}");
			need_redraw = 20; // imgui wants to animate dimming the background
		}

		if (ImGui::MenuItem("Preferences...", "Ctrl+Comma"))
			show_prefs_panel = true;

		ImGui::Separator();

		static const char *img_filter = "Image file{.bmp,.gif,.hdr,.jpg,.jpeg,.jpe,.pgm,.pic,.png,.ppm,.psd,.tga,.vda,.icb,.vst},.*";
		if (ImGui::MenuItem("Load Texture..."))
		{
			ifd::FileDialog::Instance().Open("OpenTextureDialog", "Open File", img_filter, false);
			need_redraw = 20; // imgui wants to animate dimming the background
		}
		if (ImGui::MenuItem("Open Reflection Texture..."))
		{
			ifd::FileDialog::Instance().Open("OpenRTextureDialog", "Open File", img_filter, false);
			need_redraw = 20; // imgui wants to animate dimming the background
		}
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

	if (ifd::FileDialog::Instance().IsDone("OpenTextureDialog"))
	{
		if (ifd::FileDialog::Instance().HasResult())
		{
			std::string r = ifd::FileDialog::Instance().GetResult().u8string();
			w.loadTexture(r);
		}
		ifd::FileDialog::Instance().Close();
	}
	if (ifd::FileDialog::Instance().IsDone("OpenRTextureDialog"))
	{
		if (ifd::FileDialog::Instance().HasResult())
		{
			std::string r = ifd::FileDialog::Instance().GetResult().u8string();
			w.loadReflectionTexture(r);
		}
		ifd::FileDialog::Instance().Close();
	}

}
