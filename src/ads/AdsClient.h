//
// Created by bont on 24. 5. 8.
//

#ifndef XDS_CLIENT_ADSCLIENT_H
#define XDS_CLIENT_ADSCLIENT_H

#include <grpc++/grpc++.h>
#include <thread>
#include "memory"
#include "../../schema/proto-src/xds.grpc.pb.h"

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
        : stub_(AggregatedDiscoveryService::NewStub(channel)) {}

    void AggregatedDiscoveryService(){
        ClientContext context;

        std::shared_ptr<grpc::ClientReaderWriter<DiscoveryRequest, DiscoveryResponse>>
                    stream(stub_->StreamAggregatedResources(&context));


        std::thread writer([stream](){
            // do
            DiscoveryRequest discoveryRequest;
            discoveryRequest.mutable_version_info()->append("");
            auto node_data = discoveryRequest.mutable_node();
            node_data->mutable_cluster()->append("test-cluster");
            node_data->mutable_id()->append("test-id");
            discoveryRequest.mutable_type_url()->append("type.googleapis.com/envoy.config.cluster.v3.Cluster");
            //discoveryRequest.mutable_type_url()->append("type.googleapis.com/envoy.config.route.v3.RouteConfiguration");
            //discoveryRequest.mutable_type_url()->append("type.googleapis.com/envoy.api.v3.Cluster");
            stream->Write(discoveryRequest);

            stream->WritesDone();
        });

        DiscoveryResponse discoveryResponse;
        while(stream->Read(&discoveryResponse)){
            std::cout << "version-info : " << discoveryResponse.version_info() << std::endl;
            std::cout << "nonce : " << discoveryResponse.nonce() << std::endl;
            std::cout << "type-url : " << discoveryResponse.type_url() << std::endl;

            std::cout << "Resources : "  << std::endl;
            for (const auto& resource : discoveryResponse.resources()) {
                std::cout << "  Resource Type URL: " << resource.type_url() << std::endl;
                std::cout << "  Resource Type value: " << resource.value().c_str() << std::endl;
            }
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
