//
// Created by bont on 24. 5. 8.
//

#ifndef XDS_CLIENT_ADSCLIENT_H
#define XDS_CLIENT_ADSCLIENT_H

#include "proto-src/xds.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <thread>
#include "memory"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ServerContext;

using envoy::service::discovery::v3::AggregatedDiscoveryService;
using envoy::service::discovery::v3::DiscoveryRequest;
using envoy::service::discovery::v3::DiscoveryResponse;

class AdsClient {
public:
    // Constructor
    AdsClient(const std::shared_ptr<Channel>& channel)
        : stub_(AggregatedDiscoveryService::NewStub(channel)) {

    }

    void AggregatedDiscoveryService(){
        ClientContext context;

        std::shared_ptr<grpc::ClientReaderWriter<DiscoveryRequest, DiscoveryResponse>>
                    stream(stub_->StreamAggregatedResources(&context));

        std::thread writer([stream](){
            // do
            DiscoveryRequest discoveryRequest;
            stream->Write(discoveryRequest);

            stream->WritesDone();
        });

        DiscoveryResponse discoveryResponse;
        while(stream->Read(&discoveryResponse)){

        }

        writer.join();
        Status status = stream->Finish();
        if(!status.ok()){
            std::cout << "rpc failed" << std::endl;
        }
    }
private:
    std::unique_ptr<AggregatedDiscoveryService::Stub> stub_;
};


#endif //XDS_CLIENT_ADSCLIENT_H
