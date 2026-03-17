#ifndef YHB_CIDR_SET_H
#define YHB_CIDR_SET_H

#include "yhb_net_toy/platform.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace yhb_net_toy {

/**
 * @brief 支持 IPv4 和 IPv6 的 CIDR
 */
class CIDR {
public:

    CIDR() : af(AF_UNSPEC) {}

    /**
     * @brief 根据给定的 IPv4 或 IPv6 的 CIDR 字串进行构造。若解析失败构造的对象 af == AF_UNSPEC
     *
     * @param str               形如 "fe80::/10" 或 "61.148.2.0/24" 这样的字串
     */
    explicit CIDR(const char * str);

    /**
     * @brief 用 in_addr 构造。
     *
     * @param v4            IPv4 的地址前缀
     * @param prefix_len    前缀长度 [0~32]
     */
    CIDR(const in_addr & v4, uint8_t prefix_len)
        : af(AF_INET), prefix_len(max_prefix_len(v4, prefix_len))
    {
        this->addr.v4 = v4;
    }

    /**
     * @brief 用 in6_addr 构造。
     *
     * @param v6            IPv6 的地址前缀
     * @param prefix_len    前缀长度 [0~128]
     */
    CIDR(const in6_addr & v6, uint8_t prefix_len)
        : af(AF_INET6), prefix_len(max_prefix_len(v6, prefix_len)) {
        this->addr.v6 = v6;
    }

    template<typename IP>
    CIDR(const IP & ip, uint8_t prefix_len) {
        if (ip.IsV4()) {
            this->af = AF_INET;
            this->addr.v4 = ip.GetInAddr();
            this->prefix_len = max_prefix_len(this->addr.v4, prefix_len);
        } else if (ip.IsV6()) {
            this->af = AF_INET6;
            this->addr.v6 = ip.GetIn6Addr();
            this->prefix_len = max_prefix_len(this->addr.v6, prefix_len);
        } else {
            this->af = AF_UNSPEC;
            this->prefix_len = 0;
        }
    }

    /**
     * @brief 返回地址族。可能的值为 AF_INET, AF_INET6 或 AF_UNSPEC。
     *          本对象是有效对象，当且仅当地址族为 AF_INET 或 AF_INET6。
     *
     * @return AF_INET 本对象为 IPv4。
     * @return AF_INET6 本对象为 IPv6。
     * @return AF_UNSPEC 本对象状态无效。
     */
    int GetFamily() const { return af; }

    /**
     * @brief 返回前缀长度。
     *
     * @return int。IPv4 时，前缀长度取值范围 [0~32]，IPv6 时，前缀长度取值范围 [0~128]。
     */
    uint8_t GetPrefixLen() const { return prefix_len; }

    /**
     * @brief 取 IPv4 地址。
     *
     * @return const in_addr& 若本对象不是 IPv4，返回值未定义。
     */
    const in_addr & GetV4() const {
        assert(af == AF_INET);
        return addr.v4;
    }

    /**
     * @brief 取 IPv6 地址。
     *
     * @return const in6_addr& 若本对象不是 IPv6，返回值未定义。
     */
    const in6_addr & GetV6() const {
        assert(af == AF_INET6);
        return addr.v6;
    }

    /**
     * @brief 本对象是否有效。
     *
     * @return true     有效。（当且仅当 GetFamily() 返回 AF_INET 或 AF_INET6）
     * @return false    无效。
     */
    bool IsValid() const { return af != AF_UNSPEC; }

    /**
     * @brief 本对象是 IPv4 吗？
     */
    bool IsV4() const { return af == AF_INET; }

    /**
     * @brief 本对象是 IPv6 吗？
     */
    bool IsV6() const { return af == AF_INET6; }

    /**
     * @brief 相等谓词。
     *  - 两个对象都是无效对象时，相等。
     *  - 两个对象均有效，且地址族相等、前级长度相等、地址也相等时，两个对象相等。
     */
    friend bool operator == (const CIDR & lhs, const CIDR & rhs) {
        if (lhs.af != rhs.af) {
            return false;
        }
        if (lhs.af == AF_UNSPEC) {
            return true;
        }
        if (lhs.prefix_len != rhs.prefix_len) {
            return false;
        }
        if (lhs.af == AF_INET) {
            return lhs.addr.v4.s_addr == rhs.addr.v4.s_addr;
        } else {
            return 0 == memcmp(&lhs.addr.v6, &rhs.addr.v6, sizeof(lhs.addr.v6));
        }
    }

    friend bool operator != (const CIDR & lhs, const CIDR & rhs) {
        return !(lhs == rhs);
    }

private:
    int af;
    uint8_t prefix_len;
    union {
        in_addr v4;
        in6_addr v6;
    } addr;

    template<typename ADDR_T>
    static uint8_t max_prefix_len(const ADDR_T &, uint8_t prefix_len) {
        constexpr uint8_t max_len = sizeof(ADDR_T) * 8;
        return prefix_len <= max_len ? prefix_len : max_len;
    }

    int parse_single_ip(const char * s);
};

/**
 * @brief   CIDR 集合。
 *
 * - 允许添加任意的 CIDR 项，并能判断给定的 CIDR 是否能被表中某项命中。
 *
 * - 支持 IPv4 和 IPv6。
 *
 * - 考虑到删除操作会造成前缀树的急剧膨胀，所以不提供删除功能。
 */
class CIDRSet {
public:
    struct Node {
        bool leaf;
        std::unique_ptr<Node> child[2];
        Node() : leaf(false) {}
        void SetToLeaf() {
            leaf = true;
            child[0].reset();
            child[1].reset();
        }
    };

