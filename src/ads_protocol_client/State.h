//
// Created by bont on 24. 5. 9.
//

#ifndef XDS_CLIENT_STATE_H
#define XDS_CLIENT_STATE_H


#include <grpc++/grpc++.h>
#include <thread>
#include "memory"
#include "../../schema/proto-src/xds.grpc.pb.h"

#include <string>
#include <map>
#include <set>

class State{
public:
    // <type_url, name> 이미 알려진 리소스 저장
    std::map<std::string, std::set<std::string>> known_resources_;
    // pending, xds push 중인 모든 리소스들 저장
    std::map<std::string, std::string> pending_;
    std::string demand_;  // 수신
    std::string demand_tx_; // 송신

    void add_resource(std::string type_url, std::string name){
        known_resources_[type_url].insert(name);
    }

    void notify_on_demand(){

    }
};

#endif //XDS_CLIENT_STATE_H
