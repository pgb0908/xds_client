//
// Created by bont on 24. 5. 16.
//

#ifndef XDS_CLIENT_WATCHER_H
#define XDS_CLIENT_WATCHER_H

#include <set>
#include "AdsClient.h"
#include "OpaqueResourceDecoder.h"

class Watcher {
public:
    Watcher(const std::set<std::string> &resources,
            OpaqueResourceDecoderSharedPtr resource_decoder,
            const std::string &type_url,
            AdsClient &parent)
            : type_url_(type_url),
              resource_decoder_(resource_decoder),
              parent_(parent),
              watches_(parent.apiStateFor(type_url).watches_)
    //eds_resources_cache_(eds_resources_cache)
    {
       // updateResources(resources);
    }

    ~Watcher() {
        watches_.erase(iter_);
        if (!resources_.empty()) {
            parent_.queueDiscoveryRequest(type_url_);
/*            if (eds_resources_cache_.has_value()) {
                removeResourcesFromCache(resources_);
            }*/
        }
    }

    void update(const std::set<std::string> &resources) {
        watches_.erase(iter_);
        if (!resources_.empty()) {
            parent_.queueDiscoveryRequest(type_url_);
        }
        updateResources(resources);
        parent_.queueDiscoveryRequest(type_url_);
    }

    // Maintain deterministic wire ordering via ordered std::set.
    std::set<std::string> resources_;
    const std::string type_url_;
    AdsClient &parent_;
    using WatchList = std::list<Watcher *>;
    OpaqueResourceDecoderSharedPtr resource_decoder_;
    WatchList &watches_;
    WatchList::iterator iter_;

private:
    void updateResources(const std::set<std::string> &resources) {
        // 함수 끝까지 현재 리소스 세트를 유지하고 차이를 계산하여 제거된 리소스 목록을 찾는 방법입니다.
        // 업데이트 이전에 리소스를 일시적으로 유지하여 제거된 리소스를 찾습니다.
/*        std::set<std::string> previous_resources;
        previous_resources.swap(resources_);
        std::transform(
                resources.begin(), resources.end(), std::inserter(resources_, resources_.begin()),
                [this](const std::string &resource_name) -> std::string {

                    *//*                  if (XdsResourceIdentifier::hasXdsTpScheme(resource_name)) {
                                          auto xdstp_resource_or_error = XdsResourceIdentifier::decodeUrn(resource_name);
                                          THROW_IF_STATUS_NOT_OK(xdstp_resource_or_error, throw);
                                          auto xdstp_resource = xdstp_resource_or_error.value();
                                          if (subscription_options_.add_xdstp_node_context_params_) {
                                              const auto context = XdsContextParams::encodeResource(
                                                      local_info_.contextProvider().nodeContext(), xdstp_resource.context(), {}, {});
                                              xdstp_resource.mutable_context()->CopyFrom(context);
                                          }
                                          XdsResourceIdentifier::EncodeOptions encode_options;
                                          encode_options.sort_context_params_ = true;
                                          return XdsResourceIdentifier::encodeUrn(xdstp_resource, encode_options);
                                      }*//*
                    return resource_name;
                });*/
/*        if (eds_resources_cache_.has_value()) {
            // Compute the removed resources and remove them from the cache.
            std::set<std::string> removed_resources;
            std::set_difference(previous_resources.begin(), previous_resources.end(),
                                resources_.begin(), resources_.end(),
                                std::inserter(removed_resources, removed_resources.begin()));
            removeResourcesFromCache(removed_resources);
        }*/
        // move this watch to the beginning of the list
        watches_.push_front(this);
        iter_ = watches_.begin();
    }

    void removeResourcesFromCache(const std::set<std::string> &resources_to_remove) {
        //ASSERT(eds_resources_cache_.has_value());
        // Iterate over the resources to remove, and if no other watcher
        // registered for that resource, remove it from the cache.
/*        for (const auto& resource_name : resources_to_remove) {
            // Counts the number of watchers that watch the resource.
            uint32_t resource_watchers_count = 0;
            for (const auto& watch : watches_) {
                // Skip the current watcher as it is intending to remove the resource.
                if (watch == this) {
                    continue;
                }
                if (watch->resources_.find(resource_name) != watch->resources_.end()) {
                    resource_watchers_count++;
                }
            }
            // Other than "this" watcher, the resource is not watched by any other
            // watcher, so it can be removed.
            if (resource_watchers_count == 0) {
                eds_resources_cache_->removeResource(resource_name);
            }
        }*/
    }


};


#endif //XDS_CLIENT_WATCHER_H
