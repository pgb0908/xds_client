//
// Created by bont on 24. 5. 8.
//

#include <sstream>
#include "AdsClient.h"
#include "OpaqueResourceDecoder.h"
#include "DecodedResource.h"
#include "Watcher.h"
#include "Status.h"

void AdsClient::onStreamEstablished() {
    stream_ = stub_->StreamAggregatedResources(&context_);

    std::cout << "hello" << std::endl;
    if (stream_) {
        first_stream_request_ = true;
        clearNonce();
        request_queue_ = std::make_unique<std::queue<std::string>>();
        for (const auto &type_url: subscriptions_) {
            queueDiscoveryRequest(type_url);
        }
    } else {
        std::cout << "can't make stream. something wrong!" << std::endl;
    }
}

void AdsClient::endStream() {
    stream_->WritesDone();
    Status status = stream_->Finish();
    if (!status.ok()) {
        std::cout << "rpc failed" << std::endl;
    }
    std::cout << "end stream" << std::endl;
}


AdsClient::ApiState &AdsClient::apiStateFor(std::string type_url) {
    auto itr = api_state_.find(type_url);
    if (itr == api_state_.end()) {
        api_state_.emplace(
                type_url,
                std::make_unique<ApiState>([this, type_url](const auto &expired) {
                    expiryCallback(type_url, expired);
                }));
    }

    return *api_state_.find(type_url)->second;
}

void AdsClient::expiryCallback(std::string type_url, const std::vector<std::string> &expired) {
    // The TtlManager triggers a callback with a list of all the expired elements, which we need
    // to compare against the various watched resources to return the subset that each watch is
    // subscribed to.

    // We convert the incoming list into a set in order to more efficiently perform this
    // comparison when there are a lot of watches.
    std::set<std::string> all_expired;
    all_expired.insert(expired.begin(), expired.end());

    // Note: We can blindly dereference the lookup here since the only time we call this is in a
    // callback that is created at the same time as we insert the ApiState for this type.
/*    for (auto watch : api_state_.find(type_url)->second->watches_) {
        google::protobuf::RepeatedPtrField<std::string> found_resources_for_watch;

        for (const auto& resource : expired) {
            if (all_expired.find(resource) != all_expired.end()) {
                found_resources_for_watch.Add(std::string(resource));
            }
        }

        //THROW_IF_NOT_OK(watch->callbacks_.onConfigUpdate({}, found_resources_for_watch, ""));
    }*/
}

void AdsClient::clearNonce() {
    // Iterate over all api_states (for each type_url), and clear its nonce.
    for (auto &[type_url, api_state]: api_state_) {
        if (api_state) {
            api_state->request_.clear_response_nonce();
        }
    }
}


void AdsClient::queueDiscoveryRequest(const std::string &queue_item) {
    if (stream_ == nullptr) {
        std::cout << "No stream available to queueDiscoveryRequest for " << queue_item << std::endl;
        return; // Drop this request; the reconnect will enqueue a new one.
    }

    ApiState &api_state = apiStateFor(queue_item);
    if (api_state.paused()) {
        //ENVOY_LOG(trace, "API {} paused during queueDiscoveryRequest(), setting pending.", queue_item);
        std::cout << "API " << queue_item << " paused during queueDiscoveryRequest(), setting pending." << std::endl;
        api_state.pending_ = true;
        return; // Drop this request; the unpause will enqueue a new one.
    }
    request_queue_->emplace(std::string(queue_item));
    drainRequests();
}

void AdsClient::drainRequests() {
    while (!request_queue_->empty()) {
        // Process the request, if rate limiting is not enabled at all or if it is under rate limit.
        sendDiscoveryRequest(request_queue_->front());
        request_queue_->pop();
    }
}

