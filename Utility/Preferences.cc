#include "Preferences.h"
#include "System.h"

// TODO: read ~/.config/cplot/config

namespace Preferences
{
	bool flush() { return false; }

	bool dynamic() { return true; }
	void dynamic(bool value) { assert(false); }

	bool slideback() { return true; }
	void slideback(bool value) { assert(false); }

	bool drawNormals() { return false; }
	void drawNormals(bool value) { assert(false); }

	bool depthSort() { return true; }
	void depthSort(bool value) { assert(false); }

	int  threads() { return n_cores; }
	void threads(int n) { assert(false); }
};
