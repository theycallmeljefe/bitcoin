// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_ISMINE_H
#define BITCOIN_SCRIPT_ISMINE_H

#include <script/standard.h>

#include <stdint.h>

class CKeyStore;
class CScript;

/** IsMine() return codes */
enum class isminetype
{
    NO = 0,
    WATCH_ONLY = 1,
    SPENDABLE = 2,
};

enum class isminefilter
{
    SPENDABLE,  //! Spendable only
    ALL,        //! Spendable and watch-only
};

static constexpr bool IsMineMatch(isminetype x, isminefilter p) { return x == isminetype::SPENDABLE || (x == isminetype::WATCH_ONLY && p == isminefilter::ALL); }

static constexpr isminetype IsMineBest(isminetype x, isminetype y) { return std::max(x, y); }
static constexpr isminetype IsMineWorst(isminetype x, isminetype y) { return std::min(x, y); }

/* isInvalid becomes true when the script is found invalid by consensus or policy. This will terminate the recursion
 * and return ISMINE_NO immediately, as an invalid script should never be considered as "mine". This is needed as
 * different SIGVERSION may have different network rules. Currently the only use of isInvalid is indicate uncompressed
 * keys in SigVersion::WITNESS_V0 script, but could also be used in similar cases in the future
 */
IsMineType IsMine(const CKeyStore& keystore, const CScript& scriptPubKey, bool& isInvalid);
IsMineType IsMine(const CKeyStore& keystore, const CScript& scriptPubKey);
IsMineType IsMine(const CKeyStore& keystore, const CTxDestination& dest);

#endif // BITCOIN_SCRIPT_ISMINE_H
