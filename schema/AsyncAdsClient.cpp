//
// Created by bont on 24. 5. 9.
//

#include <condition_variable>
#include "AsyncAdsClient.h"

void AsyncAdsClient::do_something() {
    class Chatter : public grpc::ClientBidiReactor<DiscoveryRequest, DiscoveryResponse> {
    public:
        Chatter(AggregatedDiscoveryService::Stub* stub){

            DiscoveryRequest discoveryRequest;
            discoveryRequest.mutable_version_info()->append("");
            auto node_data = discoveryRequest.mutable_node();
            node_data->mutable_cluster()->append("test-cluster");
            node_data->mutable_id()->append("test-id");
            discoveryRequest.mutable_type_url()->append("type.googleapis.com/envoy.config.cluster.v3.Cluster");

            requests.push_back(discoveryRequest);
            requests_iterator_ = requests.begin();

            stub->async()->StreamAggregatedResources(&context_, this);
            NextWrite();
            StartRead(&response_);
            StartCall();
        }

        void OnWriteDone(bool /*ok*/) override { NextWrite(); }
        void OnReadDone(bool ok) override {
            if (ok) {
                std::cout << "================= Read From Server ======================" << std::endl;
                std::cout << "version-info : " << response_.version_info() << std::endl;
                std::cout << "nonce : " << response_.nonce() << std::endl;
                std::cout << "type-url : " << response_.type_url() << std::endl;

                std::cout << "Resources : "  << std::endl;
                for (const auto& resource : response_.resources()) {
                    std::cout << "  Resource Type URL: " << resource.type_url() << std::endl;
                    std::cout << "  Resource Type value: " << resource.value().c_str() << std::endl;
                }
                std::cout << "=============================================" << std::endl;

                StartRead(&response_);
            }
        }
        void OnDone(const Status& s) override {
            std::unique_lock<std::mutex> l(mu_);
            status_ = s;
            done_ = true;
            cv_.notify_one();
        }
        Status Await() {
            std::unique_lock<std::mutex> l(mu_);
            cv_.wait(l, [this] { return done_; });
            return std::move(status_);
        }

    private:
        void NextWrite() {
            if (requests_iterator_ != requests.end()) {
                const auto& request = *requests_iterator_;

                std::cout << "================= Write From Client ======================" << std::endl;
                std::cout << "version-info : " << request.version_info() << std::endl;
                std::cout << "response-nonce : " << request.response_nonce() << std::endl;
                std::cout << "type-url : " << request.type_url() << std::endl;
                std::cout << "Resources : "  << std::endl;
                for (const auto& resource : request.resource_names()) {
                    std::cout << "  Resource: " << resource << std::endl;
                }
                std::cout << "==========================================" << std::endl;

                StartWrite(&request);
                requests_iterator_++;

            } else {
                StartWritesDone();
            }
        }
        ClientContext context_;
        std::vector<DiscoveryRequest> requests;
        std::vector<DiscoveryRequest>::const_iterator requests_iterator_;
        DiscoveryRequest request_;
        DiscoveryResponse response_;
        std::mutex mu_;
        std::condition_variable cv_;
        Status status_;
        bool done_ = false;
    };

    Chatter chatter(stub_.get());
    Status status = chatter.Await();
    if (!status.ok()) {
        std::cout << "RouteChat rpc failed." << std::endl;
    }
}
