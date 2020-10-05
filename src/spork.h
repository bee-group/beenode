// Copyright (c) 2020 The BeeGroup developers are EternityGroup
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SPORK_H
#define SPORK_H

#include "hash.h"
#include "net.h"
#include "utilstrencodings.h"
#include "key.h"
#include "primitives/transaction.h"
#include <unordered_map>
#include <unordered_set>
class CSporkMessage;
class CSporkManager;
class CEvolutionManager;


/*
    Don't ever reuse these IDs for other sporks
    - This would result in old clients getting confused about which spork is for what
*/
static const int SPORK_2_INSTANTSEND_ENABLED                            = 10001;
static const int SPORK_3_INSTANTSEND_BLOCK_FILTERING                    = 10002;
static const int SPORK_5_INSTANTSEND_MAX_VALUE                          = 10004;
static const int SPORK_12_RECONSIDER_BLOCKS                             = 10011;
static const int SPORK_15_DETERMINISTIC_MNS_ENABLED                     = 10014;
static const int SPORK_16_INSTANTSEND_AUTOLOCKS                         = 10015;
static const int SPORK_17_QUORUM_DKG_ENABLED                            = 10016;
static const int SPORK_19_EVOLUTION_PAYMENTS_ENFORCEMENT				= 10018;
static const int SPORK_19_CHAINLOCKS_ENABLED                            = 10021;
static const int SPORK_20_INSTANTSEND_LLMQ_BASED                        = 10022;
static const int SPORK_10_CHECK_PROTX	  		        	            = 10025;

static const int SPORK_START                                            = SPORK_2_INSTANTSEND_ENABLED;
static const int SPORK_END                                              = SPORK_10_CHECK_PROTX;

extern std::map<int, int64_t> mapSporkDefaults;
extern CSporkManager sporkManager;
extern CEvolutionManager evolutionManager;

/**
 * Sporks are network parameters used primarily to prevent forking and turn
 * on/off certain features. They are a soft consensus mechanism.
 *
 * We use 2 main classes to manage the spork system.
 *
 * SporkMessages - low-level constructs which contain the sporkID, value,
 *                 signature and a signature timestamp
 * SporkManager  - a higher-level construct which manages the naming, use of
 *                 sporks, signatures and verification, and which sporks are active according
 *                 to this node
 */

/**
 * CSporkMessage is a low-level class used to encapsulate Spork messages and
 * serialize them for transmission to other peers. This includes the internal
 * spork ID, value, spork signature and timestamp for the signature.
 */
class CSporkMessage
{
private:
    std::vector<unsigned char> vchSig;

public:
    int nSporkID;
    int64_t nValue;
    int64_t nTimeSigned;
	std::string	sWEvolution;	

    CSporkMessage(int nSporkID, int64_t nValue, std::string sEvolution, int64_t nTimeSigned) :
        nSporkID(nSporkID),
        nValue(nValue),
        nTimeSigned(nTimeSigned),
		sWEvolution( sEvolution )
        {}

    CSporkMessage() :
        nSporkID(0),
        nValue(0),
        nTimeSigned(0),
		sWEvolution("")
        {}


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nSporkID);
        READWRITE(nValue);
        READWRITE(nTimeSigned);
		READWRITE(sWEvolution);
		READWRITE(vchSig);
    }

    /**
     * GetHash returns the double-sha256 hash of the serialized spork message.
     */
    uint256 GetHash() const;

    /**
     * GetSignatureHash returns the hash of the serialized spork message
     * without the signature included. The intent of this method is to get the
     * hash to be signed.
     */
    uint256 GetSignatureHash() const;

    /**
     * Sign will sign the spork message with the given key.
     */
    bool Sign(const CKey& key);

    /**
     * CheckSignature will ensure the spork signature matches the provided public
     * key hash.
     */
    bool CheckSignature(const CKeyID& pubKeyId) const;

    /**
     * GetSignerKeyID is used to recover the spork address of the key used to
     * sign this spork message.
     *
     * This method was introduced along with the multi-signer sporks feature,
     * in order to identify which spork key signed this message.
     */
    bool GetSignerKeyID(CKeyID& retKeyidSporkSigner);

    /**
     * Relay is used to send this spork message to other peers.
     */
    void Relay(CConnman& connman);
};

/**
 * CSporkManager is a higher-level class which manages the node's spork
 * messages, rules for which sporks should be considered active/inactive, and
 * processing for certain sporks (e.g. spork 12).
 */
class CSporkManager
{
private:
    static const std::string SERIALIZATION_VERSION_STRING;

    mutable CCriticalSection cs;
    std::unordered_map<uint256, CSporkMessage> mapSporksByHash;
    std::map<int, uint256> mapSporkHashesByID;
    std::unordered_map<int, std::map<CKeyID, CSporkMessage> > mapSporksActive;

