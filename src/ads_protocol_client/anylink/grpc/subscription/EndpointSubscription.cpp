/*
 * EndpointSubscription.cpp
 *
 *  Created on: 2020. 4. 29.
 *      Author: jeongtaek
 */

#include "EndpointSubscription.h"
#include <gateway-common/src/logger/AbstractLogger.h>

namespace anylink {

    EndpointSubscription::EndpointSubscription() : Subscription("type.googleapis.com/envoy.config.endpoint.v3.ClusterLoadAssignment"){
        // TODO Auto-generated constructor stub

    }
    envoy::service::discovery::v3::DiscoveryRequest EndpointSubscription::makeRequestDiscoveryService(envoy::config::core::v3::Node node) {
        envoy::service::discovery::v3::DiscoveryRequest request;
        request.set_type_url(type_url_);
        // FIXME: this is sample resource_names. it should be service_name(CDS response field) list
        request.add_resource_names("outbound|80||gateway-test.default.svc.cluster.local");
        request.mutable_node()->MergeFrom(node);
        return request;
    }

    void EndpointSubscription::onResponseDiscoveryService(envoy::service::discovery::v3::DiscoveryResponse& response) {
        GATEWAY_LOG(logger, LogLevel::INFO, "[EndpointSubscription] %s", response.DebugString());
        //response.PrintDebugString();
    }

} /* namespace anylink */
