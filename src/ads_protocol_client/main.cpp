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

    auto temp_decoder = std::make_shared<OpaqueResourceDecoderImpl<envoy::config::cluster::v3::Cluster>>("hello");
    std::shared_ptr<OpaqueResourceDecoder> cluster_decoder = temp_decoder;

    std::set<std::string> empty_list;
    std::string type_url = "type.googleapis.com/envoy.config.cluster.v3.Cluster";
    auto watcher = client->addWatch(type_url,
                     empty_list, cluster_decoder);
    client->apiStateFor(type_url).watches_.push_back(watcher.get());

    client->onStreamEstablished();

    //client->AggregatedDiscoveryService();
    //grpc::CreateInsecureChannelFromFd();

    client->shutdown();

    return 0;
}