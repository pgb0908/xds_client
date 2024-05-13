/*
 * LocalUtil.h
 *
 *  Created on: 2020. 4. 21.
 *      Author: jeongtaek
 */

#ifndef ANYLINK_ISTIO_SRC_LOCAL_UTIL_LOCALUTIL_H_
#define ANYLINK_ISTIO_SRC_LOCAL_UTIL_LOCALUTIL_H_
#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <string>

namespace anylink {

class LocalUtil {
public:
    static std::string getLocalAddress();
};

} /* namespace anylink */

#endif /* ANYLINK_ISTIO_SRC_LOCAL_UTIL_LOCALUTIL_H_ */
