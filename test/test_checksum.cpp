#include "yhb_net_toy/checksum.h"
#include "yhb_net_toy/platform.h"
#include <gtest/gtest.h>
#include <cstring>


using Checksum = yhb_net_toy::Checksum;

size_t const IP_HEAD_LEN = 20;
size_t const UDP_HEAD_LEN = 8;

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

struct
#ifdef __GNUC__
    __attribute__((packed))
#endif
pseudo_header {
    uint32_t src_ip;
    uint32_t dest_ip;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t length;
};

#ifdef _MSC_VER
#pragma pack(pop)
#endif

static const char TEST_IP_HEAD[] =
    "\x45\x00\x00\x29\x38\xf7\x40\x00\x80\x06\x09\x8a\xc0\xa8\x11\x64"
    "\xa1\x75\x44\xcc";

static const char TEST_IP_UDP[] =
    "\x45\x00\x00\x4e\x3b\xfb\x00\x00\x80\x11\x1c\x88\xc0\xa8\x11\x64"
    "\x08\x08\x08\x08"
    "\xe5\x9d\x00\x35\x00\x3a\xbc\x89"
    "\x3b\x59\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x06\x6d\x6f\x62"
    "\x69\x6c\x65\x06\x65\x76\x65\x6e\x74\x73\x04\x64\x61\x74\x61\x09"
    "\x6d\x69\x63\x72\x6f\x73\x6f\x66\x74\x03\x63\x6f\x6d\x00\x00\x01"
    "\x00\x01";

static const char TEST_ICMP[] =
    "\x45\x00\x00\x54\x46\x9d\x00\x00\x80\x01\xe2\x2f\xc0\xa8\x11\x64"
    "\x3d\x8b\x02\x45"
    "\x08\x00\x5c\x0c\x03\x8f\x00\x06\xdd\xb8\x34\x64\x00\x00\x00\x00"
    "\xc2\x6e\x05\x00\x00\x00\x00\x00\x10\x11\x12\x13\x14\x15\x16\x17"
    "\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27"
    "\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37";

static const char TEST_TCP[] =
    "\x45\x00\x00\x29\x49\x5a\x40\x00\x80\x06\xf9\x26\xc0\xa8\x11\x64"
    "\xa1\x75\x44\xcc"
    "\x04\x4a\x01\xbb\xf9\xe0\xf2\x70\x1d\x71\x88\x56\x50\x10\x03\xfd"
    "\x5b\x6a\x00\x00"
    "\x00";

TEST(ChecksumTest, Base) {
    uint16_t cs = Checksum::Calculate(TEST_IP_HEAD, IP_HEAD_LEN);
    ASSERT_EQ(cs, 0xffff);

    char buf[512];
    memcpy(buf, TEST_IP_UDP, IP_HEAD_LEN);
    buf[10] = buf[11] = 0;
    cs = ~Checksum::Calculate(buf, IP_HEAD_LEN);
    ASSERT_EQ(cs, *(const uint16_t *)&TEST_IP_UDP[10]);
}

TEST(ChecksumTest, Udp) {
    char buf[512];
    memcpy(buf, TEST_IP_UDP, sizeof(TEST_IP_UDP) - 1);
    *(uint16_t *)(&buf[IP_HEAD_LEN + 6]) = 0;

    pseudo_header ph;
    ph.src_ip = *(uint32_t const *)&buf[12];
    ph.dest_ip = *(uint32_t const *)&buf[16];
    ph.reserved = 0;
    ph.protocol = 17;
    ph.length = htons(sizeof(TEST_IP_UDP) - 1 - IP_HEAD_LEN);

    uint16_t cs = Checksum::Calculate(&ph, sizeof(ph));
    cs = Checksum::Calculate(buf + IP_HEAD_LEN, ntohs(ph.length), cs);
    cs = ~cs;

    ASSERT_EQ(cs, *(const uint16_t *)&TEST_IP_UDP[IP_HEAD_LEN + 6]);
}

TEST(ChecksumTest, Odd) {
    char buf[512];
    memcpy(&buf[1], TEST_IP_HEAD, IP_HEAD_LEN);
    uint16_t cs = Checksum::Calculate(&buf[1], IP_HEAD_LEN);
    ASSERT_EQ(0xffff, cs);
}

TEST(ChecksumTest, Icmp) {
    uint16_t cs = Checksum::Calculate(&TEST_ICMP[IP_HEAD_LEN], sizeof(TEST_ICMP) - 1 - IP_HEAD_LEN);
    ASSERT_EQ(0xffff, cs);
}

TEST(ChecksumTest, Tcp) {
    auto const tcp_len = sizeof(TEST_TCP) - 1 - IP_HEAD_LEN;
    for (int i = 0; i < 2; ++i) {
        uint16_t cs;
        if (i == 0) {
            pseudo_header ph;
            ph.src_ip = *(const uint32_t *)(TEST_TCP + 12);
            ph.dest_ip = *(const uint32_t *)(TEST_TCP + 16);
            ph.reserved = 0;
            ph.protocol = 6;
            ph.length = htons(tcp_len);
            cs = Checksum::Calculate(&ph, sizeof(ph));
        } else {
            uint32_t u32 = *(const uint16_t *)(TEST_TCP + 12);
            u32 += *(const uint16_t *)(TEST_TCP + 14);
            u32 += *(const uint16_t *)(TEST_TCP + 16);
            u32 += *(const uint16_t *)(TEST_TCP + 18);
            u32 += htons(6);
            u32 += htons(tcp_len);
            for (;;) {
                auto const h = (u32 >> 16) & 0xffff;
                if (h == 0) {
                    break;
                }
                u32 = (u32 & 0xffff) + h;
            }
            cs = static_cast<uint16_t>(u32);
        }
        cs = Checksum::Calculate(TEST_TCP + IP_HEAD_LEN, tcp_len, cs);
        ASSERT_EQ(0xffff, cs);
    }
}
