#include "route_table.h"
#include "yhb_common.h"
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace yhb {

using CIDR = RouteTable::CIDR;

RouteTable::RouteTable() {
    containers.reserve(64);
}

/**
 * @brief Parse a given CIDR string.
 *
 * @param str The CIDR string, should be "xx.xxx.x.xx/n".
 * @param result [out] return result CIDR.
 * @return true when successful, otherwise if str parameter is not valid.
 */
static bool parse_cidr_str(const char * str, CIDR & result) {
    const char * p = str;
    for (;; ++p) {
        char const ch = *p;
        if (ch == '/') {
            break;
        } else if (UNLIKELY(ch == '\0')) {
            // No '/' present, it's a single IP address
            if (UNLIKELY(1 != inet_pton(AF_INET, str, &result.prefix))) {
                // It's a invalid CIDR object.
                return false;
            }
            result.network_bits = 32;
            return true;
        }
    }

    // Parse the network bits followed by '/'.
    char * end;
    auto const bits = strtol(p + 1, &end, 10);
    if (UNLIKELY(*end != '\0' || end == p + 1 || bits < 0 || bits > 32)) {
        return false;
    }

    char buf[128];
    auto const len = std::min<size_t>(sizeof(buf) - 1, p - str);
    memcpy(buf, str, len);
    buf[len] = '\0';
    if (UNLIKELY(1 != inet_pton(AF_INET, buf, &result.prefix))) {
        return false;
    }
    result.network_bits = bits;
    return true;
}

bool RouteTable::Insert(const char cidr_str[]) {
    CIDR cidr;
    if (LIKELY(parse_cidr_str(cidr_str, cidr))) {
        return this->Insert(cidr.prefix, cidr.network_bits);
    } else {
        return false;
    }
}

bool RouteTable::Insert(uint32_t prefix_net_order, unsigned network_bits) {
    if (network_bits > 32) {
        return false;
    }
    uint32_t const prfix_host_order = ntohl(prefix_net_order);
    uint32_t const mask = uint32_t(-1) << (32 - network_bits);
    IpRange range;
    range.first = prfix_host_order & mask;
    range.last = range.first | (~mask);
    return Insert(range);
}

bool RouteTable::Insert(const IpRange range) {
    if (!range) {
        return false;
    }

    // Find the insertion point in the ordered list to maintain the sorted order.
    // If the element at the found position is not equivalent to the range,
    // it indicates no overlap, and no adjacency, allowing for direct insertion.
    auto const pos = std::lower_bound(containers.begin(), containers.end(), range, pred_for_merge);
    if (containers.end() == pos || pred_for_merge(range, *pos)) {
        containers.insert(pos, range);
        return true;
    }

    // If the element at the insertion point is equivalent to the range,
    // it suggests overlap or adjacency, thus they should be merged.
    if (range.first < pos->first) {
        pos->first = range.first;
    }
    if (range.last > pos->last) {
        pos->last = range.last;
    }

    // After merging, check if the new range intersects with the subsequent element,
    // and continue merging if necessary.
    auto it = pos + 1;
    while (it != containers.end()) {
        if (pred_for_merge(*pos, *it)) {
            break;
        }
        if (it->last > pos->last) {
            pos->last = it->last;
        }
        ++it;
    }
    containers.erase(pos + 1, it);

    return true;
}

bool RouteTable::Find(uint32_t ip, bool host_order) const {
    IpRange r;
    r.first = host_order ? ip : ntohl(ip);
    r.last = r.first;

    auto const pos = std::lower_bound(containers.cbegin(), containers.cend(), r, pred_for_search);
    return containers.cend() != pos && !pred_for_search(r, *pos);
}

//////////////////////////////////////////////////

// Estimate the CIDR length based on the number of trailing zeros in the given IP (host bits order)
static int estimate_cidr_len(uint32_t ip) {
    uint32_t mask = 1;
    for (int i = 32; i > 0; --i) {
        if ((ip & mask) != 0) {
            return i;
        }
        mask <<= 1;
    }
    return 0;
}

// Calculate the maximum IP value (host bits order) that can be represented by a given CIDR
static inline uint32_t get_cidr_max_ip(uint32_t ip_host_order, uint32_t prefix_len) {
    if (prefix_len >= 32) {
        return ip_host_order;
    }
    uint32_t const mask = static_cast<uint32_t>(-1) >> prefix_len;
    return ip_host_order | mask;
}

/**
 * Convert IP range to CIDR segments
 *
 * @param from Host order starting IP, inclusive
 * @param to Host order ending IP, inclusive
 * @param result Processor to append resulting CIDRs
 */
static void convert_ip_range_to_cidr(uint32_t from, uint32_t to, std::function<void (CIDR)> result_processor) {
    if (from > to) {
        return;
    }
    if (from == to) {
        result_processor({htonl(from), 32});
        return;
    }

    uint32_t len = estimate_cidr_len(from);
    for (;;) {
        uint32_t max_ip = get_cidr_max_ip(from, len);
        if (max_ip <= to) {
            result_processor({htonl(from), len});
            if (max_ip < to) {
                convert_ip_range_to_cidr(max_ip + 1, to, result_processor);
            }
            return;
        }
        ++len;
    }
}

void RouteTable::IpRange::ToCIDR(std::function<void (CIDR)> each_cidr_callback) const {
    convert_ip_range_to_cidr(this->first, this->last, each_cidr_callback);
}

} // End of namespace 'yhb'
