//
// Created by bont on 24. 5. 10.
//

#ifndef XDS_CLIENT_RESOURCEDECODER_H
#define XDS_CLIENT_RESOURCEDECODER_H

#include <string>
#include "../../schema/proto-src/xds.grpc.pb.h"

template <typename Current>
class OpaqueResourceDecoderImpl  {
public:
    OpaqueResourceDecoderImpl(ProtobufMessage::ValidationVisitor& validation_visitor,
                              std::string name_field)
            : validation_visitor_(validation_visitor), name_field_(name_field) {}

    // Config::OpaqueResourceDecoder
    std::unique_ptr<google::protobuf::Message>
     decodeResource(const google::protobuf::Any& resource)  {
        auto typed_message = std::make_unique<Current>();
        // If the Any is a synthetic empty message (e.g. because the resource field was not set in
        // Resource, this might be empty, so we shouldn't decode.
        if (!resource.type_url().empty()) {
            MessageUtil::anyConvertAndValidate<Current>(resource, *typed_message, validation_visitor_);
        }
        return typed_message;
    }

    std::string resourceName(const Protobuf::Message& resource) override {
        return MessageUtil::getStringField(resource, name_field_);
    }

private:
    ProtobufMessage::ValidationVisitor& validation_visitor_;
    const std::string name_field_;
};

#endif //XDS_CLIENT_RESOURCEDECODER_H
