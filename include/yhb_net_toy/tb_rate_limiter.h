/*
MIT License

Copyright (c) 2023 YinHaiBo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef YHB_NET_TOY_TB_RATE_LIMITER
#define YHB_NET_TOY_TB_RATE_LIMITER

#include <cstddef>
#include <cstdint>

namespace yhb_net_toy {

class TBRateLimiter {
public:

    /**
     * @brief Construction parameters.
     */
    struct Params {
        uint64_t committed_info_rate;   // CIR. Traffic per second (bytes/s).
                                        //      The speed of token into the C bucket.
        uint64_t committed_burst_size;  // CBS. Size of the C bucket, in bytes.
        uint64_t excess_info_rate;      // EIR. Speed of the E bucket (bytes/s).
        uint64_t excess_burst_size;     // EBS. Size of the E bucket, in bytes.
    };


    /**
     * @brief Construct a new TBRateLimiter object.
     *
     * @param params[in]    The parameters of Token-Bucket.
     * @param now           Current time by milliseconds.
     */
    TBRateLimiter(const Params & params, uint64_t now);

    /**
     * @brief Limiting action
     */
    enum class Action {
        ALLOW,          // Allow the traffic pass through, no limiting.
        DENY,           // Traffic overflow, deny the request.
    };

    /**
     * @brief Determine whether the traffic can pass through.
     *
     * @param size[in]  Bytes of traffic.
     * @param now[in]   Current time by milliseconds.
     *
     * @return Limiting action, allow or deny.
     */
    Action Execute(size_t size, uint64_t now);

    size_t GetCBucketTokens() const {
        return this->bucket_committed.GetTokens();
    }

    size_t GetEBucketTokens() const {
        return this->bucket_excess.GetTokens();
    }

private:

    class Bucket {
    public:
        Bucket(uint64_t size, uint64_t info_rate, bool init_full);
        unsigned Put(unsigned elapsed_milliseconds);
        bool Acquire(size_t count);
        uint64_t GetTokens() const { return this->tokens; }
    private:
        uint64_t const size;               // Capacity of the bucket.
        uint64_t const info_rate;          // Speed of token generation, in bytes/s.
        uint64_t tokens;                   // Current token count.
        uint64_t generated_remainder;      // Remainder of generated tokens before division by 1000.
    };

    uint64_t last_time;
    Bucket bucket_committed;
    Bucket bucket_excess;
};

}

#endif
