#pragma once
#include <filesystem>

/**
 * For getting and setting user preferences.
 * The getters must be threadsafe.
 * Setters should be persistent.
 */

extern const int n_cores;

namespace Preferences
{
	bool flush(); // store changes into registry/ini file
	bool reset(); // reread from registry/disk

	bool dynamic(); // reduce quality during animation? 
	void dynamic(bool value);

	#ifdef _WIN32
	bool slideback(); // delta sliders animate their return to center?
	void slideback(bool value);
	#endif

	#ifdef __linux__
	std::filesystem::path directory();
	#endif

	bool drawNormals(); // debugging
	void drawNormals(bool value);

	bool depthSort();
	void depthSort(bool value);

	int  threads(bool effective = true); // number of threads, -1 for num threads = num cores
	void threads(int n);
};
