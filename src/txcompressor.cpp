#include "txcompressor.h"
#include "pubkey.h"
#include "streams.h"


enum InputType {
    INPUT_UNKNOWN,
    INPUT_NOTHING,
    INPUT_NORMAL,
    INPUT_WITNESS,
    INPUT_WITNESS_P2SH,
    INPUT_BOTH
};

int TxCompressor::IdentifyPubKey(const unsigned char* ptr, size_t len) {
    if (len == 33) {
        if (*ptr == 2) {
            return 2;
        } else if (*ptr == 3) {
            return 3;
        }
    } else if (len == 65) {
        if (*ptr == 4) {
            CPubKey pubkey;
            pubkey.Set(ptr, ptr + 65);
            if (pubkey.IsFullyValid()) {
                return ptr[64] & 1;
            }
        }
    }
    return -1;
}

int TxCompressor::IdentifySignature(const unsigned char* ptr, size_t len) {
    if (len < 9) return -1;
    if (len > 73) return -1;
    if (ptr[0] != 0x30) return -1;
    if (ptr[1] != len - 3) return -1;
    unsigned int lenR = ptr[3];
    if (5 + lenR >= len) return -1;
    unsigned int lenS = ptr[5 + lenR];
    if ((size_t)(lenR + lenS + 7) != len) return -1;
    if (ptr[2] != 0x02) return -1;
    if (lenR == 0) return -1;
    if (ptr[4] & 0x80) return -1;
    if (lenR > 1 && (ptr[4] == 0x00) && !(ptr[5] & 0x80)) return -1;
    if (ptr[lenR + 4] != 0x02) return -1;
    if (lenS == 0) return -1;
    if (ptr[lenR + 6] & 0x80) return -1;
    if (lenS > 1 && (ptr[lenR + 6] == 0x00) && !(ptr[lenR + 7] & 0x80)) return -1;
    return (ptr[len - 1] == 1);
}

std::vector<unsigned char> ExtractPubKey(const unsigned char* ptr, size_t len, int pubkey) {
    return std::vector<unsigned char>(&ptr[1], &ptr[1] + 32);
}

std::vector<unsigned char> ExtractSignature(const unsigned char* ptr, size_t len, int sig) {
    std::vector<unsigned char> ret;
    ret.resize(65 - sig);
    unsigned int lenR = ptr[3];
    unsigned int lenS = ptr[5 + lenR];
    const unsigned char* ptrR = ptr + 4;
    const unsigned char* ptrS = ptr + 6 + lenR;
    while (lenR > 0 && *ptrR == 0) {
        lenR--;
        ptrR++;
    }
    while (lenS > 0 && *ptrS == 0) {
        lenS--;
        ptrS++;
    }
    memcpy(&ret[0], ptrR, lenR);
    memcpy(&ret[32], ptrS, lenS);
    if (sig == 0) {
        ret[64] = ptr[len - 1];
    }
    return ret;
}

std::vector<unsigned char> ExtractPush(const unsigned char* ptr, size_t len) {
    std::vector<unsigned char> ret;
    ret.reserve(len + 3);
    {
        CVectorWriter ss(0, 0, ret, 0);
        ss << VARINT(len);
        ss.write((const char*)ptr, len);
    }
    return ret;
}

