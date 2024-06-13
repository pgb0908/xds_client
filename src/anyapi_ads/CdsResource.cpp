//
// Created by bong on 24. 6. 13.
//

#include "CdsResource.h"

namespace anyapi {


    void CdsResource::setResource(std::string &resource_name, const envoy::config::cluster::v3::Cluster &resource) {
        resources_map_.emplace(resource_name, ResourceData(resource));
    }

    void CdsResource::removeResource(std::string &resource_name) {
        resources_map_.erase(resource_name);
    }
}