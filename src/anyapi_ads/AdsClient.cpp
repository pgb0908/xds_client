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
        auto state = std::make_unique<State>();
        state->key_.name_ = "cds";
        state->key_.type_url_ = "type.googleapis.com/envoy.config.cluster.v3.Cluster";
        state->resourceDecoderPtr_ = std::make_shared<ResourceDecoderImpl<envoy::config::cluster::v3::Cluster>>("name");

        // request-queue 요청 집어넣음
        request_queue_.push(state->key_.type_url_);

        states_.emplace(state->key_.type_url_, std::move(state));

        std::cout << "subscribeCDS" << std::endl;
    }

    void AdsClient::subscribeEDS(const std::vector<DecodedResourcePtr>& resources) {
        auto new_state = std::make_unique<State>();
        new_state->key_.name_ = "eds";
        new_state->key_.type_url_ = "type.googleapis.com/envoy.config.endpoint.v3.ClusterLoadAssignment";
        new_state->resourceDecoderPtr_ = std::make_shared<ResourceDecoderImpl<envoy::config::endpoint::v3::ClusterLoadAssignment>>("cluster_name");
        for (const auto &resource: resources) {
            new_state->watched_resources_.push_back(resource->name_);
        }
        request_queue_.push(new_state->key_.type_url_);
        states_.emplace(new_state->key_.type_url_, std::move(new_state));
    }

    void AdsClient::detectingQueue() {

        while(1){
            // request-queue 감지
            if(!request_queue_.empty()){
                while(!request_queue_.empty()){
                    auto state = request_queue_.front();
                    auto ok = sendDiscoveryRequest(state);

                    request_queue_.pop();
                }

            }else{
                // request-queue 아무것도 없다면 listen-mode
                receiveResponse();
            }
        }


    }

    void AdsClient::startSubscribe() {
        subscribeCDS();
    }

    bool AdsClient::sendDiscoveryRequest(std::string& type_url) {
        auto state = states_[type_url].get();
        std::cout << "sendling Discovery-Request : " << type_url << std::endl;

        state->discoveryRequest_.mutable_resource_names()->Clear();

        // 중복된 리소스를 제외하고 request의 리소스를 업데이트
        std::set<std::string> resources;
        for (const auto& watchedResource : state->watched_resources_) {
            if (resources.count(watchedResource) == 0) {
                resources.emplace(watchedResource);
                state->discoveryRequest_.add_resource_names(watchedResource);
            }
        }

        if (!state->subscribed_) {
            state->discoveryRequest_.set_type_url(type_url);
            state->subscribed_ = true;
            state->discoveryRequest_.mutable_node()->CopyFrom(node());
        } else {
            state->discoveryRequest_.clear_node();
        }

        std::cout << "---------------- request ------------------" << std::endl;
        std::cout << state->discoveryRequest_.DebugString();
        std::cout << "--------------- request -------------------" << std::endl;


        void *on_tag;
        bool ok = false;
        rpc_->Write(state->discoveryRequest_, &on_tag);
        cq_.Next(&on_tag, &ok);

        // clear error_detail after the request is sent if it exists.
        if (state->discoveryRequest_.has_error_detail()) {
            state->discoveryRequest_.clear_error_detail();
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
            api_state->second->discoveryRequest_.set_response_nonce(discoveryResponse.nonce());
            if (discoveryResponse.resources().empty()) {
                // No watches and no resources. This can happen when envoy unregisters from a
                // resource that's removed from the server as well. For example, a deleted cluster
                // triggers un-watching the ClusterLoadAssignment watch, and at the same time the
                // xDS server sends an empty list of ClusterLoadAssignment resources. we'll accept
                // this update. no need to send a discovery request, as we don't watch for anything.
                api_state->second->discoveryRequest_.set_version_info(discoveryResponse.version_info());
            } else {
                // No watches and we have resources - this should not happen. send a NACK (by not
                // updating the version).
                //ENVOY_LOG(warn, "Ignoring unwatched type URL {}", type_url);
                std::cout << "Ignoring unwatched type URL " << type_url << std::endl;
                request_queue_.push(api_state->second->key_.type_url_);
            }

            return;
        }


        try {
            std::vector<DecodedResourcePtr> resources;
            auto resource_decoder = api_state->second->resourceDecoderPtr_;

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

                auto decoded = api_state->second->resourceDecoderPtr_->decodeResource(resource);
                std::cout << "---------------- decoded resource ------------------" << std::endl;
                std::cout << decoded->DebugString() << std::endl;

                auto decoded_resource = std::make_unique<DecodedResource>();
                auto resource_name = api_state->second->resourceDecoderPtr_->resourceName(*decoded);


                decoded_resource->has_resource_ = true;
                decoded_resource->version_ = discoveryResponse.version_info();
                decoded_resource->resource_ = std::move(decoded);
                decoded_resource->name_ = resource_name;

                if (!isHeartbeatResource(type_url, *decoded_resource)) {
                    resources.emplace_back(std::move(decoded_resource));
                }

            }

            resourceUpdate(resources, *(api_state->second),
                           type_url, discoveryResponse.version_info());


        } catch (std::exception &e) {
            ::google::rpc::Status *error_detail = api_state->second->discoveryRequest_.mutable_error_detail();
            error_detail->set_code(Grpc::Status::WellKnownGrpcStatus::Internal);
            error_detail->set_message(truncateGrpcStatusMessage(e.what()));
        }

        api_state->second->discoveryRequest_.set_response_nonce(discoveryResponse.nonce());

        // 응답에 대한 답변으로
        // request-queue에 새로운 요청 생성
        request_queue_.push(api_state->second->key_.type_url_);
    }


    void AdsClient::receiveResponse() {
        void *on_tag;
        bool ok;

        std::cout << "receiveResponse" << std::endl;
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


        // cds이면 eds에 대한 구독요청을 큐에 집어 넣음
        if(state.key_.name_ == "cds"){
            // cluster의 config 파싱


            // cluster의 타입이 EDS인 경우에만 EDS 구독
            subscribeEDS(resources);
        }

        state.discoveryRequest_.set_version_info(version_info);
    }

    void AdsClient::clearNonce() {
        // Iterate over all api_states (for each type_url), and clear its nonce.
        for(auto& [type_url, api_state] : states_){
            api_state->discoveryRequest_.clear_response_nonce();
        }
    }


}


