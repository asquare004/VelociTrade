#pragma once
#include <cstdint>
#pragma pack(push,1)
struct MdTop { uint64_t seq; int32_t bid_px; int32_t bid_sz; int32_t ask_px; int32_t ask_sz; uint64_t ts; };
struct MdLevel { int32_t px; int32_t sz; uint8_t side; }; // side: 0=bid,1=ask
struct Order { uint64_t client_send_ns; uint32_t id; uint8_t side; int32_t px; int32_t qty; };
struct Ack   { uint64_t exch_recv_ns; uint32_t id; uint8_t status; };
#pragma pack(pop)
