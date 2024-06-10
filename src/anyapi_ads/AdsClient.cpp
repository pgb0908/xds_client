//
// Created by bont on 24. 5. 8.
//

#include "AdsClient.h"
#include "../ads_protocol_client/Status.h"
#include "sstream"

namespace anyapi {

    std::string truncateGrpcStatusMessage(std::string error_message) {
        // GRPC sends error message via trailers, which by default has a 8KB size limit(see
        // https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md#requests). Truncates the
        // error message if it's too long.
        constexpr uint32_t kProtobufErrMsgLen = 4096;
        std::stringstream ss;
        ss << error_message.substr(0, kProtobufErrMsgLen);
        ss << (error_message.length() > kProtobufErrMsgLen ? "...(truncated)" : "");
        return ss.str();
    }

    AdsClient::AdsClient(std::string& endpoint) {
        stub_ = AggregatedDiscoveryService::NewStub(grpc::CreateChannel(endpoint,
                                                                        grpc::InsecureChannelCredentials()));
        rpc_ = stub_->PrepareAsyncStreamAggregatedResources(&context_, &cq_);

        if (rpc_) {
            bool ok;
            void* on_tag;
            clearNonce();
            rpc_->StartCall(&on_tag);
            cq_.Next(&on_tag, &ok);

        } else {
            std::cout << "can't make stream. something wrong!" << std::endl;
        }

    }


    void AdsClient::subscribeCDS() {
        State state;
        state.key_.name_ = "cds";
        state.key_.type_url_ = "type.googleapis.com/envoy.config.cluster.v3.Cluster";
        state.resourceDecoderPtr_ = std::make_shared<ResourceDecoderImpl<envoy::config::cluster::v3::Cluster>>("name");
        states_.emplace(state.key_.type_url_, state);
        auto ok = sendDiscoveryRequest(state);

        if (ok) {
            receiveResponse();
            std::cout << "rpc write success!!!" << std::endl;
        } else {
            std::cout << "rpc write fail!!!" << std::endl;
        }

        ok = sendDiscoveryRequest(state);
    }

    void AdsClient::subscribeEDS() {

    }

    void AdsClient::detectingQueue() {
        void *on_tag;
        bool ok;

        DiscoveryResponse discoveryResponse;
        rpc_->Read(&discoveryResponse, &on_tag);
        cq_.Next(&on_tag, &ok);

        if (on_tag) {
            std::cout << "rpc read success!!!" << std::endl;
        } else {
            std::cout << "rpc read fail!!!" << std::endl;
        }
    }

    void AdsClient::startSubscribe() {
        subscribeCDS();
    }

    bool AdsClient::sendDiscoveryRequest(State &state) {
        auto type_url = state.key_.type_url_;
        std::cout << "sendling Discovery-Request : " << state.key_.type_url_ << std::endl;

        state.discoveryRequest_.mutable_resource_names()->Clear();

        // 중복된 리소스를 제외하고 request의 리소스를 업데이트

        if (!state.subscribed_) {
            state.discoveryRequest_.set_type_url(type_url);
            state.subscribed_ = true;
            state.discoveryRequest_.mutable_node()->CopyFrom(node());
        } else {
            state.discoveryRequest_.clear_node();
        }

        std::cout << "---------------- request ------------------" << std::endl;
        std::cout << state.discoveryRequest_.DebugString();
        std::cout << "--------------- request -------------------" << std::endl;


        void *on_tag;
        bool ok = false;
        rpc_->Write(state.discoveryRequest_, &on_tag);
        cq_.Next(&on_tag, &ok);

        // clear error_detail after the request is sent if it exists.
        if (state.discoveryRequest_.has_error_detail()) {
            state.discoveryRequest_.clear_error_detail();
        }

        return ok;
    }

