//
// Created by bont on 24. 5. 16.
//

#ifndef XDS_CLIENT_OPAQUERESOURCEDECODER_H
#define XDS_CLIENT_OPAQUERESOURCEDECODER_H

#include <utility>

#include "envoy/service/discovery/v3/discovery.pb.h"
#include "protofile/text_format_transcoder.h"
#include "Protobuf.h"

namespace {
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
        //return name_field->name();
    }

}

class OpaqueResourceDecoder {
public:
    virtual ~OpaqueResourceDecoder() = default;

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

using OpaqueResourceDecoderSharedPtr = std::shared_ptr<OpaqueResourceDecoder>;


template <typename Current>
class OpaqueResourceDecoderImpl : public OpaqueResourceDecoder {
public:
    OpaqueResourceDecoderImpl(std::string name_field)
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

#endif //XDS_CLIENT_OPAQUERESOURCEDECODER_H
