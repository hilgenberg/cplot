#pragma once
#include <string>
#include <functional>

extern int argc;
extern char **argv;
extern const char *arg0;

double now(); ///< Must be in seconds since some reference date. Higher resolution is better.
void sleep(double dt);

extern const int n_cores; ///< For default number of threads

std::string rules_file_path();

/**
 * For getting and setting user preferences.
 * The getters must be threadsafe and return true if the value was set.
 * If the value was not set, they must not change their value parameter, because it was probably already set
 * to the default's default value.
 * Setters should be persistent.
 */

class Settings
{
public:
	Settings();
	~Settings();
	
	bool get(const std::string &name, bool &value);
	void set(const std::string &name, bool  value);

	/*bool get(const std::string &name, int32_t &value);
	void set(const std::string &name, int32_t  value);

	bool get(const std::string &name, uint32_t &value);
	void set(const std::string &name, uint32_t  value);
	
	bool get(const std::string &name,       std::string &value);
	void set(const std::string &name, const std::string &value);

	bool get(const std::string &name, Serializable *&value); // caller will delete value
	void set(const std::string &name, Serializable * value);*/

private:
	void *data;
};

extern Settings settings;

class Timer
{
public:
	Timer(double dt);
	~Timer();
	void start();
	void stop();
	bool running() const;
	double dt() const { return dt_; }
	std::function<void(void)> callback;

private:
	#ifdef _WINDOWS
	HANDLE timer;
	#endif
	double dt_;
};
