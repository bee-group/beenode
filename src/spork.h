// Copyright (c) 2019 The BeeGroup developers are EternityGroup
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SPORK_H
#define SPORK_H

#include "hash.h"
#include "net.h"
#include "utilstrencodings.h"
#include "key.h"
#include "primitives/transaction.h"
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
static const int SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT                 = 10007;
static const int SPORK_9_SUPERBLOCKS_ENABLED                            = 10008;
static const int SPORK_10_MASTERNODE_PAY_UPDATED_NODES                  = 10009;
static const int SPORK_12_RECONSIDER_BLOCKS                             = 10011;
static const int SPORK_14_REQUIRE_SENTINEL_FLAG                         = 10013;
static const int SPORK_18_EVOLUTION_PAYMENTS							= 10017;
static const int SPORK_19_EVOLUTION_PAYMENTS_ENFORCEMENT				= 10018;
static const int SPORK_20_EVOLUTION_DISABLE_NODE				        = 10019;
static const int SPORK_21_MASTERNODE_ORDER_ENABLE			        	= 10020;
static const int SPORK_22_MASTERNODE_UPDATE_PROTO			        	= 10021;

static const int SPORK_START                                            = SPORK_2_INSTANTSEND_ENABLED;
static const int SPORK_END                                              = SPORK_22_MASTERNODE_UPDATE_PROTO;

extern std::map<int, int64_t> mapSporkDefaults;
extern std::map<uint256, CSporkMessage> mapSporks;
extern CSporkManager sporkManager;
extern CEvolutionManager evolutionManager;

//
// Spork classes
// Keep track of all of the network spork settings
//

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
       /* if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(vchSig);
        }*/
    }

    uint256 GetHash() const;
    uint256 GetSignatureHash() const;

    bool Sign(const CKey& key);
    bool CheckSignature(const CKeyID& pubKeyId) const;
    void Relay(CConnman& connman);
};


class CSporkManager
{
private:
    std::vector<unsigned char> vchSig;
    std::map<int, CSporkMessage> mapSporksActive;

    CKeyID sporkPubKeyID;
    CKey sporkPrivKey;

public:

    CSporkManager() {}

    void ProcessSpork(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    void ExecuteSpork(int nSporkID, int nValue);
    bool UpdateSpork(int nSporkID, int64_t nValue,std::string sEvol,  CConnman& connman);
	
	void setActiveSpork( CSporkMessage &spork );
	int64_t getActiveSporkValue( int nSporkID );
	bool isActiveSporkInMap(int nSporkID);
	int64_t getActiveSporkTime(int nSporkID);
	
    bool IsSporkActive(int nSporkID);
	bool IsSporkWorkActive(int nSporkID);
    int64_t GetSporkValue(int nSporkID);
    int GetSporkIDByName(const std::string& strName);
    std::string GetSporkNameByID(int nSporkID);

    bool SetSporkAddress(const std::string& strAddress);
    bool SetPrivKey(const std::string& strPrivKey);
};
class CEvolutionManager
{
private:
	std::map<int, std::string> mapEvolution;
	std::map<int, std::string> mapDisableNodes;

public:

	CEvolutionManager() {}
	
	void setNewEvolutions( const std::string &sEvol );
	void setDisableNodes( const std::string &sEvol );
	std::string getEvolution( int nBlockHeight );
    std::string getDisableNode( int nBlockHeight );
	bool IsTransactionValid( const CTransactionRef& txNew, int nBlockHeight, CAmount blockCurEvolution );
	bool checkEvolutionString( const std::string &sEvol );
};
#endif
