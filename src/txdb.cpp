// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2016-2019 The PIVX developers
// Copyright (c) 2026 Agrarian Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"

#include "main.h"
#include "pow.h"
#include "uint256.h"
#include "zagr/accumulators.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/thread.hpp>

namespace {

void BatchWriteCoins(CLevelDBBatch& batch, const uint256& hash, const CCoins& coins)
{
    if (coins.IsPruned()) {
        batch.Erase(std::make_pair('c', hash));
    } else {
        batch.Write(std::make_pair('c', hash), coins);
    }
}

void BatchWriteHashBestChain(CLevelDBBatch& batch, const uint256& hash)
{
    batch.Write('B', hash);
}

} // namespace

CCoinsViewDB::CCoinsViewDB(size_t nCacheSize, bool fMemory, bool fWipe)
    : db(GetDataDir() / "chainstate", nCacheSize, fMemory, fWipe)
{
}

bool CCoinsViewDB::GetCoins(const uint256& txid, CCoins& coins) const
{
    return db.Read(std::make_pair('c', txid), coins);
}

bool CCoinsViewDB::HaveCoins(const uint256& txid) const
{
    return db.Exists(std::make_pair('c', txid));
}

uint256 CCoinsViewDB::GetBestBlock() const
{
    uint256 hashBestChain;
    if (!db.Read('B', hashBestChain)) {
        return uint256(0);
    }
    return hashBestChain;
}

bool CCoinsViewDB::BatchWrite(CCoinsMap& mapCoins, const uint256& hashBlock)
{
    CLevelDBBatch batch;
    size_t count = 0;
    size_t changed = 0;

    for (auto it = mapCoins.begin(); it != mapCoins.end();) {
        if (it->second.flags & CCoinsCacheEntry::DIRTY) {
            BatchWriteCoins(batch, it->first, it->second.coins);
            ++changed;
        }
        ++count;

        auto itOld = it++;
        mapCoins.erase(itOld);
    }

    if (hashBlock != uint256(0)) {
        BatchWriteHashBestChain(batch, hashBlock);
    }

    LogPrint("coindb",
             "Committing %u changed transactions (out of %u) to coin database...\n",
             (unsigned int)changed, (unsigned int)count);

    return db.WriteBatch(batch);
}

CBlockTreeDB::CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe)
    : CLevelDBWrapper(GetDataDir() / "blocks" / "index", nCacheSize, fMemory, fWipe)
{
}

bool CBlockTreeDB::WriteBlockIndex(const CDiskBlockIndex& blockindex)
{
    return Write(std::make_pair('b', blockindex.GetBlockHash()), blockindex);
}

bool CBlockTreeDB::WriteBlockFileInfo(int nFile, const CBlockFileInfo& info)
{
    return Write(std::make_pair('f', nFile), info);
}

bool CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo& info)
{
    return Read(std::make_pair('f', nFile), info);
}

bool CBlockTreeDB::WriteLastBlockFile(int nFile)
{
    return Write('l', nFile);
}

bool CBlockTreeDB::WriteReindexing(bool fReindexing)
{
    if (fReindexing) {
        return Write('R', '1');
    }
    return Erase('R');
}

bool CBlockTreeDB::ReadReindexing(bool& fReindexing)
{
    fReindexing = Exists('R');
    return true;
}

bool CBlockTreeDB::ReadLastBlockFile(int& nFile)
{
    return Read('l', nFile);
}

bool CCoinsViewDB::GetStats(CCoinsStats& stats) const
{
    // LevelDB iterators are non-const; we only read so const_cast the wrapper.
    std::unique_ptr<leveldb::Iterator> pcursor(const_cast<CLevelDBWrapper*>(&db)->NewIterator());
    pcursor->SeekToFirst();

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    stats.hashBlock = GetBestBlock();
    ss << stats.hashBlock;

    CAmount nTotalAmount = 0;

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            const leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);

            char chType;
            ssKey >> chType;

            if (chType == 'c') {
                const leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);

                CCoins coins;
                ssValue >> coins;

                uint256 txhash;
                ssKey >> txhash;

                ss << txhash;
                ss << VARINT(coins.nVersion);
                ss << (coins.fCoinBase ? 'c' : 'n');
                ss << VARINT(coins.nHeight);

                ++stats.nTransactions;

                for (unsigned int i = 0; i < coins.vout.size(); ++i) {
                    const CTxOut& out = coins.vout[i];
                    if (!out.IsNull()) {
                        ++stats.nTransactionOutputs;
                        ss << VARINT(i + 1);
                        ss << out;
                        nTotalAmount += out.nValue;
                    }
                }

                stats.nSerializedSize += 32 + slValue.size();
                ss << VARINT(0);
            }

            pcursor->Next();
        } catch (const std::exception& e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }

    // Height: best-effort. If best block isn't in index (e.g. empty db), return height=0.
    stats.nHeight = 0;
    const uint256 best = GetBestBlock();
    auto it = mapBlockIndex.find(best);
    if (it != mapBlockIndex.end() && it->second) {
        stats.nHeight = it->second->nHeight;
    }

    stats.hashSerialized = ss.GetHash();
    stats.nTotalAmount = nTotalAmount;
    return true;
}

