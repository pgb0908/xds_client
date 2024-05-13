/*
 * GrpcManager.cpp
 *
 *  Created on: 2020. 4. 21.
 *      Author: yejin
 */

#include "AdsManager.h"

#include <string>
#include <thread>
#include <memory>
#include <iostream>
#include <grpc/support/log.h>

#include "../../local/LocalInfo.h"
#include "../../local/util/LocalUtil.h"
#include "../subscription/Subscription.h"
#include "AdsConnection.h"
#include <gateway-common/src/logger/AbstractLogger.h>
#include <gateway-common/src/thread/GatewayThreadPool.h>

namespace anylink {

std::shared_ptr<AbstractLogger> AdsConnection::logger = nullptr;


bool IstioCallable::receiveADS(envoy::service::discovery::v3::DiscoveryResponse &response,
                    std::shared_ptr<anylink::Subscription> &sub) {
    return false; //TODO implement
}

void IstioCallable::sendACK(envoy::service::discovery::v3::DiscoveryRequest &request,
                 envoy::service::discovery::v3::DiscoveryResponse response) {
    return; //TODO implement
}

void IstioCallable::sendNACK(envoy::service::discovery::v3::DiscoveryRequest &request,
                  envoy::service::discovery::v3::DiscoveryResponse response) {
    return; //TODO implement
}


std::shared_ptr<void> IstioCallable::call() {
    thread_local static std::unique_ptr<AdsManager> istio;

    int i = 0;
    envoy::service::discovery::v3::DiscoveryResponse responseArray[4];
    envoy::service::discovery::v3::DiscoveryRequest requestArray[4];
    while(true){
        std::cout << "Started Istio Worker Thread" << std::endl;
        for (auto &url : urlList) {
            auto &sub = subscribe_map[url];
            GATEWAY_LOG(Subscription::logger, LogLevel::INFO, "[AdsManager] initSubscribe %s", sub->type_url_);
            std::cout << "while receiveADS before" << std::endl;
            receiveADS(responseArray[i],sub);
            std::cout <<"while receiveADS after" << std::endl;
            sendACK(requestArray[i], responseArray[i]);
            std::cout <<"while sendACK after" << std::endl;
            i++;
        }
        i = 0;
    }
}

} /* namespace anylink */
