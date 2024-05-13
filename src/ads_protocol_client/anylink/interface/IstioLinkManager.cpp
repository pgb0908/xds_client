/*
 * IstioLinkManager.cpp
 *
 *  Created on: 2020. 4. 28.
 *      Author: jeongtaek
 */

#include <memory>
#include <gateway-common/src/logger/AbstractLogger.h>
#include "IstioLinkManager.h"
#include "../grpc/manager/AdsManager.h"
#include "../grpc/subscription/ClusterSubscription.h"
#include "../grpc/subscription/EndpointSubscription.h"
#include "../grpc/subscription/ListenerSubscription.h"
#include "../grpc/subscription/RouteSubscription.h"


namespace anylink {

std::shared_ptr<gateway::AbstractLogger> IstioLinkManager::logger = nullptr;
std::shared_ptr<IstioLinkManager> IstioLinkManager::instance = nullptr;

void IstioLinkManager::init(std::string nodeId, std::shared_ptr<gateway::AbstractLogger> logger_) {
    GATEWAY_LOG(logger_, LogLevel::INFO, "[IstioLinkManager] init");
    if (instance == nullptr) {
        std::string path = getenv("ISTIO_ADDRESS") == nullptr ? "istiod.istio-system.svc.cluster.local:15010" : getenv("ISTIO_ADDRESS");
        instance = std::shared_ptr<IstioLinkManager>(new IstioLinkManager(path,nodeId));
    }
    if (logger_ == nullptr) {
        logger = std::make_shared<gateway::AbstractLogger>(); //아무 동작도 안함
    } else {
        logger = logger_;
        instance->adsManager->setLogger(logger);
    }
    GATEWAY_LOG(logger, LogLevel::INFO, "[IstioLinkManager] init");
    std::shared_ptr<ClusterSubscription> clusterSub = std::make_shared<ClusterSubscription>();
    instance->adsManager->addSubscription(clusterSub);
    std::shared_ptr<ListenerSubscription> listenerSub = std::make_shared<ListenerSubscription>();
    instance->adsManager->addSubscription(listenerSub);
    std::shared_ptr<EndpointSubscription> endpointSub = std::make_shared<EndpointSubscription>();
    instance->adsManager->addSubscription(endpointSub);
    std::shared_ptr<RouteSubscription> routeSub = std::make_shared<RouteSubscription>();
    instance->adsManager->addSubscription(routeSub);
}

void IstioLinkManager::init(std::string pilotAddr, std::string nodeId, std::shared_ptr<AbstractLogger> logger_) {
    GATEWAY_LOG(logger_, LogLevel::INFO, "[IstioLinkManager] init");

    if (instance == nullptr) {
        instance = std::shared_ptr<IstioLinkManager>(new IstioLinkManager(pilotAddr, nodeId));
    }
    if (logger_ == nullptr) {
        logger = std::make_shared<AbstractLogger>(); //아무 동작도 안함
    } else {
        logger = logger_;
        instance->adsManager->setLogger(logger);
    }

    std::shared_ptr<ClusterSubscription> clusterSub = std::make_shared<ClusterSubscription>();
    instance->adsManager->addSubscription(clusterSub);
    std::shared_ptr<ListenerSubscription> listenerSub = std::make_shared<ListenerSubscription>();
    instance->adsManager->addSubscription(listenerSub);
    std::shared_ptr<EndpointSubscription> endpointSub = std::make_shared<EndpointSubscription>();
    instance->adsManager->addSubscription(endpointSub);
    std::shared_ptr<RouteSubscription> routeSub = std::make_shared<RouteSubscription>();
    instance->adsManager->addSubscription(routeSub);
}

std::shared_ptr<IstioLinkManager> IstioLinkManager::getIstioLinkManager() {
    return instance;
}


IstioLinkManager::IstioLinkManager(std::string pilotAddr, std::string nodeId) :
        pilotAddr_(pilotAddr), nodeId_(nodeId) {
    adsManager = std::make_shared<AdsManager>(pilotAddr, nodeId);
    logger = std::make_shared<AbstractLogger>(); //아무 동작도 안함
}

void IstioLinkManager::startLink() {
    GATEWAY_LOG(logger, LogLevel::INFO, "[IstioLinkManager] startLink");
    adsManager->startClient();
}

} /* namespace anylink */
