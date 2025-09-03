#pragma once
#include <chrono>
#include <cstdint>
namespace utime {
inline int64_t now_ns(){
  using namespace std::chrono;
  return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}
}