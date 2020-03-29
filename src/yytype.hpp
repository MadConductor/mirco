#pragma once
#include <sstream>

struct scanner_extra_data {
  std::istream *stream;
  int lineno;
};
