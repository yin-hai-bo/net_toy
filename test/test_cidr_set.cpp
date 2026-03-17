#include "yhb_net_toy/cidr_set.h"
#include <gtest/gtest.h>

using CIDR = yhb_net_toy::CIDR;
using CIDRSet = yhb_net_toy::CIDRSet;

static in6_addr make_in6_addr(const char * ip) {
    in6_addr addr;
    inet_pton(AF_INET6, ip, &addr);
    return addr;
}

TEST(CIDRSetTest, 1) {
    CIDRSet set;
    EXPECT_TRUE(set.IsEmpty());
    ASSERT_FALSE(set.Find(make_in6_addr("7f::ffff"), 64));

    ASSERT_TRUE(set.Insert(make_in6_addr("::"), 1));
    set.Validate();
    ASSERT_TRUE(set.Find(make_in6_addr("7f00::ffff"), 64));
    ASSERT_FALSE(set.Find(make_in6_addr("8000::"), 128));

    ASSERT_TRUE(set.Insert(make_in6_addr("8000::"), 1));
    set.Validate();
    ASSERT_TRUE(set.Find(make_in6_addr("8000::ff01:3455"), 128));

    ASSERT_FALSE(set.Insert(make_in6_addr("f000:87"), 128));
    set.Validate();

    set.Clear();
    ASSERT_FALSE(set.Find(make_in6_addr("8000::ff01:3455"), 128));
    set.Validate();
}

TEST(CIDRSetTest, 2) {
    CIDRSet set;
    ASSERT_TRUE(set.Insert(make_in6_addr("::"), 3));
    set.Validate();
    ASSERT_FALSE(set.Find(make_in6_addr("::"), 2));
    ASSERT_TRUE(set.Find(make_in6_addr("::"), 3));
    ASSERT_TRUE(set.Find(make_in6_addr("::"), 4));

    ASSERT_TRUE(set.Insert(make_in6_addr("::"), 2));
    set.Validate();
    ASSERT_TRUE(set.Find(make_in6_addr("::"), 2));
    ASSERT_TRUE(set.Find(make_in6_addr("::"), 3));
    ASSERT_TRUE(set.Find(make_in6_addr("::"), 4));
    set.Validate();
}

TEST(CIDRSetTest, 3) {
    CIDRSet set;
    in6_addr prefix = make_in6_addr("2403:f500:8800:19::521");
    ASSERT_TRUE(set.Insert(prefix, 129));
    set.Validate();
    ASSERT_FALSE(set.Insert(prefix, 128));
    set.Validate();

    set.Clear();
    ASSERT_TRUE(set.IsEmpty());

    ASSERT_TRUE(set.Insert(make_in6_addr("ffff:ffff::0"), 32));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("ffff:ffff::0"), 20));
    set.Validate();
}

TEST(CIDRSetTest, PrefixZero) {
    CIDRSet set;
    ASSERT_TRUE(set.IsEmpty());
    set.Validate();

    ASSERT_TRUE(set.Insert(make_in6_addr("::"), 0));
    set.Validate();
    ASSERT_FALSE(set.IsEmpty());

    // 任意地址、任意更长前缀，均应被 /0 命中
    ASSERT_TRUE(set.Find(make_in6_addr("::1"), 128));
    ASSERT_TRUE(set.Find(make_in6_addr("abcd::"), 64));
    ASSERT_TRUE(set.Find(make_in6_addr("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"), 1));

    // 更短前缀不存在，但语义上 /0 仍应命中自身
    ASSERT_TRUE(set.Find(make_in6_addr("::"), 0));
    set.Validate();
}

TEST(CIDRSetTest, FullLengthPrefix) {
    CIDRSet set;

    in6_addr a = make_in6_addr("2001:db8::1");
    in6_addr b = make_in6_addr("2001:db8::2");

    ASSERT_TRUE(set.Insert(a, 128));
    set.Validate();

    ASSERT_TRUE(set.Find(a, 128));
    ASSERT_FALSE(set.Find(b, 128));

    // 更短前缀不能被命中
    ASSERT_FALSE(set.Find(a, 127));
    set.Validate();
}

TEST(CIDRSetTest, ShortPrefixDominatesLong) {
    CIDRSet set;

    ASSERT_TRUE(set.Insert(make_in6_addr("2000::"), 3));
    set.Validate();
    ASSERT_FALSE(set.Insert(make_in6_addr("2000::"), 4));
    set.Validate();
    ASSERT_FALSE(set.Insert(make_in6_addr("2000::"), 128));
    set.Validate();

    ASSERT_TRUE(set.Find(make_in6_addr("2000::abcd"), 64));
}

TEST(CIDRSetTest, ShorterPrefixReplacesSemantics) {
    CIDRSet set;

    ASSERT_TRUE(set.Insert(make_in6_addr("2000::"), 5));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("2000::"), 4));
    set.Validate();

    ASSERT_TRUE(set.Find(make_in6_addr("2000::"), 4));
    ASSERT_TRUE(set.Find(make_in6_addr("2000::"), 5));
    ASSERT_TRUE(set.Find(make_in6_addr("2000::"), 6));
}

