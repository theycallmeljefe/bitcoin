// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITPACKER_H
#define BITCOIN_BITPACKER_H

#include "serialize.h"
#include <stdint.h>

template <typename Stream>
class BitWriter
{
private:
    Stream* stream;

    uint16_t data;
    int bits;

public:
    BitWriter(Stream* str) : stream(str), data(0), bits(0) {}

    void writebits(uint8_t d, int b) {
        data = (data << b) | d;
        bits += b;
        if (bits >= 8) {
            bits -= 8;
            uint8_t byte = data >> bits;
            stream->write((const char*)&byte, 1);
        }
    }

    ~BitWriter() {
        if (bits) {
            uint8_t byte = data << (8 - bits);
            stream->write((const char*)&byte, 1);
        }
    }
};

template <typename Stream>
class BitReader
{
private:
    Stream* stream;

    uint16_t data;
    int bits;

public:
    BitReader(Stream* str) : stream(str), data(0), bits(0) {}

    uint8_t readbits(int b) {
        if (bits < b) {
            uint8_t byte;
            stream->read((char*)&byte, 1);
            data = (data << 8) | byte;
            bits += 8;
        }
        bits -= b;
        return (data >> bits) & ((((uint16_t)1) << b) - 1);
    }

    bool empty() const {
        return (data & ((((uint16_t)1) << bits) - 1)) == 0;
    }
};

#endif // BITCOIN_BITPACKER_H
