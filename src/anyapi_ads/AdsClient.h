//
// Created by bont on 24. 5. 8.
//

#ifndef XDS_CLIENT_ADSCLIENT_H
#define XDS_CLIENT_ADSCLIENT_H

#include <grpc++/grpc++.h>
#include <thread>
#include <queue>
#include "memory"
#include "envoy/service/discovery/v3/ads.grpc.pb.h"
#include "envoy/config/cluster/v3/cluster.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ServerContext;

using envoy::service::discovery::v3::AggregatedDiscoveryService;
using envoy::service::discovery::v3::DiscoveryRequest;
using envoy::service::discovery::v3::DiscoveryResponse;
using envoy::config::core::v3::Node;


class AdsClient {
public:
    // Constructor
    AdsClient(){
    }



private:

};


#endif //XDS_CLIENT_ADSCLIENT_H
