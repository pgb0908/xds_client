/*
 * AdsConnection.h
 *
 *  Created on: 2022. 2. 17.
 *      Author: yejin
 */

#ifndef ANYLINK_ISTIO_SRC_GRPC_MANAGER_ADSCONNECTION_H_
#define ANYLINK_ISTIO_SRC_GRPC_MANAGER_ADSCONNECTION_H_

#include <string>
#include <memory>
#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>
#include <google/protobuf/descriptor.h>

#include "../subscription/Subscription.h"
#include "../../local/LocalInfo.h"
#include "anylink_proto/target/envoy/service/discovery/v3/discovery.pb.h"
#include "anylink_proto/target/envoy/service/discovery/v3/ads.grpc.pb.h"
#include "AdsManager.h"
#include <gateway-common/src/thread/Callable.h>

namespace gateway {
    class AbstractLogger;
}

using namespace gateway;

namespace anylink {
class AdsManager;
class AbstractLogger;

class IstioCallable : public gateway::Callable {
private:
    bool receiveADS(envoy::service::discovery::v3::DiscoveryResponse &response,
                    std::shared_ptr<anylink::Subscription> &sub);

    void sendACK(envoy::service::discovery::v3::DiscoveryRequest &request,
                 envoy::service::discovery::v3::DiscoveryResponse response);

    void sendNACK(envoy::service::discovery::v3::DiscoveryRequest &request,
                  envoy::service::discovery::v3::DiscoveryResponse response);

private:
    std::unordered_map<std::string, std::shared_ptr<Subscription>> subscribe_map;
    std::string urlList[4] { "type.googleapis.com/envoy.config.cluster.v3.Cluster", "type.googleapis.com/envoy.config.listener.v3.Listener", "type.googleapis.com/envoy.config.endpoint.v3.ClusterLoadAssignment", "type.googleapis.com/envoy.config.route.v3.RouteConfiguration" };

public:
    IstioCallable(envoy::service::discovery::v3::DiscoveryResponse &response,
                  std::shared_ptr<anylink::Subscription> &sub);

    virtual ~IstioCallable() {};

    virtual std::shared_ptr<void> call();
};


class AdsConnection : public AdsManager {
private:
    std::shared_ptr<AdsManager> adsManager;

public:
    explicit AdsConnection(std::shared_ptr<AdsManager> AdsManager);
    virtual ~AdsConnection() = default;
    std::shared_ptr<AdsConnection> shared_from_this();

public:
    void setLogger(std::shared_ptr<AbstractLogger> logger);

    static std::shared_ptr<AbstractLogger> logger;
};

    std::shared_ptr<void> call();
};

#endif /* ANYLINK_ISTIO_SRC_GRPC_MANAGER_ADSMANAGER_H_ */
