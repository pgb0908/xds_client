//
// Created by bont on 24. 5. 8.
//

#ifndef XDS_CLIENT_ADSCLIENT_H
#define XDS_CLIENT_ADSCLIENT_H

#include <grpc++/grpc++.h>
#include <thread>
#include <queue>
#include <grpcpp/alarm.h>
#include "memory"
#include "ResourceName.h"
#include "DecodedResource.h"
#include "envoy/service/discovery/v3/ads.grpc.pb.h"
#include "envoy/config/cluster/v3/cluster.pb.h"
#include "envoy/config/listener/v3/listener.pb.h"
#include "envoy/config/route//v3/route.pb.h"
#include "envoy/config/endpoint/v3/endpoint.pb.h"
#include "Cleanup.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ServerContext;

using envoy::service::discovery::v3::AggregatedDiscoveryService;
using envoy::service::discovery::v3::DiscoveryRequest;
using envoy::service::discovery::v3::DiscoveryResponse;
using envoy::config::core::v3::Node;

using ScopedResume = std::unique_ptr<Cleanup>;

class Watcher;

namespace {
/*    // Returns true if `resource_name` contains the wildcard XdsTp resource, for example:
    // xdstp://test/envoy.config.cluster.v3.Cluster/foo-cluster/*
    // xdstp://test/envoy.config.cluster.v3.Cluster/foo-cluster/*?node.name=my_node
    bool isXdsTpWildcard(const std::string& resource_name) {
        return XdsResourceIdentifier::hasXdsTpScheme(resource_name) &&
               (absl::EndsWith(resource_name, "/*") || absl::StrContains(resource_name, "/*?"));
    }

    // Converts the XdsTp resource name to its wildcard equivalent.
    // Must only be called on XdsTp resource names.
    std::string convertToWildcard(const std::string& resource_name) {
        //ASSERT(XdsResourceIdentifier::hasXdsTpScheme(resource_name));
        auto resource_or_error = XdsResourceIdentifier::decodeUrn(resource_name);
        THROW_IF_STATUS_NOT_OK(resource_or_error, throw);
        xds::core::v3::ResourceName xdstp_resource = resource_or_error.value();
        const auto pos = xdstp_resource.id().find_last_of('/');
        xdstp_resource.set_id(
                pos == std::string::npos ? "*" : absl::StrCat(xdstp_resource.id().substr(0, pos), "/*"));
        XdsResourceIdentifier::EncodeOptions options;
        options.sort_context_params_ = true;
        return XdsResourceIdentifier::encodeUrn(xdstp_resource, options);
    }*/
}

class AdsClient {
public:
    // Constructor
    AdsClient(){
    }
    AdsClient(const std::shared_ptr<Channel>& channel)
        : stub_(AggregatedDiscoveryService::NewStub(channel)) {
    }

    void setChannel(const std::shared_ptr<Channel>& channel){
        stub_ = AggregatedDiscoveryService::NewStub(channel);

    }

    void onStreamEstablished();

    std::unique_ptr<Watcher> addWatch(const std::string& type_url,
                                      const std::set<std::string>& resources,
                                      const OpaqueResourceDecoderSharedPtr& resource_decoder);

    void shutdown() {
        endStream();
        shutdown_ = true;
    }

    void onDiscoveryResponse(const envoy::service::discovery::v3::DiscoveryResponse& message);


    void unpackToOrThrow(const google::protobuf::Any& any_message, google::protobuf::Message& message) {
        if (!message.ParseFromString(any_message.value())) {
            //throwEnvoyExceptionOrPanic(fmt::format("Unable to unpack as {}: {}", message.GetTypeName(), any_message.type_url()));
            std::cout << "failed to parsed" << std::endl;
        }
    }
    void queueDiscoveryRequest(const std::string& queue_item);


    ScopedResume pause(const std::string& type_url);
    ScopedResume pause(const std::vector<std::string> type_urls);

    // Per muxed API state.
    struct ApiState {
        ApiState(const std::function<void(const std::vector<std::string>&)>& callback){}

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

    static std::string truncateGrpcStatusMessage(std::string error_message);

    bool isHeartbeatResource(const std::string& type_url, const DecodedResource& resource) {
        return !resource.hasResource() &&
               resource.version() == apiStateFor(type_url).request_.version_info();
    }

    std::unique_ptr<AggregatedDiscoveryService::Stub> stub_;

    bool first_{true};
    bool isOk{false};
    std::string version_info_;
    std::string nonce_;
    DiscoveryRequest discoveryRequest_;

    //std::shared_ptr<grpc::ClientReaderWriter<DiscoveryRequest, DiscoveryResponse>> stream_;
    std::unique_ptr< ::grpc::ClientAsyncReaderWriter<DiscoveryRequest, DiscoveryResponse>> rpc_;

    grpc::CompletionQueue cq_;
    grpc::Status status_;
    ClientContext context_;
    grpc::Alarm alarm_;

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

    void recieveResponse();
};


#endif //XDS_CLIENT_ADSCLIENT_H
