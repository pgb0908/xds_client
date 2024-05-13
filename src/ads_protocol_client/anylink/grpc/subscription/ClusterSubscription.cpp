/*
 * ClusterSubscription.cpp
 *
 *  Created on: 2020. 4. 21.
 *      Author: jeongtaek
 */

#include "ClusterSubscription.h"
#include <gateway-common/src/logger/AbstractLogger.h>

namespace anylink {
    // type.googleapis.com/envoy.config.cluster.v3.Cluster
    ClusterSubscription::ClusterSubscription() : Subscription("type.googleapis.com/envoy.config.cluster.v3.Cluster"){

    }
    envoy::service::discovery::v3::DiscoveryRequest ClusterSubscription::makeRequestDiscoveryService(envoy::config::core::v3::Node node) {
        envoy::service::discovery::v3::DiscoveryRequest request;
        request.set_type_url(type_url_);
        request.mutable_node()->MergeFrom(node);
        return request;
    }

    void ClusterSubscription::onResponseDiscoveryService(envoy::service::discovery::v3::DiscoveryResponse& response) {
        GATEWAY_LOG(logger, LogLevel::INFO, "[ClusterSubscription] %s", response.DebugString());
//        response.PrintDebugString();
    }

} /* namespace anylink */
