// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2018 The PIVX developers
// Copyright (c) 2018-2019 The MasterStake developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzerocoin/Params.h"
#include "chainparams.h"
#include "random.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

/**
 * Main network
 */

//! Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress>& vSeedsOut, const SeedSpec6* data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7 * 24 * 60 * 60;
    for (unsigned int i = 0; i < count; i++) {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

//   What makes a good checkpoint block?
// + Is surrounded by blocks with reasonable timestamps
//   (no blocks before with a timestamp after, none after with
//    timestamp before)
// + Contains no strange transactions
static Checkpoints::MapCheckpoints mapCheckpoints =
    boost::assign::map_list_of
(0,  uint256("0x0000f00ba769187169bc7e9b4fd82c73ce31d355c35915a5d999ef55d3c903fb"));

static const Checkpoints::CCheckpointData data = {
    &mapCheckpoints,
    1656967050, // * UNIX timestamp of last checkpoint block
    104,     // * total number of transactions between genesis and last checkpoint
           //   (the tx=... number in the SetBestChain debug.log lines)
    480        // * estimated number of transactions per day after checkpoint
};

static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
    boost::assign::map_list_of(0, uint256("0x0000f00ba769187169bc7e9b4fd82c73ce31d355c35915a5d999ef55d3c903fb"));
static const Checkpoints::CCheckpointData dataTestnet = {
    &mapCheckpointsTestnet,
    1656967050,
    0,
    250};

static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
    boost::assign::map_list_of(0, uint256("0x0000f00ba769187169bc7e9b4fd82c73ce31d355c35915a5d999ef55d3c903fb"));
static const Checkpoints::CCheckpointData dataRegtest = {
    &mapCheckpointsRegtest,
    1656967050,
    0,
    250};

libzerocoin::ZerocoinParams* CChainParams::Zerocoin_Params(bool useModulusV1) const
{
    assert(this);
    static CBigNum bnHexModulus = 0;
    if (!bnHexModulus)
        bnHexModulus.SetHex(zerocoinModulus);
    static libzerocoin::ZerocoinParams ZCParamsHex = libzerocoin::ZerocoinParams(bnHexModulus);
    static CBigNum bnDecModulus = 0;
    if (!bnDecModulus)
        bnDecModulus.SetDec(zerocoinModulus);
    static libzerocoin::ZerocoinParams ZCParamsDec = libzerocoin::ZerocoinParams(bnDecModulus);

    if (useModulusV1)
        return &ZCParamsHex;

    return &ZCParamsDec;
}

class CMainParams : public CChainParams
{
public:
    CMainParams()
    {
        networkID = CBaseChainParams::MAIN;
        strNetworkID = "main";
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */
        pchMessageStart[0] = 0xC2;
        pchMessageStart[1] = 0x6a;
        pchMessageStart[2] = 0x53;
        pchMessageStart[3] = 0x6a;
        vAlertPubKey = ParseHex("04f1eb1815febde1b045c0302beb0649d3831dda9e1cea51e6ce2d46c3d7cff7d72b4c5e4467f4b6d72b3dd0849d084462beffee067f4e8b16df86a958efdefd64");
        nDefaultPort = 49266;
        bnProofOfWorkLimit = ~uint256(0) >> 20; // MasterStake starting difficulty is 1 / 2^12
        nMaxReorganizationDepth = 100;
        nMinerThreads = 0;
        nTargetSpacing = 3 * 60;  // MASTER: 3 minutes
        nMaturity = 24;
        nMaxMoneyOut = 10000000 * COIN;

        nSubsidyHalvingBlock = 1000;
        nMasternodeCollateral = 15000;
        strDevFeeAddress = "MHrThmoWKXH9qqhVTRmnBXtPks7u2xGABe";

        /** Height or Time Based Activations **/
        nLastPOWBlock = 200;
        nModifierUpdateBlock = 0;
        nZerocoinStartHeight = 0;
        nZerocoinStartTime = 1547096400; // Genesis time
        nBlockZerocoinV2 = 20;

        const char* pszTimestamp = "New MasterStake Genesis Block mined by Team in 07/2022";
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 0 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("04330b7eec077a1d56b2bcaac8f1a6cf340ed806955d7eb56f2bba26454bd6d30857378b272f37d976fe803bba8e3472c6129b98b026871e0d3755cfeb472c2da0") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 4;
        genesis.nTime = 1656967050;
        genesis.nBits = 0x1e0ffff0;
        genesis.nNonce = 68866;

        hashGenesisBlock = genesis.GetHash();

        //printf("Genesis mainnet - %s\n",hashGenesisBlock.ToString().c_str());
        //printf("Merkle mainnet - %s\n",genesis.hashMerkleRoot.ToString().c_str());


        assert(hashGenesisBlock == uint256("0x0000f00ba769187169bc7e9b4fd82c73ce31d355c35915a5d999ef55d3c903fb"));
        assert(genesis.hashMerkleRoot == uint256("0x318407c1baff76f6dcaa61de5525f99d8920b6b3dca2b290c68b2f1cb6726cc1"));

        vSeeds.push_back(CDNSSeedData("seed1.masterstake.net", "seed1.masterstake.net"));
        vSeeds.push_back(CDNSSeedData("seed2.masterstake.net", "seed2.masterstake.net"));
        vSeeds.push_back(CDNSSeedData("seed3.masterstake.net", "seed3.masterstake.net"));
        vSeeds.push_back(CDNSSeedData("seed4.masterstake.net", "seed4.masterstake.net"));
        vSeeds.push_back(CDNSSeedData("seed5.masterstake.net", "seed5.masterstake.net"));
        vSeeds.push_back(CDNSSeedData("seed6.masterstake.net", "seed6.masterstake.net"));
        vSeeds.push_back(CDNSSeedData("186.202.57.186", "186.202.57.186"));
        vSeeds.push_back(CDNSSeedData("191.252.109.144", "191.252.109.144"));
        vSeeds.push_back(CDNSSeedData("191.252.204.129", "191.252.204.129"));
        vSeeds.push_back(CDNSSeedData("191.252.120.128", "191.252.120.128"));
    

        // MasterStake addresses start with 'M'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 50); // M
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 63); // S 
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 110); // m 
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0xB2)(0x47)(0x46).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0xB2)(0x43)(0x08).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_COIN_TYPE] = boost::assign::list_of(0x80)(0x00)(0x02)(0x62).convert_to_container<std::vector<unsigned char> >();

        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = false;
        fAllowMinDifficultyBlocks = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fSkipProofOfWorkCheck = true;
        fTestnetToBeDeprecatedFieldRPC = false;
        fHeadersFirstSyncingActive = false;

        nPoolMaxTransactions = 3;
        strSporkKey = "044f3337e4384b2a819eb5be3745710b3791812ac68b2a73e0e40671605bf4bb6ba9033c32b2a8ffd1e543661ece322697e6b9f27d0027bf453a906f302f32c01b";
        strObfuscationPoolDummyAddress = "MWoaaxi3zmQuB6TLMvzRhQLqHdDhdFZfPc";
        nStartMasternodePayments = 1547119800; // historical param

        /** Zerocoin */
        zerocoinModulus = "25195908475657893494027183240048398571429282126204032027777137836043662020707595556264018525880784"
            "4069182906412495150821892985591491761845028084891200728449926873928072877767359714183472702618963750149718246911"
            "6507761337985909570009733045974880842840179742910064245869181719511874612151517265463228221686998754918242243363"
            "7259085141865462043576798423387184774447920739934236584823824281198163815010674810451660377306056201619676256133"
            "8441436038339044149526344321901146575444541784240209246165157233507787077498171257724679629263863563732899121548"
            "31438167899885040445364023527381951378636564391212010397122822120720357";
        nMaxZerocoinSpendsPerTransaction = 7; // Assume about 20kb each
        nMinZerocoinMintFee = 1 * CENT; //high fee required for zerocoin mints
        nMintRequiredConfirmations = 20; //the maximum amount of confirmations until accumulated in 19
        nRequiredAccumulation = 1;
        nDefaultSecurityLevel = 100; //full security level for accumulators
        nZerocoinHeaderVersion = 4; //Block headers must be this version once zerocoin is active
        nZerocoinRequiredStakeDepth = 200; //The required confirmations for a zmaster to be stakable

        nBudget_Fee_Confirmations = 6; // Number of confirmations for the finalization fee
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return data;
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
*/
class CTestNetParams : public CMainParams
{
public:
    CTestNetParams()
    {
        networkID = CBaseChainParams::TESTNET;
        strNetworkID = "test";
        pchMessageStart[0] = 0xa1;
        pchMessageStart[1] = 0xDc;
        pchMessageStart[2] = 0x1f;
        pchMessageStart[3] = 0x48;
        vAlertPubKey = ParseHex("04330b7eec077a1d56b2bcaac8f1a6cf340ed806955d7eb56f2bba26454bd6d30857378b272f37d976fe803bba8e3472c6129b98b026871e0d3755cfeb472c2da0");
        nDefaultPort = 27293;
        nMinerThreads = 0;
        nTargetSpacing = 3 * 60;
        nLastPOWBlock = 500;
        nMaturity = 24;
        nModifierUpdateBlock = 0;
        nMaxMoneyOut = 10000000 * COIN;
        nZerocoinStartHeight = 15;
        nZerocoinStartTime = 1547096400;
        nBlockZerocoinV2 = 15;

        nSubsidyHalvingBlock = 1000;
        nMasternodeCollateral = 15000;

        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1656967050;
        genesis.nNonce = 68866;

        hashGenesisBlock = genesis.GetHash();


        assert(hashGenesisBlock == uint256("0x0000f00ba769187169bc7e9b4fd82c73ce31d355c35915a5d999ef55d3c903fb"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 85); // Testnet masterstake addresses start with 'b'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 86);  // Testnet masterstake script addresses start with 'b' or 'c'
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 86);     // Testnet private keys start with 'b' or 'c'
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_COIN_TYPE] = boost::assign::list_of(0x80)(0x00)(0x02)(0x6e).convert_to_container<std::vector<unsigned char> >();

        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 2;
        strSporkKey = "04e4813f1134ccd0f8f075e64296b6b11aaf9273a7248d04cc461cd7dd42d7a2ca44de1b2d95d65cd434cbafc7cd3cb8414559d320dce7ef0e30d3dba609001bf1";
        strObfuscationPoolDummyAddress = "MWoaaxi3zmQuB6TLMvzRhQLqHdDhdFZfPc";
        nStartMasternodePayments = 1547119740 + 500 * 120;
        nBudget_Fee_Confirmations = 3;

    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataTestnet;
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CTestNetParams
{
public:
    CRegTestParams()
    {
        networkID = CBaseChainParams::REGTEST;
        strNetworkID = "regtest";
        strNetworkID = "regtest";
        pchMessageStart[0] = 0x12;
        pchMessageStart[1] = 0x5c;
        pchMessageStart[2] = 0xAa;
        pchMessageStart[3] = 0xd4;
        nMinerThreads = 1;
        nTargetSpacing = 3 * 60;
        nDefaultPort = 37293;

        bnProofOfWorkLimit = ~uint256(0) >> 1;
        genesis.nTime = 1656967050;
        genesis.nBits = 0x1e0ffff0;
        genesis.nNonce = 68866;

        hashGenesisBlock = genesis.GetHash();


        //printf("Genesis RegTest - %s\n",hashGenesisBlock.ToString().c_str());
        //printf("Merkle RegTest - %s\n",genesis.hashMerkleRoot.ToString().c_str());

        assert(hashGenesisBlock == uint256("0x0000f00ba769187169bc7e9b4fd82c73ce31d355c35915a5d999ef55d3c903fb"));

        vFixedSeeds.clear(); //! Testnet mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Testnet mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataRegtest;
    }
};
static CRegTestParams regTestParams;

/**
 * Unit test
 */
class CUnitTestParams : public CMainParams, public CModifiableParams
{
public:
    CUnitTestParams()
    {
        networkID = CBaseChainParams::UNITTEST;
        strNetworkID = "unittest";
        nDefaultPort = 49293;
        vFixedSeeds.clear(); //! Unit test mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Unit test mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fAllowMinDifficultyBlocks = false;
        fMineBlocksOnDemand = true;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        // UnitTest share the same checkpoints as MAIN
        return data;
    }

