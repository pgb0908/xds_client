//
// Created by bong on 24. 6. 10.
//

#include "AdsClient.h"

int main(){
    std::string endpoint = "127.0.0.1:18000";

    anyapi::AdsClient client(endpoint);

    client.startSubscribe();

    client.detectingQueue();


    return 0;
}