TEST(CIDRSetTest, BitBoundaryMismatch) {
    CIDRSet set;

    // 第 16 位边界
    ASSERT_TRUE(set.Insert(make_in6_addr("8000::"), 16));
    set.Validate();

    ASSERT_TRUE(set.Find(make_in6_addr("8000::1"), 128));
    ASSERT_FALSE(set.Find(make_in6_addr("7fff:ffff::"), 128));
    ASSERT_FALSE(set.Find(make_in6_addr("9000::"), 128));
}

TEST(CIDRSetTest, ClearDeepTree) {
    CIDRSet set;

    ASSERT_TRUE(set.Insert(make_in6_addr("2000::"), 3));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("4000::"), 3));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("6000::"), 3));
    set.Validate();

    ASSERT_FALSE(set.IsEmpty());

    set.Clear();
    ASSERT_TRUE(set.IsEmpty());

    ASSERT_FALSE(set.Find(make_in6_addr("2000::"), 3));
    ASSERT_FALSE(set.Find(make_in6_addr("4000::1"), 128));
}

TEST(CIDRSetTest, DuplicateInsert) {
    CIDRSet set;

    ASSERT_TRUE(set.Insert(make_in6_addr("3000::"), 12));
    set.Validate();
    ASSERT_FALSE(set.Insert(make_in6_addr("3000::"), 12));
    set.Validate();
    ASSERT_FALSE(set.Insert(make_in6_addr("3000::"), 12));
    set.Validate();

    ASSERT_TRUE(set.Find(make_in6_addr("3000::abcd"), 64));
}

TEST(CIDRSetTest, DisjointPrefixes) {
    CIDRSet set;

    ASSERT_TRUE(set.Insert(make_in6_addr("0000::"), 4));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("f000::"), 4));
    set.Validate();

    ASSERT_TRUE(set.Find(make_in6_addr("0fff::1"), 64));
    ASSERT_TRUE(set.Find(make_in6_addr("ffff::1"), 64));

    ASSERT_FALSE(set.Find(make_in6_addr("8000::"), 64));
}

TEST(CIDRSetTest, MergeTwoChildrenIntoParent) {
    CIDRSet set;

    ASSERT_TRUE(set.Insert(make_in6_addr("::"), 1));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("8000::"), 1));
    set.Validate();

    // 合并后，root 应退化为 leaf
    ASSERT_FALSE(set.IsEmpty());
    ASSERT_TRUE(set.Find(make_in6_addr("::"), 0));
    ASSERT_TRUE(set.Find(make_in6_addr("1234::"), 64));
    ASSERT_TRUE(set.Find(make_in6_addr("ffff::"), 128));
    set.Validate();
}

TEST(CIDRSetTest, NoMergeWithSingleChild) {
    CIDRSet set;

    ASSERT_TRUE(set.Insert(make_in6_addr("::"), 1));
    set.Validate();

    ASSERT_TRUE(set.Find(make_in6_addr("7fff::"), 128));
    ASSERT_FALSE(set.Find(make_in6_addr("8000::"), 128));
}

TEST(CIDRSetTest, CascadedMerge) {
    CIDRSet set;

    // 构造 00/2, 01/2, 10/2, 11/2
    ASSERT_TRUE(set.Insert(make_in6_addr("0000::"), 2));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("4000::"), 2));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("8000::"), 2));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("c000::"), 2));
    set.Validate();

    // 所有 /2 覆盖后，应最终合并到 root
    ASSERT_FALSE(set.IsEmpty());
    ASSERT_TRUE(set.Find(make_in6_addr("::"), 0));
    ASSERT_TRUE(set.Find(make_in6_addr("abcd::"), 64));
    set.Validate();
}

TEST(CIDRSetTest, MergeUnderExistingShortPrefix) {
    CIDRSet set;

    ASSERT_TRUE(set.Insert(make_in6_addr("8000::"), 1));
    set.Validate();

    // 已有 ::/1，应当直接命中，不依赖 merge
    ASSERT_TRUE(set.Find(make_in6_addr("ffff::"), 128));

    ASSERT_TRUE(set.Insert(make_in6_addr("::"), 1));
    set.Validate();
    ASSERT_TRUE(set.Find(make_in6_addr("ffff::"), 128));
}

TEST(CIDRSetTest, NoMergeWithPartialCoverage) {
    CIDRSet set;

    ASSERT_TRUE(set.Insert(make_in6_addr("0000::"), 2));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("4000::"), 2));
    set.Validate();

    // 只覆盖 0*，不应合并到 /
    ASSERT_FALSE(set.Find(make_in6_addr("8000::"), 128));
    ASSERT_FALSE(set.Find(make_in6_addr("c000::"), 128));
    set.Validate();
}

