// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "validationinterface.h"
#include "init.h"
#include "scheduler.h"
#include "sync.h"
#include "util.h"

#include <list>
#include <atomic>

#include <boost/signals2/signal.hpp>

struct CMainSignalsInstance {
    boost::signals2::signal<void (const CBlockIndex *, const CBlockIndex *, bool fInitialDownload)> UpdatedBlockTip;
    boost::signals2::signal<void (const CTransactionRef &)> TransactionAddedToMempool;
    boost::signals2::signal<void (const std::shared_ptr<const CBlock> &, const CBlockIndex *pindex, const std::vector<CTransactionRef>&)> BlockConnected;
    boost::signals2::signal<void (const std::shared_ptr<const CBlock> &)> BlockDisconnected;
    boost::signals2::signal<void (const CBlockLocator &)> SetBestChain;
    boost::signals2::signal<void (const uint256 &)> Inventory;
    boost::signals2::signal<void (int64_t nBestBlockTime, CConnman* connman)> Broadcast;
    boost::signals2::signal<void (const CBlock&, const CValidationState&)> BlockChecked;
    boost::signals2::signal<void (std::shared_ptr<CReserveScript>&)> ScriptForMining;
    boost::signals2::signal<void (const CBlockIndex *, const std::shared_ptr<const CBlock>&)> NewPoWValidBlock;

    CScheduler *scheduler = NULL;

    // We are not allowed to assume the scheduler only runs in one thread,
    // but must ensure all callbacks happen in-order, so we end up creating
    // our own queue here :(
    CCriticalSection cs_callbacksPending;
    std::list<std::function<void (void)>> callbacksPending;
    std::atomic<bool> fCallbacksRunning = ATOMIC_VAR_INIT(false);

    void ProcessQueue() {
        if (fCallbacksRunning.exchange(true)) {
            return;
        }

        while (true) {
            std::function<void (void)> callback;
            {
                LOCK(cs_callbacksPending);
                if (callbacksPending.empty()) {
                    // We must set fCallbacksRunning to false with
                    // cs_callbacksRunning held - ensuring that there is no
                    // race condition in which we might add something to the
                    // queue, have the associated ProcessQueue call fail to get
                    // the fCallbacksRunning lock, but have already given up on
                    // doing more work in the previous ProcessQueue function.
                    fCallbacksRunning = false;
                    break;
                }
                callback = callbacksPending.front();
                callbacksPending.pop_front();
            }
            try {
                callback();
            } catch (...) {
                LogPrintf("Exception thrown in processing CValidationInterface callback\n");
            }
        }
    }

    void AddToProcessQueue(const std::function<void (void)>& func) {
        if (!scheduler)
            return;
        {
            LOCK(cs_callbacksPending);
            callbacksPending.emplace_back(func);
        }
        scheduler->schedule(std::bind(&CMainSignalsInstance::ProcessQueue, this));
    }
};

static CMainSignals g_signals;

CMainSignals::CMainSignals() {
    internals = new CMainSignalsInstance();
}

CMainSignals::~CMainSignals() {
    delete internals;
}

void CMainSignals::RegisterBackgroundSignalScheduler(CScheduler& scheduler) {
    assert(!internals->scheduler);
    internals->scheduler = &scheduler;
}

void CMainSignals::UnregisterBackgroundSignalScheduler() {
    internals->scheduler = NULL;
}

CMainSignals& GetMainSignals()
{
    return g_signals;
}

void RegisterValidationInterface(CValidationInterface* pwalletIn) {
    g_signals.internals->UpdatedBlockTip.connect(boost::bind(&CValidationInterface::UpdatedBlockTip, pwalletIn, _1, _2, _3));
    g_signals.internals->TransactionAddedToMempool.connect(boost::bind(&CValidationInterface::TransactionAddedToMempool, pwalletIn, _1));
    g_signals.internals->BlockConnected.connect(boost::bind(&CValidationInterface::BlockConnected, pwalletIn, _1, _2, _3));
    g_signals.internals->BlockDisconnected.connect(boost::bind(&CValidationInterface::BlockDisconnected, pwalletIn, _1));
    g_signals.internals->SetBestChain.connect(boost::bind(&CValidationInterface::SetBestChain, pwalletIn, _1));
    g_signals.internals->Inventory.connect(boost::bind(&CValidationInterface::Inventory, pwalletIn, _1));
    g_signals.internals->Broadcast.connect(boost::bind(&CValidationInterface::ResendWalletTransactions, pwalletIn, _1, _2));
    g_signals.internals->BlockChecked.connect(boost::bind(&CValidationInterface::BlockChecked, pwalletIn, _1, _2));
    g_signals.internals->ScriptForMining.connect(boost::bind(&CValidationInterface::GetScriptForMining, pwalletIn, _1));
    g_signals.internals->NewPoWValidBlock.connect(boost::bind(&CValidationInterface::NewPoWValidBlock, pwalletIn, _1, _2));
}

