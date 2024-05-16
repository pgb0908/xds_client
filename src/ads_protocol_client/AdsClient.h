//
// Created by bont on 24. 5. 8.
//

#ifndef XDS_CLIENT_ADSCLIENT_H
#define XDS_CLIENT_ADSCLIENT_H

#include <grpc++/grpc++.h>
#include <thread>
#include <queue>
#include "memory"
#include "../../schema/proto-src/xds.grpc.pb.h"
#include "envoy/config/cluster/v3/cluster.pb.h"
#include "ResourceName.h"
#include "Watcher.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ServerContext;

using envoy::service::discovery::v3::AggregatedDiscoveryService;
using envoy::service::discovery::v3::DiscoveryRequest;
using envoy::service::discovery::v3::DiscoveryResponse;
using envoy::service::discovery::v3::Node;


class AdsClient {
public:
    // Constructor
    AdsClient(){
        subscriptions_.push_back("type.googleapis.com/envoy.config.cluster.v3.Cluster");
    }
    AdsClient(const std::shared_ptr<Channel>& channel)
        : stub_(AggregatedDiscoveryService::NewStub(channel)) {
        subscriptions_.push_back("type.googleapis.com/envoy.config.cluster.v3.Cluster");
    }

    void setChannel(const std::shared_ptr<Channel>& channel){
        stub_ = AggregatedDiscoveryService::NewStub(channel);

    }

    void onStreamEstablished();

    void shutdown() {
        endStream();
        shutdown_ = true;
    }

    void onDiscoveryResponse(const envoy::service::discovery::v3::DiscoveryResponse& message);

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


        DiscoveryResponse discoveryResponse;
        while(stream_->Read(&discoveryResponse)){
            version_info_ = discoveryResponse.version_info();
            nonce_ = discoveryResponse.nonce();
            std::cout << "================ response =======================" << std::endl;
            std::cout << discoveryResponse.ShortDebugString() << std::endl;
            std::cout << "version-info : " << discoveryResponse.version_info() << std::endl;
            std::cout << "nonce : " << discoveryResponse.nonce() << std::endl;
            std::cout << "type-url : " << discoveryResponse.type_url() << std::endl;


            std::cout << "Resources : "  << std::endl;
            for (const auto& resource : discoveryResponse.resources()) {
                envoy::config::cluster::v3::Cluster r;
                unpackToOrThrow(resource, r);
                std::cout << "  r : " << r.DebugString() << std::endl;


            }
            continueChat();
        }

        return isOk;
    }

    void unpackToOrThrow(const google::protobuf::Any& any_message, google::protobuf::Message& message) {
        if (!message.ParseFromString(any_message.value())) {
            //throwEnvoyExceptionOrPanic(fmt::format("Unable to unpack as {}: {}", message.GetTypeName(), any_message.type_url()));
            std::cout << "failed to parsed" << std::endl;
        }
    }
    void queueDiscoveryRequest(const std::string& queue_item);

    // Per muxed API state.
    struct ApiState {
        ApiState(std::function<void(const std::vector<std::string>&)> callback){}

        bool paused() const { return pauses_ > 0; }

        // Watches on the returned resources for the API;
        std::list<Watcher*> watches_;

        // Current DiscoveryRequest for API.
        envoy::service::discovery::v3::DiscoveryRequest request_;
        // Count of unresumed pause() invocations.
        uint32_t pauses_{};
        // Was a DiscoveryRequest elided during a pause?
        bool pending_{};
        // Has this API been tracked in subscriptions_?
        bool subscribed_{};
        // This resource type must have a Node sent at next request.
        bool must_send_node_{};
        // The identifier for the server that sent the most recent response, or
        // empty if there is none.
        std::string control_plane_identifier_{};
        // If true, xDS resources were previously fetched from an xDS source or an xDS delegate.
        bool previously_fetched_data_{false};
    };

    // Helper function for looking up and potentially allocating a new ApiState.
    ApiState& apiStateFor(std::string type_url);

private:

    void drainRequests();
    void sendDiscoveryRequest(std::string type_url);
    void endStream();

    void processDiscoveryResources(const std::vector<DecodedResourcePtr>& resources,
                                   ApiState& api_state, const std::string& type_url,
                                   const std::string& version_info, bool call_delegate);

    std::unique_ptr<AggregatedDiscoveryService::Stub> stub_;
    ClientContext context_;
    bool first_{true};
    bool isOk{false};
    std::string version_info_;
    std::string nonce_;
    DiscoveryRequest discoveryRequest_;
    //ClientContext context_;
    std::shared_ptr<grpc::ClientReaderWriter<DiscoveryRequest, DiscoveryResponse>> stream_;

    void clearNonce();

    // Envoy's dependency ordering.
    std::list<std::string> subscriptions_;


    std::unordered_map<std::string, std::unique_ptr<ApiState>> api_state_;

    void expiryCallback(std::string, const std::vector<std::string>& expired);

    bool first_stream_request_{true};

    // A queue to store requests while rate limited. Note that when requests
    // cannot be sent due to the gRPC stream being down, this queue does not
    // store them; rather, they are simply dropped. This string is a type
    // URL.
    std::unique_ptr<std::queue<std::string>> request_queue_;

    bool started_{false};
    // True iff Envoy is shutting down; no messages should be sent on the `grpc_stream_` when this is
    // true because it may contain dangling pointers.
    std::atomic<bool> shutdown_{false};

    Node node(){
        Node node;
        node.mutable_id()->append("test-id");
        node.mutable_cluster()->append("test-cluster");

        // 추후 여기에 pod, ip, node_name 등이 들어감
        auto meta_data = build_struct({{"test", "data1"}});
        node.mutable_metadata()->CopyFrom(meta_data);

        return node;
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
};


#endif //XDS_CLIENT_ADSCLIENT_H
