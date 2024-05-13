/*
 * RouteSubscription.cpp
 *
 *  Created on: 2020. 4. 29.
 *      Author: jeongtaek
 */

#include "RouteSubscription.h"
#include <gateway-common/src/logger/AbstractLogger.h>

namespace anylink {

RouteSubscription::RouteSubscription() : Subscription("type.googleapis.com/envoy.config.route.v3.RouteConfiguration"){
    // TODO Auto-generated constructor stub

}
envoy::service::discovery::v3::DiscoveryRequest RouteSubscription::makeRequestDiscoveryService(envoy::config::core::v3::Node node) {
    envoy::service::discovery::v3::DiscoveryRequest request;
    request.set_type_url(type_url_);
    // FIXME: this is sample resource_names. it should be config_route_name(LDS response field) list
    request.add_resource_names("80");
    request.mutable_node()->MergeFrom(node);
    return request;
}

void RouteSubscription::onResponseDiscoveryService(envoy::service::discovery::v3::DiscoveryResponse &response) {
    GATEWAY_LOG(logger, LogLevel::INFO, "[RouterSubscription] %s", response.DebugString());
    //response.PrintDebugString();
}
} /* namespace anylink */
