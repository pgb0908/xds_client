//
// Created by bong on 24. 6. 13.
//
#include "EdsResource.h"

namespace anyapi {

    void EdsResource::setResource(std::string &resource_name,
                                  const envoy::config::endpoint::v3::ClusterLoadAssignment &resource) {
        resources_map_.emplace(resource_name, ResourceData(resource));
    }

    void EdsResource::removeResource(std::string &resource_name) {
        resources_map_.erase(resource_name);
    }


}