    /**
     * @brief 判断给定 CIDR 是否被表中已有的某一项命中。
     *
     * 若表中存在前缀 P，则：
     *      - 任意与 P 比特前缀一致、且 prefix_len >= |P| 的 CIDR，在语义上均被 P 命中。
     *      - 任意 prefix_len < |P| 的 CIDR，绝不会被 P 命中。
     *
     * 例如：表中有 P = 0001::/5 时，则
     *          - 0001::/5 被命中（前缀一致，且 5 >= |P|）
     *          - 0001::/6 被命中（前缀一致，且 6 >= |P|）
     *          - 0001:8000::/5 不能被命中（前缀不一致）
     *          - 0001::/4 不能被命中（4 < |P|）
     *          - 2000::/5 不能被命中（前缀不一致）
     *
     * @param prefix     要查询的地址前缀
     * @param prefix_len 前缀长度（0–ADDR_BITS）
     *
     * @return true  给定 CIDR 被表中某项命中了。
     * @return false 给定 CIDR 无法被表中任何一项命中。
     */
    template<typename ADDR_T>
    bool Find(const ADDR_T & prefix, uint8_t prefix_len) const {
        return const_cast<CIDRSet *>(this)->check(&prefix, prefix_len, sizeof(ADDR_T) * 8, false);
    }

    /**
     * @brief 判断给定 CIDR 能否被命中，并视情况将给定的 CIDR 插入到表中。
     *
     *  - 若未命中，则将该 CIDR 插入到表中并返回 true。
     *  - 若命中，则什么也不做并返回 false。
     *
     * @param prefix     给定的地址前缀
     * @param prefix_len 前缀长度（0–ADDR_BITS）
     *
     * @return true  表中原来无法命中该 CIDR，现新 CIDR 已成功插入。
     * @return false 表中原来已能命中该 CIDR，新 CIDR 未插入，表未变。
     */
    template<typename ADDR_T>
    bool Insert(const ADDR_T & prefix, uint8_t prefix_len) {
        const bool matched = check(&prefix, prefix_len, sizeof(ADDR_T) * 8, true);
        return !matched;
    }

    /**
     * @brief 判断给定 CIDR 是否合法，能否被命中，并视情况将给定的 CIDR 插入到表中。
     *
     * @param cidr      给定的 CIDR_EX 对象
     * @param inserted  若不为空，则返回是否插入成功（含义与模板函数 Insert 的返回值一样）
     * @return AF_INET  给定的 CIDR 是 IPv4 的。
     * @return AF_INET6 给定的 CIDR 是 IPv6 的。
     * @return AF_UNSPEC 给定的 CIDR 无效（此时 inserted 返回 false）
     */
    int Insert(const CIDR & cidr, bool * inserted) {
        if (cidr.IsV4()) {
            const bool r = Insert(cidr.GetV4(), cidr.GetPrefixLen());
            if (inserted) {
                *inserted = r;
            }
            return AF_INET;
        }
        if (cidr.IsV6()) {
            const bool r = Insert(cidr.GetV6(), cidr.GetPrefixLen());
            if (inserted) {
                *inserted = r;
            }
            return AF_INET6;
        }
        if (inserted) {
            *inserted = false;
        }
        return AF_UNSPEC;
    }

    void Clear() {
        root.SetToLeaf();
        root.leaf = false;
    }

    bool IsEmpty() const {
        return !root.leaf && !root.child[0] && !root.child[1];
    }

    void Validate() const;

    /**
     * @brief 导出 CIDR_EX 列表。
     *
     * @tparam ADDR_T       表项格式，in_addr 或 in6_addr。
     * @tparam CONTAINER_T  存放 CIDR_EX 结构
     * @param result
     */
    template<typename ADDR_T, typename CONTAINER_T>
    void Export(CONTAINER_T & result) const {
        ADDR_T addr;
        export_dfs(root, 0, addr, result);
    }

    /**
     * @brief 本 CIDR 集合，是否能匹配所有地址？
     *
     * @return true 是的，能匹配所有地址。（即，0::/0）
     * @return false 否。
     */
    bool IsAllMatch() const {
        return root.leaf;
    }

private:
    Node root;

    bool check(const void * prefix, uint8_t prefix_len, uint8_t max_prefix_len, bool insert_when_not_matched);

    template<typename ADDR_T>
    static void normalized_addr(uint8_t depth, ADDR_T & addr) {
        constexpr size_t addr_size = sizeof(addr);
        size_t idx = depth / 8;
        if (idx >= addr_size) {
            return;
        }
        uint8_t * const p = reinterpret_cast<uint8_t *>(&addr);
        const uint8_t bit = depth % 8;
        if (bit != 0) {
            const uint8_t mask = 0xff << (8 - bit);
            p[idx++] &= mask;
        }
        memset(p + idx, 0, addr_size - idx);
    }

    template<typename ADDR_T, typename CONTAINER_T>
    void export_dfs(
        const Node & node,
        uint8_t depth,
        ADDR_T & addr,
        CONTAINER_T & out) const
    {
        if (node.leaf) {
            normalized_addr(depth, addr);
            out.emplace_back(addr, depth);
            return;
        }

        if (UNLIKELY(depth >= sizeof(ADDR_T) * 8)) {
            return;
        }

        uint8_t & ref_byte = reinterpret_cast<uint8_t *>(&addr)[depth / 8];
        const uint8_t mask = (1u << (7 - (depth % 8)));
        for (int bit = 0; bit <= 1; ++bit) {
            const Node * const child = node.child[bit].get();
            if (child == nullptr) {
                continue;
            }
            if (bit) {
                ref_byte |= mask;
            } else {
                ref_byte &= ~mask;
            }
            export_dfs(*child, depth + 1, addr, out);
        }
    }
};

} // End of namespace 'yhb_net_toy'

#endif
