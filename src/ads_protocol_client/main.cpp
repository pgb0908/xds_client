#include <iostream>
#include <thread>

#include "AdsClient.h"
#include "Config.h"

int main(){
    Config config("192.168.56.101:18000");

    auto client = config.build();
    client->setChannel(grpc::CreateChannel("192.168.56.101:18000",
                                           grpc::InsecureChannelCredentials()));

    client->onStreamEstablished();



    //client->AggregatedDiscoveryService();
    //grpc::CreateInsecureChannelFromFd();

    client->shutdown();

    return 0;
}