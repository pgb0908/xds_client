//
// Created by bont on 24. 5. 9.
//

#ifndef XDS_CLIENT_ASYNCADSCLIENT_H
#define XDS_CLIENT_ASYNCADSCLIENT_H

#include <grpc++/grpc++.h>
#include <grpc/grpc.h>
#include <grpcpp/alarm.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <thread>
#include "memory"
#include "envoy/service/discovery/v3/ads.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ServerContext;


using envoy::service::discovery::v3::AggregatedDiscoveryService;
using envoy::service::discovery::v3::DiscoveryRequest;
using envoy::service::discovery::v3::DiscoveryResponse;


class AsyncAdsClient {
public:
    AsyncAdsClient(const std::shared_ptr<Channel>& channel)
            : stub_(AggregatedDiscoveryService::NewStub(channel)) {}
    void do_something();
private:
    std::unique_ptr<AggregatedDiscoveryService::Stub> stub_;
};


#endif //XDS_CLIENT_ASYNCADSCLIENT_H
