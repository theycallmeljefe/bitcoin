#include <stdio.h>

#include "base58.h"
#include "key.h"
#include "util.h"

#include "ui_interface.h"

#include <vector>

CClientUIInterface uiInterface;

void test_vector(const std::vector<unsigned char> &master, const std::vector<uint32_t> &der, bool ext = false) {
    CDetKey key;
    CDetKey keyPub;
    fprintf(stderr, "* Master (hex): %s\n", HexStr(master).c_str());
    key.SetMaster(master);
    key.Neuter(keyPub);
    unsigned char fpr[4] = {0,0,0,0};
    for (unsigned int len = 0; len <= der.size(); len++) {
        CSecret secret;
        key.GetSecret(secret);
        CPubKey pubkey;
        key.GetPubKey(pubkey);
        CPubKey pubkeyNeuter;
        keyPub.GetPubKey(pubkeyNeuter);
        std::vector<unsigned char> vchPubKey = pubkey.Raw();
        std::vector<unsigned char> vchPubKeyNeuter = pubkeyNeuter.Raw();
        assert(vchPubKey.size() == vchPubKeyNeuter.size() && memcmp(&vchPubKey[0], &vchPubKeyNeuter[0], vchPubKey.size())==0);
        CKeyID keyid = pubkey.GetID();
        unsigned char *pkeyid = (unsigned char*)&keyid;
        std::vector<unsigned char> chain;
        key.GetChaincode(chain);
        std::vector<unsigned char> vchXPub;
        vchXPub.push_back(0x04); vchXPub.push_back(0x88); vchXPub.push_back(0xB2); vchXPub.push_back(0x1E);
        vchXPub.push_back((unsigned char)len);
        for (int j = 0; j < 4; j++)
           vchXPub.push_back(fpr[j]);
        for (int j = 0; j < 4; j++)
           vchXPub.push_back((unsigned char)(len == 0 ? 0 : ((der[len-1] >> (8*(3-j))) & 0xFF)));
        vchXPub.insert(vchXPub.end(), chain.begin(), chain.end());
        vchXPub.insert(vchXPub.end(), vchPubKey.begin(), vchPubKey.end());
        assert(vchXPub.size() == 78);
        std::vector<unsigned char> vchXPrv(vchXPub);
        vchXPrv.resize(46);
        vchXPrv[2] = 0xAD; vchXPrv[3] = 0xE4;
        vchXPrv[45] = 0;
        vchXPrv.insert(vchXPrv.end(), secret.begin(), secret.end());
        fprintf(stderr, "  * [Chain m");
        for (unsigned int i = 0; i < len; i++) {
            if (der[i] & 0x80000000)
                fprintf(stderr, "/%u'", der[i] & ~0x80000000);
            else
                fprintf(stderr, "/%u", der[i]);
        }
        fprintf(stderr, "]\n");
        if (ext) {
            fprintf(stderr, "    * Identifier\n");
            fprintf(stderr, "      * (hex):       %s\n", HexStr(pkeyid, pkeyid+20).c_str());
            fprintf(stderr, "      * (fpr):       0x%02x%02x%02x%02x\n", pkeyid[0], pkeyid[1], pkeyid[2], pkeyid[3]);
            fprintf(stderr, "      * (main addr): %s\n", CBitcoinAddress(keyid).ToString().c_str());
            fprintf(stderr, "    * Secret key\n");
            fprintf(stderr, "      * (hex):       %s\n", HexStr(secret.begin(), secret.end()).c_str());
            fprintf(stderr, "      * (wif):       %s\n", CBitcoinSecret(secret, true).ToString().c_str());
            fprintf(stderr, "    * Public key\n");
            fprintf(stderr, "      * (hex):       %s\n", HexStr(pubkey.Raw()).c_str());
            fprintf(stderr, "    * Chain code\n");
            fprintf(stderr, "      * (hex):       %s\n", HexStr(chain).c_str());
            fprintf(stderr, "    * Serialized\n");
            fprintf(stderr, "      * (pub hex):   %s\n", HexStr(vchXPub).c_str());
            fprintf(stderr, "      * (prv hex):   %s\n", HexStr(vchXPrv).c_str());
            fprintf(stderr, "      * (pub b58):   %s\n", EncodeBase58Check(vchXPub).c_str());
            fprintf(stderr, "      * (prv b58):   %s\n", EncodeBase58Check(vchXPrv).c_str());
        } else {
            fprintf(stderr, "    * ext pub: %s\n", EncodeBase58Check(vchXPub).c_str());
            fprintf(stderr, "    * ext prv: %s\n", EncodeBase58Check(vchXPrv).c_str());
        }
        for (int j = 0; j < 4; j++)
            fpr[j] = pkeyid[j];
        CDetKey keyChild;
        key.Derive(keyChild, der[len]);
        key = keyChild;
        if (der[len] & 0x80000000) {
            key.Neuter(keyPub);
        } else {
            CDetKey keyChildPub;
            keyPub.Derive(keyChildPub, der[len]);
            keyPub = keyChildPub;
        }
    }
}

int main() {
    char hexmst1[] = "000102030405060708090A0B0C0D0E0F";
    std::vector<unsigned char> vchMaster = ParseHex(hexmst1);
    std::vector<uint32_t> der;
    der.push_back(0x80000000);
    der.push_back(1);
    der.push_back(0x80000002);
    der.push_back(2);
    der.push_back(1000000000);
    test_vector(vchMaster, der, true);

    char hexmst2[] = "FFFCF9F6F3F0EDEAE7E4E1DEDBD8D5D2CFCCC9C6C3C0BDBAB7B4B1AEABA8A5A29F9C999693908D8A8784817E7B7875726F6C696663605D5A5754514E4B484542";
    vchMaster = ParseHex(hexmst2);
    der.clear();
    der.push_back(0);
    der.push_back(0xFFFFFFFF);
    der.push_back(1);
    der.push_back(0xFFFFFFFE);
    der.push_back(2);
    test_vector(vchMaster, der, true);
}
