#pragma once

#include <filesystem>
#include <fstream>
#include <string>

/**
 * a class that writes a temporary file when it is created,
 * holds the name, and deletes the file when it is destructed.
 */
struct TempFile {
  std::string file_path_;
  TempFile(std::string_view file_name, std::string_view content) {
    file_path_ = (std::filesystem::temp_directory_path() / file_name);
    std::ofstream f{file_path_};
    f << content << std::flush;
  }
  ~TempFile() { std::remove(file_path_.c_str()); }
};