void AdsClient::sendDiscoveryRequest(std::string type_url) {
    std::cout << "sendling Discovery-Request" << std::endl;
    if (shutdown_) {
        return;
    }

    ApiState &api_state = apiStateFor(type_url);
    auto &request = api_state.request_;
    request.mutable_resource_names()->Clear();

    // Maintain a set to avoid dupes.
    std::set<std::string> resources;
    for (const auto *watch: api_state.watches_) {
        if(!watch->resources_.empty()){
            for (const std::string &resource: watch->resources_) {
                if (resources.find(resource) == resources.end()) {
                    resources.emplace(resource);
                    request.add_resource_names(resource);
                }
            }
        }
    }

    if (api_state.must_send_node_ || first_stream_request_) {
        // Node may have been cleared during a previous request.
        if (!apiStateFor(type_url).subscribed_) {
            apiStateFor(type_url).request_.set_type_url(type_url);
            //apiStateFor(type_url).request_.mutable_node()->MergeFrom(node());
            request.mutable_node()->CopyFrom(node());
            apiStateFor(type_url).subscribed_ = true;
            subscriptions_.emplace_back(type_url);
        }

        api_state.must_send_node_ = false;
    } else {
        request.clear_node();
    }

    std::cout << "Sending DiscoveryRequest for : " << type_url << ": " << request.ShortDebugString() << std::endl;

    std::cout << "---------------- request ------------------" << std::endl;
    std::cout <<  request.DebugString();
    std::cout << "--------------- request -------------------" << std::endl;


    stream_->Write(request);
    first_stream_request_ = false;

    // clear error_detail after the request is sent if it exists.
    if (apiStateFor(type_url).request_.has_error_detail()) {
        apiStateFor(type_url).request_.clear_error_detail();
    }


    DiscoveryResponse discoveryResponse;
    while (stream_->Read(&discoveryResponse)) {
        onDiscoveryResponse(discoveryResponse);
    }

}

void AdsClient::onDiscoveryResponse(const envoy::service::discovery::v3::DiscoveryResponse &message) {
    const std::string& type_url = message.type_url();
    std::cout << "Received gRPC message for " << type_url << " at version " << message.version_info() << std::endl;

    std::cout << "---------------- response ------------------" << std::endl;
    std::cout  << message.DebugString();
    std::cout << "--------------- response -------------------" << std::endl;

    if (api_state_.count(type_url) == 0) {
        // TODO(yuval-k): This should never happen. consider dropping the stream as this is a
        // protocol violation

        std::cout << "Ignoring the message for type URL " << type_url << "as it has no current subscribers."
                  << std::endl;
        return;
    }

    ApiState &api_state = apiStateFor(type_url);

/*    if (message->has_control_plane()) {
        control_plane_stats.identifier_.set(message->control_plane().identifier());

        if (message->control_plane().identifier() != api_state.control_plane_identifier_) {
            api_state.control_plane_identifier_ = message->control_plane().identifier();
            ENVOY_LOG(debug, "Receiving gRPC updates for {} from {}", type_url,
                      api_state.control_plane_identifier_);
        }
    }*/

    if (api_state.watches_.empty()) {
        // update the nonce as we are processing this response.
        api_state.request_.set_response_nonce(message.nonce());
        if (message.resources().empty()) {
            // No watches and no resources. This can happen when envoy unregisters from a
            // resource that's removed from the server as well. For example, a deleted cluster
            // triggers un-watching the ClusterLoadAssignment watch, and at the same time the
            // xDS server sends an empty list of ClusterLoadAssignment resources. we'll accept
            // this update. no need to send a discovery request, as we don't watch for anything.
            api_state.request_.set_version_info(message.version_info());
        } else {
            // No watches and we have resources - this should not happen. send a NACK (by not
            // updating the version).
            //ENVOY_LOG(warn, "Ignoring unwatched type URL {}", type_url);
            std::cout << "Ignoring unwatched type URL " << type_url<< std::endl;
            queueDiscoveryRequest(type_url);
        }
        return;
    }

    ScopedResume same_type_resume;
    // We pause updates of the same type. This is necessary for SotW and GrpcMuxImpl, since unlike
    // delta and NewGRpcMuxImpl, independent watch additions/removals trigger updates regardless of
    // the delta state. The proper fix for this is to converge these implementations,
    // see https://github.com/envoyproxy/envoy/issues/11477.
    same_type_resume = pause(type_url);

    try{
        std::vector<DecodedResourcePtr> resources;
        OpaqueResourceDecoder &resource_decoder = *api_state.watches_.front()->resource_decoder_;

        for (const auto &resource: message.resources()) {
            // TODO(snowp): Check the underlying type when the resource is a Resource.
            if (!resource.Is<envoy::service::discovery::v3::Resource>() &&
                type_url != resource.type_url()) {

                std::stringstream ss;
                ss << resource.type_url() << " does not match the message-wide type URL "<< type_url << " in DiscoveryResponse "
                          <<message.DebugString();

                throw std::runtime_error(ss.str());
            }

            auto decoded_resource = DecodedResource::fromResource(
                    resource_decoder, resource, message.version_info());
            //auto decoded_resource = fromResource(resource_decoder, resource, message->version_info());

            std::cout << "resource debug : " << decoded_resource->resource().DebugString() << std::endl;

            if (!isHeartbeatResource(type_url, *decoded_resource)) {
                resources.emplace_back(std::move(decoded_resource));
            }

        }

        processDiscoveryResources(resources, api_state, type_url, message.version_info(),
                                  true);

/*
    // Processing point when resources are successfully ingested.
    if (xds_config_tracker_.has_value()) {
        xds_config_tracker_->onConfigAccepted(type_url, resources);
    }
*/
    }catch (std::exception& e){
/*        for (auto watch: api_state.watches_) {
            watch->callbacks_.onConfigUpdateFailed(
                    Envoy::Config::ConfigUpdateFailureReason::UpdateRejected, &e);
        }*/
        ::google::rpc::Status *error_detail = api_state.request_.mutable_error_detail();
        error_detail->set_code(Grpc::Status::WellKnownGrpcStatus::Internal);
        error_detail->set_message(truncateGrpcStatusMessage(e.what()));

        // Processing point when there is any exception during the parse and ingestion process.
/*        if (xds_config_tracker_.has_value()) {
            xds_config_tracker_->onConfigRejected(*message, error_detail->message());
        }*/
    }

    api_state.previously_fetched_data_ = true;
    api_state.request_.set_response_nonce(message.nonce());
    assert(api_state.paused());
    queueDiscoveryRequest(type_url);
}

