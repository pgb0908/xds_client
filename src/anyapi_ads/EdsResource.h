//
// Created by bong on 24. 6. 13.
//

#ifndef XDS_CLIENT_EDSRESOURCE_H
#define XDS_CLIENT_EDSRESOURCE_H
#include <string>
#include "envoy/config/endpoint/v3/endpoint.pb.h"

namespace anyapi {
    class EdsResource {
    public:
        EdsResource() = default;
        void setResource(std::string& resource_name,
                         const envoy::config::endpoint::v3::ClusterLoadAssignment& resource);
        void removeResource(std::string& resource_name);

    private:
        struct ResourceData {envoy::config::endpoint::v3::ClusterLoadAssignment resource_;
            ResourceData(const envoy::config::endpoint::v3::ClusterLoadAssignment& resource)
                    : resource_(resource) {}
        };
        // A map between a resource name and its ResourceData.
        std::map<std::string, ResourceData> resources_map_;
    };

}



#endif //XDS_CLIENT_EDSRESOURCE_H
