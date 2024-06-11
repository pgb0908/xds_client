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
#include "Protobuf.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ServerContext;

using envoy::service::discovery::v3::AggregatedDiscoveryService;
using envoy::service::discovery::v3::DiscoveryRequest;
using envoy::service::discovery::v3::DiscoveryResponse;
using envoy::config::core::v3::Node;


namespace anyapi{

    static void unpackToOrThrow(const google::protobuf::Any& any_message, google::protobuf::Message& message) {
        if (!message.ParseFromString(any_message.value())) {
            //throwEnvoyExceptionOrPanic(fmt::format("Unable to unpack as {}: {}", message.GetTypeName(), any_message.type_url()));
            std::cout << "failed to parsed" << std::endl;
        }
    }

    /**
     * Convert from google.protobuf.Any to a typed message.
     * @param message source google.protobuf.Any message.
     *
     * @return MessageType the typed message inside the Any.
     */
    template <class MessageType>
    static inline void anyConvert(const google::protobuf::Any& message, MessageType& typed_message) {
        unpackToOrThrow(message, typed_message);
    };

    /**
     * Convert and validate from google.protobuf.Any to a typed message.
     * @param message source google.protobuf.Any message.
     *
     * @return MessageType the typed message inside the Any.
     * @throw EnvoyException if the message does not satisfy its type constraints.
     */
    template <class MessageType>
    static inline void anyConvertAndValidate(const google::protobuf::Any& message,
                                             MessageType& typed_message) {
        anyConvert<MessageType>(message, typed_message);
        //validate(typed_message, validation_visitor);
    };


    /**
     * Obtain a string field from a protobuf message dynamically.
     *
     * @param message message to extract from.
     * @param field_name field name.
     *
     * @return std::string with field value.
     */
    static inline std::string getStringField(const google::protobuf::Message& message,
                                             const std::string& field_name) {
        ::google::protobuf::Message* reflectable_message = createReflectableMessage(message);
        const google::protobuf::Descriptor* descriptor = reflectable_message->GetDescriptor();
        const google::protobuf::FieldDescriptor* name_field = descriptor->FindFieldByName(field_name);
        const google::protobuf::Reflection* reflection = reflectable_message->GetReflection();
        return reflection->GetString(*reflectable_message, name_field);
    }

    class ResourceDecoder {
    public:
        virtual ~ResourceDecoder() = default;

        /**
         * @param resource some opaque resource (ProtobufWkt::Any).
         * @return ProtobufTypes::MessagePtr decoded protobuf message in the opaque resource, e.g. the
         *         RouteConfiguration for an Any containing envoy.config.route.v3.RouteConfiguration.
         */
        virtual std::unique_ptr<google::protobuf::Message> decodeResource(const google::protobuf::Any& resource) = 0;

        /**
         * @param resource some opaque resource (Protobuf::Message).
         * @return std::String the resource name in a Protobuf::Message returned by decodeResource(), e.g.
         *         the route config name for a envoy.config.route.v3.RouteConfiguration message.
         */
        virtual std::string resourceName(const google::protobuf::Message& resource) = 0;
    };

    template <typename Current>
    class ResourceDecoderImpl : public ResourceDecoder {
    public:
        explicit ResourceDecoderImpl(std::string name_field)
                :  name_field_(std::move(name_field)) {}

        // Config::OpaqueResourceDecoder
        std::unique_ptr<google::protobuf::Message> decodeResource(const google::protobuf::Any& resource)  override {
            auto typed_message = std::make_unique<Current>();
            // If the Any is a synthetic empty message (e.g. because the resource field was not set in
            // Resource, this might be empty, so we shouldn't decode.
            if (!resource.type_url().empty()) {
                anyConvertAndValidate<Current>(resource, *typed_message);
            }
            return typed_message;
        }

        std::string resourceName(const google::protobuf::Message& resource) override {
            return getStringField(resource, name_field_);
        }


    private:
        // ProtobufMessage::ValidationVisitor& validation_visitor_;
        const std::string name_field_;
    };

    struct ResourceKey{
        std::string name_;
        std::string type_url_;
    };

    struct DecodedResource {
        bool has_resource_{false};
        std::string name_;
        std::string version_;
        std::unique_ptr<google::protobuf::Message> resource_;
    };

    struct State {
        ResourceKey key_;
        DiscoveryRequest discoveryRequest_;
        bool subscribed_{false};
        std::shared_ptr<ResourceDecoder> resourceDecoderPtr_;
        std::vector<std::string> resources_;
    };

    using DecodedResourcePtr = std::unique_ptr<DecodedResource>;


    class AdsClient {
    public:
        // Constructor
        AdsClient(std::string& endpoint);

        void startSubscribe();
        void detectingQueue();

    private:
        void subscribeCDS();
        void subscribeEDS();

        bool sendDiscoveryRequest(State& requestInfo);

        void receiveResponse();
        void processResponse(const DiscoveryResponse& discoveryResponse);
        void clearNonce();

        bool isHeartbeatResource(const std::string& type_url, const DecodedResource& resource) {
            return !resource.has_resource_&&
                   resource.version_ == states_[type_url].discoveryRequest_.version_info();
        }

        std::map<std::string, State> states_; // type_url, state
        //std::map<std::string, DecodedResource> resource_map;  // type_url, DecodedResource

        std::unique_ptr<AggregatedDiscoveryService::Stub> stub_;
        std::unique_ptr< ::grpc::ClientAsyncReaderWriter<DiscoveryRequest, DiscoveryResponse>> rpc_;
        grpc::CompletionQueue cq_;
        grpc::Status status_;
        ClientContext context_;
        std::queue<State> request_queue_;


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

        void resourceUpdate(const std::vector<DecodedResourcePtr>& vector1, State& state,
                            const std::string &type_url, const std::string &version_info);
    };

}




#endif //XDS_CLIENT_ADSCLIENT_H
