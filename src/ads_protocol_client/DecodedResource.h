//
// Created by bont on 24. 5. 16.
//

#ifndef XDS_CLIENT_DECODEDRESOURCE_H
#define XDS_CLIENT_DECODEDRESOURCE_H

#include <memory>
//#include "envoy/service/discovery/v3/discovery.pb.h"
#include "../../schema/proto-src/xds.grpc.pb.h"
#include "xds/core/v3/collection_entry.pb.h"

#include "OpaqueResourceDecoder.h"

namespace {

    std::vector<std::string>
    repeatedPtrFieldToVector(const google::protobuf::RepeatedField<std::string>& xs) {
        std::vector<std::string> ys;
        std::copy(xs.begin(), xs.end(), std::back_inserter(ys));
        return ys;
    }
}


class DecodedResource;
using DecodedResourcePtr = std::unique_ptr<DecodedResource>;


class DecodedResource {
public:
    static DecodedResourcePtr fromResource(OpaqueResourceDecoder& resource_decoder,
                                               const google::protobuf::Any& resource,
                                               const std::string& version) {
        if (resource.Is<envoy::service::discovery::v3::Resource>()) {
            envoy::service::discovery::v3::Resource r;
            unpackToOrThrow(resource, r);

            r.set_version(version);

            return std::make_unique<DecodedResource>(resource_decoder, r);
        }

        return std::unique_ptr<DecodedResource>(new DecodedResource(
                resource_decoder, "",
                google::protobuf::RepeatedField<std::string>(),
                        resource,
                        true,
                        version));
    }

    static DecodedResourcePtr
    fromResource(OpaqueResourceDecoder& resource_decoder,
                 const envoy::service::discovery::v3::Resource& resource) {
        auto temp = std::make_unique<DecodedResource>(resource_decoder, resource);
        return temp;
    }

    DecodedResource(OpaqueResourceDecoder& resource_decoder, const envoy::service::discovery::v3::Resource& resource)
            : DecodedResource( resource_decoder,
                               resource.name(),
                               google::protobuf::RepeatedField<std::string>() ,
                               resource.resource(),
                               resource.has_resource(),
                               resource.version()) {}
    DecodedResource(OpaqueResourceDecoder& resource_decoder, const xds::core::v3::CollectionEntry::InlineEntry& inline_entry)
            : DecodedResource(resource_decoder, inline_entry.name(),
                                  google::protobuf::RepeatedField<std::string>(), inline_entry.resource(),
                                  true, inline_entry.version()) {}
    DecodedResource(std::unique_ptr<google::protobuf::Message> resource, const std::string& name,
                        const std::vector<std::string>& aliases, const std::string& version)
            : resource_(std::move(resource)), has_resource_(true), name_(name), aliases_(aliases),
              version_(version){}



    // Config::DecodedResource
    const std::string& name() const { return name_; }
    const std::vector<std::string>& aliases() const { return aliases_; }
    const std::string& version() const { return version_; };
    const google::protobuf::Message& resource() const { return *resource_; };
    bool hasResource() const { return has_resource_; }
    //std::chrono::milliseconds ttl() const  { return ttl_; }
/*    const envoy::config::core::v3::Metadata metadata() const  {
        //return metadata_.has_value() ? metadata_.value() : nullptr;

        return metadata_;
    }*/



private:
    DecodedResource(OpaqueResourceDecoder& resource_decoder, const std::string& name,
                    const google::protobuf::RepeatedField<std::string>& aliases,
                    const google::protobuf::Any& resource, bool has_resource,
                    const std::string& version)
            : resource_(resource_decoder.decodeResource(resource)), has_resource_(has_resource),
              name_(!name.empty() ? name : resource_decoder.resourceName(*resource_)),
              aliases_(repeatedPtrFieldToVector(aliases)), version_(version){}



    const std::unique_ptr<google::protobuf::Message> resource_;
    const bool has_resource_;
    const std::string name_;
    const std::vector<std::string> aliases_;
    const std::string version_;
    // Per resource TTL.
    //const std::chrono::milliseconds ttl_;

    // This is the metadata info under the Resource wrapper.
    // It is intended to be consumed in the xds_config_tracker extension.
    //const envoy::config::core::v3::Metadata metadata_;
};

/*struct DecodedResourcesWrapper {
    DecodedResourcesWrapper() = default;
    DecodedResourcesWrapper(OpaqueResourceDecoder& resource_decoder,
                            const Protobuf::RepeatedPtrField<ProtobufWkt::Any>& resources,
                            const std::string& version) {
        for (const auto& resource : resources) {
            pushBack((DecodedResourceImpl::fromResource(resource_decoder, resource, version)));
        }
    }

    void pushBack(Config::DecodedResourcePtr&& resource) {
        owned_resources_.push_back(std::move(resource));
        refvec_.emplace_back(*owned_resources_.back());
    }

    std::vector<Config::DecodedResourcePtr> owned_resources_;
    std::vector<Config::DecodedResourceRef> refvec_;
};*/


#endif //XDS_CLIENT_DECODEDRESOURCE_H
