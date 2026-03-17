#include <gtest/gtest.h>
#include "route_table.h"
#include "test_platform.h"

namespace yhb {

using IpRange = RouteTable::IpRange;
using CIDR = RouteTable::CIDR;

TEST(CIDR, IpRange) {
    // 非自反
    IpRange r1{1000, 2000};
    ASSERT_FALSE(RouteTable::pred_for_merge(r1, r1));
    ASSERT_FALSE(RouteTable::pred_for_search(r1, r1));

    // 非对称
    IpRange r2{3000, 4000};
    ASSERT_TRUE(RouteTable::pred_for_merge(r1, r2));
    ASSERT_FALSE(RouteTable::pred_for_merge(r2, r1));
    ASSERT_TRUE(RouteTable::pred_for_search(r1, r2));
    ASSERT_FALSE(RouteTable::pred_for_search(r2, r1));

    // 等价
    IpRange r3{2000, 2400};
    ASSERT_FALSE(RouteTable::pred_for_merge(r1, r3));
    ASSERT_FALSE(RouteTable::pred_for_merge(r3, r1));
    ASSERT_FALSE(RouteTable::pred_for_search(r1, r3));
    ASSERT_FALSE(RouteTable::pred_for_search(r3, r1));

    // 传递
    IpRange r4{5000, 6000};
    ASSERT_TRUE(RouteTable::pred_for_merge(r2, r4));
    ASSERT_TRUE(RouteTable::pred_for_merge(r1, r4));
    ASSERT_TRUE(RouteTable::pred_for_search(r2, r4));
    ASSERT_TRUE(RouteTable::pred_for_search(r1, r4));

    IpRange r5{0, 0xffffffff};
    ASSERT_FALSE(RouteTable::pred_for_merge(r5, r1));
    ASSERT_FALSE(RouteTable::pred_for_merge(r1, r5));
    ASSERT_FALSE(RouteTable::pred_for_search(r5, r1));
    ASSERT_FALSE(RouteTable::pred_for_search(r1, r5));
}

TEST(CIDR, Insert) {
    RouteTable tab;
    ASSERT_TRUE(tab.containers.empty());

    // 无效的 CIDR 字符串
    ASSERT_FALSE(tab.Insert("abcdef"));
    ASSERT_TRUE(tab.containers.empty());
    ASSERT_FALSE(tab.Insert("8.8.8.8/33"));
    ASSERT_TRUE(tab.containers.empty());
    ASSERT_FALSE(tab.Insert("8.8.8.8/-1"));
    ASSERT_TRUE(tab.containers.empty());
    ASSERT_FALSE(tab.Insert("abced/24"));
    ASSERT_TRUE(tab.containers.empty());
    ASSERT_FALSE(tab.Insert("0.0.0.0/"));
    ASSERT_TRUE(tab.containers.empty());
    ASSERT_FALSE(tab.Insert("/24"));
    ASSERT_TRUE(tab.containers.empty());
    ASSERT_FALSE(tab.Insert("0.0.0.0/x"));
    ASSERT_TRUE(tab.containers.empty());

    ASSERT_FALSE(tab.Insert(0x88888888, 33));

    // 单IP
    ASSERT_TRUE(tab.Insert("1.2.3.4"));
    ASSERT_FALSE(tab.containers.empty());
    auto const * r = &tab.containers[0];
    ASSERT_EQ(0x01020304, r->first);
    ASSERT_EQ(0x01020304, r->last);
    ASSERT_TRUE(tab.Find(0x01020304, true));
    ASSERT_FALSE(tab.Find(0x01010101, false));
    ASSERT_FALSE(tab.Find(0x08080808, false));

    // IP段
    ASSERT_TRUE(tab.Insert("1.1.1.1/25"));
    ASSERT_EQ(2, tab.containers.size());
    r = &tab.containers[0];
    ASSERT_EQ(0x01010100, r->first);
    ASSERT_EQ(0x0101017f, r->last);
    r = &tab.containers[1];
    ASSERT_EQ(0x01020304, r->first);
    ASSERT_EQ(0x01020304, r->last);

    auto const f = [&] {
        ASSERT_TRUE(tab.Find(0x01020304, true));
        ASSERT_TRUE(tab.Find(0x01010101, true));
        ASSERT_FALSE(tab.Find(0x010101f0, true));
    };
    f();

    // 插入一个交错的，但范围更小的
    ASSERT_TRUE(tab.Insert("1.1.1.66/31"));
    ASSERT_EQ(2, tab.containers.size());
    f();

    // 插入一个紧挨着的
    tab.Insert("1.2.3.3");
    ASSERT_EQ(2, tab.containers.size());
    ASSERT_TRUE(tab.Find(0x01020303, true));

    ///////////////////////////////////

    tab.Clear();
    ASSERT_TRUE(tab.containers.empty());

    ASSERT_FALSE(tab.Insert(IpRange{1234, 1233}));

    ASSERT_TRUE(tab.Insert("6.6.6.0/24"));
    ASSERT_TRUE(tab.Insert("6.6.8.0/24"));
    ASSERT_TRUE(tab.Insert("6.6.10.0/24"));
    ASSERT_TRUE(tab.Insert("6.6.12.0/24"));
    ASSERT_TRUE(tab.Insert("6.6.15.0/24"));
    ASSERT_TRUE(tab.Insert("6.6.20.0/24"));
    ASSERT_EQ(6, tab.containers.size());

    ASSERT_TRUE(tab.Insert(IpRange{0x06060810, 0x06060d80}));
    ASSERT_EQ(4, tab.containers.size());
    ASSERT_EQ((IpRange{0x06060600, 0x060606ff}), tab.containers[0]);
    ASSERT_EQ((IpRange{0x06060800, 0x06060d80}), tab.containers[1]);
    ASSERT_EQ((IpRange{0x06060f00, 0x06060fff}), tab.containers[2]);
    ASSERT_EQ((IpRange{0x06061400, 0x060614ff}), tab.containers[3]);

    RouteTable tab2 = tab;
    ASSERT_TRUE(tab2.Insert(IpRange{0x06060d81, 0x060613ff}));
    ASSERT_EQ(2, tab2.containers.size());
    ASSERT_EQ((IpRange{0x06060600, 0x060606ff}), tab2.containers[0]);
    ASSERT_EQ((IpRange{0x06060800, 0x060614ff}), tab2.containers[1]);

    ASSERT_TRUE(tab.Insert(IpRange{0x0606060f, 0x06061500}));
    ASSERT_EQ(1, tab.containers.size());
    ASSERT_EQ((IpRange{0x06060600, 0x06061500}), tab.containers[0]);

    // 测试0.0.0.0/1和128.0.0.0/1
    tab.Clear();
    ASSERT_TRUE(tab.containers.empty());
    ASSERT_TRUE(tab.Insert("125.10.6.1"));
    ASSERT_TRUE(tab.Insert("125.0.0.1/16"));
    ASSERT_TRUE(tab.Insert("0.0.0.0/1"));
    ASSERT_EQ(1, tab.containers.size());
    ASSERT_EQ((IpRange{0x00, 0x07fffffff}), tab.containers[0]);

    ASSERT_TRUE(tab.Insert("125.0.6.1"));
    ASSERT_TRUE(tab.Insert("124.0.0.1/16"));
    ASSERT_TRUE(tab.Insert("128.0.0.0/1"));

    ASSERT_TRUE(tab.Insert("124.0.0.1"));
    ASSERT_TRUE(tab.Insert("192.168.0.1/16"));
    ASSERT_EQ(1, tab.containers.size());
    ASSERT_EQ((IpRange{0x00, 0x0ffffffff}), tab.containers[0]);

    tab.Clear();
    ASSERT_TRUE(tab.containers.empty());
    ASSERT_TRUE(tab.Insert("0.0.0.0"));
    ASSERT_TRUE(tab.Insert("0.0.0.0/32"));
    ASSERT_EQ(1, tab.containers.size());
    ASSERT_EQ((IpRange{0x00000000, 0x00000000}), tab.containers[0]);
    ASSERT_TRUE(tab.Find(0x00000000, true));
    ASSERT_FALSE(tab.Find(0x00000001, true));

    tab.Clear();
    ASSERT_TRUE(tab.containers.empty());
    ASSERT_TRUE(tab.Insert("0.0.0.0/0"));
    ASSERT_EQ(1, tab.containers.size());
    ASSERT_EQ((IpRange{0x00000000, 0xffffffff}), tab.containers[0]);
    ASSERT_TRUE(tab.Find(0x00000000, true));
    ASSERT_TRUE(tab.Find(0x7f000001, true));
    ASSERT_TRUE(tab.Find(0xffffffff, true));
}

TEST(CIDR, Single) {
    RouteTable tab;

    ASSERT_TRUE(tab.Insert("1.2.3.4"));
    ASSERT_FALSE(tab.Find(0x01020303, true));
    ASSERT_TRUE(tab.Find(0x01020304, true));
    ASSERT_FALSE(tab.Find(0x01020305, true));
    ASSERT_FALSE(tab.Find(0x01020306, true));

    ASSERT_TRUE(tab.Insert("1.2.3.6"));
    ASSERT_FALSE(tab.Find(0x01020303, true));
    ASSERT_TRUE(tab.Find(0x01020304, true));
    ASSERT_FALSE(tab.Find(0x01020305, true));
    ASSERT_TRUE(tab.Find(0x01020306, true));

    ASSERT_TRUE(tab.Insert("1.2.3.5"));
    ASSERT_FALSE(tab.Find(0x01020303, true));
    ASSERT_TRUE(tab.Find(0x01020304, true));
    ASSERT_TRUE(tab.Find(0x01020305, true));
    ASSERT_TRUE(tab.Find(0x01020306, true));

    ASSERT_EQ(1, tab.containers.size());
    ASSERT_EQ((RouteTable::IpRange{0x01020304, 0x01020306}), tab.containers[0]);
}

static uint32_t str_to_ip(const char ip[]) {
    uint32_t result;
    inet_pton(AF_INET, ip, &result);
    return result;
}

struct CIDRStr {
    const char * ip;
    decltype(CIDR::network_bits) len;
};

static void test_cidr_convert(const char first_ip[], const char last_ip[], const std::vector<CIDRStr> & expected_result) {
    IpRange range{ ntohl(str_to_ip(first_ip)), ntohl(str_to_ip(last_ip)) };
    std::vector<CIDR> list;
    range.ToCIDR([&] (CIDR cidr) -> void {
        list.push_back(cidr);
    });
    ASSERT_EQ(expected_result.size(), list.size());

    for (size_t i = 0; i < list.size(); ++i) {
        CIDRStr const & expected = expected_result[i];
        CIDR const & actual = list[i];
        ASSERT_EQ(str_to_ip(expected.ip), actual.prefix) << expected.ip;
        ASSERT_EQ(expected.len, actual.network_bits) << expected.ip;
    }
}

TEST(CIDR, Convert) {
    test_cidr_convert("192.168.6.73", "192.168.6.132", std::vector<CIDRStr> {
        { "192.168.6.73", 32 },
        { "192.168.6.74", 31 },
        { "192.168.6.76", 30 },
        { "192.168.6.80", 28 },
        { "192.168.6.96", 27 },
        { "192.168.6.128", 30 },
        { "192.168.6.132", 32 },
    });
    test_cidr_convert("0.0.0.0", "255.255.255.255", std::vector<CIDRStr> {
        { "0.0.0.0", 0 },
    });
    test_cidr_convert("255.255.255.255", "255.255.255.255", std::vector<CIDRStr> {
        { "255.255.255.255", 32 },
    });
    test_cidr_convert("255.255.255.7", "255.255.255.5", std::vector<CIDRStr>());
    test_cidr_convert("127.255.255.255", "255.255.255.255", std::vector<CIDRStr> {
        { "127.255.255.255", 32 },
        { "128.0.0.0", 1 },
    });
    test_cidr_convert("0.0.0.0", "127.255.255.255", std::vector<CIDRStr> {
        { "0.0.0.0", 1 },
    });
}

}
