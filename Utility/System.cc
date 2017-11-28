#include "System.h"
#include <chrono>
#include <thread>
#include <time.h>
#include <cmath>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>

const int n_cores = (int)std::thread::hardware_concurrency();

std::string rules_file_path()
{
	return "bla"; // TODO: remove external rules file
}
