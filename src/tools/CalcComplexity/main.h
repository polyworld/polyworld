#pragma once

#include <string>

void show_usage(std::string msg = "");

const char *consume_arg(int &argc, char *argv[], int argi);
long get_long_option(std::string option, int &argc, char *argv[], int argi);
float get_float_option(std::string option, int &argc, char *argv[], int argi);

// eof
