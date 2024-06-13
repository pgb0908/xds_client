//
// Created by bong on 24. 6. 13.
//

#ifndef XDS_CLIENT_CDSRESOURCE_H
#define XDS_CLIENT_CDSRESOURCE_H

#include <string>
#include "envoy/config/cluster/v3/cluster.pb.h"

namespace anyapi{
    class CdsResource {
    public:
        CdsResource() = default;
        void setResource(std::string& resource_name,
                         const envoy::config::cluster::v3::Cluster& resource);
        void removeResource(std::string& resource_name);

    private:
        struct ResourceData {
            envoy::config::cluster::v3::Cluster resource_;

            ResourceData(const envoy::config::cluster::v3::Cluster& resource)
                    : resource_(resource) {}
        };
        // A map between a resource name and its ResourceData.
        std::map<std::string, ResourceData> resources_map_;
    };
}



#endif //XDS_CLIENT_CDSRESOURCE_H
