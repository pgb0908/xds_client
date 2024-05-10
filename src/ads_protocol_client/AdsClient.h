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
    AdsClient(){}
    AdsClient(const std::shared_ptr<Channel>& channel)
        : stub_(AggregatedDiscoveryService::NewStub(channel)) {

    }


    void setChannel(const std::shared_ptr<Channel>& channel){
        stub_ = AggregatedDiscoveryService::NewStub(channel);

    }

    void continueChat(){
        std::cout << "================ request =======================" << std::endl;
        // do
        discoveryRequest_.set_version_info(version_info_);
        discoveryRequest_.set_response_nonce(nonce_);
        //discoveryRequest.mutable_type_url()->append("type.googleapis.com/envoy.config.route.v3.RouteConfiguration");
        //discoveryRequest.mutable_type_url()->append("type.googleapis.com/envoy.api.v3.Cluster");
        //discoveryRequest.add_resource_names("dddd");

        std::cout << "version-info : " << discoveryRequest_.version_info() << std::endl;
        std::cout << "response-nonce : " << discoveryRequest_.response_nonce() << std::endl;
        std::cout << "type-url : " << discoveryRequest_.type_url() << std::endl;
        std::cout << "Resources-names : "  << std::endl;
        for (const auto& resource : discoveryRequest_.resource_names()) {
            std::cout << "  Resource Type URL: " << resource << std::endl;
        }

        stream_->Write(discoveryRequest_);
        //stream_->WritesDone();

        isOk = true;
    }

    bool AggregatedDiscoveryService(){
        ClientContext context;
        stream_ = stub_->StreamAggregatedResources(&context);

        std::cout << "================ request =======================" << std::endl;
        // do
        discoveryRequest_.set_version_info("");
        auto node_data = discoveryRequest_.mutable_node();
        node_data->mutable_cluster()->append("test-cluster");
        node_data->mutable_id()->append("test-id");
        discoveryRequest_.set_type_url("type.googleapis.com/envoy.config.cluster.v3.Cluster");
        //discoveryRequest.mutable_type_url()->append("type.googleapis.com/envoy.config.route.v3.RouteConfiguration");
        //discoveryRequest.mutable_type_url()->append("type.googleapis.com/envoy.api.v3.Cluster");
        //discoveryRequest_.add_resource_names("*");

        std::cout << "version-info : " << discoveryRequest_.version_info() << std::endl;
        std::cout << "response-nonce : " << discoveryRequest_.response_nonce() << std::endl;
        std::cout << "type-url : " << discoveryRequest_.type_url() << std::endl;
        std::cout << "Resources-names : "  << std::endl;
        for (const auto& resource : discoveryRequest_.resource_names()) {
            std::cout << "  Resource Type URL: " << resource << std::endl;
        }

        stream_->Write(discoveryRequest_);
        //stream_->WritesDone();
        first_ = false;


        DiscoveryResponse discoveryResponse;
        while(stream_->Read(&discoveryResponse)){
            version_info_ = discoveryResponse.version_info();
            nonce_ = discoveryResponse.nonce();
            std::cout << "================ response =======================" << std::endl;
            std::cout << "version-info : " << discoveryResponse.version_info() << std::endl;
            std::cout << "nonce : " << discoveryResponse.nonce() << std::endl;
            std::cout << "type-url : " << discoveryResponse.type_url() << std::endl;

            std::cout << "Resources : "  << std::endl;
            for (const auto& resource : discoveryResponse.resources()) {
                std::cout << "  Resource Type URL: " << resource.type_url() << std::endl;
                std::cout << "  Resource Type value: " << resource.value().c_str() << std::endl;
            }

            continueChat();
        }


        Status status = stream_->Finish();
        if(!status.ok()){
            std::cout << "rpc failed" << std::endl;
        }

        return isOk;
    }
private:
    std::unique_ptr<AggregatedDiscoveryService::Stub> stub_;
    bool first_{true};
    bool isOk{false};
    std::string version_info_;
    std::string nonce_;
    DiscoveryRequest discoveryRequest_;
    //ClientContext context_;
    std::shared_ptr<grpc::ClientReaderWriter<DiscoveryRequest, DiscoveryResponse>> stream_;
};


#endif //XDS_CLIENT_ADSCLIENT_H
