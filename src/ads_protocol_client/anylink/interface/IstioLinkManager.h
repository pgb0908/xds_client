/*
 * IstioLinkManager.h
 *
 *  Created on: 2020. 4. 28.
 *      Author: jeongtaek
 */

#ifndef ANYLINK_ISTIO_SRC_INTERFACE_ISTIOLINKMANAGER_H_
#define ANYLINK_ISTIO_SRC_INTERFACE_ISTIOLINKMANAGER_H_

#include "../grpc/manager/AdsManager.h"
#include <memory>

namespace gateway {
class AbstractLogger;
}


namespace anylink {

class IstioLinkManager {
private:
    static std::shared_ptr<gateway::AbstractLogger> logger;
    static std::shared_ptr<IstioLinkManager> instance;
public:
    IstioLinkManager(std::string pilotAddr, std::string nodeId);
    virtual ~IstioLinkManager() = default;
public:
    static void init(std::string pilotAddr, std::string nodeId, std::shared_ptr<AbstractLogger> logger);
    static void init(std::string nodeId, std::shared_ptr<AbstractLogger> logger);
    static std::shared_ptr<IstioLinkManager> getIstioLinkManager();
    void startLink();
private:
    std::string pilotAddr_;
    std::string nodeId_;
    std::shared_ptr<AdsManager> adsManager;
};

} /* namespace anylink */

#endif /* ANYLINK_ISTIO_SRC_INTERFACE_ISTIOLINKMANAGER_H_ */
