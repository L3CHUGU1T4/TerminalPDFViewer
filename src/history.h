#pragma once
#include <string>
#include <vector>

std::vector<std::string> load_history();
void                     save_to_history(const std::string &path);
