#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <chrono>

#include "core/time.hpp"
#include "core/log.hpp"
#include "core/util.hpp"
#include "core/framing.hpp"
#include "net/udp_rx.hpp"
#include "book/order_book_l2.hpp"
#include "proto/messages.hpp"

static int tcp_connect(const char* ip, uint16_t port){
  int s=::socket(AF_INET,SOCK_STREAM,0); if(s<0){ perror("socket TCP"); return -1; }
  sockaddr_in d{}; d.sin_family=AF_INET; d.sin_port=htons(port);
  if(inet_pton(AF_INET,ip,&d.sin_addr)!=1){ perror("inet_pton"); return -1; }
  if(connect(s,(sockaddr*)&d,sizeof(d))<0){ return -1; }
  return s;
}

int main(int argc, char** argv){
  if(argc<3){ std::fprintf(stderr,"Usage: %s <config.yaml> <num_orders>\n", argv[0]); return 1; }
  auto kv = load_yaml_kv(argv[1]);
  std::string md_ip = as_str(kv,"md_ip","127.0.0.1");
  uint16_t md_port=as_int(kv,"md_port",5001);
  std::string ord_ip= as_str(kv,"ord_ip","127.0.0.1");
  uint16_t ord_port=as_int(kv,"ord_port",9001);
  int DEPTH=as_int(kv,"levels",8);
  int BAND=as_int(kv,"risk_band_ticks",20);
  long long CAP=as_int(kv,"risk_notional_cap",1'000'000'000);
  int MAX_RATE=as_int(kv,"risk_max_rate",2000);
  int HB_MS=as_int(kv,"heartbeat_ms",200);
  int RECON_MS=as_int(kv,"reconnect_ms",500);
  int NUM=std::atoi(argv[2]); (void)DEPTH; (void)HB_MS; // used in book/config

  // 1) Market data thread → L2Book
  int mdfd = udp_bind_any(md_port); if(mdfd<0){ perror("udp bind"); return 1; }
  L2Book<32> book; std::atomic<bool> run{true};
  std::thread mdthr([&]{
    MdTop m{};
    while(run.load(std::memory_order_relaxed)){
      ssize_t n=recv(mdfd,&m,sizeof(m),0);
      if(n<0){ if(errno==EINTR) continue; perror("recv md"); break; }
      if(n==(ssize_t)sizeof(m)) apply_top(book,m);
    }
  });

  // 2) TCP connect with retry
  int s=-1; while(s<0){ s=tcp_connect(ord_ip.c_str(),ord_port);
    if(s<0){ LOGF("connect failed, retrying..."); std::this_thread::sleep_for(std::chrono::milliseconds(RECON_MS)); }
  }

  // 3) Strategy→risk→order with framed TCP and latency logging
  std::ofstream bin("../latency.bin", std::ios::binary);
  std::vector<int64_t> rtt; rtt.reserve(NUM);
  long long notion=0; int rate=0; long long last_sec=0;

  for(int i=0;i<NUM;i++){
    int32_t bp=book.bid_px[0], bs=book.bid_sz[0], ap=book.ask_px[0], as_=book.ask_sz[0];
    if(bp==0||ap==0){ std::this_thread::sleep_for(std::chrono::milliseconds(1)); --i; continue; }
    double imb=(double)(bs-as_)/(double)(bs+as_+1);
    uint8_t side=(imb>0.2)?0:((imb<-0.2)?1:2);
    if(side==2){ std::this_thread::sleep_for(std::chrono::microseconds(200)); --i; continue; }
    int32_t px=(side==0)?bp:ap; int32_t qty=1; int mid=(bp+ap)/2;
    if(px<mid-BAND||px>mid+BAND){ --i; continue; }
    int64_t now=utime::now_ns(); int64_t sec=now/1'000'000'000LL;
    if(sec!=last_sec){ last_sec=sec; rate=0; }
    if(rate++>MAX_RATE){ std::this_thread::sleep_for(std::chrono::milliseconds(1)); --i; continue; }
    if(notion + (int64_t)px*qty > CAP){ --i; continue; }

    Order ord{}; ord.id=(uint32_t)i; ord.side=side; ord.px=px; ord.qty=qty; ord.client_send_ns=now;
    if(!send_frame(s,1,&ord,sizeof(ord))){
      LOGF("send failed; reconnect");
      ::close(s); s=-1; while(s<0){ s=tcp_connect(ord_ip.c_str(),ord_port); std::this_thread::sleep_for(std::chrono::milliseconds(RECON_MS)); }
      --i; continue;
    }
    // Wait for Ack or heartbeat
    while(true){
      uint8_t type; Ack ack{}; uint32_t len=0;
      if(!recv_frame(s,type,&ack,sizeof(ack),len)){
        LOGF("recv failed; reconnect");
        ::close(s); s=-1; while(s<0){ s=tcp_connect(ord_ip.c_str(),ord_port); std::this_thread::sleep_for(std::chrono::milliseconds(RECON_MS)); }
        --i; break;
      }
      if(type==3) continue; // heartbeat
      if(type==2 && len==sizeof(ack) && ack.id==ord.id){
        int64_t d=utime::now_ns()-ord.client_send_ns; rtt.push_back(d); bin.write((char*)&d,sizeof(d)); notion += (int64_t)px*qty; break;
      }
    }
  }

  run=false; shutdown(s,SHUT_RDWR); ::close(s); ::close(mdfd); mdthr.join(); bin.close();

  if(!rtt.empty()){
    std::sort(rtt.begin(), rtt.end());
    auto pick=[&](double p){ size_t i=(size_t)(p*(rtt.size()-1)); return rtt[i]; };
    long long max=rtt.back(); double mean=0; for(auto x:rtt) mean+=x; mean/=rtt.size();
    std::printf("count=%zu mean=%.1f ns p50=%lld ns p99=%lld ns p99.9=%lld ns max=%lld ns\n",
      rtt.size(), mean, pick(0.5), pick(0.99), pick(0.999), max);
  }
  return 0;
}