void
AdsClient::processDiscoveryResources(const std::vector<DecodedResourcePtr> &resources, AdsClient::ApiState &api_state,
                                     const std::string &type_url, const std::string &version_info, bool call_delegate) {
    // To avoid O(n^2) explosion (e.g. when we have 1000s of EDS watches), we
    // build a map here from resource name to resource and then walk watches_.
    // We have to walk all watches (and need an efficient map as a result) to
    // ensure we deliver empty config updates when a resource is dropped. We make the map ordered
    // for test determinism.
    std::unordered_map<std::string, std::reference_wrapper<DecodedResource>> resource_ref_map;
    std::vector<std::reference_wrapper<DecodedResource>> all_resource_refs;

    //const auto scoped_ttl_update = api_state.ttl_.scopedTtlUpdate();

    for (const auto& resource : resources) {
/*        if (resource->ttl()) {
            api_state.ttl_.add(*resource->ttl(), resource->name());
        } else {
            api_state.ttl_.clear(resource->name());
        }*/

        all_resource_refs.emplace_back(*resource);
        if (XdsResourceIdentifier::hasXdsTpScheme(resource->name())) {
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
        }
    }

    // Execute external config validators if there are any watches.
    if (!api_state.watches_.empty()) {
        config_validators_->executeValidators(type_url, resources);
    }

    for (auto watch : api_state.watches_) {
        // onConfigUpdate should be called in all cases for single watch xDS (Cluster and
        // Listener) even if the message does not have resources so that update_empty stat
        // is properly incremented and state-of-the-world semantics are maintained.
        if (watch->resources_.empty()) {
            THROW_IF_NOT_OK(watch->callbacks_.onConfigUpdate(all_resource_refs, version_info));
            continue;
        }
        std::vector<DecodedResourceRef> found_resources;
        for (const auto& watched_resource_name : watch->resources_) {
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
                for (const auto& resource_ref_it : resource_ref_map) {
                    if (XdsResourceIdentifier::hasXdsTpScheme(resource_ref_it.first) &&
                        convertToWildcard(resource_ref_it.first) == watched_resource_name) {
                        found_resources.emplace_back(resource_ref_it.second);
                    }
                }
            }
        }

        // onConfigUpdate should be called only on watches(clusters/listeners) that have
        // updates in the message for EDS/RDS.
        if (!found_resources.empty()) {
            THROW_IF_NOT_OK(watch->callbacks_.onConfigUpdate(found_resources, version_info));
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
        }
    }

    // All config updates have been applied without throwing an exception, so we'll call the xDS
    // resources delegate, if any.
    if (call_delegate && xds_resources_delegate_.has_value()) {
        xds_resources_delegate_->onConfigUpdated(XdsConfigSourceId{target_xds_authority_, type_url},
                                                 all_resource_refs);
    }

    // TODO(mattklein123): In the future if we start tracking per-resource versions, we
    // would do that tracking here.
    api_state.request_.set_version_info(version_info);
    Memory::Utils::tryShrinkHeap();
}

