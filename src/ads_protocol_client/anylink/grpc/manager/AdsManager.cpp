/*
 * GrpcManager.cpp
 *
 *  Created on: 2020. 4. 21.
 *      Author: jeongtaek
 */

#include "AdsManager.h"

#include <string>
#include <thread>
#include <memory>
#include <iostream>
#include <grpc/support/log.h>

#include "../../local/LocalInfo.h"
#include "../../local/util/LocalUtil.h"
#include <gateway-common/src/logger/AbstractLogger.h>
#include <gateway-common/src/thread/GatewayThreadPool.h>

namespace anylink {

std::shared_ptr<AbstractLogger> Subscription::logger = nullptr;
std::shared_ptr<AbstractLogger> AdsManager::logger = nullptr;

AdsManager::AdsManager(std::string pilotUrl, std::string nodeId) :
        pilotUrl_(pilotUrl), localInfo_(std::unique_ptr<LocalInfo>(new LocalInfo(LocalUtil::getLocalAddress(), "????????????", nodeId))) {

    if (logger == nullptr) {
        logger = std::make_shared<AbstractLogger>(); //아무 동작도 안함
    }
}

void AdsManager::setLogger(std::shared_ptr<AbstractLogger> logger_) {
    if (logger_ == nullptr) {
        logger = std::make_shared<AbstractLogger>(); //아무 동작도 안함
    } else {
        logger = logger_;
    }
}

void Subscription::setLogger(std::shared_ptr<AbstractLogger> logger_) {
    if (logger_ == nullptr) {
        logger = std::make_shared<AbstractLogger>(); //아무 동작도 안함
    } else {
        logger = logger_;
    }
}

void AdsManager::startClient() {
    GATEWAY_LOG(logger, LogLevel::INFO, "[AdsManager] StartClient Node[%s]", getNode().DebugString());
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(pilotUrl_, grpc::InsecureChannelCredentials());
    stub_ = std::move(envoy::service::discovery::v3::AggregatedDiscoveryService::NewStub(channel));
    context_.set_authority("anylink.system");
    rpc = std::move(stub_->PrepareAsyncStreamAggregatedResources(&context_, &cq_));
    std::cout <<"startcall before" << std::endl;
    startcall();
    std::cout <<"startcall after" << std::endl;

    envoy::service::discovery::v3::DiscoveryResponse responseArray[4];
    envoy::service::discovery::v3::DiscoveryRequest requestArray[4];
    int index=0;
    for (auto &url : urlList) {
        auto &sub = subscribe_map[url];
        envoy::service::discovery::v3::DiscoveryRequest request;
        envoy::service::discovery::v3::DiscoveryResponse response;
        std::cout <<"initSubscribe before log" << std::endl;
        GATEWAY_LOG(logger, LogLevel::INFO, "[AdsManager] initSubscribe %s", sub->type_url_);
        std::cout <<"initSubscribe before" << std::endl;
        initSubscribe(request, sub);
        std::cout <<"initSubscribe after" << std::endl;
        receiveADS(response,sub);
        std::cout <<"receiveADS after" << std::endl;
        sendACK(request, response);
        std::cout <<"sendAck after" << std::endl;
        requestArray[index] = request;
        responseArray[index++] = response;
    }

    //istio스레드는 여기서 불리면안됨(gateway-core에 종속적인 라이브러리면 안됨)
    //GatewayManager::getManager()->getThreadPool()->createDetachThread("istioThread_");


}
void AdsManager::addSubscription(const std::shared_ptr<Subscription> &sub) {
    GATEWAY_LOG(logger, LogLevel::INFO, "[AdsManager] Add Subsription Type_uri[%s]", sub->getTypeUrl());
    // type.googleapis.com/envoy.config.cluster.v3.Cluster
   // std::cout << "getTypeURL!!!" << sub->getTypeUrl() << "sub!!!!:::" << sub << std::endl;
    subscribe_map.insert(std::make_pair(sub->getTypeUrl(), sub));
}

void AdsManager::startcall() {
    void *init_tag;
    bool ok = false;
    rpc->StartCall(&init_tag);
    cq_.Next(&init_tag, &ok);
    if (init_tag && ok == true) {
        GATEWAY_LOG(logger, LogLevel::INFO, "GrpcClient init_tag() success %s", (void* )init_tag);
        std::cout <<"!!--------------------------------!" << std::endl;
    } else {
        GATEWAY_LOG(logger, LogLevel::INFO, "GrpcClient init_tag() failed %s", (void* )init_tag);
    }
}
void AdsManager::initSubscribe(envoy::service::discovery::v3::DiscoveryRequest &request,
                                   std::shared_ptr<Subscription> &sub) {
    sub->setLogger(logger);

    void *write_tag;
    bool ok = false;
    request = sub->makeRequestDiscoveryService(getNode());
    rpc->Write(request, &write_tag);
    cq_.Next(&write_tag, &ok);
    std::cout << "write_tag: " << write_tag << std::endl;
    std::cout << "ok :" << ok << std::endl;
    if (ok == true) {
        GATEWAY_LOG(logger, LogLevel::INFO, "GrpcClient write_tag() success %s", (void* )write_tag);
    } else {
        GATEWAY_LOG(logger, LogLevel::INFO, "GrpcClient write_tag() failed %s", (void* )write_tag);
    }
}

bool AdsManager::receiveADS(envoy::service::discovery::v3::DiscoveryResponse &response, std::shared_ptr<Subscription> &sub) {
    void *read_tag;
    bool ok = false;
    rpc->Read(&response, &read_tag);
    cq_.Next(&read_tag, &ok);
    if (ok == true) {
        GATEWAY_LOG(logger, LogLevel::INFO, "GrpcClient read_tag() success %s", (void* )read_tag);
        subscribe_map[response.type_url()]->onResponseDiscoveryService(response);
    } else {
        GATEWAY_LOG(logger, LogLevel::INFO, "GrpcClient read_tag() failed %s", (void* )read_tag);
    }
    return false;
}

void AdsManager::sendACK(envoy::service::discovery::v3::DiscoveryRequest &request,
                             envoy::service::discovery::v3::DiscoveryResponse response) {
    void *write_tag;
    bool ok = false;
    request.set_version_info(response.version_info());
    request.set_response_nonce(response.nonce());
    rpc->Write(request, &write_tag);
    cq_.Next(&write_tag, &ok);
    if (ok == true) {
        GATEWAY_LOG(logger, LogLevel::INFO, "GrpcClient sendACK() success %s", (void* )write_tag);
    } else {
        GATEWAY_LOG(logger, LogLevel::INFO, "GrpcClient sendACK() failed %s", (void* )write_tag);
    }
}
void AdsManager::sendNACK(envoy::service::discovery::v3::DiscoveryRequest &request,
                              envoy::service::discovery::v3::DiscoveryResponse response) {
    void *write_tag;
    bool ok = false;
    request.set_response_nonce(response.nonce());
    rpc->Write(request, &write_tag);
    cq_.Next(&write_tag, &ok);
    if (ok == true) {
        GATEWAY_LOG(logger, LogLevel::INFO, "GrpcClient sendNACK() success %s", (void* )write_tag);
    } else {
        GATEWAY_LOG(logger, LogLevel::INFO, "GrpcClient sendNACK() failed %s", (void* )write_tag);
    }
}

envoy::config::core::v3::Node AdsManager::getNode() {
    return localInfo_->node();
}
std::unordered_map<std::string, std::shared_ptr<Subscription>>& AdsManager::getSubscribeMap() {
    return subscribe_map;
}




} /* namespace anylink */
