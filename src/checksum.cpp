/*
 * Original code by Curt McDowell
 * Broadcom Corp.
 * csm@broadcom.com
 *
 * Modified by YinHaiBo 2023.11
 */

#include "yhb_net_toy/checksum.h"

/** Split an u32_t in two u16_ts and add them up */
#define FOLD_U32T(u) \
    ((uint32_t)(((u) >> 16) + ((u) & 0x0000ffffUL)))

/** Swap the bytes in an u16_t: much like htons() for little-endian */
#define SWAP_BYTES_IN_WORD(w) \
    (((w) & 0xff) << 8) | (((w) & 0xff00) >> 8)

uint16_t yhb_net_toy::Checksum::Calculate(const void * dataptr, size_t len, uint16_t additional) {
    const uint8_t * pb = (const uint8_t *)dataptr;
    const int odd = ((intptr_t)pb & 1);

    uint16_t t = 0;

    /* Get aligned to uint16_t */
    if (odd && len > 0) {
        ((uint8_t *)&t)[1] = *pb++;
        len--;
    }

    /* Add the bulk of the data */
    const uint16_t * ps = (const uint16_t *)(const void *)pb;
    uint32_t sum = 0;
    while (len > 1) {
        sum += *ps++;
        len -= 2;
    }

    /* Consume left-over byte, if any */
    if (len > 0) {
        ((uint8_t *)&t)[0] = *(const uint8_t *)ps;
    }

    /* Add end bytes */
    sum += t;

    sum += additional;

    /* Fold 32-bit sum to 16 bits
       calling this twice is probably faster than if statements... */
    sum = FOLD_U32T(sum);
    sum = FOLD_U32T(sum);

    /* Swap if alignment was odd */
    if (odd) {
        sum = SWAP_BYTES_IN_WORD(sum);
    }

    return (uint16_t)sum;
}
