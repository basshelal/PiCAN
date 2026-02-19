#pragma once
#include <cstdio>

namespace stacktrace {

void
initialize(char** argv);

void
print_stacktrace(std::FILE* file = stderr, int skipFrames = 0);

}  // namespace stacktrace
