#ifndef BITCOIN_TXCOMPRESSOR_H_
#define BITCOIN_TXCOMPRESSOR_H_ 1

#include "primitives/transaction.h"
#include "serialize.h"
#include "bitpacker.h"
#include "compressor.h"
#include "utilstrencodings.h"

class TxCompressor
{
    const CTransaction* tx;

    struct InputScriptResult {
        std::vector<unsigned char> header;
        std::vector<std::pair<bool, std::vector<unsigned char>>> data;
        size_t pos;
        bool set;
        InputScriptResult() : set(false) {}
    };

    struct InputScriptRegister {
        int base;
        InputScriptResult results[8];

        InputScriptRegister() : base(0) {}
    };

    static int IdentifyPubKey(const unsigned char* data, size_t len);
    static int IdentifySignature(const unsigned char* data, size_t len);
    static InputScriptResult CompressInputScript(const CScript& scriptSig, const CScriptWitness& scriptWitness);

public:

    TxCompressor(const CTransaction* tx_) : tx(tx_) {}

    template<typename Stream>
    void SerializeTxinScript(Stream& s, size_t pos, InputScriptRegister& reg) const
    {
        static const CScriptWitness scriptWitnessDummy;
        const CScript& scriptSig = tx->vin[pos].scriptSig;
        const CScriptWitness& scriptWitness = pos < tx->wit.vtxinwit.size() ? tx->wit.vtxinwit[pos].scriptWitness : scriptWitnessDummy;
        // Search for an exact match.
        for (int i = 0; i < 8; i++) {
            const InputScriptResult& presult = reg.results[(reg.base + 7 - i) & 7];
            if (!presult.set) {
                break;
            }
            const CScript& scriptSigPrev = tx->vin[presult.pos].scriptSig;
            const CScriptWitness& scriptWitnessPrev = presult.pos < tx->wit.vtxinwit.size() ? tx->wit.vtxinwit[presult.pos].scriptWitness : scriptWitnessDummy;
            if (scriptSig == scriptSigPrev && scriptWitness.stack == scriptWitnessPrev.stack) {
//                fprintf(stderr, "exactmatch(%i)\n", i);
                s << (unsigned char)(0xD0 + i);
                return;
            }
        }
        // Compress input, ignoring repetition.
        InputScriptResult result = CompressInputScript(scriptSig, scriptWitness);
        // Search for partial matches.
        for (int i = 0; i < 8; i++) {
            const InputScriptResult& presult = reg.results[(reg.base + 7 - i) & 7];
            if (!presult.set) {
                break;
            }
//            fprintf(stderr, "Trying match with input %i\n", (int)presult.pos);
            if (presult.header != result.header) {
                continue;
            }
            bool ok = true;
            for (size_t p = 0; p < result.data.size(); p++) {
                if (!presult.data[p].first && presult.data[p].second != result.data[p].second) {
                    ok = false;
                    break;
                }
            }
            if (!ok) {
                continue;
            }
//            fprintf(stderr, "partmatch(%i)\n", i);
            s << (unsigned char)(0xD8 + i);
            for (size_t p = 0; p < result.data.size(); p++) {
                if (presult.data[p].first) {
                    s.write((const char*)result.data[p].second.data(), result.data[p].second.size());
                }
            }
            return;
        }
        // Write explicitly.
//        fprintf(stderr, "header(%i, hex=%s)\n", (int)result.header.size(), HexStr(result.header.begin(), result.header.end()).c_str());
        s.write((const char*)result.header.data(), result.header.size());
        for (size_t p = 0; p < result.data.size(); p++) {
//            fprintf(stderr, "%cdata(%i, hex=%s)\n", result.data[p].first ? 'r' : 'n', (int)result.data[p].second.size(), HexStr(result.data[p].second.begin(), result.data[p].second.end()).c_str());
            s.write((const char*)result.data[p].second.data(), result.data[p].second.size());
        }
        // Remember result for future exact matches.
        reg.results[reg.base].pos = pos;
        reg.results[reg.base].header = std::move(result.header);
        reg.results[reg.base].data = std::move(result.data);
        reg.results[reg.base].set = true;
        reg.base = (reg.base + 1) & 7;
    }

