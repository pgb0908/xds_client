//
// Created by bong on 24. 6. 13.
//

#ifndef XDS_CLIENT_RESOURCECACHE_H
#define XDS_CLIENT_RESOURCECACHE_H

#include <string>
#include "envoy/config/endpoint/v3/endpoint.pb.h"
#include "envoy/config/cluster/v3/cluster.pb.h"

namespace anyapi{

    template<typename f>
    class ResourceCache {
    public:
        ResourceCache() = default;
        void setResource(std::string& resource_name,
                         const f& resource){
            resources_map_.emplace(resource_name, ResourceData(resource));
        }
        void removeResource(std::string& resource_name){
            resources_map_.erase(resource_name);
        }

    private:
        struct ResourceData {f resource_;
            ResourceData(const f& resource)
                    : resource_(resource) {}
        };
        // A map between a resource name and its ResourceData.
        std::map<std::string, ResourceData> resources_map_;
    };
}




#endif //XDS_CLIENT_RESOURCECACHE_H