    void AdsClient::processResponse(const DiscoveryResponse &discoveryResponse) {
        const std::string &type_url = discoveryResponse.type_url();
        std::cout << "---------------- response ------------------" << std::endl;
        std::cout << discoveryResponse.DebugString();
        std::cout << "--------------- response -------------------" << std::endl;

        if (states_.count(type_url) == 0) {
            std::cout << "Ignoring the message for type URL " << type_url << "as it has no current subscribers."
                      << std::endl;
            return;
        }


        auto api_state = states_.find(type_url);
        if (api_state == states_.end()) {
            // update the nonce as we are processing this response.
            api_state->second.discoveryRequest_.set_response_nonce(discoveryResponse.nonce());
            if (discoveryResponse.resources().empty()) {
                // No watches and no resources. This can happen when envoy unregisters from a
                // resource that's removed from the server as well. For example, a deleted cluster
                // triggers un-watching the ClusterLoadAssignment watch, and at the same time the
                // xDS server sends an empty list of ClusterLoadAssignment resources. we'll accept
                // this update. no need to send a discovery request, as we don't watch for anything.
                api_state->second.discoveryRequest_.set_version_info(discoveryResponse.version_info());
            } else {
                // No watches and we have resources - this should not happen. send a NACK (by not
                // updating the version).
                //ENVOY_LOG(warn, "Ignoring unwatched type URL {}", type_url);
                std::cout << "Ignoring unwatched type URL " << type_url << std::endl;
                //queueDiscoveryRequest(type_url);
            }

            return;
        }


        try {
            std::vector<DecodedResourcePtr> resources;
            auto resource_decoder = api_state->second.resourceDecoderPtr_;

            for (const auto &resource: discoveryResponse.resources()) {
                // TODO(snowp): Check the underlying type when the resource is a Resource.
                if (!resource.Is<envoy::service::discovery::v3::Resource>() &&
                    type_url != resource.type_url()) {

                    std::stringstream ss;
                    ss << resource.type_url() << " does not match the message-wide type URL " << type_url
                       << " in DiscoveryResponse "
                       << discoveryResponse.DebugString();

                    throw std::runtime_error(ss.str());
                }

                auto decoded_resource = api_state->second.resourceDecoderPtr_->decodeResource(resource);

                std::cout << "resource debug : " << decoded_resource->DebugString() << std::endl;

/*                if (!isHeartbeatResource(type_url, *decoded_resource)) {
                    resources.emplace_back(std::move(decoded_resource));
                }*/

            }

            resourceUpdate(resources, api_state->second, type_url, discoveryResponse.version_info());


        } catch (std::exception &e) {
            ::google::rpc::Status *error_detail = api_state->second.discoveryRequest_.mutable_error_detail();
            error_detail->set_code(Grpc::Status::WellKnownGrpcStatus::Internal);
            error_detail->set_message(truncateGrpcStatusMessage(e.what()));
        }

        api_state->second.discoveryRequest_.set_response_nonce(discoveryResponse.nonce());
    }


    void AdsClient::receiveResponse() {
        void *on_tag;
        bool ok;

        DiscoveryResponse discoveryResponse;
        rpc_->Read(&discoveryResponse, &on_tag);
        cq_.Next(&on_tag, &ok);

        if (on_tag) {
            std::cout << "rpc read success!!!" << std::endl;
            processResponse(discoveryResponse);
        } else {
            std::cout << "rpc read fail!!!" << std::endl;
        }
    }

