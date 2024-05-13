/*
 * LocalInfo.h
 *
 *  Created on: 2020. 4. 20.
 *      Author: jeongtaek
 */

#ifndef ANYLINK_ISTIO_SRC_GRPC_LOCALINFO_H_
#define ANYLINK_ISTIO_SRC_GRPC_LOCALINFO_H_
#include <anylink_proto/target/envoy/config/core/v3/base.pb.h>

namespace anylink {

class LocalInfo {
public:
    LocalInfo(std::string address, std::string nodeId) :
            address_(address) {
        if (!nodeId.empty()) {
            node_.set_id(nodeId);
        }
    }
    LocalInfo(std::string address, std::string clusterName, std::string nodeId) :
            address_(address) {

        std::string fullNodeId = "sidecar~"+address.substr(0,address.find(":"))+"~"+nodeId+"~"+"testDNS";
        if (!clusterName.empty()) {
            node_.set_cluster(std::string(clusterName));
        }
        if (!nodeId.empty()) {
            node_.set_id(std::string(fullNodeId));
        }
        node_.set_user_agent_name(std::string("anylinkLibrary"));

        auto metaData = node_.mutable_metadata()->mutable_fields();
        google::protobuf::Value v;
        v.set_string_value("Kubernetes");
        (*metaData)["CLUSTER_ID"] = v;
        v.set_string_value("default");
        (*metaData)["CONFIG_NAMESPACE"] = v;
        v.set_string_value("1.5.1");
        (*metaData)["ISTIO_VERSION"] = v;
        v.set_string_value("test");
        (*metaData)["NAME"] = v;
        v.set_string_value("default");
        (*metaData)["NAMESPACE"] = v;
        v.set_string_value("test");
        (*metaData)["POD_NAME"] = v;
    }
public:
    const std::string& address() {
        return address_;
    }
    ;
    const std::string& zoneName() {
        return node_.locality().zone();
    }
    const std::string& clusterName() {
        return node_.cluster();
    }
    const std::string& nodeId() {
        return node_.id();
    }
    envoy::config::core::v3::Node node() {
        return node_;
    }

private:
    envoy::config::core::v3::Node node_;
    std::string address_;
};

} /* namespace anylink */

#endif /* ANYLINK_ISTIO_SRC_GRPC_LOCALINFO_H_ */
