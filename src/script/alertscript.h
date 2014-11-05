// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_ALERTSCRIPT_H
#define BITCOIN_SCRIPT_ALERTSCRIPT_H

#include "hash.h"
#include "key.h"
#include "keystore.h"
#include "pubkey.h"
#include "alert.h"
#include "script/script.h"
#include "script/interpreter.h"
#include "script/sign.h"

class SimpleSignatureChecker : public BaseSignatureChecker {
    uint256 hash;

public:
    SimpleSignatureChecker(const uint256& hashIn) : hash(hashIn) {};
    bool CheckSig(const std::vector<unsigned char>& vchSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode) const {
        CPubKey pubkey(vchPubKey);
        if (!pubkey.IsValid())
            return false;
        return pubkey.Verify(hash, vchSig);
    }
};

class SimpleSignatureCreator : public BaseSignatureCreator {
    uint256 hash;
    SimpleSignatureChecker checker;

public:
    SimpleSignatureCreator(const CKeyStore& keystoreIn, const uint256& hashIn) : BaseSignatureCreator(keystoreIn), hash(hashIn), checker(hashIn) {};
    const BaseSignatureChecker& Checker() const { return checker; }
    bool CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode) const {
        CKey key;
        if (!keystore.GetKey(keyid, key))
            return false;
        return key.Sign(hash, vchSig);
    }
};

bool VerifyAlertScript(const CScript& scriptSig, const CScript& scriptPubKey, unsigned int flags, const CUnsignedAlert& alert) {
    return VerifyScript(scriptSig, scriptPubKey, flags, SimpleSignatureChecker(SerializeHash(alert)));
}

bool SignAlertScript(const CKeyStore& keystore, const CScript& fromPubKey, const CUnsignedAlert& alert, CScript& scriptSig) {
    return ProduceSignature(SimpleSignatureCreator(keystore, SerializeHash(alert)), fromPubKey, scriptSig);
}

CScript CombineAlertSignatures(CScript scriptPubKey, const CUnsignedAlert& alert, const CScript& scriptSig1, const CScript& scriptSig2) {
    return CombineSignatures(scriptPubKey, SimpleSignatureChecker(SerializeHash(alert)), scriptSig1, scriptSig2);
}

#endif