    std::set<CKeyID> setSporkPubKeyIDs;
    int nMinSporkKeys;
    CKey sporkPrivKey;

    /**
     * SporkValueIsActive is used to get the value agreed upon by the majority
     * of signed spork messages for a given Spork ID.
     */
    bool SporkValueIsActive(int nSporkID, int64_t& nActiveValueRet) const;

public:

    CSporkManager() {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        std::string strVersion;
        if(ser_action.ForRead()) {
            READWRITE(strVersion);
            if (strVersion != SERIALIZATION_VERSION_STRING) {
                return;
            }
        } else {
            strVersion = SERIALIZATION_VERSION_STRING;
            READWRITE(strVersion);
        }
        // we don't serialize pubkey ids because pubkeys should be
        // hardcoded or be setted with cmdline or options, should
        // not reuse pubkeys from previous beenoded run
        READWRITE(mapSporksByHash);
        READWRITE(mapSporksActive);
        // we don't serialize private key to prevent its leakage
    }

    /**
     * Clear is used to clear all in-memory active spork messages. Since spork
     * public and private keys are set in init.cpp, we do not clear them here.
     *
     * This method was introduced along with the spork cache.
     */
    void Clear();

    /**
     * CheckAndRemove is defined to fulfill an interface as part of the on-disk
     * cache used to cache sporks between runs. If sporks that are restored
     * from cache do not have valid signatures when compared against the
     * current spork private keys, they are removed from in-memory storage.
     *
     * This method was introduced along with the spork cache.
     */
    void CheckAndRemove();

    /**
     * ProcessSpork is used to handle the 'getsporks' and 'spork' p2p messages.
     *
     * For 'getsporks', it sends active sporks to the requesting peer. For 'spork',
     * it validates the spork and adds it to the internal spork storage and
     * performs any necessary processing.
     */
    void ProcessSpork(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);

    /**
     * ExecuteSpork is used to perform certain actions based on the spork value.
     *
     * Currently only used with Spork 12.
     */
    void ExecuteSpork(int nSporkID, int nValue);
   	 /**
     * UpdateSpork is used by the spork RPC command to set a new spork value, sign
     * and broadcast the spork message.
     */
    bool UpdateSpork(int nSporkID, int64_t nValue,std::string sEvol,  CConnman& connman);
	

    /**
     * IsSporkActive returns a bool for time-based sporks, and should be used
     * to determine whether the spork can be considered active or not.
     *
     * For value-based sporks such as SPORK_5_INSTANTSEND_MAX_VALUE, the spork
     * value should not be considered a timestamp, but an integer value
     * instead, and therefore this method doesn't make sense and should not be
     * used.
     */
    bool IsSporkActive(int nSporkID);
	int64_t getActiveSporkValue( int nSporkID,CSporkMessage& spork  );

    /**
     * GetSporkValue returns the spork value given a Spork ID. If no active spork
     * message has yet been received by the node, it returns the default value.
     */
	bool IsSporkWorkActive(int nSporkID);
    int64_t GetSporkValue(int nSporkID);

    /**
     * GetSporkIDByName returns the internal Spork ID given the spork name.
     */
    int GetSporkIDByName(const std::string& strName);

    /**
     * GetSporkNameByID returns the spork name as a string, given a Spork ID.
     */
    std::string GetSporkNameByID(int nSporkID);

    /**
     * GetSporkByHash returns a spork message given a hash of the spork message.
     *
     * This is used when a requesting peer sends a MSG_SPORK inventory message with
     * the hash, to quickly lookup and return the full spork message. We maintain a
     * hash-based index of sporks for this reason, and this function is the access
     * point into that index.
     */
    bool GetSporkByHash(const uint256& hash, CSporkMessage &sporkRet);

    /**
     * SetSporkAddress is used to set a public key ID which will be used to
     * verify spork signatures.
     *
     * This can be called multiple times to add multiple keys to the set of
     * valid spork signers.
     */
    bool SetSporkAddress(const std::string& strAddress);

    /**
     * SetMinSporkKeys is used to set the required spork signer threshold, for
     * a spork to be considered active.
     *
     * This value must be at least a majority of the total number of spork
     * keys, and for obvious resons cannot be larger than that number.
     */
    bool SetMinSporkKeys(int minSporkKeys);

    /**
     * SetPrivKey is used to set a spork key to enable setting / signing of
     * spork values.
     *
     * This will return false if the private key does not match any spork
     * address in the set of valid spork signers (see SetSporkAddress).
     */
    bool SetPrivKey(const std::string& strPrivKey);

    /**
     * ToString returns the string representation of the SporkManager.
     */
    std::string ToString() const;
};

