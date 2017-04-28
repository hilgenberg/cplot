#include "System.h"
#include <chrono>
#include <thread>
#include <time.h>
#include <cmath>

double now()
{
	auto now = std::chrono::high_resolution_clock::now();
	auto duration = now.time_since_epoch();
	using sec = std::chrono::duration<double, std::ratio<1, 1>>;
	return std::chrono::duration_cast<sec>(duration).count();
}

void sleep(double t)
{
	if (t <= 0.0) return;
	Sleep((DWORD)(t*1000.0));
}

const int n_cores = (int)std::thread::hardware_concurrency();

std::string rules_file_path()
{
	return "bla"; // TODO: remove external rules file
}

// TODO: read ~/.config/cplot/config
Settings::Settings() : data(NULL) { }
Settings::~Settings() { }

bool Settings::get(const std::string &name, bool &value)
{
	return false;
}
void Settings::set(const std::string &name, bool  value)
{
}

/*bool Settings::get(const std::string &name, int32_t &value);
void Settings::set(const std::string &name, int32_t  value);

bool Settings::get(const std::string &name, uint32_t &value);
void Settings::set(const std::string &name, uint32_t  value);

bool Settings::get(const std::string &name,       std::string &value);
void Settings::set(const std::string &name, const std::string &value);

bool Settings::get(const std::string &name, Serializable *&value); // caller will delete value
void Settings::set(const std::string &name, Serializable * value);*/


Settings settings;

