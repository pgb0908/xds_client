//
// Created by bont on 24. 5. 9.
//

#ifndef XDS_CLIENT_CONFIG_H
#define XDS_CLIENT_CONFIG_H
#include <string>
#include <map>
#include <grpc++/grpc++.h>
#include <thread>
#include "memory"

using envoy::service::discovery::v3::AggregatedDiscoveryService;
using envoy::service::discovery::v3::DiscoveryRequest;
using envoy::service::discovery::v3::DiscoveryResponse;
using envoy::config::core::v3::Node;

class RejectedConfig{
public:
    RejectedConfig(std::string name, std::string error):
        name_(name), error_(error){

    }
    std::string name_;
    std::string error_;
};

class Handler{
public:
    void handle(){

    }
};


class Config{
public:
    Config(std::string address):
    address_(std::move(address)){

    }

    google::protobuf::Struct build_struct(const std::vector<std::pair<std::string,std::string>>& keyValue){
        google::protobuf::Struct struct_data;

        for(const auto& item : keyValue){
            google::protobuf::Value value;
            value.set_string_value(item.second);
            struct_data.mutable_fields()->insert({item.first, value});
        }

        return struct_data;
    }

    Node node(){
        Node node;
        node.mutable_id()->append("test-id");
        node.mutable_cluster()->append("test-cluster");

        // 추후 여기에 pod, ip, node_name 등이 들어감
        auto meta_data = build_struct({{"test", "data1"}});
        node.mutable_metadata()->CopyFrom(meta_data);

        return node;
    }

    DiscoveryRequest construct_initial_request(const std::string& request_type, bool no_on_demand){
        DiscoveryRequest request;

        request.mutable_node()->CopyFrom(node());
        request.mutable_type_url()->append(request_type);

        return request;
    }

    std::unique_ptr<AdsClient> build(){
        return std::make_unique<AdsClient>();
    }

private:
    std::string address_;
    //auth
    std::map<std::string, std::string> proxy_metadata_;
    std::map<std::string, Handler> handler_;
    std::vector<DiscoveryRequest> initial_requests_;
    bool on_demand_;


};

#endif //XDS_CLIENT_CONFIG_H