class CEvolutionManager
{
private:
	std::map<int, std::string> mapEvolution=
	{
        std::pair<int, std::string>( 0 ,"BRnxLmNeSMocPC6Gvs5yaT4T7wc52pokQA"),
        std::pair<int, std::string>( 1 ,"BBaypjqNTtbUKShtJTYw3c9V2nc39YFfhp"),
        std::pair<int, std::string>( 2 ,"BPLioKUHog1EMLeBLNnCWvh3FFVXw4KiZe"),
        std::pair<int, std::string>( 3 ,"BPZnB9QmekKfBGporAYHwmF2i8YDJcKscF"),
        std::pair<int, std::string>( 4 ,"BT2qqDX5JtbbZW9G37bgsDmSPipTicrs7K"),
        std::pair<int, std::string>( 5 ,"BR8MoXNvx7mVANynd4F1Uzfdaqv4gE3VwW"),
        std::pair<int, std::string>( 6 ,"B8wtSjRdgma5biQvCUBD5rLJEfPQig2gkx"),
        std::pair<int, std::string>( 7 ,"BJvgwiqzTQsg7aZac4o8mssAzCsTW46UvE"),
        std::pair<int, std::string>( 8 ,"BMmY8cuCD6Hds5CkaTrxzBC3aRasKHYmUf"),
        std::pair<int, std::string>( 9 ,"BACy6bZooe5DZby5vfvGNtB6abDXgJ3G49"),
        std::pair<int, std::string>( 10 ,"BKUJ1tKimUFPvATQQz4x5hB2K31KGSrFVT"),
        std::pair<int, std::string>( 11 ,"BAc8KYJjVkMXtAHohoTTBNTTyUssVmcJZ3"),
        std::pair<int, std::string>( 12 ,"BNmYswfQ4LtQtmLujE39htad2fKombcS4D"),
        std::pair<int, std::string>( 13 ,"BBdKttwkSTPYpC3CcBdsnMpuW5ezgcWnbQ"),
        std::pair<int, std::string>( 14 ,"BTkgvytnKaNKb39DkXyvU8x3aqZr3UopdY"),
        std::pair<int, std::string>( 15 ,"BEg8jUN7b6KrzKCDRrk5gTT4hBLHrQUiDo"),
        std::pair<int, std::string>( 16 ,"BEb8LToo34dJqCqizXkHEcq4BZHTFw29mE"),
        std::pair<int, std::string>( 17 ,"BKQ2vHdLMLBu72tLL6s86QaZqXLAEcHE36"),
        std::pair<int, std::string>( 18 ,"BEjGAUgqU36GDRrgVVsZsb7aqtnHkkbiSx"),
        std::pair<int, std::string>( 19 ,"BPEdpWEKVV1FMrM3ySpaSn7v3vKCfoEbZL"),
        std::pair<int, std::string>( 20 ,"BQwByqSANuW8Pdmxz8uu65cU2azj1o1fmm"),
        std::pair<int, std::string>( 21 ,"BBVjG6kSHwB1PyFYJrHpG4QkXDX4AuhFzp"),
        std::pair<int, std::string>( 22 ,"BLLQ7J2Txt6BbVXeJbA1rscWKvcLSqYo3g"),
        std::pair<int, std::string>( 23 ,"B5ZjX7jkHAFpW6iMbCUXjYBdj6nSUAdd8a"),
        std::pair<int, std::string>( 24 ,"BNVCJ8Yg1EwKqhE5UJ2i8Z1cc5qLKZj2mz"),
        std::pair<int, std::string>( 25 ,"BNMkU3WY6ZqzqZxzVjud59xiCjdaAEmMBu"),
        std::pair<int, std::string>( 26 ,"B5HgZZX5jPQX1y5MA9rBq7VmW7RdqkW22h"),
        std::pair<int, std::string>( 27 ,"BTJai8DcSpdHTp9WC8riKdPMyLgcGnsLsR"),
        std::pair<int, std::string>( 28 ,"B7nMoprmuZMBEcA22b1h6xGwjrRLbdQh6Y"),
        std::pair<int, std::string>( 29 ,"B87yZp5ApKQweYTR9Pq9qghKYAQGQAMtrG"),
        
    };
	std::map<int, std::string> mapDisableNodes;

public:

	CEvolutionManager(){}
	
	void setNewEvolutions( const std::string &sEvol );
	void setDisableNodes( const std::string &sEvol );
	std::string getEvolution( int nBlockHeight );
    std::string getDisableNode( int nBlockHeight );
	bool IsTransactionValid( const CTransactionRef& txNew, int nBlockHeight, CAmount blockCurEvolution );
	bool checkEvolutionString( const std::string &sEvol );
};
#endif
