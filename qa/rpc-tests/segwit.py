#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test the SegWit changeover logic
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import sha256, ripemd160, CTransaction, CTxIn, COutPoint, CTxOut
from test_framework.script import CScript, OP_HASH160, OP_CHECKSIG, OP_0, hash160, OP_EQUAL, OP_DUP, OP_EQUALVERIFY, OP_1, OP_2, OP_3, OP_CHECKMULTISIG

NODE_0 = 0
NODE_1 = 1
NODE_2 = 2
WIT_V0 = 0
WIT_V1 = 1

def witness_script(version, pubkey):
    if (version == 0):
        pubkeyhash = bytes_to_hex_str(ripemd160(sha256(hex_str_to_bytes(pubkey))))
        pkscript = "0014" + pubkeyhash
    elif (version == 1):
        # 1-of-1 multisig
        scripthash = bytes_to_hex_str(sha256(hex_str_to_bytes("5121" + pubkey + "51ae")))
        pkscript = "0020" + scripthash
    else:
        assert("Wrong version" == "0 or 1")
    return pkscript

def addlength(script):
    scriptlen = format(len(script)//2, 'x')
    assert(len(scriptlen) == 2)
    return scriptlen + script

def create_witnessprogram(version, node, utxo, pubkey, encode_p2sh, amount):
    pkscript = witness_script(version, pubkey);
    if (encode_p2sh):
        p2sh_hash = bytes_to_hex_str(ripemd160(sha256(hex_str_to_bytes(pkscript))))
        pkscript = "a914"+p2sh_hash+"87"
    inputs = []
    outputs = {}
    inputs.append({ "txid" : utxo["txid"], "vout" : utxo["vout"]} )
    DUMMY_P2SH = "2MySexEGVzZpRgNQ1JdjdP5bRETznm3roQ2" # P2SH of "OP_1 OP_DROP"
    outputs[DUMMY_P2SH] = amount
    tx_to_witness = node.createrawtransaction(inputs,outputs)
    #replace dummy output with our own
    tx_to_witness = tx_to_witness[0:110] + addlength(pkscript) + tx_to_witness[-8:]
    return tx_to_witness

def send_to_witness(version, node, utxo, pubkey, encode_p2sh, amount, sign=True, insert_redeem_script=""):
    tx_to_witness = create_witnessprogram(version, node, utxo, pubkey, encode_p2sh, amount)
    if (sign):
        signed = node.signrawtransaction(tx_to_witness)
        assert("errors" not in signed or len(["errors"]) == 0)
        return node.sendrawtransaction(signed["hex"])
    else:
        if (insert_redeem_script):
            tx_to_witness = tx_to_witness[0:82] + addlength(insert_redeem_script) + tx_to_witness[84:]

    return node.sendrawtransaction(tx_to_witness)

def getutxo(txid):
    utxo = {}
    utxo["vout"] = 0
    utxo["txid"] = txid
    return utxo

def find_unspent(node, min_value):
    for utxo in node.listunspent():
        if utxo['amount'] >= min_value:
            return utxo

class SegWitTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-logtimemicros", "-debug", "-walletprematurewitness"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-logtimemicros", "-debug", "-blockversion=4", "-promiscuousmempoolflags=517", "-prematurewitness", "-walletprematurewitness"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-logtimemicros", "-debug", "-blockversion=536870915", "-promiscuousmempoolflags=517", "-prematurewitness", "-walletprematurewitness"]))
        connect_nodes(self.nodes[1], 0)
        connect_nodes(self.nodes[2], 1)
        connect_nodes(self.nodes[0], 2)
        self.is_network_split = False
        self.sync_all()

    def success_mine(self, node, txid, sign, redeem_script=""):
        send_to_witness(1, node, getutxo(txid), self.pubkey[0], False, Decimal("49.998"), sign, redeem_script)
        block = node.generate(1)
        assert_equal(len(node.getblock(block[0])["tx"]), 2)
        sync_blocks(self.nodes)

    def skip_mine(self, node, txid, sign, redeem_script=""):
        send_to_witness(1, node, getutxo(txid), self.pubkey[0], False, Decimal("49.998"), sign, redeem_script)
        block = node.generate(1)
        assert_equal(len(node.getblock(block[0])["tx"]), 1)
        sync_blocks(self.nodes)

    def fail_accept(self, node, txid, sign, redeem_script=""):
        try:
            send_to_witness(1, node, getutxo(txid), self.pubkey[0], False, Decimal("49.998"), sign, redeem_script)
        except JSONRPCException as exp:
            assert(exp.error["code"] == -26)
        else:
            raise AssertionError("Tx should not have been accepted")

    def fail_mine(self, node, txid, sign, redeem_script=""):
        send_to_witness(1, node, getutxo(txid), self.pubkey[0], False, Decimal("49.998"), sign, redeem_script)
        try:
            node.generate(1)
        except JSONRPCException as exp:
            assert(exp.error["code"] == -1)
        else:
            raise AssertionError("Created valid block when TestBlockValidity should have failed")
        sync_blocks(self.nodes)

    def run_test(self):
        self.nodes[0].generate(161) #block 161

        print("Verify sigops are counted in GBT with pre-BIP141 rules before the fork")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tmpl = self.nodes[0].getblocktemplate({})
        assert(tmpl['sigoplimit'] == 20000)
        assert(tmpl['transactions'][0]['hash'] == txid)
        assert(tmpl['transactions'][0]['sigops'] == 2)
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit']})
        assert(tmpl['sigoplimit'] == 20000)
        assert(tmpl['transactions'][0]['hash'] == txid)
        assert(tmpl['transactions'][0]['sigops'] == 2)
        self.nodes[0].generate(1) #block 162

        balance_presetup = self.nodes[0].getbalance()
        self.pubkey = []
        p2sh_ids = [] # p2sh_ids[NODE][VER] is an array of txids that spend to a witness version VER pkscript to an address for NODE embedded in p2sh
        wit_ids = [] # wit_ids[NODE][VER] is an array of txids that spend to a witness version VER pkscript to an address for NODE via bare witness
        for i in range(3):
            newaddress = self.nodes[i].getnewaddress()
            self.pubkey.append(self.nodes[i].validateaddress(newaddress)["pubkey"])
            multiaddress = self.nodes[i].addmultisigaddress(1, [self.pubkey[-1]])
            self.nodes[i].addwitnessaddress(newaddress)
            self.nodes[i].addwitnessaddress(multiaddress)
            p2sh_ids.append([])
            wit_ids.append([])
            for v in range(2):
                p2sh_ids[i].append([])
                wit_ids[i].append([])

        for i in range(5):
            for n in range(3):
                for v in range(2):
                    wit_ids[n][v].append(send_to_witness(v, self.nodes[0], find_unspent(self.nodes[0], 50), self.pubkey[n], False, Decimal("49.999")))
                    p2sh_ids[n][v].append(send_to_witness(v, self.nodes[0], find_unspent(self.nodes[0], 50), self.pubkey[n], True, Decimal("49.999")))

        self.nodes[0].generate(1) #block 163
        sync_blocks(self.nodes)

        # Make sure all nodes recognize the transactions as theirs
        assert_equal(self.nodes[0].getbalance(), balance_presetup - 60*50 + 20*Decimal("49.999") + 50)
        assert_equal(self.nodes[1].getbalance(), 20*Decimal("49.999"))
        assert_equal(self.nodes[2].getbalance(), 20*Decimal("49.999"))

        self.nodes[0].generate(260) #block 423
        sync_blocks(self.nodes)

        print("Verify default node can't accept any witness format txs before fork")
        # unsigned, no scriptsig
        self.fail_accept(self.nodes[0], wit_ids[NODE_0][WIT_V0][0], False)
        self.fail_accept(self.nodes[0], wit_ids[NODE_0][WIT_V1][0], False)
        self.fail_accept(self.nodes[0], p2sh_ids[NODE_0][WIT_V0][0], False)
        self.fail_accept(self.nodes[0], p2sh_ids[NODE_0][WIT_V1][0], False)
        # unsigned with redeem script
        self.fail_accept(self.nodes[0], p2sh_ids[NODE_0][WIT_V0][0], False, addlength(witness_script(0, self.pubkey[0])))
        self.fail_accept(self.nodes[0], p2sh_ids[NODE_0][WIT_V1][0], False, addlength(witness_script(1, self.pubkey[0])))
        # signed
        self.fail_accept(self.nodes[0], wit_ids[NODE_0][WIT_V0][0], True)
        self.fail_accept(self.nodes[0], wit_ids[NODE_0][WIT_V1][0], True)
        self.fail_accept(self.nodes[0], p2sh_ids[NODE_0][WIT_V0][0], True)
        self.fail_accept(self.nodes[0], p2sh_ids[NODE_0][WIT_V1][0], True)

        print("Verify witness txs are skipped for mining before the fork")
        self.skip_mine(self.nodes[2], wit_ids[NODE_2][WIT_V0][0], True) #block 424
        self.skip_mine(self.nodes[2], wit_ids[NODE_2][WIT_V1][0], True) #block 425
        self.skip_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V0][0], True) #block 426
        self.skip_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V1][0], True) #block 427

        # TODO: An old node would see these txs without witnesses and be able to mine them

        print("Verify unsigned bare witness txs in versionbits-setting blocks are valid before the fork")
        self.success_mine(self.nodes[2], wit_ids[NODE_2][WIT_V0][1], False) #block 428
        self.success_mine(self.nodes[2], wit_ids[NODE_2][WIT_V1][1], False) #block 429

        print("Verify unsigned p2sh witness txs without a redeem script are invalid")
        self.fail_accept(self.nodes[2], p2sh_ids[NODE_2][WIT_V0][1], False)
        self.fail_accept(self.nodes[2], p2sh_ids[NODE_2][WIT_V1][1], False)

        print("Verify unsigned p2sh witness txs with a redeem script in versionbits-settings blocks are valid before the fork")
        self.success_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V0][1], False, addlength(witness_script(0, self.pubkey[2]))) #block 430
        self.success_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V1][1], False, addlength(witness_script(1, self.pubkey[2]))) #block 431

        print("Verify previous witness txs skipped for mining can now be mined")
        assert_equal(len(self.nodes[2].getrawmempool()), 4)
        block = self.nodes[2].generate(1) #block 432 (first block with new rules; 432 = 144 * 3)
        sync_blocks(self.nodes)
        assert_equal(len(self.nodes[2].getrawmempool()), 0)
        assert_equal(len(self.nodes[2].getblock(block[0])["tx"]), 5)

        print("Verify witness txs without witness data are invalid after the fork")
        self.fail_mine(self.nodes[2], wit_ids[NODE_2][WIT_V0][2], False)
        self.fail_mine(self.nodes[2], wit_ids[NODE_2][WIT_V1][2], False)
        self.fail_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V0][2], False, addlength(witness_script(0, self.pubkey[2])))
        self.fail_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V1][2], False, addlength(witness_script(1, self.pubkey[2])))

        print("Verify default node can now use witness txs")
        self.success_mine(self.nodes[0], wit_ids[NODE_0][WIT_V0][0], True) #block 432
        self.success_mine(self.nodes[0], wit_ids[NODE_0][WIT_V1][0], True) #block 433
        self.success_mine(self.nodes[0], p2sh_ids[NODE_0][WIT_V0][0], True) #block 434
        self.success_mine(self.nodes[0], p2sh_ids[NODE_0][WIT_V1][0], True) #block 435

        print("Verify sigops are counted in GBT with BIP141 rules after the fork")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit']})
        assert(tmpl['sigoplimit'] == 80000)
        assert(tmpl['transactions'][0]['txid'] == txid)
        assert(tmpl['transactions'][0]['sigops'] == 8)

        print("Verify non-segwit miners get a valid GBT response after the fork")
        send_to_witness(1, self.nodes[0], find_unspent(self.nodes[0], 50), self.pubkey[0], False, Decimal("49.998"))
        try:
            tmpl = self.nodes[0].getblocktemplate({})
            assert(len(tmpl['transactions']) == 1)  # Doesn't include witness tx
            assert(tmpl['sigoplimit'] == 20000)
            assert(tmpl['transactions'][0]['hash'] == txid)
            assert(tmpl['transactions'][0]['sigops'] == 2)
            assert(('!segwit' in tmpl['rules']) or ('segwit' not in tmpl['rules']))
        except JSONRPCException:
            # This is an acceptable outcome
            pass

        print("Verify behaviour of addwitnessaddress and listunspent")
        # Import a compressed key and an uncompressed key, generate some multisig addresses
        self.nodes[0].importprivkey("92e6XLo5jVAVwrQKPNTs93oQco8f8sDNBcpv73Dsrs397fQtFQn")
        uncompressed_spendable_address = ["mvozP4UwyGD2mGZU4D2eMvMLPB9WkMmMQu"]
        self.nodes[0].importprivkey("cSEjNvNPtXJvm73v9jjaJXMqEzcoKidaVs8VPeoqV5rSpJTK54Rt")
        compressed_spendable_address = ["mhc6haiMVa5Drgdref1GLMpwQ7a3BNpV56"]
        assert ((self.nodes[0].validateaddress(uncompressed_spendable_address[0])['iscompressed'] == False))
        assert ((self.nodes[0].validateaddress(compressed_spendable_address[0])['iscompressed'] == True))

        self.nodes[0].importpubkey("033A1F833F58D60E725A3BB3EA28D0ED4C0EE955130044DAC7DC4E3A7380640A9C")
        compressed_solvable_address = ["mmthm53DWuHt1Pn3L2188B82V9cbaYS3Jh"]
        self.nodes[0].importpubkey("04AD299EC23EEE72765FCF3D072797E01420C6DB0D924B13E905F0DC87630257413CDC5B62CCC15C10F20D3CE7B11CD81A164A71DDFDEE6229D48B4E9240BBD8E1")
        uncompressed_solvable_address = ["motjpmSATrH5AVpinJd3VtcK34Mh3cKyfv"]

        spendable_anytime = []                      # These outputs should be seen anytime after importprivkey and addmultisigaddress
        spendable_after_importaddress = []          # These outputs should be seen after importaddress
        watchonly_after_importaddress=[]            # These outputs should be seen after importaddress but not spendable
        watchonly_anytime = []                      # These outputs should be watchonly after importpubkey
        unseen_anytime = []                         # These outputs should never be seen

        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], uncompressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], uncompressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(3, [uncompressed_spendable_address[0], compressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(3, [compressed_spendable_address[0], uncompressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(3, [compressed_spendable_address[0], compressed_spendable_address[0], uncompressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(3, [uncompressed_spendable_address[0], uncompressed_spendable_address[0], uncompressed_spendable_address[0]]))
        compressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_spendable_address[0]]))
        compressed_spendable_address.append(self.nodes[0].addmultisigaddress(3, [compressed_spendable_address[0], compressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_solvable_address.append(self.nodes[0].addmultisigaddress(3, [compressed_spendable_address[0], compressed_solvable_address[0], uncompressed_solvable_address[0]]))
        compressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_solvable_address[0]]))
        uncompressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_solvable_address[0], uncompressed_solvable_address[0]]))
        unknown_address = ["mtKKyoHabkk6e4ppT7NaM7THqPUt7AzPrT", "2NDP3jLWAFT8NDAiUa9qiE6oBt2awmMq7Dx"]

        # Test multisig_without_privkey
        # We have 2 public keys without private keys, use addmultisigaddress to add to wallet.
        # Money sent to P2SH of this should only be seen after importaddress with the BASE58 P2SH address.
        key_without_privkey = ["022E31789DC5769877ADC5D1E9CA03AF3E7A1E52C4AA2D825E36D6A600B972B2AC","033EE56C6C7E1365D042FAF028077713159A0CF5C5D54897AD8E5F44ACF964F2AD"]
        multisig_without_privkey_address = self.nodes[0].addmultisigaddress(2, [key_without_privkey[0], key_without_privkey[1]])
        script = CScript([OP_2, hex_str_to_bytes(key_without_privkey[0]), hex_str_to_bytes(key_without_privkey[1]), OP_2, OP_CHECKMULTISIG])
        watchonly_after_importaddress.append(CScript([OP_HASH160, hash160(script), OP_EQUAL]))

        for i in compressed_spendable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # bare and p2sh multisig with compressed keys should always be spendable
                spendable_anytime.extend([bare, p2sh])
                # P2WSH and P2SH(P2WSH) multisig with compressed keys are spendable after direct importaddress
                spendable_after_importaddress.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with compressed keys should always be spendable
                spendable_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK, P2SH_P2PKH, and witness with compressed keys are spendable after direct importaddress
                spendable_after_importaddress.extend([p2wpkh, p2sh_p2wpkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        for i in uncompressed_spendable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # bare and p2sh multisig with uncompressed keys should always be spendable
                spendable_anytime.extend([bare, p2sh])
                # P2WSH and P2SH(P2WSH) multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with uncompressed keys should always be spendable
                spendable_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK and P2SH_P2PKH are spendable after direct importaddress
                spendable_after_importaddress.extend([p2sh_p2pk, p2sh_p2pkh])
                # witness with uncompressed keys are never seen
                unseen_anytime.extend([p2wpkh, p2sh_p2wpkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        for i in compressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                # Multisig without private is not seen after addmultisigaddress, but seen after importaddress
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                watchonly_after_importaddress.extend([bare, p2sh, p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with compressed keys should always be seen
                watchonly_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK, P2SH_P2PKH, and witness with compressed keys are seen after direct importaddress
                watchonly_after_importaddress.extend([p2wpkh, p2sh_p2wpkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        for i in uncompressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # Base uncompressed multisig without private is not seen after addmultisigaddress, but seen after importaddress
                watchonly_after_importaddress.extend([bare, p2sh])
                # P2WSH and P2SH(P2WSH) multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with uncompressed keys should always be seen
                watchonly_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK, P2SH_P2PKH with uncompressed keys are seen after direct importaddress
                watchonly_after_importaddress.extend([p2sh_p2pk, p2sh_p2pkh])
                # witness with uncompressed keys are never seen
                unseen_anytime.extend([p2wpkh, p2sh_p2wpkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        # 2N7MGY19ti4KDMSzRfPAssP6Pxyuxoi6jLe is the P2SH(P2PKH) version of mjoE3sSrb8ByYEvgnC3Aox86u1CHnfJA4V
        # 2ND8PB9RrfCaAcjfjP1Y6nAgFd9zWHYX4DN is P2SH(OP_1)
        # 2N7naYWiG3M21cQWc3nJgPRq76v8AKtkYFs is P2SH(OP_0)
        insolvable_address = ["mjoE3sSrb8ByYEvgnC3Aox86u1CHnfJA4V", "2N7MGY19ti4KDMSzRfPAssP6Pxyuxoi6jLe", "2ND8PB9RrfCaAcjfjP1Y6nAgFd9zWHYX4DN", "2N7naYWiG3M21cQWc3nJgPRq76v8AKtkYFs"]
        insolvable_address_key = hex_str_to_bytes("02341AEC7587A51CDE5279E0630A531AEA2615A9F80B17E8D9376327BAEAA59E3D")
        insolvablep2pkh = CScript([OP_DUP, OP_HASH160, hash160(insolvable_address_key), OP_EQUALVERIFY, OP_CHECKSIG])
        insolvablep2shp2pkh = CScript([OP_HASH160, hash160(insolvablep2pkh), OP_EQUAL])
        insolvablep2wshp2pkh = CScript([OP_0, sha256(insolvablep2pkh)])
        insolvablep2shp2wshp2pkh = CScript([OP_HASH160, hash160(insolvablep2wshp2pkh), OP_EQUAL])
        op1 = CScript([OP_1])
        op0 = CScript([OP_0])
        p2shop1 = CScript([OP_HASH160, hash160(op1), OP_EQUAL])
        p2shop0 = CScript([OP_HASH160, hash160(op0), OP_EQUAL])
        p2wshop1 = CScript([OP_0, sha256(op1)])
        p2shp2wshop1 = CScript([OP_HASH160, hash160(p2wshop1), OP_EQUAL])
        watchonly_after_importaddress.append(insolvablep2pkh)
        watchonly_after_importaddress.append(insolvablep2shp2pkh)
        watchonly_after_importaddress.append(insolvablep2wshp2pkh)
        watchonly_after_importaddress.append(insolvablep2shp2wshp2pkh)
        watchonly_after_importaddress.append(op1) # OP_1 will be imported as script
        watchonly_after_importaddress.append(p2shop1)
        watchonly_after_importaddress.append(p2wshop1)
        watchonly_after_importaddress.append(p2shp2wshop1)
        unseen_anytime.append(op0) # OP_0 will be imported as P2SH address with no script provided
        watchonly_after_importaddress.append(p2shop0)


        self.mine_and_test_listunspent(spendable_anytime, 2)
        self.mine_and_test_listunspent(watchonly_anytime, 1)
        self.mine_and_test_listunspent(spendable_after_importaddress + watchonly_after_importaddress + unseen_anytime, 0)

        importlist = []
        for i in compressed_spendable_address + uncompressed_spendable_address + compressed_solvable_address + uncompressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                bare = hex_str_to_bytes(v['hex'])
                importlist.append(bytes_to_hex_str(bare))
                importlist.append(bytes_to_hex_str(CScript([OP_0, sha256(bare)])))
            else:
                pubkey = hex_str_to_bytes(v['pubkey'])
                p2pk = CScript([pubkey, OP_CHECKSIG])
                p2pkh = CScript([OP_DUP, OP_HASH160, hash160(pubkey), OP_EQUALVERIFY, OP_CHECKSIG])
                importlist.append(bytes_to_hex_str(p2pk))
                importlist.append(bytes_to_hex_str(p2pkh))
                importlist.append(bytes_to_hex_str(CScript([OP_0, hash160(pubkey)])))
                importlist.append(bytes_to_hex_str(CScript([OP_0, sha256(p2pk)])))
                importlist.append(bytes_to_hex_str(CScript([OP_0, sha256(p2pkh)])))

        importlist.append(bytes_to_hex_str(insolvablep2pkh))
        importlist.append(bytes_to_hex_str(insolvablep2wshp2pkh))
        importlist.append(bytes_to_hex_str(op1))
        importlist.append(bytes_to_hex_str(p2wshop1))


        for i in importlist:
            try:
                self.nodes[0].importaddress(i,"",False,True)
                #print (i)
            except JSONRPCException as exp:
                assert_equal(exp.error["message"], "The wallet already contains the private key for this address or script")

        self.nodes[0].importaddress("2N7naYWiG3M21cQWc3nJgPRq76v8AKtkYFs") # import OP_0 as address only
        self.nodes[0].importaddress(multisig_without_privkey_address) # Test multisig_without_privkey

        self.mine_and_test_listunspent(spendable_anytime + spendable_after_importaddress, 2)
        self.mine_and_test_listunspent(watchonly_anytime + watchonly_after_importaddress, 1)
        self.mine_and_test_listunspent(unseen_anytime, 0)

        # addwitnessaddress should refuse to return a witness address if an uncompressed key is used or the address is
        # not in the wallet
        # note that no witness address should be returned by insolvable addresses
        for i in uncompressed_spendable_address + uncompressed_solvable_address + unknown_address + insolvable_address:
            try:
                self.nodes[0].addwitnessaddress(i)
            except JSONRPCException as exp:
                assert_equal(exp.error["message"], "Public key or redeemscript not known to wallet, or the key is uncompressed")
            else:
                assert(False)

        for i in compressed_spendable_address + compressed_solvable_address:
            witaddress = self.nodes[0].addwitnessaddress(i)
            # addwitnessaddress should return the same address if it is a known P2SH-P2WSH address
            assert_equal(witaddress, self.nodes[0].addwitnessaddress(witaddress))

        self.mine_and_test_listunspent(spendable_anytime + spendable_after_importaddress, 2)
        self.mine_and_test_listunspent(watchonly_anytime + watchonly_after_importaddress, 1)
        self.mine_and_test_listunspent(unseen_anytime, 0)

        # Import a compressed key and an uncompressed key, generate some multisig addresses
        self.nodes[0].importprivkey("927pw6RW8ZekycnXqBQ2JS5nPyo1yRfGNN8oq74HeddWSpafDJH")
        uncompressed_spendable_address = ["mguN2vNSCEUh6rJaXoAVwY3YZwZvEmf5xi"]
        self.nodes[0].importprivkey("cMcrXaaUC48ZKpcyydfFo8PxHAjpsYLhdsp6nmtB3E2ER9UUHWnw")
        compressed_spendable_address = ["n1UNmpmbVUJ9ytXYXiurmGPQ3TRrXqPWKL"]
        assert ((self.nodes[0].validateaddress(uncompressed_spendable_address[0])['iscompressed'] == False))
        assert ((self.nodes[0].validateaddress(compressed_spendable_address[0])['iscompressed'] == True))

        print (self.nodes[0].importpubkey("03B6A2A8B6AE9AA2EAE02CF8865F229004CE4C5D4113041FA3042A3479FD9CF413"))
        compressed_solvable_address = ["mmn5B35V357eAfGDczWABrrycivVYnNqpc"]
        self.nodes[0].importpubkey("04A495A177F32253349EE44FB40D587D634498D9C4BF60F376F319DF157171BD4DF729B98A2F2186DD4EABDD2F27E7CB994449611BB0E05152A29BF91EDD7FC818")
        uncompressed_solvable_address = ["mvTkS9asFkVXpDpexV8nsvckGb7sxzHUhb"]

        spendable_anytime = []                      # These outputs should be seen anytime after importprivkey and addmultisigaddress
        spendable_after_importaddress = []          # These outputs should be seen after importaddress
        spendable_after_addwitnessaddress = []      # These outputs should be seen after importaddress
        watchonly_after_importaddress=[]            # These outputs should be seen after importaddress but not spendable
        watchonly_anytime = []                      # These outputs should be watchonly after importpubkey
        unseen_anytime = []                         # These outputs should never be seen

        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], uncompressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], uncompressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(3, [uncompressed_spendable_address[0], compressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(3, [compressed_spendable_address[0], uncompressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(3, [compressed_spendable_address[0], compressed_spendable_address[0], uncompressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(3, [uncompressed_spendable_address[0], uncompressed_spendable_address[0], uncompressed_spendable_address[0]]))
        compressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_solvable_address.append(self.nodes[0].addmultisigaddress(3, [compressed_spendable_address[0], compressed_solvable_address[0], uncompressed_solvable_address[0]]))
        compressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_solvable_address[0]]))
        uncompressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_solvable_address[0], uncompressed_solvable_address[0]]))
        unknown_address = ["mtKKyoHabkk6e4ppT7NaM7THqPUt7AzPrT", "2NDP3jLWAFT8NDAiUa9qiE6oBt2awmMq7Dx"]

        # Test multisig_without_privkey
        # We have 2 public keys without private keys, use addmultisigaddress to add to wallet.
        # Money sent to P2SH of this should only be seen after importaddress with the BASE58 P2SH address.
        key_without_privkey = ["03F72C1E06E81AC65EB9712E95060101E8FE19258F9EEEFBD803AA01CC5C6DE3C6","03123858683B875347735E4B87FE6C619F04BE5CBB53F08F377C370B08408A8C8E"]
        multisig_without_privkey_address = self.nodes[0].addmultisigaddress(2, [key_without_privkey[0], key_without_privkey[1]])
        script = CScript([OP_2, hex_str_to_bytes(key_without_privkey[0]), hex_str_to_bytes(key_without_privkey[1]), OP_2, OP_CHECKMULTISIG])
        #watchonly_after_importaddress.append(CScript([OP_HASH160, hash160(script), OP_EQUAL]))

        for i in compressed_spendable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # bare and p2sh multisig with compressed keys should always be spendable
                spendable_anytime.extend([bare, p2sh])
                # P2WSH and P2SH(P2WSH) multisig with compressed keys are spendable after direct importaddress
                spendable_after_addwitnessaddress.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with compressed keys should always be spendable
                spendable_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK, P2SH_P2PKH, and witness with compressed keys are spendable after direct importaddress
                spendable_after_addwitnessaddress.extend([p2wpkh, p2sh_p2wpkh])

        for i in uncompressed_spendable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # bare and p2sh multisig with uncompressed keys should always be spendable
                spendable_anytime.extend([bare, p2sh])
                # P2WSH and P2SH(P2WSH) multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with uncompressed keys should always be spendable
                spendable_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK and P2SH_P2PKH are spendable after direct importaddress
                # spendable_after_importaddress.extend([p2sh_p2pk, p2sh_p2pkh])
                # witness with uncompressed keys are never seen
                unseen_anytime.extend([p2wpkh, p2sh_p2wpkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        for i in compressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                # Multisig without private is not seen after addmultisigaddress, but seen after importaddress
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # watchonly_after_importaddress.extend([bare, p2sh, p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with compressed keys should always be seen
                watchonly_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK, P2SH_P2PKH, and witness with compressed keys are seen after direct importaddress
                watchonly_after_importaddress.extend([p2wpkh, p2sh_p2wpkh])

        for i in uncompressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # Base uncompressed multisig without private is not seen after addmultisigaddress, but seen after importaddress
                # watchonly_after_importaddress.extend([bare, p2sh])
                # P2WSH and P2SH(P2WSH) multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with uncompressed keys should always be seen
                watchonly_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK, P2SH_P2PKH with uncompressed keys are seen after direct importaddress
                # watchonly_after_importaddress.extend([p2sh_p2pk, p2sh_p2pkh])
                # witness with uncompressed keys are never seen
                unseen_anytime.extend([p2wpkh, p2sh_p2wpkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        #  is the P2SH(P2PKH) version of mk1Wx9RG8gGXNDWcbATXz7iTXSVcU81sFn
        #  is P2SH(OP_2)
        #  is P2SH(OP_3)
        insolvable_address = ["mk1Wx9RG8gGXNDWcbATXz7iTXSVcU81sFn"]
        insolvable_address_key = hex_str_to_bytes("03739353E39717A52BE8F54F41BDEEEAE4B435CBECB451410768B7FF4223AA3D8B")
        insolvablep2pkh = CScript([OP_DUP, OP_HASH160, hash160(insolvable_address_key), OP_EQUALVERIFY, OP_CHECKSIG])
        insolvablep2shp2pkh = CScript([OP_HASH160, hash160(insolvablep2pkh), OP_EQUAL])
        insolvablep2wshp2pkh = CScript([OP_0, sha256(insolvablep2pkh)])
        insolvablep2shp2wshp2pkh = CScript([OP_HASH160, hash160(insolvablep2wshp2pkh), OP_EQUAL])
        op2 = CScript([OP_2])
        op3 = CScript([OP_3])
        p2shop2 = CScript([OP_HASH160, hash160(op2), OP_EQUAL])
        p2shop3 = CScript([OP_HASH160, hash160(op3), OP_EQUAL])
        p2wshop2 = CScript([OP_0, sha256(op2)])
        p2shp2wshop2 = CScript([OP_HASH160, hash160(p2shop2), OP_EQUAL])
        # watchonly_after_importaddress.append(insolvablep2pkh)
        # watchonly_after_importaddress.append(insolvablep2shp2pkh)
        # watchonly_after_importaddress.append(insolvablep2wshp2pkh)
        # watchonly_after_importaddress.append(insolvablep2shp2wshp2pkh)
        # watchonly_after_importaddress.append(op2) # OP_1 will be imported as script
        # watchonly_after_importaddress.append(p2shop2)
        # watchonly_after_importaddress.append(p2wshop2)
        # watchonly_after_importaddress.append(p2shp2wshop2)
        unseen_anytime.append(op3) # OP_3 will be imported as P2SH address with no script provided
        # watchonly_after_importaddress.append(p2shop3)


        self.mine_and_test_listunspent(spendable_anytime, 2)
        self.mine_and_test_listunspent(watchonly_anytime, 1)
        self.mine_and_test_listunspent(spendable_after_addwitnessaddress + watchonly_after_importaddress + unseen_anytime, 0)

        # importlist = []
        # for i in compressed_spendable_address + uncompressed_spendable_address + compressed_solvable_address + uncompressed_solvable_address:
        #     v = self.nodes[0].validateaddress(i)
        #     if (v['isscript']):
        #         bare = hex_str_to_bytes(v['hex'])
        #         importlist.append(bytes_to_hex_str(bare))
        #         importlist.append(bytes_to_hex_str(CScript([OP_0, sha256(bare)])))
        #     else:
        #         pubkey = hex_str_to_bytes(v['pubkey'])
        #         p2pk = CScript([pubkey, OP_CHECKSIG])
        #         p2pkh = CScript([OP_DUP, OP_HASH160, hash160(pubkey), OP_EQUALVERIFY, OP_CHECKSIG])
        #         importlist.append(bytes_to_hex_str(p2pk))
        #         importlist.append(bytes_to_hex_str(p2pkh))
        #         importlist.append(bytes_to_hex_str(CScript([OP_0, hash160(pubkey)])))
        #         importlist.append(bytes_to_hex_str(CScript([OP_0, sha256(p2pk)])))
        #         importlist.append(bytes_to_hex_str(CScript([OP_0, sha256(p2pkh)])))
        #
        # importlist.append(bytes_to_hex_str(insolvablep2pkh))
        # importlist.append(bytes_to_hex_str(insolvablep2wshp2pkh))
        # importlist.append(bytes_to_hex_str(op1))
        # importlist.append(bytes_to_hex_str(p2wshop1))
        #
        #
        # for i in importlist:
        #     try:
        #         self.nodes[0].importaddress(i,"",False,True)
        #     except JSONRPCException as exp:
        #         assert_equal(exp.error["message"], "The wallet already contains the private key for this address or script")
        #
        # self.nodes[0].importaddress("2N7naYWiG3M21cQWc3nJgPRq76v8AKtkYFs") # import OP_0 as address only
        # self.nodes[0].importaddress(multisig_without_privkey_address) # Test multisig_without_privkey
        #
        # self.mine_and_test_listunspent(spendable_anytime + spendable_after_importaddress, 2)
        # self.mine_and_test_listunspent(watchonly_anytime + watchonly_after_importaddress, 1)
        # self.mine_and_test_listunspent(unseen_anytime, 0)

        # addwitnessaddress should refuse to return a witness address if an uncompressed key is used or the address is
        # not in the wallet
        # note that no witness address should be returned by insolvable addresses
        # note that merely solvable address are rejected without explictly added with importaddress first
        for i in uncompressed_spendable_address + uncompressed_solvable_address + unknown_address + insolvable_address + [compressed_solvable_address[1]] : # TODO: fix the P2WPKH watch bug and disable
        # for i in uncompressed_spendable_address + uncompressed_solvable_address + unknown_address + insolvable_address + compressed_solvable_address : # TODO: fix the P2WPKH watch bug and enable
            try:
                self.nodes[0].addwitnessaddress(i)
            except JSONRPCException as exp:
                assert_equal(exp.error["message"], "Public key or redeemscript not known to wallet, or the key is uncompressed")
            else:
                #print (i)
                assert(False)

        for i in compressed_spendable_address:
            witaddress = self.nodes[0].addwitnessaddress(i)
            # addwitnessaddress should return the same address if it is a known P2SH-P2WSH address
            assert_equal(witaddress, self.nodes[0].addwitnessaddress(witaddress))

        self.mine_and_test_listunspent(spendable_anytime + spendable_after_addwitnessaddress, 2)
        # self.mine_and_test_listunspent(watchonly_after_importaddress, 1) # TODO: fix the P2WPKH watch bug and enable
        # self.mine_and_test_listunspent(unseen_anytime, 0)


    def mine_and_test_listunspent(self, script_list, ismine):
        utxo = find_unspent(self.nodes[0], 50)
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(int('0x'+utxo['txid'],0), utxo['vout'])))
        tx.vout.append(CTxOut(0, CScript()))
        for i in script_list:
            tx.vout.append(CTxOut(0, i))
        tx.rehash()
        signresults = self.nodes[0].signrawtransaction(bytes_to_hex_str(tx.serialize_without_witness()))['hex']
        txid = self.nodes[0].sendrawtransaction(signresults, True)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)
        watchcount = 0
        spendcount = 0
        for i in self.nodes[0].listunspent():
            # if (i['solvable'] == False):
            #     print(i)
            if (i['txid'] == txid):
                watchcount += 1
                if (i['spendable'] == True):
                    spendcount += 1
        if (ismine == 2):
            assert_equal(spendcount, len(script_list))
        elif (ismine == 1):
            assert_equal(watchcount, len(script_list))
            assert_equal(spendcount, 0)
        else:
            assert_equal(watchcount, 0)

    def p2sh_address_to_script(self,v):
        bare = CScript(hex_str_to_bytes(v['hex']))
        p2sh = CScript(hex_str_to_bytes(v['scriptPubKey']))
        p2wsh = CScript([OP_0, sha256(bare)])
        p2sh_p2wsh = CScript([OP_HASH160, hash160(p2wsh), OP_EQUAL])
        return([bare, p2sh, p2wsh, p2sh_p2wsh])

    def p2pkh_address_to_script(self,v):
        pubkey = hex_str_to_bytes(v['pubkey'])
        p2wpkh = CScript([OP_0, hash160(pubkey)])
        p2sh_p2wpkh = CScript([OP_HASH160, hash160(p2wpkh), OP_EQUAL])
        p2pk = CScript([pubkey, OP_CHECKSIG])
        p2pkh = CScript(hex_str_to_bytes(v['scriptPubKey']))
        p2sh_p2pk = CScript([OP_HASH160, hash160(p2pk), OP_EQUAL])
        p2sh_p2pkh = CScript([OP_HASH160, hash160(p2pkh), OP_EQUAL])
        p2wsh_p2pk = CScript([OP_0, sha256(p2pk)])
        p2wsh_p2pkh = CScript([OP_0, sha256(p2pkh)])
        p2sh_p2wsh_p2pk = CScript([OP_HASH160, hash160(p2wsh_p2pk), OP_EQUAL])
        p2sh_p2wsh_p2pkh = CScript([OP_HASH160, hash160(p2wsh_p2pkh), OP_EQUAL])
        return [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh]


if __name__ == '__main__':
    SegWitTest().main()
