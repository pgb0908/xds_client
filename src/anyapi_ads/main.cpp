//
// Created by bong on 24. 6. 10.
//

#include "AdsClient.h"

int main(){
    std::string endpoint = "127.0.0.1:18000";

    anyapi::AdsClient client(endpoint);

    client.startSubscribe();

    while(1){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        client.detectingQueue();
    }



    return 0;
}