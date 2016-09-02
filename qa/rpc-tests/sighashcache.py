#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.mininode import CTransaction, CTxOut, CTxIn, COutPoint
from test_framework.util import *
from test_framework.key import CECKey, CPubKey
from test_framework.script import CScript, OP_0, OP_1, OP_12, OP_14, OP_CHECKSIG, OP_CHECKMULTISIG, OP_CODESEPARATOR, OP_CHECKMULTISIGVERIFY, OP_CHECKSIGVERIFY, OP_HASH160, OP_EQUAL, SignatureHash, SIGHASH_ANYONECANPAY, SIGHASH_NONE, SIGHASH_SINGLE, SIGHASH_ALL, hash160, sha256
from io import BytesIO
import time
from random import randint

dummykey = hex_str_to_bytes("0300112233445566778899aabbccddeeff00112233445566778899aabbccddeeff")

class SigHashCacheTest(BitcoinTestFramework):
    """Tests transaction signing via RPC command "signrawtransaction"."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True

    def setup_network(self, split=False):
        self.nodes = [start_node(0, self.options.tmpdir, ["-promiscuousmempoolflags=8189"])]
        self.is_network_split = False

    def testtx(self, offset, nIn = 38, nSigOps = 14, hashsize = 500000000):
        tx = CTransaction()
        for i in range(nIn):
            tx.vin.append(CTxIn(COutPoint(self.txid,i+offset),nSequence=4294967295))
            tx.vout.append(CTxOut(1, hex_str_to_bytes("00" * ((hashsize // (nIn * nIn * nSigOps)) - 50))))
        tx.rehash()
        return tx

    def validation_time(self, tx):
        print ("Transaction weight : " + str(len(tx.serialize_without_witness()) * 3 + len(tx.serialize_with_witness())))
        start = time.time()
        self.nodes[0].sendrawtransaction(bytes_to_hex_str(tx.serialize_with_witness()), True)
        t = time.time() - start
        print ("Validation time    : " + str(t))
        self.nodes[0].generate(1)
        return t

    def test_preparation(self):
        self.nIn = 38
        self.coinbase_blocks = self.nodes[0].generate(1)
        coinbase_txid = []
        for i in self.coinbase_blocks:
            coinbase_txid.append(int("0x" + self.nodes[0].getblock(i)['tx'][0], 0))
        self.nodes[0].generate(450)
        self.key = CECKey()
        self.key.set_secretbytes(b"9")
        self.key.set_compressed(1)
        pubkey = CPubKey(self.key.get_pubkey())
        self.script = []
        scriptpubkey = []
        manychecksig = CScript([pubkey, OP_CHECKSIGVERIFY] * 136 + [pubkey, OP_CHECKSIG])
        self.script.append(CScript([OP_1, pubkey] + [dummykey] * 13 + [OP_14, OP_CHECKMULTISIG]))
        self.script.append(CScript([OP_14] + [pubkey] * 14 + [OP_14, OP_CHECKMULTISIG]))
        self.script.append(CScript([pubkey, OP_CHECKSIGVERIFY, OP_1, pubkey] + [dummykey] * 11 + [OP_12, OP_CHECKMULTISIGVERIFY, pubkey, OP_CHECKSIG]))
        self.script.append(CScript([pubkey, OP_CHECKSIGVERIFY] * 13 + [pubkey, OP_CHECKSIG]))
        self.script.append(CScript([pubkey, OP_CHECKSIGVERIFY, OP_CODESEPARATOR] * 13 + [pubkey, OP_CHECKSIG]))
        self.script.append(manychecksig) # Not valid for P2SH due to too big

        self.csscript = []
        for i in range(14):
            self.csscript.append(CScript([pubkey, OP_CHECKSIGVERIFY, OP_CODESEPARATOR] * i + [pubkey, OP_CHECKSIG]))

        for i in self.script:
            scriptpubkey.append(CScript([OP_HASH160, hash160(i), OP_EQUAL]))
            scriptpubkey.append(CScript([OP_0, sha256(i)]))

        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(coinbase_txid[0])))
        for i in scriptpubkey:
            for j in range(1000):
                tx.vout.append(CTxOut(200000,i))
        for i in range(10):
            tx.vout.append(CTxOut(2000000,manychecksig))
        #print (bytes_to_hex_str(tx.vout[12000].scriptPubKey))
        signresult = self.nodes[0].signrawtransaction(bytes_to_hex_str(tx.serialize_without_witness()))['hex']
        self.txid = int("0x" + self.nodes[0].sendrawtransaction(signresult, True), 0)
        self.nodes[0].generate(1)

    def P2SH_14_of_14_different_ALL(self):
        script = self.script[1]
        print ("Test: 14-of-14 CHECKMULTISIG P2SH inputs with different variations of SIGHASH_ALL")
        tx = self.testtx(2000)
        for i in range(self.nIn):
            sig = []
            for h in range(4,18):
                sighash = SignatureHash(script, tx, i, h)
                sig.append(self.key.sign(sighash[0]) + chr(h).encode('latin-1'))
            tx.vin[i].scriptSig = CScript([OP_0] + sig + [script])
        self.banchmark = self.validation_time(tx)

    def P2SH_1_of_14_ALL(self):
        script = self.script[0]
        print ("Test: 1-of-14 CHECKMULTISIG P2SH inputs with SIGHASH_ALL")
        tx = self.testtx(0)
        for i in range(self.nIn):
            sighash = SignatureHash(script, tx, i, SIGHASH_ALL)
            sig = self.key.sign(sighash[0]) + chr(SIGHASH_ALL).encode('latin-1')
            tx.vin[i].scriptSig = CScript([OP_0, sig, script])
        t = self.validation_time(tx)
        assert(self.banchmark / t > 5)

    def P2SH_14_of_14_same_ALL(self):
        script = self.script[1]
        print ("Test: 14-of-14 CHECKMULTISIG P2SH inputs with same SIGHASH_ALL")
        tx = self.testtx(2100)
        for i in range(self.nIn):
            sighash = SignatureHash(script, tx, i, SIGHASH_ALL)
            sig = self.key.sign(sighash[0]) + chr(SIGHASH_ALL).encode('latin-1')
            tx.vin[i].scriptSig = CScript([OP_0] + [sig] * 14 + [script])
        t = self.validation_time(tx)
        assert(self.banchmark / t > 5)

    def P2SH_mix_CHECKSIG_CHECKMULTISIG_same_ALL(self):
        script = self.script[2]
        print ("Test: CHECKSIG 1-of-13 CHECKMULTISIG CHECKSIG P2SH inputs with same SIGHASH_ALL")
        tx = self.testtx(4000)
        for i in range(self.nIn):
            sighash = SignatureHash(script, tx, i, SIGHASH_ALL)
            sig = self.key.sign(sighash[0]) + chr(SIGHASH_ALL).encode('latin-1')
            tx.vin[i].scriptSig = CScript([sig, OP_0, sig, sig, script])
        t = self.validation_time(tx)
        assert(self.banchmark / t > 5)


    def P2SH_14_CHECKSIG_same_ALL(self):
        script = self.script[3]
        print ("Test: 14 CHECKSIG inputs with same SIGHASH_ALL")
        tx = self.testtx(6000)
        for i in range(self.nIn):
            sighash = SignatureHash(script, tx, i, SIGHASH_ALL)
            sig = self.key.sign(sighash[0]) + chr(SIGHASH_ALL).encode('latin-1')
            tx.vin[i].scriptSig = CScript([sig] * 14 + [script])
        t = self.validation_time(tx)
        assert(self.banchmark / t > 5)

    def P2SH_14_CHECKSIG_different_ALL(self):
        script = self.script[3]
        print ("Test: 14 CHECKSIG inputs with different variations of SIGHASH_ALL")
        tx = self.testtx(6100)
        for i in range(self.nIn):
            sig = []
            for h in range(4,18):
                sighash = SignatureHash(script, tx, i, h)
                sig.append(self.key.sign(sighash[0]) + chr(h).encode('latin-1'))
            tx.vin[i].scriptSig = CScript(sig + [script])
        t = self.validation_time(tx)
        assert(self.banchmark / t < 2)
        assert(t / self.banchmark < 2)

    def P2SH_14_CHECKSIG_CODESEPERATOR_same_ALL(self):
        script = self.script[4]
        print ("Test: 14 CHECKSIG CODESEPERATOR inputs with same SIGHASH_ALL")
        tx = self.testtx(8000)
        for i in range(self.nIn):
            sig = []
            for s in self.csscript:
                sighash = SignatureHash(s, tx, i, SIGHASH_ALL)
                sig.append(self.key.sign(sighash[0]) + chr(SIGHASH_ALL).encode('latin-1'))
            tx.vin[i].scriptSig = CScript(sig + [script])
        t = self.validation_time(tx)
        assert(self.banchmark / t < 2)
        assert(t / self.banchmark < 2)

    def P2SH_14_CHECKSIG_CODESEPERATOR_different_ALL(self):
        script = self.script[4]
        print ("Test: 14 CHECKSIG CODESEPERATOR inputs with different SIGHASH_ALL")
        tx = self.testtx(8100)
        for i in range(self.nIn):
            sig = []
            for j in range(14):
                sighash = SignatureHash(self.csscript[j], tx, i, j+4)
                sig.append(self.key.sign(sighash[0]) + chr(j+4).encode('latin-1'))
            tx.vin[i].scriptSig = CScript(sig + [script])
        t = self.validation_time(tx)
        assert(self.banchmark / t < 2)
        assert(t / self.banchmark < 2)

    def Bare_137_CHECKSIG_random_flag(self):
        script = self.script[5]
        print ("Test: 137 CHECKSIG bare inputs with random SIGHASH")
        tx = self.testtx(12000, 1, 137, 100000000)
        for i in range(1):
            sig = []
            for j in range(137):
                flag = randint(0,255)
                sighash = SignatureHash(script, tx, i, flag)
                sig.append(self.key.sign(sighash[0]) + chr(flag).encode('latin-1'))
            tx.vin[i].scriptSig = CScript(sig)
        self.validation_time(tx)


    def run_test(self):
        self.test_preparation()
        self.banchmark = 7
        self.P2SH_14_of_14_different_ALL()
        self.P2SH_14_of_14_same_ALL()
        self.P2SH_1_of_14_ALL()
        self.P2SH_mix_CHECKSIG_CHECKMULTISIG_same_ALL()
        self.P2SH_14_CHECKSIG_same_ALL()
        self.P2SH_14_CHECKSIG_different_ALL()
        self.P2SH_14_CHECKSIG_CODESEPERATOR_same_ALL()
        self.P2SH_14_CHECKSIG_CODESEPERATOR_different_ALL()
        self.Bare_137_CHECKSIG_random_flag()

if __name__ == '__main__':
    SigHashCacheTest().main()
