// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "versionbits.h"

namespace Consensus
{

ThresholdState AbstractThresholdConditionChecker::GetStateFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const
{
    int nPeriod = Period(params);
    int nThreshold = Threshold(params);
    int64_t nTimeStart = BeginTime(params);
    int64_t nTimeTimeout = EndTime(params);

    // A block's state is always the same as that of the first of its period, so it is computed based on a pindexPrev whose height equals a multiple of nPeriod - 1.
    if (pindexPrev != NULL) {
        pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - ((pindexPrev->nHeight + 1) % nPeriod));
    }

    // Walk backwards in steps of nPeriod to find a pindexPrev whose information is known
    std::vector<const CBlockIndex*> vToCompute;
    while (cache.count(pindexPrev) == 0) {
        if (pindexPrev == NULL) {
            // The genesis block is by definition defined.
            cache[pindexPrev] = DEFINED;
            break;
        }
        if (pindexPrev->GetMedianTimePast() < nTimeStart) {
            // Optimizaton: don't recompute down further, as we know every earlier block will be before the start time
            cache[pindexPrev] = DEFINED;
            break;
        }
        vToCompute.push_back(pindexPrev);
        pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);
    }

    // At this point, cache[pindexPrev] is known
    assert(cache.count(pindexPrev));
    ThresholdState state = cache[pindexPrev];

    // Now walk forward and compute the state of descendants of pindexPrev
    while (!vToCompute.empty()) {
        ThresholdState stateNext = state;
        pindexPrev = vToCompute.back();
        vToCompute.pop_back();

        switch (state) {
            case DEFINED: {
                if (pindexPrev->GetMedianTimePast() >= nTimeTimeout) {
                    stateNext = FAILED;
                } else if (pindexPrev->GetMedianTimePast() >= nTimeStart) {
                    stateNext = STARTED;
                }
                break;
            }
            case STARTED: {
                if (pindexPrev->GetMedianTimePast() >= nTimeTimeout) {
                    stateNext = FAILED;
                    break;
                }
                // We need to count
                const CBlockIndex* pindexCount = pindexPrev;
                int count = 0;
                for (int i = 0; i < nPeriod; i++) {
                    if (Condition(pindexCount, params)) {
                        count++;
                    }
                    pindexCount = pindexCount->pprev;
                }
                if (count >= nThreshold) {
                    stateNext = LOCKEDIN;
                }
                break;
            }
            case LOCKEDIN: {
                // Always progresses into ACTIVE.
                stateNext = ACTIVE;
                break;
            }
            case FAILED:
            case ACTIVE: {
                // Nothing happens, these are terminal states.
                break;
            }
        }
        cache[pindexPrev] = state = stateNext;
    }

    return state;
}

namespace
{
/**
 * Class to implement versionbits logic.
 */
class VersionBitsConditionChecker : public AbstractThresholdConditionChecker {
private:
    const DeploymentPos id;

protected:
    int64_t BeginTime(const Params& params) const { return params.vDeployments[id].nStartTime; }
    int64_t EndTime(const Params& params) const { return params.vDeployments[id].nTimeout; }
    int Period(const Params& params) const { return params.nMinerConfirmationWindow; }
    int Threshold(const Params& params) const { return params.nRuleChangeActivationThreshold; }

    bool Condition(const CBlockIndex* pindex, const Params& params) const
    {
        return (((pindex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) && (pindex->nVersion & Mask(params)) != 0);
    }

public:
    VersionBitsConditionChecker(DeploymentPos id_) : id(id_) {}
    uint32_t Mask(const Params& params) const { return params.vDeployments[id].bitmask; }
};

}

bool IsVersionBitsSet(const CBlockIndex* pindexPrev, const Params& params, DeploymentPos pos, VersionBitsCache& cache)
{
    return VersionBitsConditionChecker(pos).IsSetFor(pindexPrev, params, cache.caches[pos]);
}

bool IsVersionBitsActive(const CBlockIndex* pindexPrev, const Params& params, DeploymentPos pos, VersionBitsCache& cache)
{
    return VersionBitsConditionChecker(pos).IsActiveFor(pindexPrev, params, cache.caches[pos]);
}

uint32_t VersionBitsMask(const Params& params, DeploymentPos pos)
{
    return VersionBitsConditionChecker(pos).Mask(params);
}

}
