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

enum ThresholdState {
    DEFINED,
    STARTED,
    LOCKEDIN,
    ACTIVE,
    FAILED,
};

// A map that gives the state for blocks whose height is a multiple of Period().
// The map is indexed by the block's parent, however, so all keys in the map
// will either be NULL or a block with (height + 1) % Period() == 0.
typedef std::map<const CBlockIndex*, ThresholdState> ThresholdConditionCache;

/**
 * Abstract class that implements BIP9-style threshold logic, and caches results.
 */
class AbstractThresholdConditionChecker {
protected:
    virtual bool Condition(const CBlockIndex* pindex, const Params& params) const =0;
    virtual int64_t BeginTime(const Params& params) const =0;
    virtual int64_t EndTime(const Params& params) const =0;
    virtual int Period(const Params& params) const =0;
    virtual int Threshold(const Params& params) const =0;

    ThresholdState GetStateFor(const CBlockIndex* pindexPrev, const Params& params, ThresholdConditionCache& cache) const;

public:
    // Note that the functions below take a pindexPrev as input: they compute information for block B based on its parent.
    bool IsActiveFor(const CBlockIndex* pindexPrev, const Params& params, ThresholdConditionCache& cache) const
    {
        return GetStateFor(pindexPrev, params, cache) == ACTIVE;
    }

    bool IsSetFor(const CBlockIndex* pindexPrev, const Params& params, ThresholdConditionCache& cache) const
    {
        ThresholdState state = GetStateFor(pindexPrev, params, cache);
        return state == STARTED || state == LOCKEDIN;
    }

    bool IsLockedIn(const CBlockIndex* pindexPrev, const Params& params, ThresholdConditionCache& cache) const
    {
        ThresholdState state = GetStateFor(pindexPrev, params, cache);
        return state == ACTIVE || state == LOCKEDIN;
    }
};

struct VersionBitsCache
{
    ThresholdConditionCache caches[MAX_VERSION_BITS_DEPLOYMENTS];
};

bool IsVersionBitsSet(const CBlockIndex* pindexPrev, const Params& params, DeploymentPos pos, VersionBitsCache& cache);
bool IsVersionBitsActive(const CBlockIndex* pindexPrev, const Params& params, DeploymentPos pos, VersionBitsCache& cache);
uint32_t VersionBitsMask(const Params& params, DeploymentPos pos);

}

#endif
