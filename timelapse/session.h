#pragma once
#include <initializer_list>

namespace timelapse { namespace session {
  
void setup(const char* file_path = nullptr);
void shutdown();

bool is_valid();
const char* working_dir();
const char* file_path();

}}
