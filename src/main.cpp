#include <iostream>
#include <thread>

/*
void start_server(){
    greetings_server gs;
    gs.RunServer();
}

int main() {
    std::thread t1 = std::thread(start_server);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    greeting_client gc;
    gc.interativeGRPC();

    return 0;
}*/

#include "../schema/AdsClient.h"
#include "../schema/AsyncAdsClient.h"

int main(){
/*    AdsClient adsClient(grpc::CreateChannel("192.168.56.101:18000",
                                            grpc::InsecureChannelCredentials()));*/
    //adsClient.AggregatedDiscoveryService();

    AsyncAdsClient adsClient(grpc::CreateChannel("192.168.56.101:18000",
                                            grpc::InsecureChannelCredentials()));

    adsClient.do_something();

    return 0;
}