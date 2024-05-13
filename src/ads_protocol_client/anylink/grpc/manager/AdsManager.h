/*
 * GrpcManager.h
 *
 *  Created on: 2020. 4. 21.
 *      Author: jeongtaek
 */

#ifndef ANYLINK_ISTIO_SRC_GRPC_MANAGER_ADSMANAGER_H_
#define ANYLINK_ISTIO_SRC_GRPC_MANAGER_ADSMANAGER_H_

#include <string>
#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>
#include <google/protobuf/descriptor.h>

#include "../subscription/Subscription.h"
#include "../../local/LocalInfo.h"
#include "anylink_proto/target/envoy/service/discovery/v3/discovery.pb.h"
#include "anylink_proto/target/envoy/service/discovery/v3/ads.grpc.pb.h"

namespace gateway {
class AbstractLogger;
}

using namespace gateway;

namespace anylink {

class AdsManager {
public:
    AdsManager(std::string pilotUrl, std::string nodeId);
    virtual ~AdsManager() = default;
public:
    void setLogger(std::shared_ptr<AbstractLogger> logger);

    void startClient();
    void addSubscription(const std::shared_ptr<Subscription> &sub);
    void handleEvent(void* tag, bool ok);
    envoy::config::core::v3::Node getNode();
    std::unordered_map<std::string, std::shared_ptr<Subscription>>& getSubscribeMap();

private:
    static std::shared_ptr<AbstractLogger> logger;

    void startcall();

    void initSubscribe(envoy::service::discovery::v3::DiscoveryRequest& request, std::shared_ptr<Subscription>& sub);
    bool receiveADS(envoy::service::discovery::v3::DiscoveryResponse& response, std::shared_ptr<Subscription>& sub);
    void sendACK(envoy::service::discovery::v3::DiscoveryRequest &request, envoy::service::discovery::v3::DiscoveryResponse response);
    void sendNACK(envoy::service::discovery::v3::DiscoveryRequest &request, envoy::service::discovery::v3::DiscoveryResponse response);

private:
    std::string pilotUrl_;
    std::unique_ptr<LocalInfo> localInfo_;
    std::unordered_map<std::string, std::shared_ptr<Subscription>> subscribe_map;
    std::unique_ptr<envoy::service::discovery::v3::AggregatedDiscoveryService::Stub> stub_;
    grpc::CompletionQueue cq_;
    grpc::ClientContext context_;
    grpc::Status status_;

    std::unique_ptr<grpc::ClientAsyncReaderWriter<envoy::service::discovery::v3::DiscoveryRequest, envoy::service::discovery::v3::DiscoveryResponse>> rpc;
    //std::string urlList[1] { "type.googleapis.com/envoy.config.cluster.v3.Cluster"};
    std::string urlList[4] { "type.googleapis.com/envoy.config.cluster.v3.Cluster", "type.googleapis.com/envoy.config.listener.v3.Listener", "type.googleapis.com/envoy.config.endpoint.v3.ClusterLoadAssignment", "type.googleapis.com/envoy.config.route.v3.RouteConfiguration" };



    struct AsyncClientCall {
        envoy::service::discovery::v3::DiscoveryResponse reply_;
        grpc::ClientContext context;
        grpc::Status status;
        std::unique_ptr<grpc::ClientAsyncResponseReader<envoy::service::discovery::v3::DiscoveryResponse>> response_reader;
    };
};

} /* namespace anylink */

#endif /* ANYLINK_ISTIO_SRC_GRPC_MANAGER_ADSMANAGER_H_ */