    //! Published setters to allow changing values in unit test cases
    virtual void setSubsidyHalvingBlock(int anSubsidyHalvingBlock) { nSubsidyHalvingBlock = anSubsidyHalvingBlock; }
    virtual void setDefaultConsistencyChecks(bool afDefaultConsistencyChecks) { fDefaultConsistencyChecks = afDefaultConsistencyChecks; }
    virtual void setAllowMinDifficultyBlocks(bool afAllowMinDifficultyBlocks) { fAllowMinDifficultyBlocks = afAllowMinDifficultyBlocks; }
    virtual void setSkipProofOfWorkCheck(bool afSkipProofOfWorkCheck) { fSkipProofOfWorkCheck = afSkipProofOfWorkCheck; }
};
static CUnitTestParams unitTestParams;


static CChainParams* pCurrentParams = 0;

CModifiableParams* ModifiableParams()
{
    assert(pCurrentParams);
    assert(pCurrentParams == &unitTestParams);
    return (CModifiableParams*)&unitTestParams;
}

const CChainParams& Params()
{
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(CBaseChainParams::Network network)
{
    switch (network) {
    case CBaseChainParams::MAIN:
        return mainParams;
    case CBaseChainParams::TESTNET:
        return testNetParams;
    case CBaseChainParams::REGTEST:
        return regTestParams;
    case CBaseChainParams::UNITTEST:
        return unitTestParams;
    default:
        assert(false && "Unimplemented network");
        return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);
    return true;
}
