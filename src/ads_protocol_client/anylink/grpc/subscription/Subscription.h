/*
 * subscription.h
 *
 *  Created on: 2020. 4. 21.
 *      Author: jeongtaek
 */

#ifndef ANYLINK_ISTIO_SRC_GRPC_SUBSCRIPTION_SUBSCRIPTION_H_
#define ANYLINK_ISTIO_SRC_GRPC_SUBSCRIPTION_SUBSCRIPTION_H_

#include <string>
#include <memory>
#include <anylink_proto/target/envoy/config/core/v3/base.pb.h>
#include <anylink_proto/target/envoy/service/discovery/v3/discovery.pb.h>



namespace gateway {
class AbstractLogger;
}

using namespace gateway;

namespace anylink {
class Subscription {
public:
    Subscription(std::string type_url) :
            type_url_(type_url) {
    }
    ;
    virtual ~Subscription() = default;
    virtual envoy::service::discovery::v3::DiscoveryRequest makeRequestDiscoveryService(
            envoy::config::core::v3::Node node) = 0;
    virtual void onResponseDiscoveryService(envoy::service::discovery::v3::DiscoveryResponse &response)=0;
    std::string& getTypeUrl() {
        return type_url_;
    }
    ;
public:
    void setLogger(std::shared_ptr<AbstractLogger> logger);
    std::string type_url_;
    static std::shared_ptr<AbstractLogger> logger;
};
}
#endif /* ANYLINK_ISTIO_SRC_GRPC_SUBSCRIPTION_SUBSCRIPTION_H_ */