TxCompressor::InputScriptResult TxCompressor::CompressInputScript(const CScript& scriptSig, const CScriptWitness& scriptWitness)
{
    InputScriptResult result;

    CVectorWriter ss(0, 0, result.header, 0);

    InputType typ;

    {
        // Input script header
        BitWriter<CVectorWriter> writer(&ss);
        if (scriptSig.size() == 0 && scriptWitness.stack.size() == 0) {
//            fprintf(stderr, "<nothing>\n");
            typ = INPUT_NOTHING;
            writer.writebits(0xC8, 8);
        } else if (scriptWitness.stack.size() == 0) {
//            fprintf(stderr, "<normal>\n");
            typ = INPUT_NORMAL;
            writer.writebits(0, 2);
        } else if (scriptSig.size() == 0) {
//            fprintf(stderr, "<witness>\n");
            typ = INPUT_WITNESS;
            writer.writebits(1, 2);
        } else {
            typ = INPUT_BOTH;
            if (scriptSig.size() == 23 && scriptWitness.stack.size() == 2 && IdentifyPubKey(scriptWitness.stack[1].data(), scriptWitness.stack[1].size()) != -1 && IdentifySignature(scriptWitness.stack[0].data(), scriptWitness.stack[0].size())) {
                unsigned char h160[20];
                CHash160().Write(scriptWitness.stack[1].data(), scriptWitness.stack[1].size()).Finalize(h160);
                if (scriptSig[0] == 22 && scriptSig[1] == 0 && scriptSig[2] == 20 && memcmp(&scriptSig[0] + 3, h160, 20) == 0) {
//                    fprintf(stderr, "<p2sh p2wpkh>\n");
                    typ = INPUT_WITNESS_P2SH;
                    writer.writebits(2, 2);
                }
            } else if (scriptSig.size() == 35) {
                unsigned char h256[32];
                CHash256().Write(scriptWitness.stack.back().data(), scriptWitness.stack.back().size()).Finalize(h256);
                if (scriptSig[0] == 34 && scriptSig[1] == 0 && scriptSig[2] == 32 && memcmp(&scriptSig[0] + 3, h256, 32) == 0) {
//                    fprintf(stderr, "<p2sh p2ws>\n");
                    typ = INPUT_WITNESS_P2SH;
                    writer.writebits(2, 2);
                }
            }
            if (typ == INPUT_BOTH) {
//                fprintf(stderr, "<both>\n");
                writer.writebits(0x18, 5);
            }
        }

        // Process scriptSig items
        if (typ == INPUT_NORMAL || typ == INPUT_WITNESS_P2SH || typ == INPUT_BOTH) {
            CScript::const_iterator it = scriptSig.begin(); // Before the current (optional) push
            while (it != scriptSig.end()) {
                CScript::const_iterator it2 = it; // After the optional push, before signature/pubkey.
                int sig = -1;
                // Process potentially multiple pushes, until we reach the end, or a pubkey.
                while (it2 != scriptSig.end()) {
                    if (*it2 >= 60 && *it2 <= 73) {
                        sig = IdentifySignature(&*(it2 + 1), *it2);
                        if (sig != -1) {
                            break;
                        }
                    }
                    opcodetype optype;
                    if (!scriptSig.GetOp(it2, optype)) {
                        it2 = scriptSig.end();
                    }
                }
                if (it2 != it) { // We have a push.
//                    fprintf(stderr, "push(%i)\n", (int)(it2 - it));
                    writer.writebits(((it2 != scriptSig.end()) << 2) | 2, 3);
                    result.data.emplace_back(false, ExtractPush(&*it, it2 - it));
                    it = it2;
                }
                if (sig != -1) {
                    CScript::const_iterator it3 = it2 + 1 + (*it2); // After the signature, before optional pubkey.
                    CScript::const_iterator it4 = it3; // After the optional pubkey.
                    int pubkey = -1;
                    if (it3 != scriptSig.end() && (*it3 == 33 || *it3 == 65)) {
                        pubkey = IdentifyPubKey(&*(it3 + 1), *it3);
                        if (pubkey != -1) {
                            it4 = it3 + 1 + *it3;
                        }
                    }
                    result.data.emplace_back(true, ExtractSignature(&*(it2 + 1), *it2, sig));
                    if (pubkey != -1) {
//                        fprintf(stderr, "sigpubkey(sig=%i,pubkey=%i)\n", sig, pubkey);
                        writer.writebits((it4 != scriptSig.end()) << 5 | pubkey << 1 | sig, 6);
                        result.data.emplace_back(false, ExtractPubKey(&*(it3 + 1), *it3, pubkey));
                    } else {
//                        fprintf(stderr, "sig(sig=%i)\n", sig);
                        writer.writebits((it4 != scriptSig.end()) << 3 | 2 | sig, 4);
                    }
                    it = it4;
                }
            }
        }

        // Process witness items
        if (typ == INPUT_WITNESS || typ == INPUT_BOTH) {
            size_t i = 0;
            size_t count = scriptWitness.stack.size();
            while (i != count) {
                const auto& item = scriptWitness.stack[i];
                int sig = IdentifySignature(item.data(), item.size());
                if (sig != -1) {
                    int pubkey = -1;
                    if (i + 1 != count) {
                        const auto& nitem = scriptWitness.stack[i + 1];
                        pubkey = IdentifyPubKey(nitem.data(), nitem.size());
                    }
                    result.data.emplace_back(false, ExtractSignature(item.data(), item.size(), sig));
                    if (pubkey != -1) {
//                        fprintf(stderr, "wsigpubkey(sig=%i,pubkey=%i)\n", sig, pubkey);
                        const auto& nitem = scriptWitness.stack[i + 1];
                        writer.writebits((i + 2 != count) << 5 | pubkey << 1 | sig, 6);
                        result.data.emplace_back(true, ExtractPubKey(nitem.data(), nitem.size(), pubkey));
                        i++;
                    } else {
//                        fprintf(stderr, "wsig(sig=%i)\n", sig);
                        writer.writebits((i + 1 != count) << 3 | 2 | sig, 4);
                    }
                } else {
//                    fprintf(stderr, "push(len=%i)\n", (int)item.size());
                    writer.writebits((i + 1 != count) << 2 | 2, 3);
                    result.data.emplace_back(false, ExtractPush(item.data(), item.size()));
                }
                i++;
            }
        }
    }

    return result;
}