    void
    AdsClient::resourceUpdate(const std::vector<DecodedResourcePtr> &resources, State &state,
                              const std::string &type_url, const std::string &version_info) {

        // To avoid O(n^2) explosion (e.g. when we have 1000s of EDS watches), we
        // build a map here from resource name to resource and then walk watches_.
        // We have to walk all watches (and need an efficient map as a result) to
        // ensure we deliver empty config updates when a resource is dropped. We make the map ordered
        // for test determinism.
        std::unordered_map<std::string, std::reference_wrapper<DecodedResource>> resource_ref_map;
        std::vector<std::reference_wrapper<DecodedResource>> all_resource_refs;

        //const auto scoped_ttl_update = api_state.ttl_.scopedTtlUpdate();

        for (const auto &resource: resources) {
/*        if (resource->ttl()) {
            api_state.ttl_.add(*resource->ttl(), resource->name());
        } else {
            api_state.ttl_.clear(resource->name());
        }*/

            all_resource_refs.emplace_back(*resource);

/*        if (XdsResourceIdentifier::hasXdsTpScheme(resource->name())) {
            // Sort the context params of an xdstp resource, so we can compare them easily.
            auto resource_or_error = XdsResourceIdentifier::decodeUrn(resource->name());
            THROW_IF_STATUS_NOT_OK(resource_or_error, throw);
            xds::core::v3::ResourceName xdstp_resource = resource_or_error.value();
            XdsResourceIdentifier::EncodeOptions options;
            options.sort_context_params_ = true;
            resource_ref_map.emplace(XdsResourceIdentifier::encodeUrn(xdstp_resource, options),
                                     *resource);
        } else {
            resource_ref_map.emplace(resource->name(), *resource);
        }*/

            resource_ref_map.emplace(resource->name_, *resource);
        }

        // Execute external config validators if there are any watches.
/*    if (!api_state.watches_.empty()) {
        config_validators_->executeValidators(type_url, resources);
    }*/

/*        for (auto watch: api_state.watches_) {
            // onConfigUpdate should be called in all cases for single watch xDS (Cluster and
            // Listener) even if the message does not have resources so that update_empty stat
            // is properly incremented and state-of-the-world semantics are maintained.
            if (watch->resources_.empty()) {
                //THROW_IF_NOT_OK(watch->callbacks_.onConfigUpdate(all_resource_refs, version_info));
                /// watch를 통해 관련 config 업데이트 함
                continue;
            }
            std::vector<std::reference_wrapper<DecodedResource>> found_resources;
            for (const auto &watched_resource_name: watch->resources_) {
                // Look for a singleton subscription.
                auto it = resource_ref_map.find(watched_resource_name);
                if (it != resource_ref_map.end()) {
                    found_resources.emplace_back(it->second);
                } else if (isXdsTpWildcard(watched_resource_name)) {
                    // See if the resources match the xdstp wildcard subscription.
                    // Note: although it is unlikely that Envoy will need to support a resource that is mapped
                    // to both a singleton and collection watch, this code still supports this use case.
                    // TODO(abeyad): This could be made more efficient, e.g. by pre-computing and having a map
                    // entry for each wildcard watch.
                    for (const auto &resource_ref_it: resource_ref_map) {
                        if (XdsResourceIdentifier::hasXdsTpScheme(resource_ref_it.first) &&
                            convertToWildcard(resource_ref_it.first) == watched_resource_name) {
                            found_resources.emplace_back(resource_ref_it.second);
                        }
                    }
                }
            }*/

        // onConfigUpdate should be called only on watches(clusters/listeners) that have
        // updates in the message for EDS/RDS.
/*        if (!found_resources.empty()) {
            //THROW_IF_NOT_OK(watch->callbacks_.onConfigUpdate(found_resources, version_info));
            // Resource cache is only used for EDS resources.
            if (eds_resources_cache_ &&
                (type_url == Config::getTypeUrl<envoy::config::endpoint::v3::ClusterLoadAssignment>())) {
                for (const auto& resource : found_resources) {
                    const envoy::config::endpoint::v3::ClusterLoadAssignment& cluster_load_assignment =
                            dynamic_cast<const envoy::config::endpoint::v3::ClusterLoadAssignment&>(
                                    resource.get().resource());
                    eds_resources_cache_->setResource(resource.get().name(), cluster_load_assignment);
                }
                // No need to remove resources from the cache, as currently only non-collection
                // subscriptions are supported, and these resources are removed in the call
                // to updateWatchInterest().
            }
        }*/
        //}

        // All config updates have been applied without throwing an exception, so we'll call the xDS
        // resources delegate, if any.
/*    if (call_delegate && xds_resources_delegate_.has_value()) {
        xds_resources_delegate_->onConfigUpdated(XdsConfigSourceId{target_xds_authority_, type_url},
                                                 all_resource_refs);
    }*/

        state.discoveryRequest_.set_version_info(version_info);
    }

    void AdsClient::clearNonce() {
        // Iterate over all api_states (for each type_url), and clear its nonce.
        for(auto& [type_url, api_state] : states_){
            api_state.discoveryRequest_.clear_response_nonce();
        }
    }


}


