// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <keystore.h>

#include <util.h>

void CBasicKeyStore::ImplicitlyLearnRelatedKeyScripts(const CPubKey& pubkey)
{
    AssertLockHeld(cs_KeyStore);
    CKeyID key_id = pubkey.GetID();
    // We must actually know about this key already.
    assert(HaveKey(key_id) || mapWatchKeys.count(key_id));
    // This adds the redeemscripts necessary to detect P2WPKH and P2SH-P2WPKH
    // outputs. Technically P2WPKH outputs don't have a redeemscript to be
    // spent. However, our current IsMine logic requires the corresponding
    // P2SH-P2WPKH redeemscript to be present in the wallet in order to accept
    // payment even to P2WPKH outputs.
    // Also note that having superfluous scripts in the keystore never hurts.
    // They're only used to guide recursion in signing and IsMine logic - if
    // a script is present but we can't do anything with it, it has no effect.
    // "Implicitly" refers to fact that scripts are derived automatically from
    // existing keys, and are present in memory, even without being explicitly
    // loaded (e.g. from a file).
    if (pubkey.IsCompressed()) {
        CScript script = GetScriptForDestination(WitnessV0KeyHash(key_id));
        // This does not use AddCScript, as it may be overridden.
        CScriptID id(script);
        mapScripts[id] = std::move(script);
    }
}

bool CBasicKeyStore::GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut) const
{
    CKey key;
    if (!GetKey(address, key)) {
        LOCK(cs_KeyStore);
        WatchKeyMap::const_iterator it = mapWatchKeys.find(address);
        if (it != mapWatchKeys.end()) {
            vchPubKeyOut = it->second;
            return true;
        }
        return false;
    }
    vchPubKeyOut = key.GetPubKey();
    return true;
}

bool CBasicKeyStore::AddKeyPubKey(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    mapKeys[pubkey.GetID()] = key;
    ImplicitlyLearnRelatedKeyScripts(pubkey);
    return true;
}

bool CBasicKeyStore::HaveKey(const CKeyID &address) const
{
    LOCK(cs_KeyStore);
    return mapKeys.count(address) > 0;
}

std::set<CKeyID> CBasicKeyStore::GetKeys() const
{
    LOCK(cs_KeyStore);
    std::set<CKeyID> set_address;
    for (const auto& mi : mapKeys) {
        set_address.insert(mi.first);
    }
    return set_address;
}

bool CBasicKeyStore::GetKey(const CKeyID &address, CKey &keyOut) const
{
    LOCK(cs_KeyStore);
    KeyMap::const_iterator mi = mapKeys.find(address);
    if (mi != mapKeys.end()) {
        keyOut = mi->second;
        return true;
    }
    return false;
}

bool CBasicKeyStore::AddCScript(const CScript& redeemScript)
{
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
        return error("CBasicKeyStore::AddCScript(): redeemScripts > %i bytes are invalid", MAX_SCRIPT_ELEMENT_SIZE);

    LOCK(cs_KeyStore);
    mapScripts[CScriptID(redeemScript)] = redeemScript;
    return true;
}

bool CBasicKeyStore::HaveCScript(const CScriptID& hash) const
{
    LOCK(cs_KeyStore);
    return mapScripts.count(hash) > 0;
}

std::set<CScriptID> CBasicKeyStore::GetCScripts() const
{
    LOCK(cs_KeyStore);
    std::set<CScriptID> set_script;
    for (const auto& mi : mapScripts) {
        set_script.insert(mi.first);
    }
    return set_script;
}

bool CBasicKeyStore::GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const
{
    LOCK(cs_KeyStore);
    ScriptMap::const_iterator mi = mapScripts.find(hash);
    if (mi != mapScripts.end())
    {
        redeemScriptOut = (*mi).second;
        return true;
    }
    return false;
}

static bool ExtractPubKey(const CScript &dest, CPubKey& pubKeyOut)
{
    //TODO: Use Solver to extract this?
    CScript::const_iterator pc = dest.begin();
    opcodetype opcode;
    std::vector<unsigned char> vch;
    if (!dest.GetOp(pc, opcode, vch) || !CPubKey::ValidSize(vch))
        return false;
    pubKeyOut = CPubKey(vch);
    if (!pubKeyOut.IsFullyValid())
        return false;
    if (!dest.GetOp(pc, opcode, vch) || opcode != OP_CHECKSIG || dest.GetOp(pc, opcode, vch))
        return false;
    return true;
}

bool CBasicKeyStore::AddWatchOnly(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.insert(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey)) {
        mapWatchKeys[pubKey.GetID()] = pubKey;
        ImplicitlyLearnRelatedKeyScripts(pubKey);
    }
    return true;
}

