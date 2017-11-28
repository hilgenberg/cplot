#pragma once
#include <string>
#include "Timer.h"
#include "Preferences.h"

extern int argc;
extern char **argv;
extern const char *arg0;

extern const int n_cores; ///< For default number of threads

std::string rules_file_path();
