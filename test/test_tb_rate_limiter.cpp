#include <gtest/gtest.h>
#include "tb_rate_limiter.h"

using TBRateLimiter = yhb::TBRateLimiter;
using Action = yhb::TBRateLimiter::Action;

TEST(TBRateLimiter, Generic) {
    const TBRateLimiter::Params params {
        1000,   // CIR
        5000,   // CBS
        1000,   // EIR
        6000,   // EBS
    };
    TBRateLimiter trl(params, 0);
    ASSERT_EQ(5000, trl.GetCBucketTokens());                // C bucket is full as beginning.
    ASSERT_EQ(0, trl.GetEBucketTokens());                   // And E bucket is empty.

    uint64_t now = 0;
    ASSERT_EQ(Action::DENY, trl.Execute(5001, now));        // Deny (C bucket has 5000 only, and E bucket is empty)

    now += 5001;
    ASSERT_EQ(Action::DENY, trl.Execute(5002, now));
    ASSERT_EQ(5000, trl.GetCBucketTokens());                // The count of token remains unchanged, even request has been rejected.
    ASSERT_EQ(5001, trl.GetEBucketTokens());                // Now, there are 5001 tokens in the E bucket.

    now += 1;
    ASSERT_EQ(Action::ALLOW, trl.Execute(5002, now));      // E bucket: 5002
    ASSERT_EQ(5000, trl.GetCBucketTokens());                // C bucket remains unchanged.
    ASSERT_EQ(0, trl.GetEBucketTokens());                   // E bucket is empty now.

    ASSERT_EQ(Action::ALLOW, trl.Execute(5000, now));
    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    now += 1;
    ASSERT_EQ(Action::DENY, trl.Execute(2, now));           // There are one token in the C bucket (1ms elapsed)
    now += 1;
    ASSERT_EQ(Action::ALLOW, trl.Execute(2, now));
    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    now += 5700;
    ASSERT_EQ(Action::DENY, trl.Execute(100000, now));
    ASSERT_EQ(5000, trl.GetCBucketTokens());                // 5700 ms elapsed, C bucket is full.
    ASSERT_EQ(700, trl.GetEBucketTokens());                 // And 700 tokens into E bucket.

    now += 1000000;
    ASSERT_EQ(Action::ALLOW, trl.Execute(5700, now));
    ASSERT_EQ(5000, trl.GetCBucketTokens());                // C bucket remains unchanged (should allocate from E bucket)
    ASSERT_EQ(300, trl.GetEBucketTokens());                 // There are 300 tokens remain in the E bucket.
}

TEST(TBRateLimiter, LowRateAccumulatesTokens) {
    const TBRateLimiter::Params params {
        500,    // CIR
        4,      // CBS
        250,    // EIR
        4,      // EBS
    };
    TBRateLimiter trl(params, 0);

    ASSERT_EQ(4, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    // Consume all initial tokens from the C bucket.
    ASSERT_EQ(Action::ALLOW, trl.Execute(4, 0));
    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    // 500 bytes/s means 1 token every 2 ms, so 1 ms is still not enough.
    ASSERT_EQ(Action::DENY, trl.Execute(1, 1));
    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    // After 2 ms in total, the C bucket has accumulated exactly 1 token.
    ASSERT_EQ(Action::ALLOW, trl.Execute(1, 2));
    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    // Another 8 ms refills the C bucket back to its 4-token capacity.
    ASSERT_EQ(Action::ALLOW, trl.Execute(4, 10));
    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    // After 4 ms more, the C bucket has accumulated 2 tokens, and this request consumes 1 of them.
    ASSERT_EQ(Action::ALLOW, trl.Execute(1, 14));
    ASSERT_EQ(1, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());
}

TEST(TBRateLimiter, ZeroRateNeverRefillsBuckets) {
    const TBRateLimiter::Params params {
        0,      // CIR
        4,      // CBS
        0,      // EIR
        4,      // EBS
    };
    TBRateLimiter trl(params, 0);

    ASSERT_EQ(4, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    ASSERT_EQ(Action::ALLOW, trl.Execute(4, 0));
    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    ASSERT_EQ(Action::DENY, trl.Execute(1, 10000));
    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());
}

TEST(TBRateLimiter, ZeroCapacityNeverStoresTokens) {
    const TBRateLimiter::Params params {
        1000,   // CIR
        0,      // CBS
        1000,   // EIR
        0,      // EBS
    };
    TBRateLimiter trl(params, 0);

    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());

    ASSERT_EQ(Action::DENY, trl.Execute(1, 0));
    ASSERT_EQ(Action::DENY, trl.Execute(1, 10000));
    ASSERT_EQ(0, trl.GetCBucketTokens());
    ASSERT_EQ(0, trl.GetEBucketTokens());
}
