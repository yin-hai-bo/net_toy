#ifndef YHB_NET_TOY_CIDR_TAB_H
#define YHB_NET_TOY_CIDR_TAB_H

#include "yhb_net_toy/platform.h"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <functional>

namespace yhb_net_toy {

class RouteTable {
public:
    RouteTable();

    /**
     * @brief Add a route match rule by CIDR string.
     *
     * @param cidr_str CIDR string, "a.b.c.d/n" (such as "61.4.55.0/24").
     *                 If "/n" not present, it's a single IP address (equivalent to "/32"）.
     *                 "a.b.c.d" should be a valid IPv4 address, and n must between in range [0,32].
     *                 添加失败，函数返回 false。
     * @return Returning true when succeeded. Returning false indicates failure,
     *          such as when the input parameters are invalid.
     */
    bool Insert(const char * cidr_str);

    /**
     * @brief Add a route match rule by address prefix and length.
     *
     * @param prefix_net_order IPv4 address prefix, by net bits order.
     * @param network_bits Length of the prefix. How many continuous 1 generate the subnet mask. In [0,32].
     * @return Returning true if success, otherwise return false (when network_bits large than 32).
     */
    bool Insert(uint32_t prefix_net_order, unsigned network_bits);

    struct CIDR {
        uint32_t prefix;       // Network address prefix, by net bits order.
        uint32_t network_bits; // Net mask length. [0,32].
        friend bool operator == (const CIDR & lv, const CIDR & rv) {
            return lv.prefix == rv.prefix && lv.network_bits == rv.network_bits;
        }
    };

    struct IpRange {
        uint32_t first;     // Starting IP, inclusive, by host bits order, should be less or equal than the 'last'.
        uint32_t last;      // Ending IP, inclusive, by host bits order, should be large or equal than the 'first'.

        friend bool operator < (const IpRange &, const IpRange &) = delete;
        friend bool operator == (const IpRange & lv, const IpRange & rv) {
            return lv.first == rv.first && lv.last == rv.last;
        }
        friend bool operator != (const IpRange & lv, const IpRange & rv) {
            return !(operator==(lv, rv));
        }
        operator bool() const {
            return first <= last;
        }
        bool operator!() const {
            return !operator bool();
        }

        /**
         * @brief Convert self to equivalent CIDR set.
         *
         * @param each_cidr_callback A range of the starting and ending address maybe equivalent a CIDR set.
         *      For each CIDR entry will invoke this callback.
         */
        void ToCIDR(std::function<void (CIDR)> each_cidr_callback) const;
    };

    /**
     * @brief Add a match rule by IP range
     *
     * @return Returning false if range parameter is not valid.
     */
    bool Insert(IpRange range);

    /**
     * @brief Check if the given IP address matches in the routing table.
     *
     * @param ip The IP.
     * @param host_order Is the IP address host or net bits order.
     *
     * @return Returning true when match.
     */
    bool Find(uint32_t ip, bool host_order) const;

    void Clear() {
        containers.clear();
    }

    bool IsEmpty() const {
        return containers.empty();
    }

    size_t GetCount() const {
        return containers.size();
    }

    std::vector<IpRange>::const_iterator begin() const {
        return containers.cbegin();
    }

    std::vector<IpRange>::const_iterator end() const {
        return containers.cend();
    }

private:

    /**
     * @brief A predicate used for merging entries, determining if A is a strict predecessor of B,
     * if and only if A comes before B, and the scope of A and B does not intersect and is not adjacent.
     */
    static bool pred_for_merge(const RouteTable::IpRange & lv, const RouteTable::IpRange & rv) {
        if (UNLIKELY(lv.last == 0xffffffff)) {
            return false;
        } else {
            return lv.last + 1 < rv.first;
        }
    }

    /**
     * @brief A predicate used for match, determining if A is a strict predecessor of B,
     * if and only if A comes before B, and the scope of A and B does not intersect.
     */
    static bool pred_for_search(const RouteTable::IpRange & lv, const RouteTable::IpRange & rv) {
        return lv.last < rv.first;
    }

private:
    std::vector<IpRange> containers;

    FRIEND_GTEST(CIDR, IpRange);
    FRIEND_GTEST(CIDR, Insert);
    FRIEND_GTEST(CIDR, Single);
};

} // End of namespace 'yhb_net_toy'

#endif