std::unique_ptr<Watcher> AdsClient::addWatch(const std::string &type_url, const std::set<std::string> &resources,
                                             const OpaqueResourceDecoderSharedPtr& resource_decoder) {
    // Resource cache is only used for EDS resources.
/*    EdsResourcesCacheOptRef resources_cache{absl::nullopt};
    if (eds_resources_cache_ &&
        (type_url == Config::getTypeUrl<envoy::config::endpoint::v3::ClusterLoadAssignment>())) {
        resources_cache = makeOptRefFromPtr(eds_resources_cache_.get());
    }*/
    auto watch = std::make_unique<Watcher>(resources, resource_decoder, type_url,
                                                    *this);

    std::cout << "gRPC mux addWatch for " << type_url << std::endl;

    // Lazily kick off the requests based on first subscription. This has the
    // convenient side-effect that we order messages on the channel based on
    // Envoy's internal dependency ordering.
    // TODO(gsagula): move TokenBucketImpl params to a config.
    if (!apiStateFor(type_url).subscribed_) {
        apiStateFor(type_url).request_.set_type_url(type_url);
        apiStateFor(type_url).request_.mutable_node()->MergeFrom(node());
        apiStateFor(type_url).subscribed_ = true;
        subscriptions_.emplace_back(type_url);
    }

    // This will send an updated request on each subscription.
    // TODO(htuch): For RDS/EDS, this will generate a new DiscoveryRequest on each resource we added.
    // Consider in the future adding some kind of collation/batching during CDS/LDS updates so that we
    // only send a single RDS/EDS update after the CDS/LDS update.
    queueDiscoveryRequest(type_url);

    return watch;

}

std::string AdsClient::truncateGrpcStatusMessage(std::string error_message) {
    // GRPC sends error message via trailers, which by default has a 8KB size limit(see
    // https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md#requests). Truncates the
    // error message if it's too long.
    constexpr uint32_t kProtobufErrMsgLen = 4096;
    std::stringstream ss;
    ss << error_message.substr(0, kProtobufErrMsgLen) ;
    ss << (error_message.length() > kProtobufErrMsgLen ? "...(truncated)" : "");
    return ss.str();
}

ScopedResume AdsClient::pause(const std::string &type_url) {
    return pause(std::vector<std::string>{type_url});
}

ScopedResume AdsClient::pause(const std::vector<std::string> type_urls) {
    for (const auto& type_url : type_urls) {
        ApiState& api_state = apiStateFor(type_url);
        std::cout << "Pausing discovery requests for " << type_url << " (previous count "<< api_state.pauses_  << ")"<< std::endl;
        ++api_state.pauses_;
    }
    return std::make_unique<Cleanup>([this, type_urls]() {
        for (const auto& type_url : type_urls) {
            ApiState& api_state = apiStateFor(type_url);
            std::cout << "Decreasing pause count on discovery requests for " << type_url << " (previous count "<< api_state.pauses_  << ")"<< std::endl;
            //ASSERT(api_state.paused());

            if (--api_state.pauses_ == 0 && api_state.pending_ && api_state.subscribed_) {
                std::cout << "Resuming discovery requests for  " << type_url << std::endl;
                queueDiscoveryRequest(type_url);
                api_state.pending_ = false;
            }
        }
    });
}

