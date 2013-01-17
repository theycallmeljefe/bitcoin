#include <stdio.h>
#include <string>

#include "key.h"

using namespace std;

LockedPageManager LockedPageManager::instance;

int main(void) {
    std::vector<CKey> keys;
    std::vector<CPubKey> pubkeys;
    std::vector<std::vector<unsigned char> > sigs;
    std::vector<uint256> msgs;
    keys.resize(256);

    char strKey[20] = "_ = Very secret key";
    char strMsg[24] = "_ = Very secret message";

    for (int i=0; i<256; i++) {
        CSecret secret;
        strKey[0] = i;
        strMsg[0] = i;
        uint256 hashkey = Hash(&strKey[0], &strKey[20]);
        uint256 hashMsg = Hash(&strMsg[0], &strMsg[24]);
        secret.resize(32);
        memcpy(&secret[0], &hashkey, 32);
        CKey key;
        key.SetSecret(secret);
        std::vector<unsigned char> sig;
        key.Sign(hashMsg, sig);

        pubkeys.push_back(key.GetPubKey());
        msgs.push_back(hashMsg);
        sigs.push_back(sig);
    }

    int count = 0;
    for (int j=0; j<256; j++) {
        for (int i=0; i<256; i++) {
            CKey key;
            key.SetPubKey(pubkeys[i]);
            count += key.Verify(msgs[i], sigs[i]);
        }
    }

    fprintf(stderr, "Did %i verifications!\n", count);
    return 0;
}