bool CBlockTreeDB::ReadTxIndex(const uint256& txid, CDiskTxPos& pos)
{
    return Read(std::make_pair('t', txid), pos);
}

bool CBlockTreeDB::WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos>>& vect)
{
    CLevelDBBatch batch;
    for (const auto& it : vect) {
        batch.Write(std::make_pair('t', it.first), it.second);
    }
    return WriteBatch(batch);
}

bool CBlockTreeDB::WriteFlag(const std::string& name, bool fValue)
{
    return Write(std::make_pair('F', name), fValue ? '1' : '0');
}

bool CBlockTreeDB::ReadFlag(const std::string& name, bool& fValue)
{
    char ch;
    if (!Read(std::make_pair('F', name), ch)) {
        return false;
    }
    fValue = (ch == '1');
    return true;
}

bool CBlockTreeDB::WriteInt(const std::string& name, int nValue)
{
    return Write(std::make_pair('I', name), nValue);
}

bool CBlockTreeDB::ReadInt(const std::string& name, int& nValue)
{
    return Read(std::make_pair('I', name), nValue);
}

bool CBlockTreeDB::LoadBlockIndexGuts()
{
    std::unique_ptr<leveldb::Iterator> pcursor(NewIterator());

    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << std::make_pair('b', uint256(0));
    pcursor->Seek(ssKeySet.str());

    uint256 nPreviousCheckpoint;

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            const leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);

            char chType;
            ssKey >> chType;

            if (chType != 'b') {
                break; // finished loading block index
            }

            const leveldb::Slice slValue = pcursor->value();
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);

            CDiskBlockIndex diskindex;
            ssValue >> diskindex;

            // Construct block index object
            CBlockIndex* pindexNew = InsertBlockIndex(diskindex.GetBlockHash());
            pindexNew->pprev = InsertBlockIndex(diskindex.hashPrev);
            pindexNew->pnext = InsertBlockIndex(diskindex.hashNext);
            pindexNew->nHeight = diskindex.nHeight;
            pindexNew->nFile = diskindex.nFile;
            pindexNew->nDataPos = diskindex.nDataPos;
            pindexNew->nUndoPos = diskindex.nUndoPos;
            pindexNew->nVersion = diskindex.nVersion;
            pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
            pindexNew->nTime = diskindex.nTime;
            pindexNew->nBits = diskindex.nBits;
            pindexNew->nNonce = diskindex.nNonce;
            pindexNew->nStatus = diskindex.nStatus;
            pindexNew->nTx = diskindex.nTx;

            // Zerocoin
            pindexNew->nAccumulatorCheckpoint = diskindex.nAccumulatorCheckpoint;
            pindexNew->mapZerocoinSupply = diskindex.mapZerocoinSupply;
            pindexNew->vMintDenominationsInBlock = diskindex.vMintDenominationsInBlock;

            // Proof of Stake
            pindexNew->nMint = diskindex.nMint;
            pindexNew->nMoneySupply = diskindex.nMoneySupply;
            pindexNew->nFlags = diskindex.nFlags;
            pindexNew->nStakeModifier = diskindex.nStakeModifier;
            pindexNew->prevoutStake = diskindex.prevoutStake;
            pindexNew->nStakeTime = diskindex.nStakeTime;
            pindexNew->hashProofOfStake = diskindex.hashProofOfStake;

            // Hybrid PoW+PoS: only enforce PoW proof check for PoW blocks (not just height).
            // If your codebase doesn't define IsProofOfWork(), keep the height gate or adjust at call sites.
            if (pindexNew->IsProofOfWork()) {
                if (!CheckProofOfWork(pindexNew->GetBlockHash(), pindexNew->nBits)) {
                    return error("LoadBlockIndex() : CheckProofOfWork failed: %s", pindexNew->ToString());
                }
            }

            // Populate accumulator checksum map in memory
            if (pindexNew->nAccumulatorCheckpoint != 0 && pindexNew->nAccumulatorCheckpoint != nPreviousCheckpoint) {
                // Don't load any checkpoints that exist before v2 zagr. The accumulator is invalid for v1 and not used.
                if (pindexNew->nHeight >= Params().Zerocoin_Block_V2_Start()) {
                    LoadAccumulatorValuesFromDB(pindexNew->nAccumulatorCheckpoint);
                }
                nPreviousCheckpoint = pindexNew->nAccumulatorCheckpoint;
            }

            pcursor->Next();
        } catch (const std::exception& e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }

    return true;
}

