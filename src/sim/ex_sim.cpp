#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "core/util.hpp"
#include "core/time.hpp"
#include "core/log.hpp"
#include "core/framing.hpp"
#include "proto/messages.hpp"

static int udp_tx(const char* ip, uint16_t port, sockaddr_in& dst){
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0); if(fd<0){ perror("socket UDP"); return -1; }
  std::memset(&dst,0,sizeof(dst)); dst.sin_family=AF_INET; dst.sin_port=htons(port);
  if(inet_pton(AF_INET, ip, &dst.sin_addr)!=1){ perror("inet_pton"); return -1; }
  return fd;
}
static int tcp_listen(uint16_t port){
  int s=::socket(AF_INET, SOCK_STREAM,0); if(s<0){ perror("socket TCP"); return -1; }
  int yes=1; setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
  sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(port); a.sin_addr.s_addr=htonl(INADDR_ANY);
  if(bind(s,(sockaddr*)&a,sizeof(a))<0){ perror("bind"); return -1; }
  if(listen(s,8)<0){ perror("listen"); return -1; }
  return s;
}

int main(int argc, char** argv){
  if(argc<2){ std::fprintf(stderr,"Usage: %s <config.yaml>\n", argv[0]); return 1; }
  auto kv = load_yaml_kv(argv[1]);
  std::string md_ip = as_str(kv,"md_ip","127.0.0.1");
  uint16_t md_port = as_int(kv,"md_port",5001);
  uint16_t ord_port= as_int(kv,"ord_port",9001);
  int hb_ms = as_int(kv,"heartbeat_ms",200);

  sockaddr_in md_dst{}; int mdfd = udp_tx(md_ip.c_str(), md_port, md_dst); if(mdfd<0) return 1;

  // Publish L1 at ~10k/s
  std::thread pub([&]{
    MdTop m{}; m.bid_px=10000; m.ask_px=10002; m.bid_sz=10; m.ask_sz=10; m.seq=0;
    while(true){
      m.seq++;
      if((m.seq%101)==0){ m.bid_px++; m.ask_px++; }
      m.bid_sz=8+(m.seq&7); m.ask_sz=8+((m.seq>>3)&7);
      m.ts=utime::now_ns();
      sendto(mdfd,&m,sizeof(m),0,(sockaddr*)&md_dst,sizeof(md_dst));
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  }); pub.detach();

  int ls = tcp_listen(ord_port); if(ls<0) return 1; LOGF("ex_sim: md %s:%u, orders :%u", md_ip.c_str(), md_port, ord_port);

  while(true){
    sockaddr_in cli{}; socklen_t sl=sizeof(cli); int c=accept(ls,(sockaddr*)&cli,&sl); if(c<0){ perror("accept"); continue; }
    LOGF("client connected");
    std::thread([c,hb_ms]{
      auto last=utime::now_ns();
      while(true){
        auto now=utime::now_ns();
        if(now-last > hb_ms*1'000'000LL){ send_frame(c,3,nullptr,0); last=now; }
        fd_set rf; FD_ZERO(&rf); FD_SET(c,&rf); timeval tv{0,1000};
        int r = select(c+1,&rf,nullptr,nullptr,&tv);
        if(r>0 && FD_ISSET(c,&rf)){
          uint8_t type; Order ord{}; Ack ack{}; uint32_t len=0;
          if(!recv_frame(c,type,&ord,sizeof(ord),len)){ break; }
          if(type==1 && len==sizeof(Order)){
            ack.exch_recv_ns=utime::now_ns(); ack.id=ord.id; ack.status=0; send_frame(c,2,&ack,sizeof(ack));
          }
        }
      }
      ::close(c); LOGF("client disconnected");
    }).detach();
  }
}