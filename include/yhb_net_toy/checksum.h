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

#ifndef YHB_NET_TOY_CHECKSUM_H
#define YHB_NET_TOY_CHECKSUM_H

#include <cstdint>
#include <cstddef>

namespace yhb_net_toy {

struct Checksum {

    /**
     * @brief Calculate the checksum (one's complement sum) of the given memory block.
     *
     * @param dataptr       Pointer to the beginning of the memory block.
     * @param len           Length of the memory block.
     * @param additional    Additional value to be accumulated in the result (e.g., for segmented calculations).
     * @return uint16_t     The result.
     * @note Please note that this result is the 16-bit sum. If it is intended to be used as an IP or TCP header checksum,
     * it needs to be using the bitwise NOT operator '~'.
     */
    static uint16_t Calculate(const void * dataptr, size_t len, uint16_t additional = 0);
};

} // End of namespace

#endif