CZerocoinDB::CZerocoinDB(size_t nCacheSize, bool fMemory, bool fWipe)
    : CLevelDBWrapper(GetDataDir() / "zerocoin", nCacheSize, fMemory, fWipe)
{
}

bool CZerocoinDB::WriteCoinMintBatch(const std::vector<std::pair<libzerocoin::PublicCoin, uint256>>& mintInfo)
{
    CLevelDBBatch batch;
    size_t count = 0;

    for (const auto& it : mintInfo) {
        const libzerocoin::PublicCoin& pubCoin = it.first;
        const uint256 hash = GetPubCoinHash(pubCoin.getValue());
        batch.Write(std::make_pair('m', hash), it.second);
        ++count;
    }

    LogPrint("zero", "Writing %u coin mints to db.\n", (unsigned int)count);
    return WriteBatch(batch, true);
}

bool CZerocoinDB::ReadCoinMint(const CBigNum& bnPubcoin, uint256& hashTx)
{
    return ReadCoinMint(GetPubCoinHash(bnPubcoin), hashTx);
}

bool CZerocoinDB::ReadCoinMint(const uint256& hashPubcoin, uint256& hashTx)
{
    return Read(std::make_pair('m', hashPubcoin), hashTx);
}

bool CZerocoinDB::EraseCoinMint(const CBigNum& bnPubcoin)
{
    const uint256 hash = GetPubCoinHash(bnPubcoin);
    return Erase(std::make_pair('m', hash));
}

bool CZerocoinDB::WriteCoinSpendBatch(const std::vector<std::pair<libzerocoin::CoinSpend, uint256>>& spendInfo)
{
    CLevelDBBatch batch;
    size_t count = 0;

    for (const auto& it : spendInfo) {
        const CBigNum bnSerial = it.first.getCoinSerialNumber();
        CDataStream ss(SER_GETHASH, 0);
        ss << bnSerial;
        const uint256 hash = Hash(ss.begin(), ss.end());
        batch.Write(std::make_pair('s', hash), it.second);
        ++count;
    }

    LogPrint("zero", "Writing %u coin spends to db.\n", (unsigned int)count);
    return WriteBatch(batch, true);
}

bool CZerocoinDB::ReadCoinSpend(const CBigNum& bnSerial, uint256& txHash)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << bnSerial;
    const uint256 hash = Hash(ss.begin(), ss.end());
    return Read(std::make_pair('s', hash), txHash);
}

bool CZerocoinDB::ReadCoinSpend(const uint256& hashSerial, uint256& txHash)
{
    return Read(std::make_pair('s', hashSerial), txHash);
}

bool CZerocoinDB::EraseCoinSpend(const CBigNum& bnSerial)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << bnSerial;
    const uint256 hash = Hash(ss.begin(), ss.end());
    return Erase(std::make_pair('s', hash));
}

bool CZerocoinDB::WipeCoins(const std::string& strType)
{
    if (strType != "spends" && strType != "mints") {
        return error("%s: did not recognize type %s", __func__, strType);
    }

    std::unique_ptr<leveldb::Iterator> pcursor(NewIterator());

    const char type = (strType == "spends" ? 's' : 'm');
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << std::make_pair(type, uint256(0));
    pcursor->Seek(ssKeySet.str());

    std::set<uint256> setDelete;

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            const leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);

            char chType;
            ssKey >> chType;

            if (chType != type) {
                break; // finished
            }

            const leveldb::Slice slValue = pcursor->value();
            CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);

            uint256 hash;
            ssValue >> hash;
            setDelete.insert(hash);

            pcursor->Next();
        } catch (const std::exception& e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }

    for (const auto& hash : setDelete) {
        if (!Erase(std::make_pair(type, hash))) {
            LogPrintf("%s: error failed to delete %s\n", __func__, hash.GetHex());
        }
    }

    return true;
}

bool CZerocoinDB::WriteAccumulatorValue(const uint32_t& nChecksum, const CBigNum& bnValue)
{
    LogPrint("zero", "%s : checksum:%d val:%s\n", __func__, nChecksum, bnValue.GetHex());
    return Write(std::make_pair('2', nChecksum), bnValue);
}

bool CZerocoinDB::ReadAccumulatorValue(const uint32_t& nChecksum, CBigNum& bnValue)
{
    return Read(std::make_pair('2', nChecksum), bnValue);
}

bool CZerocoinDB::EraseAccumulatorValue(const uint32_t& nChecksum)
{
    LogPrint("zero", "%s : checksum:%d\n", __func__, nChecksum);
    return Erase(std::make_pair('2', nChecksum));
}
