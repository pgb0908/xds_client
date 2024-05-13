/*
 * EndpointSubscription.h
 *
 *  Created on: 2020. 4. 29.
 *      Author: jeongtaek
 */

#ifndef ANYLINK_ISTIO_SRC_GRPC_SUBSCRIPTION_ENDPOINTSUBSCRIPTION_H_
#define ANYLINK_ISTIO_SRC_GRPC_SUBSCRIPTION_ENDPOINTSUBSCRIPTION_H_
#include "Subscription.h"

namespace anylink {

    class EndpointSubscription: public Subscription {
    public:
        EndpointSubscription();
        ~EndpointSubscription() override = default;
    public:
        envoy::service::discovery::v3::DiscoveryRequest makeRequestDiscoveryService(envoy::config::core::v3::Node node) override;
        void onResponseDiscoveryService(envoy::service::discovery::v3::DiscoveryResponse &response) override;
    };

} /* namespace anylink */

#endif /* ANYLINK_ISTIO_SRC_GRPC_SUBSCRIPTION_ENDPOINTSUBSCRIPTION_H_ */
