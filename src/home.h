#pragma once
#include <string>
#include <vector>

// Shows the home screen and returns the chosen file path.
// Returns an empty string if the user quits.
std::string run_home(std::vector<std::string> &history);
