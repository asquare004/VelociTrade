#pragma once
#include <cstdint>
#include <array>
#include <atomic>
#include "proto/messages.hpp"

template<size_t DEPTH>
struct L2Book {
  std::array<int32_t,DEPTH> bid_px{}, bid_sz{}, ask_px{}, ask_sz{};
  std::atomic<uint64_t> seq{0};
};

template<size_t DEPTH>
inline void apply_top(L2Book<DEPTH>& b, const MdTop& m){
  b.bid_px[0]=m.bid_px; b.bid_sz[0]=m.bid_sz; b.ask_px[0]=m.ask_px; b.ask_sz[0]=m.ask_sz;
  b.seq.store(m.seq, std::memory_order_release);
}