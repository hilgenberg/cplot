#include "System.h"
#include <thread>

const int n_cores = (int)std::thread::hardware_concurrency();

std::string rules_file_path()
{
	return "bla"; // TODO: remove external rules file
}
