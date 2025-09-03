#pragma once
#include <cstdint>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>

// Wire frame: [uint32 length][uint8 type][payload...]
// Types: 1=Order, 2=Ack, 3=Heartbeat

inline bool send_frame(int fd, uint8_t type, const void* buf, uint32_t len){
  uint32_t L = htonl(len + 1);
  uint8_t hdr[5]; std::memcpy(hdr, &L, 4); hdr[4] = type;
  if(::send(fd, hdr, 5, 0) != 5) return false;
  if(len > 0 && ::send(fd, buf, len, 0) != (ssize_t)len) return false;
  return true;
}
inline bool recv_frame(int fd, uint8_t& type, void* buf, uint32_t cap, uint32_t& out_len){
  uint32_t Lnet; ssize_t n = ::recv(fd, &Lnet, 4, MSG_WAITALL); if(n<=0) return false;
  uint32_t L = ntohl(Lnet);
  uint8_t t; n = ::recv(fd, &t, 1, MSG_WAITALL); if(n<=0) return false; type = t;
  uint32_t payload = L - 1; out_len = payload;
  if(payload > cap) return false;
  if(payload > 0){
    n = ::recv(fd, buf, payload, MSG_WAITALL);
    if(n != (ssize_t)payload) return false;
  }
  return true;
}
