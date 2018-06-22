#pragma once

namespace timelapse { namespace session {
  
void setup(const char* file_path = nullptr);

bool is_valid();
const char* working_dir();
const char* file_path();

void shutdown();

}}
