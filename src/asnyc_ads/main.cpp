#include <iostream>
#include <thread>
#include "../ads/AdsClient.h"
#include "AsyncAdsClient.h"

int main(){

    AsyncAdsClient adsClient(grpc::CreateChannel("192.168.56.101:18000",
                                            grpc::InsecureChannelCredentials()));

    adsClient.do_something();

    return 0;
}