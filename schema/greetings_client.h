//
// Created by bong on 21. 9. 1..
//

#ifndef GRPC_EXERCISE_GREETINGS_CLIENT_H
#define GRPC_EXERCISE_GREETINGS_CLIENT_H



#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

//#include "proto-src/greetings.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using greetings::HelloRequest;
using greetings::HelloReply;
using greetings::Greeter;

class GreeterClient {
public:
    // Constructor
    GreeterClient(std::shared_ptr<Channel> channel): stub_(Greeter::NewStub(channel)) {}
    // Assembles the client's payload, sends it and presents the response back from the server.
    std::string SayHello(const std::string & user) {
        // Data we are sending to the server.
        HelloRequest request;
        request.set_name(user);

        // Container for the data we expect from the server.
        HelloReply reply;

        // Context for the client.
        // It could be used to convey extra information to the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        Status status = stub_->SayHello(&context, request, &reply);

        // Act upon its status.
        if (status.ok()) {
            return reply.message();
        }
        else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return "gRPC failed";
        }
    }



private:
    std::unique_ptr<Greeter::Stub> stub_;
};

class greeting_client{
public:
    void interativeGRPC();
};

#endif //GRPC_EXERCISE_GREETINGS_CLIENT_H
