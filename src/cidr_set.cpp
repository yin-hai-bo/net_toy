#include "yhb_net_toy/cidr_set.h"
#include <string>
#include <cassert>

namespace yhb_net_toy {

int CIDR::parse_single_ip(const char * s) {
    if (1 == inet_pton(AF_INET, s, &this->addr.v4)) {
        return this->af = AF_INET;
    }
    if (1 == inet_pton(AF_INET6, s, &this->addr.v6)) {
        return this->af = AF_INET6;
    }
    return this->af = AF_UNSPEC;
}

CIDR::CIDR(const char * str) {
    const char * p = strchr(str, '/');
    if (!p) {
        parse_single_ip(str);
        switch (this->af) {
        case AF_INET:
            this->prefix_len = 32;
            break;
        case AF_INET6:
            this->prefix_len = 128;
            break;
        default:
            break;
        }
        return;
    }

    char * end;
    auto const bits = strtol(p + 1, &end, 10);
    if (UNLIKELY(*end != '\0' || end == p + 1 || bits < 0 || bits > 128)) {
        this->af = AF_UNSPEC;
        return;
    }
    this->prefix_len = static_cast<uint8_t>(bits);

    char buf[128];
    auto const len = std::min<size_t>(sizeof(buf) - 1, p - str);
    memcpy(buf, str, len);
    buf[len] = '\0';
    parse_single_ip(buf);
    switch (this->af) {
    case AF_INET:
        if (bits > 32) {
            this->af = AF_UNSPEC;
            return;
        }
        break;
    case AF_INET6:
        // 前面已经判断过 bits 不会大于 128。
        break;
    default:
        break;
    }
}

template<typename T, size_t Capacity>
class FixedCapacityVector {
public:
    static constexpr size_t capacity = Capacity;

    void push_back(const T & t) {
        array_[size_++] = t;
    }

    size_t size() const { return size_; }

    T & operator[](size_t i) { return array_[i]; }
    const T & operator[](size_t i) const { return array_[i]; }

private:
    T array_[Capacity];
    size_t size_ = 0;
};

class BitGetter {
    const uint8_t * p;
    uint8_t mask;
public:
    BitGetter(const void * addr)
        : p(static_cast<const uint8_t *>(addr))
        , mask(0x80) {}

    uint8_t Get() const {
        return (*p & mask) ? 1 : 0;
    }

    BitGetter & operator++() {
        mask >>= 1;
        if (mask == 0) {
            ++p;
            mask = 0x80;
        }
        return *this;
    }

    uint8_t Next() {
        uint8_t result = Get();
        ++(*this);
        return result;
    }
};

using CidrPath = FixedCapacityVector<CIDRSet::Node *, sizeof(in6_addr) * 8 + 1>;

static void cidr_set_try_to_merge(const CidrPath & path) {
    for (int i = static_cast<int>(path.size()) - 2; i >= 0; --i) {
        auto node = path[i];
        if (!node->child[0] || !node->child[1]) {
            break;
        }
        if (!node->child[0]->leaf || !node->child[1]->leaf) {
            break;
        }
        node->SetToLeaf();
    }
}

bool CIDRSet::check(const void * prefix, uint8_t prefix_len, uint8_t max_prefix_len, bool insert_when_not_match) {
    if (prefix_len > max_prefix_len) {
        prefix_len = max_prefix_len;
    }

    CidrPath path;   // 保存从 ROOT 开始的匹配前缀
    assert(max_prefix_len < CidrPath::capacity);

    BitGetter bit_getter(prefix);
    auto cur = &root;
    path.push_back(cur);
    uint8_t depth = 0;
    for (; depth < prefix_len; ++depth) {
        if (cur->leaf) {
            return true; // 提前遇到叶节点，即 prefix_len >= |P|，命中。
        }
        auto & child = cur->child[bit_getter.Get()];
        if (!child) {
            break; // 找不到匹配的前缀
        }
        ++bit_getter;
        cur = child.get();
        path.push_back(cur);
    }

    if (depth == prefix_len && cur->leaf) {
        // 存在一个 P 的比特前缀 与 prefix 完全一致，且 prefix_len == |P|，命中了
        return true;
    }

    // 前缀匹配了但 P 更长，或根本就没找到匹配的前缀。
    if (insert_when_not_match) {
        // 要在当前位置插入新的子树，原来的子树作废。
        // 原来有二进制 0001/4，现在要插入 00/2，则直接将当前位置变成叶节点
        // 原来有二进制 0001/4，现在要插入 01/2，则直接在当前位置构造子树
        for (; depth < prefix_len; ++depth) {
            // 进入循环的条件是 depth < prefix_len，说明 cur->child[b] 为空
            auto & child = cur->child[bit_getter.Next()];
            assert(!child);
            child.reset(new Node);
            cur = child.get();
            path.push_back(cur);
        }
        cur->SetToLeaf();

        // 回溯，向上尝试合并
        cidr_set_try_to_merge(path);
    }
    return false;
}

#ifndef NDEBUG
static void cidr_set_validate(CIDRSet::Node const * node, bool is_root) {
    assert(node);

    const bool has0 = (node->child[0] != nullptr);
    const bool has1 = (node->child[1] != nullptr);

    // 不变量：leaf 节点必须是终止节点
    if (node->leaf) {
        assert(!has0 && !has1);
        return;
    }

    // 不变量：非 leaf 节点不能是空节点（除了 root）
    if (!is_root) {
        assert(has0 || has1);
    }

    // 不变量：不能存在“可合并但未合并”的状态
    if (has0 && has1) {
        assert(!(node->child[0]->leaf && node->child[1]->leaf));
    }

    // 递归校验子节点
    if (has0) {
        cidr_set_validate(node->child[0].get(), false);
    }
    if (has1) {
        cidr_set_validate(node->child[1].get(), false);
    }
}

void CIDRSet::Validate() const {
    cidr_set_validate(&root, true);
}

#endif

} // End of namespace 'yhb_net_toy'
