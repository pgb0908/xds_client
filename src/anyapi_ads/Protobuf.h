//
// Created by bong on 24. 5. 24.
//

#ifndef XDS_CLIENT_PROTOBUF_H
#define XDS_CLIENT_PROTOBUF_H

#include <google/protobuf/message.h>

static google::protobuf::Message *createReflectableMessage(const google::protobuf::Message &message) {
    return const_cast<google::protobuf::Message*>(&message);
}


#endif //XDS_CLIENT_PROTOBUF_H
