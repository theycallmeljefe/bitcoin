// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_WALLETDB_H
#define BITCOIN_WALLETDB_H

#include "logdb.h"
#include "base58.h"

class CWallet;
class CBlockLocator;
class CWalletTx;
class CKeyPool;
class CAccount;
class CAccountingEntry;

/** Error statuses for the wallet database */
enum DBErrors
{
    DB_LOAD_OK,
    DB_CORRUPT,
    DB_TOO_NEW,
    DB_LOAD_FAIL,
    DB_NEED_REWRITE
};

/** Access to the wallet database (wallet.dat) */
class CWalletDB : public CLogDB
{
public:
//    CWalletDB(std::string strFilename, const char* pszMode="r+") : CDB(strFilename.c_str(), pszMode)
    CWalletDB(CLogDBFile *db, const char* pszMode="r+") : CLogDB(db, (!strchr(pszMode, '+') && !strchr(pszMode, 'w')))
    {
    }
private:
    CWalletDB(const CWalletDB&);
    void operator=(const CWalletDB&);
public:
    bool ReadName(const std::string& strAddress, std::string& strName)
    {
        strName = "";
        return Read(std::make_pair(std::string("name"), strAddress), strName);
    }

    bool WriteName(const std::string& strAddress, const std::string& strName);

    bool EraseName(const std::string& strAddress);

    bool ReadTx(uint256 hash, CWalletTx& wtx)
    {
        return Read(std::make_pair(std::string("tx"), hash), wtx);
    }

    bool WriteTx(uint256 hash, const CWalletTx& wtx)
    {
        return Write(std::make_pair(std::string("tx"), hash), wtx);
    }

    bool EraseTx(uint256 hash)
    {
        return Erase(std::make_pair(std::string("tx"), hash));
    }

    bool ReadKey(const CPubKey& vchPubKey, CPrivKey& vchPrivKey)
    {
        vchPrivKey.clear();
        return Read(std::make_pair(std::string("key"), vchPubKey.Raw()), vchPrivKey);
    }

    bool WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey)
    {
        return Write(std::make_pair(std::string("key"), vchPubKey.Raw()), vchPrivKey, false);
    }

    bool WriteCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, bool fEraseUnencryptedKey = true)
    {
        if (!Write(std::make_pair(std::string("ckey"), vchPubKey.Raw()), vchCryptedSecret, false))
            return false;
        if (fEraseUnencryptedKey)
        {
            Erase(std::make_pair(std::string("key"), vchPubKey.Raw()));
            Erase(std::make_pair(std::string("wkey"), vchPubKey.Raw()));
        }
        return true;
    }

    bool WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey)
    {
        return Write(std::make_pair(std::string("mkey"), nID), kMasterKey, true);
    }

    // Support for BIP 0013 : see https://en.bitcoin.it/wiki/BIP_0013
    bool ReadCScript(const uint160 &hash, CScript& redeemScript)
    {
        redeemScript.clear();
        return Read(std::make_pair(std::string("cscript"), hash), redeemScript);
    }

    bool WriteCScript(const uint160& hash, const CScript& redeemScript)
    {
        return Write(std::make_pair(std::string("cscript"), hash), redeemScript, false);
    }

    bool WriteBestBlock(const CBlockLocator& locator)
    {
        return Write(std::string("bestblock"), locator);
    }

    bool ReadBestBlock(CBlockLocator& locator)
    {
        return Read(std::string("bestblock"), locator);
    }

    bool ReadDefaultKey(std::vector<unsigned char>& vchPubKey)
    {
        vchPubKey.clear();
        return Read(std::string("defaultkey"), vchPubKey);
    }

    bool WriteDefaultKey(const CPubKey& vchPubKey)
    {
        return Write(std::string("defaultkey"), vchPubKey.Raw());
    }

    bool ReadPool(int64 nPool, CKeyPool& keypool)
    {
        return Read(std::make_pair(std::string("pool"), nPool), keypool);
    }

    bool WritePool(int64 nPool, const CKeyPool& keypool)
    {
        return Write(std::make_pair(std::string("pool"), nPool), keypool);
    }

    bool ErasePool(int64 nPool)
    {
        return Erase(std::make_pair(std::string("pool"), nPool));
    }

    // Settings are no longer stored in wallet.dat; these are
    // used only for backwards compatibility:
    template<typename T>
    bool ReadSetting(const std::string& strKey, T& value)
    {
        return Read(std::make_pair(std::string("setting"), strKey), value);
    }
    template<typename T>
    bool WriteSetting(const std::string& strKey, const T& value)
    {
        return Write(std::make_pair(std::string("setting"), strKey), value);
    }
    bool EraseSetting(const std::string& strKey)
    {
        return Erase(std::make_pair(std::string("setting"), strKey));
    }

    bool WriteMinVersion(int nVersion)
    {
        return Write(std::string("minversion"), nVersion);
    }

    bool ReadAccount(const std::string& strAccount, CAccount& account);
    bool WriteAccount(const std::string& strAccount, const CAccount& account);
    bool WriteAccountingEntry(const CAccountingEntry& acentry);
    int64 GetAccountCreditDebit(const std::string& strAccount);
    void ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& acentries);

    int LoadWallet(CWallet* pwallet);
};

#endif // BITCOIN_WALLETDB_H
