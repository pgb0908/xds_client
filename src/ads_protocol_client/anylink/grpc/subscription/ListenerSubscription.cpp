/*
 * ListenerSubscription.cpp
 *
 *  Created on: 2020. 4. 22.
 *      Author: jeongtaek
 */

#include "ListenerSubscription.h"
#include <gateway-common/src/logger/AbstractLogger.h>

namespace anylink {

    ListenerSubscription::ListenerSubscription() :
            Subscription("type.googleapis.com/envoy.config.listener.v3.Listener") {
    }

    envoy::service::discovery::v3::DiscoveryRequest ListenerSubscription::makeRequestDiscoveryService(envoy::config::core::v3::Node node) {
        envoy::service::discovery::v3::DiscoveryRequest request;
        request.set_type_url(type_url_);
        request.mutable_node()->MergeFrom(node);
        return request;
    }

    void ListenerSubscription::onResponseDiscoveryService(envoy::service::discovery::v3::DiscoveryResponse &response) {
        GATEWAY_LOG(logger, LogLevel::INFO, "[ListenerSubscription] %s", response.DebugString());
        //response.PrintDebugString();
    }

} /* namespace anylink */
