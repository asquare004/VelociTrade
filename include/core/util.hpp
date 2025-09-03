#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>

inline std::unordered_map<std::string,std::string> load_yaml_kv(const std::string& path){
  std::unordered_map<std::string,std::string> kv; std::ifstream f(path);
  std::string line;
  while(std::getline(f,line)){
    if(line.empty() || line[0]=='#') continue;
    auto pos=line.find(':'); if(pos==std::string::npos) continue;
    std::string k=line.substr(0,pos), v=line.substr(pos+1);
    auto trim=[](std::string s){
      size_t a=s.find_first_not_of(" \t");
      size_t b=s.find_last_not_of(" \t\r");
      return (a==std::string::npos)?std::string():s.substr(a,b-a+1);
    };
    kv[trim(k)] = trim(v);
  }
  return kv;
}
inline int as_int(const std::unordered_map<std::string,std::string>& kv, const char* k, int d){
  auto it=kv.find(k); return (it==kv.end())?d:std::stoi(it->second);
}
inline std::string as_str(const std::unordered_map<std::string,std::string>& kv, const char* k, const char* d){
  auto it=kv.find(k); return (it==kv.end())?std::string(d):it->second;
}