    template<typename Stream>
    void SerializeTxin(Stream& s, const CTxIn& txin, bool cont, uint32_t& nSequenceRegister) const
    {
        uint8_t txin_byte = cont << 7;
        bool prevout_index_varint = false;
        bool nsequence_uint32 = false;
        bool prevout_hash = true;
        if (txin.prevout.IsNull()) {
            txin_byte += 23;
            prevout_hash = false;
        } else if (txin.prevout.n <= 22) {
            txin_byte += txin.prevout.n;
        } else {
            txin_byte += 24;
            prevout_index_varint = true;
        }
        if (txin.nSequence == 0xFFFFFFFF) {
            txin_byte += 25;
        } else if (txin.nSequence == 0xFFFFFFFE) {
            txin_byte += 50;
        } else if (txin.nSequence == nSequenceRegister) {
            txin_byte += 100;
        } else {
            nsequence_uint32 = true;
            txin_byte += 75;
        }
        s << txin_byte;
        if (prevout_index_varint) {
            s << VARINT(txin.prevout.n);
        }
        if (nsequence_uint32) {
            s << txin.nSequence;
            nSequenceRegister = txin.nSequence;
        }
        if (prevout_hash) {
            s << txin.prevout.hash;
        }
    }

    template<typename Stream>
    void SerializeTxout(Stream& s, const CTxOut& txout, bool cont) const
    {
        const CScript& sc = txout.scriptPubKey;
        size_t size = sc.size();
        uint8_t header = cont << 7;
        int pubkey;
        if (size == 25 && sc[0] == OP_DUP && sc[1] == OP_HASH160 && sc[2] == 20 && sc[23] == OP_EQUALVERIFY && sc[24] == OP_CHECKSIG) {
            s << header;
            s.write((const char*)&sc[3], 20);
        } else if (size == 23 && sc[0] == OP_HASH160 && sc[1] == 20 && sc[22] == OP_EQUAL) {
            header |= 1;
            s << header;
            s.write((const char*)&sc[2], 20);
        } else if (size == 22 && sc[0] == OP_0 && sc[1] == 20) {
            header |= 2;
            s << header;
            s.write((const char*)&sc[2], 20);
        } else if (size == 34 && sc[0] == OP_0 && sc[1] == 32) {
            header |= 3;
            s << header;
            s.write((const char*)&sc[2], 32);
        } else if (size == 34 && sc[0] >= OP_1 && sc[0] <= OP_16 && sc[1] == 32) {
            header |= (0x20 + sc[0] - OP_1);
            s << header;
            s.write((const char*)&sc[2], 32);
        } else if (size >= 4 && size <= 42 && sc[0] >= OP_1 && sc[0] <= OP_16 && sc[1] == size - 2) {
            header |= (0x30 + sc[0] - OP_1);
            s << header;
            s << (unsigned char)(size - 2);
            s.write((const char*)&sc[2], size - 2);
        } else if (size <= 62) {
            header |= (0x40 + size);
            s << header;
            s.write((const char*)&sc[0], size);
        } else if ((size == 35 || size == 67) && sc[0] == size - 2 && sc.back() == OP_CHECKSIG && (pubkey = IdentifyPubKey(&sc[2], sc[0])) != -1) {
            header |= (4 + pubkey);
            s << header;
            s.write((const char*)&sc[2], 32);
        } else {
            header |= 0x7F;
            s << header;
            s << VARINT(size - 63);
            s.write((const char*)&sc[0], size);
        }
        uint64_t camount = CTxOutCompressor::CompressAmount(txout.nValue);
        s << VARINT(camount);
    }

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        bool nlocktime_uint32 = false;
        bool nlocktime_varint = false;
        bool nversion_uint32 = false;
        uint8_t header_byte = 0;
        if (tx->nLockTime && tx->nLockTime <= 2113663) {
            header_byte |= 1 << 4;
            nlocktime_varint = true;
        } else if (tx->nLockTime) {
            header_byte |= 2 << 4;
            nlocktime_uint32 = true;
        }
        if (tx->nVersion <= 14 && tx->nVersion >= 0) {
            header_byte |= tx->nVersion;
        } else {
            header_byte |= 15;
            nversion_uint32 = true;
        }
        s << header_byte;
        if (nlocktime_uint32) {
            s << tx->nLockTime;
        }
        if (nlocktime_varint) {
            s << VARINT(tx->nLockTime);
        }
        if (nversion_uint32) {
            s << tx->nVersion;
        }
        uint32_t nsequence_register = 0xFFFFFFFD;
        InputScriptRegister inputscript_register;
        for (size_t i = 0; i < tx->vin.size(); i++) {
            SerializeTxin(s, tx->vin[i], i + 1 < tx->vin.size(), nsequence_register);
            SerializeTxinScript(s, i, inputscript_register);
        }
        for (size_t i = 0; i < tx->vout.size(); i++) {
            SerializeTxout(s, tx->vout[i], i + 1 < tx->vout.size());
        }
    }
};

class TxDecompressor
{
    CMutableTransaction* tx;

public:
    TxDecompressor(CMutableTransaction* tx_) : tx(tx_) {}

    template<typename Stream>
    void Unserialize(Stream& s)
    {
    }
};

#endif