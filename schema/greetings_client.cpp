//
// Created by bong on 21. 9. 1..
//

#include "greetings_client.h"

void greeting_client::interativeGRPC() {
    // Instantiate the client. It requires a channel, out of which the actual RPCs are created.
    // This channel models a connection to an endpoint (in this case, localhost at port 50051).
    // We indicate that the channel isn't authenticated (use of InsecureChannelCredentials()).
    GreeterClient greeter(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    while (true) {
        std::string user;
        std::cout << "Please enter your user name:" << std::endl;
        // std::cin >> user;
        std::getline(std::cin, user);
        std::string reply = greeter.SayHello(user);
        if (reply == "gRPC failed") {
            std::cout << "gRPC failed" << std::endl;
        }
        std::cout << "gRPC returned: " << std::endl;
        std::cout << reply << std::endl;
    }
}
