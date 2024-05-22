//
// Created by bong on 24. 5. 22.
//

#include "HttpUtility.h"



std::string PercentEncoding::encode(std::string value,
                                             std::string reserved_chars) {
    std::set<char> reserved_char_set{reserved_chars.begin(), reserved_chars.end()};
    for (size_t i = 0; i < value.size(); ++i) {
        const char& ch = value[i];
        // The escaping characters are defined in
        // https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md#responses.
        //
        // We do checking for each char in the string. If the current char is included in the defined
        // escaping characters, we jump to "the slow path" (append the char [encoded or not encoded]
        // to the returned string one by one) started from the current index.
        if (ch < ' ' || ch >= '~' || reserved_char_set.find(ch) != reserved_char_set.end()) {
            return PercentEncoding::encode(value, i, reserved_char_set);
        }
    }
    return std::string(value);
}

std::string PercentEncoding::encode(std::string value, const size_t index,
                                             const std::set<char>& reserved_char_set) {
    std::string encoded;
    if (index > 0) {
        encoded.append(value.substr(0, index));
    }

    for (size_t i = index; i < value.size(); ++i) {
        const char& ch = value[i];
        if (ch < ' ' || ch >= '~' || reserved_char_set.find(ch) != reserved_char_set.end()) {
            // For consistency, URI producers should use uppercase hexadecimal digits for all
            // percent-encodings. https://tools.ietf.org/html/rfc3986#section-2.1.
            encoded.append(fmt::format("%{:02X}", static_cast<const unsigned char&>(ch)));
        } else {
            encoded.push_back(ch);
        }
    }
    return encoded;
}

std::string PercentEncoding::decode(std::string encoded) {
    std::string decoded;
    decoded.reserve(encoded.size());
    for (size_t i = 0; i < encoded.size(); ++i) {
        char ch = encoded[i];
        if (ch == '%' && i + 2 < encoded.size()) {
            const char& hi = encoded[i + 1];
            const char& lo = encoded[i + 2];
            if (absl::ascii_isxdigit(hi) && absl::ascii_isxdigit(lo)) {
                if (absl::ascii_isdigit(hi)) {
                    ch = hi - '0';
                } else {
                    ch = absl::ascii_toupper(hi) - 'A' + 10;
                }

                ch *= 16;
                if (absl::ascii_isdigit(lo)) {
                    ch += lo - '0';
                } else {
                    ch += absl::ascii_toupper(lo) - 'A' + 10;
                }
                i += 2;
            }
        }
        decoded.push_back(ch);
    }
    return decoded;
}

namespace {
// %-encode all ASCII character codepoints, EXCEPT:
// ALPHA | DIGIT | * | - | . | _
// SPACE is encoded as %20, NOT as the + character
    constexpr std::array<uint32_t, 8> kUrlEncodedCharTable = {
            // control characters
            0b11111111111111111111111111111111,
            // !"#$%&'()*+,-./0123456789:;<=>?
            0b11111111110110010000000000111111,
            //@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_
            0b10000000000000000000000000011110,
            //`abcdefghijklmnopqrstuvwxyz{|}~
            0b10000000000000000000000000011111,
            // extended ascii
            0b11111111111111111111111111111111,
            0b11111111111111111111111111111111,
            0b11111111111111111111111111111111,
            0b11111111111111111111111111111111,
    };

    constexpr std::array<uint32_t, 8> kUrlDecodedCharTable = {
            // control characters
            0b00000000000000000000000000000000,
            // !"#$%&'()*+,-./0123456789:;<=>?
            0b01011111111111111111111111110101,
            //@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_
            0b11111111111111111111111111110101,
            //`abcdefghijklmnopqrstuvwxyz{|}~
            0b11111111111111111111111111100010,
            // extended ascii
            0b00000000000000000000000000000000,
            0b00000000000000000000000000000000,
            0b00000000000000000000000000000000,
            0b00000000000000000000000000000000,
    };

    bool shouldPercentEncodeChar(char c) { return testCharInTable(kUrlEncodedCharTable, c); }

    bool shouldPercentDecodeChar(char c) { return testCharInTable(kUrlDecodedCharTable, c); }
} // namespace

std::string PercentEncoding::urlEncodeQueryParameter(std::string value) {
    std::string encoded;
    encoded.reserve(value.size());
    for (char ch : value) {
        if (shouldPercentEncodeChar(ch)) {
            // For consistency, URI producers should use uppercase hexadecimal digits for all
            // percent-encodings. https://tools.ietf.org/html/rfc3986#section-2.1.
            absl::StrAppend(&encoded, fmt::format("%{:02X}", static_cast<const unsigned char&>(ch)));
        } else {
            encoded.push_back(ch);
        }
    }
    return encoded;
}

std::string PercentEncoding::urlDecodeQueryParameter(std::string encoded) {
    std::string decoded;
    decoded.reserve(encoded.size());
    for (size_t i = 0; i < encoded.size(); ++i) {
        char ch = encoded[i];
        if (ch == '%' && i + 2 < encoded.size()) {
            const char& hi = encoded[i + 1];
            const char& lo = encoded[i + 2];
            if (absl::ascii_isxdigit(hi) && absl::ascii_isxdigit(lo)) {
                if (absl::ascii_isdigit(hi)) {
                    ch = hi - '0';
                } else {
                    ch = absl::ascii_toupper(hi) - 'A' + 10;
                }

                ch *= 16;
                if (absl::ascii_isdigit(lo)) {
                    ch += lo - '0';
                } else {
                    ch += absl::ascii_toupper(lo) - 'A' + 10;
                }
                if (shouldPercentDecodeChar(ch)) {
                    // Decode the character only if it is present in the characters_to_decode
                    decoded.push_back(ch);
                } else {
                    // Otherwise keep it as is.
                    decoded.push_back('%');
                    decoded.push_back(hi);
                    decoded.push_back(lo);
                }
                i += 2;
            }
        } else {
            decoded.push_back(ch);
        }
    }
    return decoded;
}


void extractHostPathFromUri(const std::string& uri, std::string& host,
                                     std::string& path) {
    /**
     *  URI RFC: https://www.ietf.org/rfc/rfc2396.txt
     *
     *  Example:
     *  uri  = "https://example.com:8443/certs"
     *  pos:         ^
     *  host_pos:       ^
     *  path_pos:                       ^
     *  host = "example.com:8443"
     *  path = "/certs"
     */
    const auto pos = uri.find("://");
    // Start position of the host
    const auto host_pos = (pos == std::string::npos) ? 0 : pos + 3;
    // Start position of the path
    const auto path_pos = uri.find('/', host_pos);
    if (path_pos == std::string::npos) {
        // If uri doesn't have "/", the whole string is treated as host.
        host = uri.substr(host_pos);
        path = "/";
    } else {
        host = uri.substr(host_pos, path_pos - host_pos);
        path = uri.substr(path_pos);
    }
}
