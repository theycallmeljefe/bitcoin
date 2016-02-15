// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_VERSIONBITS
#define BITCOIN_CONSENSUS_VERSIONBITS

#include "chain.h"
#include <map>

namespace Consensus
{

/** What block version to use for new blocks (pre versionbits) */
static const int32_t VERSIONBITS_LAST_OLD_BLOCK_VERSION = 4;
/** What bits to set in version for versionbits blocks */
static const int32_t VERSIONBITS_TOP_BITS = 0x20000000UL;
/** What bitmask determines whether versionbits is in use */
static const int32_t VERSIONBITS_TOP_MASK = 0xE0000000UL;

/**
 * Abstract class that implements BIP9-style threshold logic, and caches results.
 */
class AbstractThresholdConditionChecker {
protected:
    virtual bool Condition(const CBlockIndex* pindex, const Params& params) =0;
    virtual int64_t BeginTime(const Params& params) =0;
    virtual int64_t EndTime(const Params& params) =0;
    virtual int Period(const Params& params) =0;
    virtual int Threshold(const Params& params) =0;

    enum State {
        DEFINED,
        STARTED,
        LOCKEDIN,
        ACTIVE,
        FAILED,
    };

    State GetStateFor(const CBlockIndex* pindexPrev, const Params& params);

private:
    // A map that gives the state for blocks whose height is a multiple of Period().
    // The map is indexed by the block's parent, however, so all keys in the map
    // will either be NULL or a block with (height + 1) % Period() == 0.
    std::map<const CBlockIndex*, State> cache;

public:
    // Note that the functions below take a pindexPrev as input: they compute information for block B based on its parent B-1.

    bool IsActiveFor(const CBlockIndex* pindexPrev, const Params& params)
    {
        return GetStateFor(pindexPrev, params) == ACTIVE;
    }

    bool IsSetFor(const CBlockIndex* pindexPrev, const Params& params)
    {
        State state = GetStateFor(pindexPrev, params);
        return state == STARTED || state == LOCKEDIN;
    }

    bool IsLockedIn(const CBlockIndex* pindexPrev, const Params& params)
    {
        State state = GetStateFor(pindexPrev, params);
        return state == ACTIVE || state == LOCKEDIN;
    }
};

/**
 * Class to implement versionbits logic.
 */
class VersionBitsConditionChecker : public AbstractThresholdConditionChecker {
private:
    const DeploymentPos id;

protected:

    int64_t BeginTime(const Params& params) { return params.vDeployments[id].nStartTime; }
    int64_t EndTime(const Params& params) { return params.vDeployments[id].nTimeout; }
    int Period(const Params& params) { return params.nMinerConfirmationWindow; }
    int Threshold(const Params& params) { return params.nRuleChangeActivationThreshold; }

    bool Condition(const CBlockIndex* pindex, const Params& params)
    {
        return (((pindex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) && (pindex->nVersion & Mask(params)) != 0);
    }

public:
    VersionBitsConditionChecker(DeploymentPos id_) : id(id_) {}
    uint32_t Mask(const Params& params) { return params.vDeployments[id].bitmask; }
};

}

#endif
