#pragma once

#include "resource.h"

#include <string>

namespace tl {

bool setup(const char* file_path);
std::string execute_command(const char* cmd, const char* working_directory);
void render(int display_w, int display_h);

}

