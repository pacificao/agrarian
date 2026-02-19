// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2019 The PIVX developers
// Copyright (c) 2026 Agrarian Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef AGRARIAN_MASTERNODE_PAYMENTS_H
#define AGRARIAN_MASTERNODE_PAYMENTS_H

#include "key.h"
#include "main.h"
#include "masternode.h"

#include <map>
#include <string>
#include <vector>

extern CCriticalSection cs_vecPayments;
extern CCriticalSection cs_mapMasternodeBlocks;
extern CCriticalSection cs_mapMasternodePayeeVotes;

class CMasternodePayments;
class CMasternodePaymentWinner;
class CMasternodeBlockPayees;

extern CMasternodePayments masternodePayments;

#define MNPAYMENTS_SIGNATURES_REQUIRED 6
#define MNPAYMENTS_SIGNATURES_TOTAL 10

void ProcessMessageMasternodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
bool IsBlockPayeeValid(const CBlock& block, int nBlockHeight);
std::string GetRequiredPaymentsString(int nBlockHeight);
bool IsBlockValueValid(const CBlock& block, CAmount nExpectedValue, CAmount nMinted);
void FillBlockPayee(CMutableTransaction& txNew, CAmount nFees, bool fProofOfStake, bool fZAGRStake);

void DumpMasternodePayments();

/** Save Masternode Payment Data (mnpayments.dat) */
class CMasternodePaymentDB
{
private:
    boost::filesystem::path pathDB;
    std::string strMagicMessage;

public:
    enum ReadResult {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    CMasternodePaymentDB();
    bool Write(const CMasternodePayments& objToSave);
    ReadResult Read(CMasternodePayments& objToLoad, bool fDryRun = false);
};

class CMasternodePayee
{
public:
    CScript scriptPubKey;
    int nVotes;

    CMasternodePayee() : scriptPubKey(CScript()), nVotes(0) {}
    CMasternodePayee(const CScript& payee, int nVotesIn) : scriptPubKey(payee), nVotes(nVotesIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(scriptPubKey);
        READWRITE(nVotes);
    }
};

// Keep track of votes for payees from masternodes
class CMasternodeBlockPayees
{
public:
    int nBlockHeight;
    std::vector<CMasternodePayee> vecPayments;

    CMasternodeBlockPayees() : nBlockHeight(0), vecPayments() {}
    explicit CMasternodeBlockPayees(int nBlockHeightIn) : nBlockHeight(nBlockHeightIn), vecPayments() {}

    void AddPayee(const CScript& payeeIn, int nIncrement)
    {
        LOCK(cs_vecPayments);

        for (CMasternodePayee& payee : vecPayments) {
            if (payee.scriptPubKey == payeeIn) {
                payee.nVotes += nIncrement;
                return;
            }
        }

        vecPayments.emplace_back(payeeIn, nIncrement);
    }

    bool GetPayee(CScript& payee)
    {
        LOCK(cs_vecPayments);

        int nVotes = -1;
        for (CMasternodePayee& p : vecPayments) {
            if (p.nVotes > nVotes) {
                payee = p.scriptPubKey;
                nVotes = p.nVotes;
            }
        }

        return (nVotes > -1);
    }

    bool HasPayeeWithVotes(const CScript& payee, int nVotesReq)
    {
        LOCK(cs_vecPayments);

        for (CMasternodePayee& p : vecPayments) {
            if (p.nVotes >= nVotesReq && p.scriptPubKey == payee) {
                return true;
            }
        }

        return false;
    }

    bool IsTransactionValid(const CTransaction& txNew);
    std::string GetRequiredPaymentsString();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(nBlockHeight);
        READWRITE(vecPayments);
    }
};

// For storing the winning payments
class CMasternodePaymentWinner
{
public:
    CTxIn vinMasternode;
    int nBlockHeight;
    CScript payee;
    std::vector<unsigned char> vchSig;

    CMasternodePaymentWinner() : vinMasternode(CTxIn()), nBlockHeight(0), payee(CScript()), vchSig() {}
    explicit CMasternodePaymentWinner(const CTxIn& vinIn) : vinMasternode(vinIn), nBlockHeight(0), payee(CScript()), vchSig() {}

    uint256 GetHash()
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << payee;
        ss << nBlockHeight;
        ss << vinMasternode.prevout;
        return ss.GetHash();
    }

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool IsValid(CNode* pnode, std::string& strError);
    bool SignatureValid();
    void Relay();

    void AddPayee(const CScript& payeeIn) { payee = payeeIn; }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vinMasternode);
        READWRITE(nBlockHeight);
        READWRITE(payee);
        READWRITE(vchSig);
    }

    std::string ToString()
    {
        std::string ret;
        ret += vinMasternode.ToString();
        ret += ", " + std::to_string(nBlockHeight);
        ret += ", " + payee.ToString();
        ret += ", " + std::to_string((int)vchSig.size());
        return ret;
    }
};

//
// Masternode Payments Class
// Keeps track of who should get paid for which blocks
//
class CMasternodePayments
{
private:
    int nSyncedFromPeer;
    int nLastBlockHeight;

public:
    std::map<uint256, CMasternodePaymentWinner> mapMasternodePayeeVotes;
    std::map<int, CMasternodeBlockPayees> mapMasternodeBlocks;

    // NOTE: This legacy keying scheme (outpoint.hash + outpoint.n) can theoretically collide.
    // Changing it requires a coordinated refactor of both header and implementation.
    std::map<uint256, int> mapMasternodesLastVote; // prevout.hash + prevout.n, nBlockHeight

    CMasternodePayments() : nSyncedFromPeer(0), nLastBlockHeight(0) {}

    void Clear()
    {
        LOCK2(cs_mapMasternodeBlocks, cs_mapMasternodePayeeVotes);
        mapMasternodeBlocks.clear();
        mapMasternodePayeeVotes.clear();
    }

    bool AddWinningMasternode(CMasternodePaymentWinner& winner);
    bool ProcessBlock(int nBlockHeight);

    void Sync(CNode* node, int nCountNeeded);
    void CleanPaymentList();
    int LastPayment(CMasternode& mn);

    bool GetBlockPayee(int nBlockHeight, CScript& payee);
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);
    bool IsScheduled(CMasternode& mn, int nNotBlockHeight);

    bool CanVote(const COutPoint& outMasternode, int nBlockHeight)
    {
        LOCK(cs_mapMasternodePayeeVotes);

        const uint256 key = outMasternode.hash + outMasternode.n;
        if (mapMasternodesLastVote.count(key)) {
            if (mapMasternodesLastVote[key] == nBlockHeight) {
                return false;
            }
        }

        // record this masternode voted
        mapMasternodesLastVote[key] = nBlockHeight;
        return true;
    }

    int GetMinMasternodePaymentsProto();
    void ProcessMessageMasternodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    std::string GetRequiredPaymentsString(int nBlockHeight);
    void FillBlockPayee(CMutableTransaction& txNew, int64_t nFees, bool fProofOfStake, bool fZAGRStake);
    std::string ToString() const;
    int GetOldestBlock();
    int GetNewestBlock();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(mapMasternodePayeeVotes);
        READWRITE(mapMasternodeBlocks);
    }
};

#endif // AGRARIAN_MASTERNODE_PAYMENTS_H
