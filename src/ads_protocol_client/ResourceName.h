//
// Created by bont on 24. 5. 16.
//

#ifndef XDS_CLIENT_RESOURCENAME_H
#define XDS_CLIENT_RESOURCENAME_H

#include <string>

/**
 * Get resource name from api type.
 */
template <typename Current>
std::string getResourceName() {
  //  return createReflectableMessage(Current())->GetDescriptor()->full_name();
  return "";
}

/**
 * Get type url from api type.
 */
template <typename Current>
std::string getTypeUrl() {
    return "type.googleapis.com/" + getResourceName<Current>();
}

#endif //XDS_CLIENT_RESOURCENAME_H
