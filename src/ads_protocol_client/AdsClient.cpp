//
// Created by bont on 24. 5. 8.
//

#include "AdsClient.h"

void AdsClient::onStreamEstablished() {
    stream_ = stub_->StreamAggregatedResources(&context_);

    if(stream_){
        first_stream_request_ = true;
        clearNonce();
        request_queue_ = std::make_unique<std::queue<std::string>>();
        for (const auto& type_url : subscriptions_) {
           // queueDiscoveryRequest(type_url);
        }
    }else{
        std::cout << "can't make stream. something wrong!" << std::endl;
    }
}

void AdsClient::endStream() {
    stream_->WritesDone();
    Status status = stream_->Finish();
    if(!status.ok()){
        std::cout << "rpc failed" << std::endl;
    }
    std::cout << "end stream" << std::endl;
}


AdsClient::ApiState &AdsClient::apiStateFor(std::string type_url) {
    auto itr = api_state_.find(type_url);
    if (itr == api_state_.end()) {
        api_state_.emplace(
                type_url,
                std::make_unique<ApiState>([this, type_url](const auto& expired) {
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
    for (auto& [type_url, api_state] : api_state_) {
        if (api_state) {
            api_state->request_.clear_response_nonce();
        }
    }
}



void AdsClient::queueDiscoveryRequest(std::string queue_item) {
    if(stream_ == nullptr){
        std::cout << "No stream available to queueDiscoveryRequest for " << queue_item << std::endl;
        return; // Drop this request; the reconnect will enqueue a new one.
    }

    ApiState& api_state = apiStateFor(queue_item);
    if (api_state.paused()) {
        //ENVOY_LOG(trace, "API {} paused during queueDiscoveryRequest(), setting pending.", queue_item);
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
    if (shutdown_) {
        return;
    }

    ApiState& api_state = apiStateFor(type_url);
    auto& request = api_state.request_;
    request.mutable_resource_names()->Clear();

    // Maintain a set to avoid dupes.
/*    std::set<std::string> resources;
    for (const auto* watch : api_state.watches_) {
        for (const std::string& resource : watch->resources_) {
            if (!resources.contains(resource)) {
                resources.emplace(resource);
                request.add_resource_names(resource);
            }
        }
    }*/


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

    std::cout << "Sending DiscoveryRequest for : " << type_url << ": "  <<  request.ShortDebugString() << std::endl;
    stream_->Write(request);
    first_stream_request_ = false;

    // clear error_detail after the request is sent if it exists.
    if (apiStateFor(type_url).request_.has_error_detail()) {
        apiStateFor(type_url).request_.clear_error_detail();
    }
}

void AdsClient::onDiscoveryResponse(std::unique_ptr<envoy::service::discovery::v3::DiscoveryResponse> &&message) {
/*    const std::string type_url = message->type_url();
    std::cout << "Received gRPC message for " << type_url << " at version " << message->version_info() << std::endl;

    if (api_state_.count(type_url) == 0) {
        // TODO(yuval-k): This should never happen. consider dropping the stream as this is a
        // protocol violation

        std::cout << "Ignoring the message for type URL " << type_url << "as it has no current subscribers."<< std::endl;
        return;
    }

    ApiState& api_state = apiStateFor(type_url);

*//*    if (message->has_control_plane()) {
        control_plane_stats.identifier_.set(message->control_plane().identifier());

        if (message->control_plane().identifier() != api_state.control_plane_identifier_) {
            api_state.control_plane_identifier_ = message->control_plane().identifier();
            ENVOY_LOG(debug, "Receiving gRPC updates for {} from {}", type_url,
                      api_state.control_plane_identifier_);
        }
    }*//*

*//*    if (api_state.watches_.empty()) {
        // update the nonce as we are processing this response.
        api_state.request_.set_response_nonce(message->nonce());
        if (message->resources().empty()) {
            // No watches and no resources. This can happen when envoy unregisters from a
            // resource that's removed from the server as well. For example, a deleted cluster
            // triggers un-watching the ClusterLoadAssignment watch, and at the same time the
            // xDS server sends an empty list of ClusterLoadAssignment resources. we'll accept
            // this update. no need to send a discovery request, as we don't watch for anything.
            api_state.request_.set_version_info(message->version_info());
        } else {
            // No watches and we have resources - this should not happen. send a NACK (by not
            // updating the version).
            ENVOY_LOG(warn, "Ignoring unwatched type URL {}", type_url);
            queueDiscoveryRequest(type_url);
        }
        return;
    }*//*
    //ScopedResume same_type_resume;
    // We pause updates of the same type. This is necessary for SotW and GrpcMuxImpl, since unlike
    // delta and NewGRpcMuxImpl, independent watch additions/removals trigger updates regardless of
    // the delta state. The proper fix for this is to converge these implementations,
    // see https://github.com/envoyproxy/envoy/issues/11477.
   // same_type_resume = pause(type_url);

        std::vector<DecodedResourcePtr> resources;
        OpaqueResourceDecoder& resource_decoder = *api_state.watches_.front()->resource_decoder_;

        for (const auto& resource : message->resources()) {
            // TODO(snowp): Check the underlying type when the resource is a Resource.
            if (!resource.Is<envoy::service::discovery::v3::Resource>() &&
                type_url != resource.type_url()) {
                throw EnvoyException(
                        fmt::format("{} does not match the message-wide type URL {} in DiscoveryResponse {}",
                                    resource.type_url(), type_url, message->DebugString()));
            }

            auto decoded_resource =
                    DecodedResourceImpl::fromResource(resource_decoder, resource, message->version_info());

            if (!isHeartbeatResource(type_url, *decoded_resource)) {
                resources.emplace_back(std::move(decoded_resource));
            }
        }

        processDiscoveryResources(resources, api_state, type_url, message->version_info(),
        *//*call_delegate=*//*true);

        // Processing point when resources are successfully ingested.
        if (xds_config_tracker_.has_value()) {
            xds_config_tracker_->onConfigAccepted(type_url, resources);
        }


    catch (const EnvoyException& e) {
        for (auto watch : api_state.watches_) {
            watch->callbacks_.onConfigUpdateFailed(
                    Envoy::Config::ConfigUpdateFailureReason::UpdateRejected, &e);
        }
        ::google::rpc::Status* error_detail = api_state.request_.mutable_error_detail();
        error_detail->set_code(Grpc::Status::WellKnownGrpcStatus::Internal);
        error_detail->set_message(Config::Utility::truncateGrpcStatusMessage(e.what()));

        // Processing point when there is any exception during the parse and ingestion process.
        if (xds_config_tracker_.has_value()) {
            xds_config_tracker_->onConfigRejected(*message, error_detail->message());
        }
    }
    api_state.previously_fetched_data_ = true;
    api_state.request_.set_response_nonce(message->nonce());
    //ASSERT(api_state.paused());
    queueDiscoveryRequest(type_url);*/
}