TEST(CIDRSetTest, InsertAfterMerge) {
    CIDRSet set;

    ASSERT_TRUE(set.Insert(make_in6_addr("::"), 1));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("8000::"), 1));
    set.Validate();

    // 此时 root 已是 leaf
    ASSERT_FALSE(set.Insert(make_in6_addr("4000::"), 2));
    set.Validate();

    ASSERT_TRUE(set.Find(make_in6_addr("4000::"), 128));
    set.Validate();
}

TEST(CIDRSetTest, ClearAfterMerge) {
    CIDRSet set;

    ASSERT_TRUE(set.Insert(make_in6_addr("::"), 1));
    set.Validate();
    ASSERT_TRUE(set.Insert(make_in6_addr("8000::"), 1));
    set.Validate();

    set.Clear();
    set.Validate();

    ASSERT_FALSE(set.Find(make_in6_addr("::"), 0));
    set.Validate();
    ASSERT_TRUE(set.IsEmpty());
    set.Validate();
}

TEST(CIDRSetTest, Export) {
    CIDRSet set;
    ASSERT_TRUE(set.Insert(make_in6_addr("C000::"), 1));
    set.Validate();
    std::vector<CIDR> container;
    set.Export<in6_addr>(container);
    ASSERT_EQ(1, container.size());
    ASSERT_EQ(container[0], CIDR(make_in6_addr("8000::"), 1));
}

TEST(CIDRSetTest, Export2) {

    // /0
    CIDRSet set;
    ASSERT_TRUE(set.Insert(make_in6_addr("FFFF::"), 0));
    set.Validate();

    std::vector<CIDR> v;
    set.Export<in6_addr>(v);

    ASSERT_EQ(1u, v.size());
    ASSERT_EQ(v[0], CIDR(make_in6_addr("::"), 0));

    // /128
    set.Clear();
    ASSERT_TRUE(set.Insert(make_in6_addr("2001:db8::1"), 128));
    set.Validate();

    v.clear();
    set.Export<in6_addr>(v);

    ASSERT_EQ(1u, v.size());
    ASSERT_EQ(v[0], CIDR(make_in6_addr("2001:db8::1"), 128));

    {
        CIDRSet set;
        ASSERT_TRUE(set.Insert(make_in6_addr("F000::"), 3));
        set.Validate();

        v.clear();
        set.Export<in6_addr>(v);

        ASSERT_EQ(1u, v.size());
        ASSERT_EQ(v[0], CIDR(make_in6_addr("E000::"), 3));
    }
    {
        CIDRSet set;
        ASSERT_TRUE(set.Insert(make_in6_addr("1234:5678::"), 16));
        set.Validate();

        v.clear();
        set.Export<in6_addr>(v);

        ASSERT_EQ(1u, v.size());
        ASSERT_EQ(v[0], CIDR(make_in6_addr("1234::"), 16));
    }
    {
        CIDRSet set;
        ASSERT_TRUE(set.Insert(make_in6_addr("2000::"), 3));
        ASSERT_FALSE(set.Insert(make_in6_addr("3000::"), 4)); // 被 /3 覆盖
        set.Validate();

        v.clear();
        set.Export<in6_addr>(v);

        ASSERT_EQ(1u, v.size());
        ASSERT_EQ(v[0], CIDR(make_in6_addr("2000::"), 3));
    }
    {
        CIDRSet set;
        ASSERT_TRUE(set.Insert(make_in6_addr("4000::"), 2)); // 01xx
        ASSERT_TRUE(set.Insert(make_in6_addr("8000::"), 1)); // 1xxx
        set.Validate();

        v.clear();
        set.Export<in6_addr>(v);

        ASSERT_EQ(2u, v.size());

        // 顺序取决于 DFS 实现，但内容必须一致
        ASSERT_TRUE(
            (v[0] == CIDR(make_in6_addr("4000::"), 2) &&
            v[1] == CIDR(make_in6_addr("8000::"), 1)) ||
            (v[1] == CIDR(make_in6_addr("4000::"), 2) &&
            v[0] == CIDR(make_in6_addr("8000::"), 1))
        );
    }
    {
        CIDRSet set;
        ASSERT_TRUE(set.Insert(make_in6_addr("A000::"), 3));
        ASSERT_TRUE(set.Insert(make_in6_addr("8000::"), 1));
        set.Validate();

        std::vector<CIDR> v1;
        set.Export<in6_addr>(v1);

        CIDRSet set2;
        ASSERT_TRUE(set2.Insert(make_in6_addr("8000::"), 1));
        ASSERT_FALSE(set2.Insert(make_in6_addr("A000::"), 3));
        set2.Validate();

        std::vector<CIDR> v2;
        set2.Export<in6_addr>(v2);

        ASSERT_EQ(v1.size(), v2.size());
        ASSERT_EQ(v1[0], v2[0]);
    }
}
