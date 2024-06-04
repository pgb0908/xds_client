#include <iostream>
#include <thread>

#include "AdsClient.h"
#include "Config.h"
#include "Watcher.h"

int main(){
    std::string endpoint = "127.0.0.1:18000";

    Config config(endpoint);

    auto client = config.build();
    client->setChannel(grpc::CreateChannel(endpoint,
                                           grpc::InsecureChannelCredentials()));
    client->onStreamEstablished();



    auto temp_decoder = std::make_shared<OpaqueResourceDecoderImpl<envoy::config::cluster::v3::Cluster>>("name");
    std::shared_ptr<OpaqueResourceDecoder> cluster_decoder = temp_decoder;
    std::set<std::string> empty_list;
    std::string type_url = "type.googleapis.com/envoy.config.cluster.v3.Cluster";
    auto watcher = client->addWatch(type_url,
                     empty_list, cluster_decoder);
    client->apiStateFor(type_url).watches_.push_back(watcher.get());


    auto temp_decoder2 = std::make_shared<OpaqueResourceDecoderImpl<envoy::config::listener::v3::Listener>>("listener");
    std::shared_ptr<OpaqueResourceDecoder> listener_decoder = temp_decoder2;
    std::set<std::string> empty_list2;
    std::string type_url2 = "type.googleapis.com/envoy.config.listener.v3.Listener";
    auto watcher2 = client->addWatch(type_url2,
                                     empty_list2, listener_decoder);
    client->apiStateFor(type_url2).watches_.push_back(watcher2.get());


    //client->AggregatedDiscoveryService();
    //grpc::CreateInsecureChannelFromFd();

    std::this_thread::sleep_for(std::chrono::seconds(20));

    client->shutdown();

    return 0;
}