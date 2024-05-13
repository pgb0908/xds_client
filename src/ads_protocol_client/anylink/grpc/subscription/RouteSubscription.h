/*
 * RouteSubscription.h
 *
 *  Created on: 2020. 4. 29.
 *      Author: jeongtaek
 */

#ifndef ANYLINK_ISTIO_SRC_GRPC_SUBSCRIPTION_ROUTESUBSCRIPTION_H_
#define ANYLINK_ISTIO_SRC_GRPC_SUBSCRIPTION_ROUTESUBSCRIPTION_H_
#include "Subscription.h"

namespace anylink {

class RouteSubscription: public Subscription {
public:
    RouteSubscription();
    ~RouteSubscription() override = default;
public:
    envoy::service::discovery::v3::DiscoveryRequest makeRequestDiscoveryService(envoy::config::core::v3::Node node) override;
    void onResponseDiscoveryService(envoy::service::discovery::v3::DiscoveryResponse &response) override;
};

} /* namespace anylink */

#endif /* ANYLINK_ISTIO_SRC_GRPC_SUBSCRIPTION_ROUTESUBSCRIPTION_H_ */
