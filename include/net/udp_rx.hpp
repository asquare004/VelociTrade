#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

inline int udp_bind_any(uint16_t port){
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0); if(fd<0) return -1;
  int reuse=1; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(port); addr.sin_addr.s_addr=htonl(INADDR_ANY);
  if(bind(fd,(sockaddr*)&addr,sizeof(addr))<0){ ::close(fd); return -1; }
  return fd;
}