//
// Created by bong on 21. 8. 31..
//

#ifndef GRPC_EXERCISE_GREETINGS_SERVER_H
#define GRPC_EXERCISE_GREETINGS_SERVER_H

#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
//#include "proto-src/greetings.grpc.pb.h"


// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public greetings::Greeter::Service {
    grpc::Status SayHello(grpc::ServerContext * context, const greetings::HelloRequest * request, greetings::HelloReply * reply) override {
        std::string prefix("Hello ");
        reply->set_message(prefix + request->name() + "!");
        return grpc::Status::OK;
    }
};

class greetings_server {
public:
    greetings_server() {
        server_address = "0.0.0.0:50051";
    };
    void RunServer();

private:
    std::string server_address;
};


#endif //GRPC_EXERCISE_GREETINGS_SERVER_H
