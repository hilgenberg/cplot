#include "GUI.h"
#include "imgui/imgui.h"
#include "ImFileDialog/ImFileDialog.h"
#include "PlotWindow.h"


struct GUI_FileMenu : public GUI_Menu
{
	GUI_FileMenu(GUI &gui) : GUI_Menu(gui)
	{
		static bool init_done = false;
		if (!init_done)
		{
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
			init_done = true;
		}
	}
	void operator()()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New")) gui.confirm(gui.w.ut.have_changes(), "Lose all changes?", [this]{ gui.w.clear(); gui.redraw(); });
			if (ImGui::MenuItem("Open...", "Ctrl+O")) open_file();
			if (ImGui::MenuItem("Save", "Ctrl+S")) save();
			if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) save_as();
			if (ImGui::MenuItem("Preferences...", "Ctrl+Comma"))
			{
				gui.show_prefs_panel = true;
				gui.show();
			}

			ImGui::Separator();

			static const char *img_filter = "Image file{.bmp,.gif,.hdr,.jpg,.jpeg,.jpe,.pgm,.pic,.png,.ppm,.psd,.tga,.vda,.icb,.vst},.*";
			if (ImGui::MenuItem("Load Texture..."))
			{
				ifd::FileDialog::Instance().Open("OpenTextureDialog", "Open File", img_filter, false);
				gui.redraw(20); // imgui wants to animate dimming the background
			}
			if (ImGui::MenuItem("Open Reflection Texture..."))
			{
				ifd::FileDialog::Instance().Open("OpenRTextureDialog", "Open File", img_filter, false);
				gui.redraw(20); // imgui wants to animate dimming the background
			}
			if (ImGui::MenuItem("Open Alpha Mask..."))
			{
				ifd::FileDialog::Instance().Open("OpenMaskDialog", "Open File", img_filter, false);
				gui.redraw(20); // imgui wants to animate dimming the background
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Quit", "Ctrl+Q"))
			{
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
				gui.confirm(gui.w.ut.have_changes(), "Document has unsaved changes. Continue?", 
					[this,r]{ gui.w.load(r); gui.redraw(); });
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
					gui.w.saveAs(r);
				}
				catch (std::exception &e)
				{
					gui.error(e.what());
				}
			}
			ifd::FileDialog::Instance().Close();
		}

		if (ifd::FileDialog::Instance().IsDone("OpenTextureDialog"))
		{
			if (ifd::FileDialog::Instance().HasResult())
			{
				std::string r = ifd::FileDialog::Instance().GetResult().u8string();
				gui.w.loadTexture(r);
			}
			ifd::FileDialog::Instance().Close();
		}
		if (ifd::FileDialog::Instance().IsDone("OpenRTextureDialog"))
		{
			if (ifd::FileDialog::Instance().HasResult())
			{
				std::string r = ifd::FileDialog::Instance().GetResult().u8string();
				gui.w.loadReflectionTexture(r);
			}
			ifd::FileDialog::Instance().Close();
		}
		if (ifd::FileDialog::Instance().IsDone("OpenMaskDialog"))
		{
			if (ifd::FileDialog::Instance().HasResult())
			{
				std::string r = ifd::FileDialog::Instance().GetResult().u8string();
				gui.w.loadCustomMask(r);
			}
			ifd::FileDialog::Instance().Close();
		}
	}

	void open_file()
	{
		ifd::FileDialog::Instance().Open("OpenDialog", "Open File", "CPlot files (*.cplot){.cplot},.*", false);
		gui.redraw(20); // imgui wants to animate dimming the background
	}
	void save_as()
	{
		ifd::FileDialog::Instance().Save("SaveDialog", "Save File", "*.cplot {.cplot}");
		gui.redraw(20); // imgui wants to animate dimming the background
	}
	void save()
	{
		try
		{
			gui.w.save();
		}
		catch (std::exception &e)
		{
			gui.error(e.what());
		}
	}

	bool handle(const SDL_Event &event)
	{
		if (event.type != SDL_KEYDOWN) return false;

		auto key = event.key.keysym.sym;
		auto m   = event.key.keysym.mod;
		constexpr int SHIFT = 1, CTRL = 2, ALT = 4;
		const int  shift = (m & (KMOD_LSHIFT|KMOD_RSHIFT)) ? SHIFT : 0;
		const int  ctrl  = (m & (KMOD_LCTRL|KMOD_RCTRL)) ? CTRL : 0;
		const int  alt   = (m & (KMOD_LALT|KMOD_RALT)) ? ALT : 0;
		const int  mods  = shift + ctrl + alt;

		switch (key)
		{
			case SDLK_q:
				if (mods == CTRL)
				{
					SDL_Event e; e.type = SDL_QUIT;
					SDL_PushEvent(&e);
					return true;
				}
				break;
			case SDLK_COMMA:
				if (mods == CTRL)
				{
					gui.show_prefs_panel = true;
					gui.show();
					return true;
				}
				break;
			case SDLK_s:
				if (mods == CTRL+SHIFT)
				{
					save_as();
					gui.show();
					return true;
				}
				else if (mods == CTRL)
				{
					save();
					return true;
				}
				break;
			case SDLK_o:
				if (mods == CTRL)
				{
					open_file();
					gui.show();
					return true;
				}
				break;
		}
		return false;
	}
};
GUI_Menu *new_file_menu(GUI &gui) { return new GUI_FileMenu(gui); }