void UnregisterValidationInterface(CValidationInterface* pwalletIn) {
    g_signals.internals->ScriptForMining.disconnect(boost::bind(&CValidationInterface::GetScriptForMining, pwalletIn, _1));
    g_signals.internals->BlockChecked.disconnect(boost::bind(&CValidationInterface::BlockChecked, pwalletIn, _1, _2));
    g_signals.internals->Broadcast.disconnect(boost::bind(&CValidationInterface::ResendWalletTransactions, pwalletIn, _1, _2));
    g_signals.internals->Inventory.disconnect(boost::bind(&CValidationInterface::Inventory, pwalletIn, _1));
    g_signals.internals->SetBestChain.disconnect(boost::bind(&CValidationInterface::SetBestChain, pwalletIn, _1));
    g_signals.internals->TransactionAddedToMempool.disconnect(boost::bind(&CValidationInterface::TransactionAddedToMempool, pwalletIn, _1));
    g_signals.internals->BlockConnected.disconnect(boost::bind(&CValidationInterface::BlockConnected, pwalletIn, _1, _2, _3));
    g_signals.internals->BlockDisconnected.disconnect(boost::bind(&CValidationInterface::BlockDisconnected, pwalletIn, _1));
    g_signals.internals->UpdatedBlockTip.disconnect(boost::bind(&CValidationInterface::UpdatedBlockTip, pwalletIn, _1, _2, _3));
    g_signals.internals->NewPoWValidBlock.disconnect(boost::bind(&CValidationInterface::NewPoWValidBlock, pwalletIn, _1, _2));
}

void UnregisterAllValidationInterfaces() {
    g_signals.internals->ScriptForMining.disconnect_all_slots();
    g_signals.internals->BlockChecked.disconnect_all_slots();
    g_signals.internals->Broadcast.disconnect_all_slots();
    g_signals.internals->Inventory.disconnect_all_slots();
    g_signals.internals->SetBestChain.disconnect_all_slots();
    g_signals.internals->TransactionAddedToMempool.disconnect_all_slots();
    g_signals.internals->BlockConnected.disconnect_all_slots();
    g_signals.internals->BlockDisconnected.disconnect_all_slots();
    g_signals.internals->UpdatedBlockTip.disconnect_all_slots();
    g_signals.internals->NewPoWValidBlock.disconnect_all_slots();
}

void CMainSignals::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) {
    internals->UpdatedBlockTip(pindexNew, pindexFork, fInitialDownload);
}

void CMainSignals::TransactionAddedToMempool(const CTransactionRef &ptx) {
    internals->TransactionAddedToMempool(ptx);
}

void CMainSignals::BlockConnected(const std::shared_ptr<const CBlock> &pblock, const CBlockIndex *pindex, const std::vector<CTransactionRef>& vtxConflicted) {
    internals->BlockConnected(pblock, pindex, vtxConflicted);
}

void CMainSignals::BlockDisconnected(const std::shared_ptr<const CBlock> &pblock) {
    internals->BlockDisconnected(pblock);
}

void CMainSignals::SetBestChain(const CBlockLocator &locator) {
    internals->SetBestChain(locator);
}

void CMainSignals::Inventory(const uint256 &hash) {
    internals->Inventory(hash);
}

void CMainSignals::Broadcast(int64_t nBestBlockTime, CConnman* connman) {
    internals->Broadcast(nBestBlockTime, connman);
}

void CMainSignals::BlockChecked(const CBlock& block, const CValidationState& state) {
    internals->BlockChecked(block, state);
}

void CMainSignals::ScriptForMining(std::shared_ptr<CReserveScript>& pscript) {
    internals->ScriptForMining(pscript);
}

void CMainSignals::NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock> &block) {
    internals->NewPoWValidBlock(pindex, block);
}