bool CBasicKeyStore::RemoveWatchOnly(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.erase(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey)) {
        mapWatchKeys.erase(pubKey.GetID());
    }
    // Related CScripts are not removed; having superfluous scripts around is
    // harmless (see comment in ImplicitlyLearnRelatedKeyScripts).
    return true;
}

bool CBasicKeyStore::HaveWatchOnly(const CScript &dest) const
{
    LOCK(cs_KeyStore);
    return setWatchOnly.count(dest) > 0;
}

bool CBasicKeyStore::HaveWatchOnly() const
{
    LOCK(cs_KeyStore);
    return (!setWatchOnly.empty());
}

CKeyID GetKeyForDestination(const CKeyStore& store, const CTxDestination& dest)
{
    // Only supports destinations which map to single public keys, i.e. P2PKH,
    // P2WPKH, and P2SH-P2WPKH.
    if (auto id = boost::get<CKeyID>(&dest)) {
        return *id;
    }
    if (auto witness_id = boost::get<WitnessV0KeyHash>(&dest)) {
        return CKeyID(*witness_id);
    }
    if (auto script_id = boost::get<CScriptID>(&dest)) {
        CScript script;
        CTxDestination inner_dest;
        if (store.GetCScript(*script_id, script) && ExtractDestination(script, inner_dest)) {
            if (auto inner_witness_id = boost::get<WitnessV0KeyHash>(&inner_dest)) {
                return CKeyID(*inner_witness_id);
            }
        }
    }
    return CKeyID();
}

bool HaveKey(const CKeyStore& store, const CKey& key)
{
    CKey key2;
    key2.Set(key.begin(), key.end(), !key.IsCompressed());
    return store.HaveKey(key.GetPubKey().GetID()) || store.HaveKey(key2.GetPubKey().GetID());
}

namespace {
class BasicKeyStoreOutputProducer : public OutputProducer {

    int step = 0;
    int inner_step = 0;
    KeyMap::const_iterator it1, it1end;
    WatchKeyMap::const_iterator it2, it2end;
    ScriptMap::const_iterator it3, it3end;
    WatchOnlySet::const_iterator it4, it4end;

public:
    BasicKeyStoreOutputProducer(const KeyMap& a, const WatchKeyMap& b, const ScriptMap& c, const WatchOnlySet& d) : it1(a.begin()), it1end(a.end()), it2(b.begin()), it2end(b.end()), it3(c.begin()), it3end(c.end()), it4(d.begin()), it4end(d.end()) {}

    bool Produce(CScript& script) override
    {
        script.clear();
        switch (step) {
        case 0:
            if (it1 != it1end) {
                if (inner_step == 0) {
                    CPubKey pubkey = it1->second.GetPubKey();
                    script << MakeSpan(pubkey) << OP_CHECKSIG;
                } else {
                    script << OP_DUP << OP_HASH160 << MakeSpan(it1->first) << OP_EQUALVERIFY << OP_CHECKSIG;
                    ++it1;
                }
                inner_step = !inner_step;
                return true;
            } else {
                step = 1;
            }
        case 1:
            if (it2 != it2end) {
                if (inner_step == 0) {
                    script << MakeSpan(it2->second) << OP_CHECKSIG;
                } else {
                    script << OP_DUP << OP_HASH160 << MakeSpan(it2->first) << OP_EQUALVERIFY << OP_CHECKSIG;
                    ++it2;
                }
                inner_step = !inner_step;
                return true;
            } else {
                step = 2;
            }
        case 2:
            if (it3 != it3end) {
                if (inner_step == 0) {
                    script = it3->second;
                } else if (inner_step == 1) {
                    script << OP_HASH160 << MakeSpan(it3->first) << OP_EQUAL;
                } else {
                    unsigned char hash[32];
                    CSHA256().Write(it3->second.data(), it3->second.size()).Finalize(hash);
                    script << OP_0 << Span<const unsigned char>(hash, 32);
                    ++it3;
                }
                inner_step = (inner_step + 1) % 3;
                return true;
            } else {
                step = 3;
            }
        case 3:
            if (it4 != it4end) {
                script = *it4;
                ++it4;
                return true;
            } else {
                step = 4;
            }
        default:
            return false;
        }
    }
};
}

std::unique_ptr<OutputProducer> CBasicKeyStore::Producer() const {
    OutputProducer* producer = new BasicKeyStoreOutputProducer(mapKeys, mapWatchKeys, mapScripts, setWatchOnly);
    return std::unique_ptr<OutputProducer>(producer);
}
