#include "tb_rate_limiter.h"

namespace yhb {

TBRateLimiter::TBRateLimiter(const Params & params, uint64_t now)
    : last_time(now)
    , bucket_committed(params.committed_burst_size, params.committed_info_rate, true)
    , bucket_excess(params.excess_burst_size, params.excess_info_rate, false)
{}

TBRateLimiter::Action TBRateLimiter::Execute(size_t size, uint64_t now) {
    uint64_t elapsed;
    if (now > this->last_time) {
        elapsed = now - this->last_time;
        this->last_time = now;
    } else {
        elapsed = 0;
    }

    unsigned const remain_time = bucket_committed.Put(static_cast<unsigned>(elapsed));
    if (remain_time != 0) {
        bucket_excess.Put(remain_time);
    }

    // First, try to take tokens from the C bucket.
    if (bucket_committed.Acquire(size)) {
        return Action::ALLOW;
    }

    // Not enough tokens in the C bucket, try to take from E bucket.
    if (bucket_excess.Acquire(size)) {
        return Action::ALLOW;
    }

    // Both two buckets are full
    return Action::DENY;
}

/**
 * @brief Construct a bucket.
 *
 * @param size[in]      Maximum size of the bucket, in bytes.
 * @param info_rate[in] Speed of traffic passing, in bytes/s.
 * @param init_full[in] Full the bucket when initialization.
 */
TBRateLimiter::Bucket::Bucket(uint64_t size, uint64_t info_rate, bool init_full)
    : size(size)
    , info_rate(info_rate)
    , tokens(init_full ? size : 0)
    , generated_remainder(0)
{}

/**
 * @brief Try to throw tokens into the bucket.
 *
 * @param elapsed_milliseconds[in]  Elapsed milliseconds though last call Put().
 *                                  The count of tokens which need throw into bucket equals
 *                                  elapsed_milliseconds * info_rate / 1000.
 * @return A time value in milliseconds. ('elapsed_milliseconds' subtract consumed time).
 */
unsigned TBRateLimiter::Bucket::Put(unsigned elapsed_milliseconds) {
    // When bucket full, no time consumed, return the elapsed time immediately.
    uint64_t const remain_space = this->size - this->tokens;
    if (remain_space == 0) {
        return elapsed_milliseconds;
    }
    if (elapsed_milliseconds == 0 || this->info_rate == 0) {
        return 0;
    }

    // Keep all calculations in the "numerator before division by 1000" form:
    //   generated / 1000 == newly generated tokens.
    // This preserves sub-token precision for low rates without using floating point.
    uint64_t const generated = static_cast<uint64_t>(elapsed_milliseconds) * this->info_rate +
                               this->generated_remainder;

    // Tokens are measured in bytes, so convert the remaining capacity to the same scale
    // as 'generated' before comparing them.
    uint64_t const need_to_fill = remain_space * 1000;

    if (generated >= need_to_fill) {
        // The bucket becomes full during this call. Work out how many milliseconds
        // were actually needed to reach full capacity, then return the leftover time
        // so the caller can continue refilling the E bucket.
        //
        // The bucket becomes full during this call. 'missing' is the remaining
        // generated amount needed to fill the bucket after accounting for the
        // carried remainder from previous calls.
        //
        // Because this branch guarantees:
        //   elapsed_milliseconds * info_rate + generated_remainder >= need_to_fill
        // we have:
        //   elapsed_milliseconds * info_rate >= missing
        //
        // So the minimum whole milliseconds needed to fill the bucket is:
        //   consumed_milliseconds = ceil(missing / info_rate)
        // which can never exceed 'elapsed_milliseconds'. The remaining time can
        // therefore be safely returned to refill the E bucket.
        uint64_t const missing = need_to_fill - this->generated_remainder;
        uint64_t const consumed_milliseconds = (missing + this->info_rate - 1) / this->info_rate;

        this->tokens = this->size;
        this->generated_remainder = 0;
        return static_cast<unsigned>(elapsed_milliseconds - consumed_milliseconds);
    }

    // Not enough elapsed time to fill the bucket. Store the whole-token part and keep
    // the remainder for the next call.
    uint64_t const add_tokens = generated / 1000;
    this->generated_remainder = generated % 1000;
    this->tokens += add_tokens;
    return 0;
}

/**
 * @brief   Try to acquire token.
 *
 * @param count[in] How many tokens do I want acquire.
 * @return          When there are enough tokens in bucket, consume 'count' tokens
 *                  and return true. otherwise return false, and the tokens in bucket
 *                  remain unchanged.
 */
bool TBRateLimiter::Bucket::Acquire(size_t count) {
    if (count <= this->tokens) {
        this->tokens -= count;
        return true;
    }
    return false;
}

} // End of namespace 'yhb'
