#pragma once

/**
 * For getting and setting user preferences.
 * The getters must be threadsafe.
 * Setters should be persistent.
 */

namespace Preferences
{
	bool flush(); // store changes into registry (for preference dialog)
	bool reset(); // reread from registry

	bool dynamic(); // reduce quality during animation? 
	void dynamic(bool value);

	bool slideback(); // delta sliders animate their return to center?
	void slideback(bool value);

	bool drawNormals(); // debugging
	void drawNormals(bool value);

	bool depthSort();
	void depthSort(bool value);

	int  threads(); // number of threads, -1 for num threads = num cores
	void threads(int n);
};
