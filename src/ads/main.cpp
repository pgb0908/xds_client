#include <iostream>
#include <thread>

#include "AdsClient.h"

int main(){
    AdsClient adsClient(grpc::CreateChannel("192.168.56.101:18000",
                                            grpc::InsecureChannelCredentials()));
    adsClient.AggregatedDiscoveryService();

    return 0;
}