#pragma once
#include <cstdio>
#define LOGF(fmt, ...) std::fprintf(stderr, "[uTrade] " fmt "\n", ##__VA_ARGS__)