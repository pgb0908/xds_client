//
// Created by bong on 24. 5. 22.
//

#ifndef XDS_CLIENT_HTTPUTILITY_H
#define XDS_CLIENT_HTTPUTILITY_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "envoy/config/core/v3/http_uri.pb.h"
#include "envoy/config/core/v3/protocol.pb.h"
#include "envoy/config/route/v3/route_components.pb.h"


enum UrlComponents {
    UcSchema = 0,
    UcHost = 1,
    UcPort = 2,
    UcPath = 3,
    UcQuery = 4,
    UcFragment = 5,
    UcUserinfo = 6,
    UcMax = 7
};


class PercentEncoding {
public:
    /**
     * Encodes string view to its percent encoded representation. Non-visible ASCII is always escaped,
     * in addition to a given list of reserved chars.
     *
     * @param value supplies string to be encoded.
     * @param reserved_chars list of reserved chars to escape. By default the escaped chars in
     *        https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md#responses are used.
     * @return std::string percent-encoded string.
     */
    static std::string encode(std::string value, std::string reserved_chars = "%");

    /**
     * Decodes string view from its percent encoded representation.
     * @param encoded supplies string to be decoded.
     * @return std::string decoded string https://tools.ietf.org/html/rfc3986#section-2.1.
     */
    static std::string decode(std::string encoded);

    /**
     * Encodes string view for storing it as a query parameter according to the
     * x-www-form-urlencoded spec:
     * https://www.w3.org/TR/html5/forms.html#application/x-www-form-urlencoded-encoding-algorithm
     * @param value supplies string to be encoded.
     * @return std::string encoded string according to
     * https://www.w3.org/TR/html5/forms.html#application/x-www-form-urlencoded-encoding-algorithm
     *
     * Summary:
     * The x-www-form-urlencoded spec mandates that all ASCII codepoints are %-encoded except the
     * following: ALPHA | DIGIT | * | - | . | _
     *
     * NOTE: the space character is encoded as %20, NOT as the + character
     */
    static std::string urlEncodeQueryParameter(std::string value);

    /**
     * Decodes string view that represents URL in x-www-form-urlencoded query parameter.
     * @param encoded supplies string to be decoded.
     * @return std::string decoded string compliant with https://datatracker.ietf.org/doc/html/rfc3986
     *
     * This function decodes a query parameter assuming it is a URL. It only decodes characters
     * permitted in the URL - the unreserved and reserved character sets.
     * unreserved-set := ALPHA | DIGIT | - | . | _ | ~
     * reserved-set := sub-delims | gen-delims
     * sub-delims := ! | $ | & | ` | ( | ) | * | + | , | ; | =
     * gen-delims := : | / | ? | # | [ | ] | @
     *
     * The following characters are not decoded:
     * ASCII controls <= 0x1F, space, DEL (0x7F), extended ASCII > 0x7F
     * As well as the following characters without defined meaning in URL
     * " | < | > | \ | ^ | { | }
     * and the "pipe" `|` character
     */
    static std::string urlDecodeQueryParameter(std::string encoded);

private:
    // Encodes string view to its percent encoded representation, with start index.
    static std::string encode(std::string value, const size_t index,
                              const std::set<char>& reserved_char_set);
};

/**
 * Extract host and path from a URI. The host may contain port.
 * This function doesn't validate if the URI is valid. It only parses the URI with following
 * format: scheme://host/path.
 * @param the input URI string
 * @param the output host string.
 * @param the output path string.
 */
void extractHostPathFromUri(const std::string& uri, std::string& host,
                            std::string& path);



#endif //XDS_CLIENT_HTTPUTILITY_H
