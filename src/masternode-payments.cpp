// Copyright (c) 2019 The BeeGroup developers are EternityGroup
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "consensus/validation.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "netfulfilledman.h"
#include "netmessagemaker.h"
#include "spork.h"
#include "util.h"
#include "script/standard.h"
#include "base58.h"

#include <boost/lexical_cast.hpp>

/** Object for who's going to get paid on which blocks */
CMasternodePayments mnpayments;

CCriticalSection cs_vecPayees;
CCriticalSection cs_mapMasternodeBlocks;
CCriticalSection cs_mapMasternodePaymentVotes;

/**
* IsBlockValueValid
*
*   Determine if coinbase outgoing created money is the correct value
*
*   Why is this needed?
*   - In Beenode some blocks are superblocks, which output much higher amounts of coins
*   - Otherblocks are 10% lower in outgoing value, so in total, no extra coins are created
*   - When non-superblocks are detected, the normal schedule should be maintained
*/

bool IsBlockValueValid(const CBlock& block, int nBlockHeight, CAmount blockReward, std::string &strErrorRet)
{
    strErrorRet = "";

    bool isBlockRewardValueMet = (block.vtx[0]->GetValueOut() <= blockReward);
    if(fDebug) LogPrintf("block.vtx[0]->GetValueOut() %lld <= blockReward %lld\n", block.vtx[0]->GetValueOut(), blockReward);


    if(!masternodeSync.IsSynced()) {
        if(!isBlockRewardValueMet) {
            strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, only regular blocks are allowed at this height",
                                    nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
        }
        // it MUST be a regular block otherwise
        return isBlockRewardValueMet;
    }
	
    // we are synced, let's try to check as much data as we can
	
	// should NOT allow superblocks at all, when superblocks are disabled
	LogPrint("gobject", "IsBlockValueValid -- Superblocks are disabled, no superblocks allowed\n");
	if(!isBlockRewardValueMet) {
		strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, superblocks are disabled",
								nBlockHeight, block.vtx[0]->GetValueOut(), blockReward);
	}
	
    // it MUST be a regular block
    return isBlockRewardValueMet;
}

bool IsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight, CAmount blockReward)
{
    if(!masternodeSync.IsSynced()) {
        //there is no budget data to use to check anything, let's just accept the longest chain
        if(fDebug) LogPrintf("IsBlockPayeeValid -- WARNING: Client not synced, skipping block payee checks\n");
        return true;
    }
	
	// should NOT allow superblocks at all, when superblocks are disabled
	LogPrint("gobject", "IsBlockPayeeValid -- Superblocks are disabled, no superblocks allowed\n");
    
    // IF THIS ISN'T A SUPERBLOCK OR SUPERBLOCK IS INVALID, IT SHOULD PAY A MASTERNODE DIRECTLY
    if(mnpayments.IsTransactionValid(txNew, nBlockHeight)) {
        LogPrint("mnpayments", "IsBlockPayeeValid -- Valid masternode payment at height %d: %s", nBlockHeight, txNew.ToString());
        return true;
    }

    if(sporkManager.IsSporkActive(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)) {
        LogPrintf("IsBlockPayeeValid -- ERROR: Invalid masternode payment detected at height %d: %s", nBlockHeight, txNew.ToString());
        return false;
    }

    LogPrintf("IsBlockPayeeValid -- WARNING: Masternode payment enforcement is disabled, accepting any payee\n");
    return true;
}

void FillBlockPayments(CMutableTransaction& txNew, int nBlockHeight, CAmount blockReward, CAmount blockEvolution, CTxOut& txoutMasternodeRet, std::vector<CTxOut>& voutSuperblockRet)
{
    // only create superblocks if spork is enabled 
	if(  sporkManager.IsSporkWorkActive(SPORK_18_EVOLUTION_PAYMENTS) ){	
		CMasternodePayments::CreateEvolution(  txNew, nBlockHeight, blockEvolution, voutSuperblockRet  );
    }		
	
    // FILL BLOCK PAYEE WITH MASTERNODE PAYMENT OTHERWISE
    mnpayments.FillBlockPayee(txNew, nBlockHeight, blockReward, txoutMasternodeRet);
    LogPrint("mnpayments", "FillBlockPayments -- nBlockHeight %d blockReward %lld txoutMasternodeRet %s txNew %s",
                            nBlockHeight, blockReward, txoutMasternodeRet.ToString(), txNew.ToString());
}

std::string GetRequiredPaymentsString(int nBlockHeight)
{
    // OTHERWISE, PAY MASTERNODE
    return mnpayments.GetRequiredPaymentsString(nBlockHeight);
}

void CMasternodePayments::Clear()
{
    LOCK2(cs_mapMasternodeBlocks, cs_mapMasternodePaymentVotes);
    mapMasternodeBlocks.clear();
    mapMasternodePaymentVotes.clear();
}

bool CMasternodePayments::UpdateLastVote(const CMasternodePaymentVote& vote)
{
    LOCK(cs_mapMasternodePaymentVotes);
	
    const auto it = mapMasternodesLastVote.find(vote.masternodeOutpoint);
    if (it != mapMasternodesLastVote.end()) {
        if (it->second == vote.nBlockHeight)
            return false;
        it->second = vote.nBlockHeight;
        return true;
    }

    //record this masternode voted
    mapMasternodesLastVote.emplace(vote.masternodeOutpoint, vote.nBlockHeight);
    return true;
}

/**
*   FillBlockPayee
*
*   Fill Masternode ONLY payment block
*/

void CMasternodePayments::FillBlockPayee(CMutableTransaction& txNew, int nBlockHeight, CAmount blockReward, CTxOut& txoutMasternodeRet) const
{
    // make sure it's not filled yet
    txoutMasternodeRet = CTxOut();

    CScript payee;

    if(!GetBlockPayee(nBlockHeight, payee)) {
        // no masternode detected...
		LogPrintf("CMasternodePayments::FillBlockPayee -- Failed to detect masternode to pay\n");
		return;
    }

    // GET MASTERNODE PAYMENT VARIABLES SETUP
    CAmount masternodePayment = GetMasternodePayment(nBlockHeight, blockReward);

    // split reward between miner ...
    txNew.vout[0].nValue -= masternodePayment;
    // ... and masternode
    txoutMasternodeRet = CTxOut(masternodePayment, payee);
    txNew.vout.push_back(txoutMasternodeRet);

    CTxDestination address1;
    ExtractDestination(payee, address1);
    CBitcoinAddress address2(address1);

    LogPrintf("CMasternodePayments::FillBlockPayee -- Masternode payment %lld to %s\n", masternodePayment, address2.ToString());
}

int CMasternodePayments::GetMinMasternodePaymentsProto() const {
	if(!sporkManager.IsSporkActive(SPORK_22_MASTERNODE_UPDATE_PROTO))
	{
		return sporkManager.IsSporkActive(SPORK_10_MASTERNODE_PAY_UPDATED_NODES)
				? MIN_MASTERNODE_PAYMENT_PROTO_VERSION_2
				: MIN_MASTERNODE_PAYMENT_PROTO_VERSION_1;
	}
	else
	{
		return sporkManager.IsSporkActive(SPORK_10_MASTERNODE_PAY_UPDATED_NODES)
				? MIN_MASTERNODE_PAYMENT_PROTO_VERSION_3
				: MIN_MASTERNODE_PAYMENT_PROTO_VERSION_1;
		
	}
}

void CMasternodePayments::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if(fLiteMode) return; // disable all Beenode specific functionality

    if (strCommand == NetMsgType::MASTERNODEPAYMENTSYNC) { //Masternode Payments Request Sync

        if(pfrom->nVersion < GetMinMasternodePaymentsProto()) {
            LogPrint("mnpayments", "MASTERNODEPAYMENTSYNC -- peer=%d using obsolete version %i\n", pfrom->id, pfrom->nVersion);
            connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::REJECT, strCommand, REJECT_OBSOLETE,
                               strprintf("Version must be %d or greater", GetMinMasternodePaymentsProto())));
            return;
        }

        // Ignore such requests until we are fully synced.
        // We could start processing this after masternode list is synced
        // but this is a heavy one so it's better to finish sync first.
        if (!masternodeSync.IsSynced()) return;

        // DEPRECATED, should be removed on next protocol bump
        if(pfrom->nVersion == 70208) {
            int nCountNeeded;
            vRecv >> nCountNeeded;
        }

        if(netfulfilledman.HasFulfilledRequest(pfrom->addr, NetMsgType::MASTERNODEPAYMENTSYNC)) {
            LOCK(cs_main);
            // Asking for the payments list multiple times in a short period of time is no good
            LogPrintf("MASTERNODEPAYMENTSYNC -- peer already asked me for the list, peer=%d\n", pfrom->id);
            Misbehaving(pfrom->GetId(), 20);
            return;
        }
        netfulfilledman.AddFulfilledRequest(pfrom->addr, NetMsgType::MASTERNODEPAYMENTSYNC);

        Sync(pfrom, connman);
        LogPrintf("MASTERNODEPAYMENTSYNC -- Sent Masternode payment votes to peer=%d\n", pfrom->id);

    } else if (strCommand == NetMsgType::MASTERNODEPAYMENTVOTE) { // Masternode Payments Vote for the Winner

		CMasternodePaymentVote vote;
		vRecv >> vote;

        if(pfrom->nVersion < GetMinMasternodePaymentsProto()) {
            LogPrint("mnpayments", "MASTERNODEPAYMENTVOTE -- peer=%d using obsolete version %i\n", pfrom->id, pfrom->nVersion);
            connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::REJECT, strCommand, REJECT_OBSOLETE,
                               strprintf("Version must be %d or greater", GetMinMasternodePaymentsProto())));
            return;
        }

        uint256 nHash = vote.GetHash();

        pfrom->setAskFor.erase(nHash);

        // TODO: clear setAskFor for MSG_MASTERNODE_PAYMENT_BLOCK too
        if(!masternodeSync.IsMasternodeListSynced()) return;

        {
            LOCK(cs_mapMasternodePaymentVotes);

            auto res = mapMasternodePaymentVotes.emplace(nHash, vote);

            if(!res.second && res.first->second.IsVerified()) {
                LogPrint("mnpayments", "MASTERNODEPAYMENTVOTE -- hash=%s, nBlockHeight=%d/%d seen\n",
                            nHash.ToString(), vote.nBlockHeight, nCachedBlockHeight);
                return;
            }
            res.first->second.MarkAsNotVerified();
        }

        int nFirstBlock = nCachedBlockHeight - GetStorageLimit();
        if(vote.nBlockHeight < nFirstBlock || vote.nBlockHeight > nCachedBlockHeight+20) {
            LogPrint("mnpayments", "MASTERNODEPAYMENTVOTE -- vote out of range: nFirstBlock=%d, nBlockHeight=%d, nHeight=%d\n", nFirstBlock, vote.nBlockHeight, nCachedBlockHeight);
            return;
        }

        std::string strError = "";
        if(!vote.IsValid(pfrom, nCachedBlockHeight, strError, connman)) {
            LogPrint("mnpayments", "MASTERNODEPAYMENTVOTE -- invalid message, error: %s\n", strError);
            return;
        }

        masternode_info_t mnInfo;
        if(!mnodeman.GetMasternodeInfo(vote.masternodeOutpoint, mnInfo)) {
            // mn was not found, so we can't check vote, some info is probably missing
            LogPrintf("MASTERNODEPAYMENTVOTE -- masternode is missing %s\n", vote.masternodeOutpoint.ToStringShort());
            mnodeman.AskForMN(pfrom, vote.masternodeOutpoint, connman);
            return;
        }

        int nDos = 0;
        if(!vote.CheckSignature(mnInfo.pubKeyMasternode, nCachedBlockHeight, nDos)) {
            if(nDos) {
                LOCK(cs_main);
                LogPrintf("MASTERNODEPAYMENTVOTE -- ERROR: invalid signature\n");
                Misbehaving(pfrom->GetId(), nDos);
            } else {
                // only warn about anything non-critical (i.e. nDos == 0) in debug mode
                LogPrint("mnpayments", "MASTERNODEPAYMENTVOTE -- WARNING: invalid signature\n");
            }
            // Either our info or vote info could be outdated.
            // In case our info is outdated, ask for an update,
            mnodeman.AskForMN(pfrom, vote.masternodeOutpoint, connman);
            // but there is nothing we can do if vote info itself is outdated
            // (i.e. it was signed by a mn which changed its key),
            // so just quit here.
            return;
        }
		if(sporkManager.IsSporkActive(SPORK_21_MASTERNODE_ORDER_ENABLE)) 
		{
	        int nCount = 0;
	        //masternode_info_t mnInfo;
			if (!mnodeman.GetNextMasternodeInQueueForMasterPayment(vote.nBlockHeight, true, nCount, mnInfo)) {
				LogPrintf("CMasternodePayments::ProcessBlock -- ERROR: Failed to find masternode to pay\n");
	            return;
			}
			LogPrintf("CMasternodePayments::ProcessBlock -- Masternode found : %s\n", mnInfo.outpoint.ToStringShort());
	
			CScript payee = GetScriptForDestination(mnInfo.pubKeyCollateralAddress.GetID());
			CMasternodePaymentVote voteNew(vote.masternodeOutpoint, vote.nBlockHeight, payee);
	       
	        if(!UpdateLastVote(voteNew)) {
	            LogPrintf("MASTERNODEPAYMENTVOTE -- masternode already voted, masternode=%s\n", voteNew.masternodeOutpoint.ToStringShort());
	            return;
	        }

	        CTxDestination address1;
	        ExtractDestination(voteNew.payee, address1);
	        CBitcoinAddress address2(address1);
			
	
	        LogPrint("mnpayments"," MASTERNODEPAYMENTVOTE -- vote: address=%s, nBlockHeight=%d, nHeight=%d, prevout=%s, hash=%s new\n",
	                    address2.ToString(), voteNew.nBlockHeight, nCachedBlockHeight, voteNew.masternodeOutpoint.ToStringShort(), nHash.ToString());
			
	        if(AddOrUpdatePaymentVote(voteNew)){
	            voteNew.Relay(connman);
	            masternodeSync.BumpAssetLastTime("MASTERNODEPAYMENTVOTE");
	        }
		}
		else
		{
	        CTxDestination address1;
	        ExtractDestination(vote.payee, address1);
	        CBitcoinAddress address2(address1);
			
	
	        LogPrint("mnpayments"," MASTERNODEPAYMENTVOTE -- vote: address=%s, nBlockHeight=%d, nHeight=%d, prevout=%s, hash=%s new\n",
	                    address2.ToString(), vote.nBlockHeight, nCachedBlockHeight, vote.masternodeOutpoint.ToStringShort(), nHash.ToString());
			
	        if(AddOrUpdatePaymentVote(vote)){
	            vote.Relay(connman);
	            masternodeSync.BumpAssetLastTime("MASTERNODEPAYMENTVOTE");
	        }
		}
    }
}

uint256 CMasternodePaymentVote::GetHash() const
{
    // Note: doesn't match serialization

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << *(CScriptBase*)(&payee);
    ss << nBlockHeight;
    ss << masternodeOutpoint;
    return ss.GetHash();
}

uint256 CMasternodePaymentVote::GetSignatureHash() const
{
    return SerializeHash(*this);
}

bool CMasternodePaymentVote::Sign()
{
    std::string strError;
	std::string strMessage = masternodeOutpoint.ToStringShort() +
			boost::lexical_cast<std::string>(nBlockHeight) +
            ScriptToAsmStr(payee);
	if(!CMessageSigner::SignMessage(strMessage, vchSig, activeMasternode.keyMasternode)) {
		LogPrintf("CMasternodePaymentVote::Sign -- SignMessage() failed\n");
        return false;
    }
	if(!CMessageSigner::VerifyMessage(activeMasternode.pubKeyMasternode, vchSig, strMessage, strError)) {
		LogPrintf("CMasternodePaymentVote::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CMasternodePayments::GetBlockPayee(int nBlockHeight, CScript& payeeRet) const
{
    LOCK(cs_mapMasternodeBlocks);

    auto it = mapMasternodeBlocks.find(nBlockHeight);
	
    return it != mapMasternodeBlocks.end() && it->second.GetBestPayee(payeeRet);
}

// Is this masternode scheduled to get paid soon?
// -- Only look ahead up to 8 blocks to allow for propagation of the latest 2 blocks of votes
bool CMasternodePayments::IsScheduled(const masternode_info_t& mnInfo, int nNotBlockHeight) const
{
    LOCK(cs_mapMasternodeBlocks);

    if(!masternodeSync.IsMasternodeListSynced()) return false;

    CScript mnpayee;
    mnpayee = GetScriptForDestination(mnInfo.pubKeyCollateralAddress.GetID());

    CScript payee;
    for(int64_t h = nCachedBlockHeight; h <= nCachedBlockHeight + 8; h++){
        if(h == nNotBlockHeight) continue;
        if(GetBlockPayee(h, payee) && mnpayee == payee) {
            return true;
        }
    }

    return false;
}

bool CMasternodePayments::AddOrUpdatePaymentVote(const CMasternodePaymentVote& vote)
{
    uint256 blockHash = uint256();
    if(!GetBlockHash(blockHash, vote.nBlockHeight - 101)) return false;

    uint256 nVoteHash = vote.GetHash();

    if(HasVerifiedPaymentVote(nVoteHash)) return false;

    LOCK2(cs_mapMasternodeBlocks, cs_mapMasternodePaymentVotes);

	if(sporkManager.IsSporkActive(SPORK_21_MASTERNODE_ORDER_ENABLE))
	{
		auto it2 = mapMasternodeBlocks.find(vote.nBlockHeight);
		if(it2!=mapMasternodeBlocks.end())
			return false;
	}
    
	CTxDestination address1;
	ExtractDestination(vote.payee, address1);
	CBitcoinAddress address2(address1);
	
    if(vote.nBlockHeight > sporkManager.GetSporkValue(SPORK_20_EVOLUTION_DISABLE_NODE))
	{
		CBitcoinAddress address( evolutionManager.getDisableNode(vote.nBlockHeight) );	
        if(address.ToString().compare(address2.ToString())==0)
            return false;
    }
    
    
	
    auto it = mapMasternodeBlocks.emplace(vote.nBlockHeight, CMasternodeBlockPayees(vote.nBlockHeight)).first;

    it->second.AddPayee(vote);

    LogPrint("mnpayments", "CMasternodePayments::AddOrUpdatePaymentVote -- added, hash=%s\n", nVoteHash.ToString());

    return true;
}

bool CMasternodePayments::HasVerifiedPaymentVote(const uint256& hashIn) const
{
	
    LOCK(cs_mapMasternodePaymentVotes);
    const auto it = mapMasternodePaymentVotes.find(hashIn);
    return it != mapMasternodePaymentVotes.end() && it->second.IsVerified();
	
	return true;
}

void CMasternodeBlockPayees::AddPayee(const CMasternodePaymentVote& vote)
{
    LOCK(cs_vecPayees);

    uint256 nVoteHash = vote.GetHash();

    for (auto& payee : vecPayees) {
        if (payee.GetPayee() == vote.payee) {
            payee.AddVoteHash(nVoteHash);
            return;
        }
    }
	
    CMasternodePayee payeeNew(vote.payee, nVoteHash);
    vecPayees.push_back(payeeNew);
}

bool CMasternodeBlockPayees::GetBestPayee(CScript& payeeRet) const
{
    LOCK(cs_vecPayees);

    if(vecPayees.empty()) {
        LogPrint("mnpayments", "CMasternodeBlockPayees::GetBestPayee -- ERROR: couldn't find any payee\n");
        return false;
    }

	if(sporkManager.IsSporkActive(SPORK_21_MASTERNODE_ORDER_ENABLE)) 
	{
	    for (const auto& payee : vecPayees) {
			payeeRet = payee.GetPayee();
	    }
    	return true;
	}
	else
	{
	 	int nVotes = -1;
	    for (const auto& payee : vecPayees) {
	        if (payee.GetVoteCount() > nVotes) {
	            payeeRet = payee.GetPayee();
	            nVotes = payee.GetVoteCount();
	        }
	    }
	    return (nVotes > -1);
	}

}

bool CMasternodeBlockPayees::HasPayeeWithVotes(const CScript& payeeIn, int nVotesReq) const
{
    LOCK(cs_vecPayees);

    for (const auto& payee : vecPayees) {
        if (payee.GetPayee() == payeeIn) {
            return true;
        }
    }

    LogPrint("mnpayments", "CMasternodeBlockPayees::HasPayeeWithVotes -- ERROR: couldn't find any payee with %d+ votes\n", nVotesReq);
    return false;
}

bool CMasternodeBlockPayees::IsTransactionValid(const CTransaction& txNew) const
{
    LOCK(cs_vecPayees);

    int nMaxSignatures = 0;
    std::string strPayeesPossible = "";

    CAmount nMasternodePayment;
	if( sporkManager.IsSporkWorkActive(SPORK_18_EVOLUTION_PAYMENTS) ){
		CScript payeeEvo;
		CBitcoinAddress address( evolutionManager.getEvolution(nBlockHeight) );
		payeeEvo = GetScriptForDestination( address.Get() );
		nMasternodePayment = GetMasternodePayment(nBlockHeight, txNew.GetValueOutWOEvol(payeeEvo));
	}else{
		nMasternodePayment = GetMasternodePayment(nBlockHeight, txNew.GetValueOut());
	}
    //require at least MNPAYMENTS_SIGNATURES_REQUIRED signatures

	if(!sporkManager.IsSporkActive(SPORK_21_MASTERNODE_ORDER_ENABLE)) 
	{
		for (const auto& payee : vecPayees) {
	        if (payee.GetVoteCount() >= nMaxSignatures) {
	            nMaxSignatures = payee.GetVoteCount();
	        }
		}
	}
    // if we don't have at least MNPAYMENTS_SIGNATURES_REQUIRED signatures on a payee, approve whichever is the longest chain
    if(nMaxSignatures < MNPAYMENTS_SIGNATURES_REQUIRED) return true;

    for (const auto& payee : vecPayees) {
        if (!sporkManager.IsSporkActive(SPORK_21_MASTERNODE_ORDER_ENABLE) && payee.GetVoteCount() >= MNPAYMENTS_SIGNATURES_REQUIRED) 
        {
            for (const auto& txout : txNew.vout) {
                if (payee.GetPayee() == txout.scriptPubKey && nMasternodePayment == txout.nValue) {
                    LogPrint("mnpayments", "CMasternodeBlockPayees::IsTransactionValid -- Found required payment\n");
                    return true;
                }
            }

            CTxDestination address1;
            ExtractDestination(payee.GetPayee(), address1);
            CBitcoinAddress address2(address1);

            if(strPayeesPossible == "") {
                strPayeesPossible = address2.ToString();
            } else {
                strPayeesPossible += "," + address2.ToString();
            }
        }
    }

    LogPrintf("CMasternodeBlockPayees::IsTransactionValid -- ERROR: Missing required payment, possible payees: '%s', amount: %f BEENODE\n", strPayeesPossible, (float)nMasternodePayment/COIN);
    return false;
}

std::string CMasternodeBlockPayees::GetRequiredPaymentsString() const
{
    LOCK(cs_vecPayees);

    std::string strRequiredPayments = "";
    for (const auto& payee : vecPayees)
    {
        CTxDestination address1;
        ExtractDestination(payee.GetPayee(), address1);
        CBitcoinAddress address2(address1);

        if (!strRequiredPayments.empty())
            strRequiredPayments += ", ";
        strRequiredPayments += strprintf("%s:%d ", address2.ToString(), payee.GetVoteCount());
    }
    if (strRequiredPayments.empty())
        return "Unknown";

    return strRequiredPayments;
}
std::string CMasternodePayments::GetRequiredPaymentsString(int nBlockHeight) const
{
    LOCK(cs_mapMasternodeBlocks);
    const auto it = mapMasternodeBlocks.find(nBlockHeight);
    return it == mapMasternodeBlocks.end() ? "Unknown" : it->second.GetRequiredPaymentsString();
}

bool CMasternodePayments::IsTransactionValid(const CTransaction& txNew, int nBlockHeight) const
{
    LOCK(cs_mapMasternodeBlocks);

    const auto it = mapMasternodeBlocks.find(nBlockHeight);
    return it == mapMasternodeBlocks.end() ? true : it->second.IsTransactionValid(txNew);
}

void CMasternodePayments::CheckAndRemove()
{
    if(!masternodeSync.IsBlockchainSynced()) return;

    LOCK2(cs_mapMasternodeBlocks, cs_mapMasternodePaymentVotes);

    int nLimit = GetStorageLimit();

    std::map<uint256, CMasternodePaymentVote>::iterator it = mapMasternodePaymentVotes.begin();
    while(it != mapMasternodePaymentVotes.end()) {
        CMasternodePaymentVote vote = (*it).second;

        if(nCachedBlockHeight - vote.nBlockHeight > nLimit) {
            LogPrint("mnpayments", "CMasternodePayments::CheckAndRemove -- Removing old Masternode payment: nBlockHeight=%d\n", vote.nBlockHeight);
            mapMasternodePaymentVotes.erase(it++);
            mapMasternodeBlocks.erase(vote.nBlockHeight);
        } else {
            ++it;
        }
    }
    LogPrintf("CMasternodePayments::CheckAndRemove -- %s\n", ToString());
}

bool CMasternodePaymentVote::IsValid(CNode* pnode, int nValidationHeight, std::string& strError, CConnman& connman) const
{
    masternode_info_t mnInfo;

    if(!mnodeman.GetMasternodeInfo(masternodeOutpoint, mnInfo)) {
        strError = strprintf("Unknown masternode=%s", masternodeOutpoint.ToStringShort());
        // Only ask if we are already synced and still have no idea about that Masternode
        if(masternodeSync.IsMasternodeListSynced()) {
            mnodeman.AskForMN(pnode, masternodeOutpoint, connman);
        }

        return false;
    }

    int nMinRequiredProtocol;
    if(nBlockHeight >= nValidationHeight) {
        // new votes must comply SPORK_10_MASTERNODE_PAY_UPDATED_NODES rules
        nMinRequiredProtocol = mnpayments.GetMinMasternodePaymentsProto();
    } else {
        // allow non-updated masternodes for old blocks
        nMinRequiredProtocol = MIN_MASTERNODE_PAYMENT_PROTO_VERSION_1;
    }

    if(mnInfo.nProtocolVersion < nMinRequiredProtocol) {
        strError = strprintf("Masternode protocol is too old: nProtocolVersion=%d, nMinRequiredProtocol=%d", mnInfo.nProtocolVersion, nMinRequiredProtocol);
        return false;
    }

    // Only masternodes should try to check masternode rank for old votes - they need to pick the right winner for future blocks.
    // Regular clients (miners included) need to verify masternode rank for future block votes only.
    if(!fMasternodeMode && nBlockHeight < nValidationHeight) return true;

    return true;
}

bool CMasternodePayments::ProcessBlock(int nBlockHeight, CConnman& connman)
{
    // DETERMINE IF WE SHOULD BE VOTING FOR THE NEXT PAYEE

    if(fLiteMode || !fMasternodeMode) return false;

    // We have little chances to pick the right winner if winners list is out of sync
    // but we have no choice, so we'll try. However it doesn't make sense to even try to do so
    // if we have not enough data about masternodes.
    if(!masternodeSync.IsMasternodeListSynced()) return false;

    // LOCATE THE NEXT MASTERNODE WHICH SHOULD BE PAID

    LogPrintf("CMasternodePayments::ProcessBlock -- Start: nBlockHeight=%d, masternode=%s\n", nBlockHeight, activeMasternode.outpoint.ToStringShort());

    // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
    int nCount = 0;
    masternode_info_t mnInfo;

    if (!mnodeman.GetNextMasternodeInQueueForMasterPayment(nBlockHeight, true, nCount, mnInfo)) {
        LogPrintf("CMasternodePayments::ProcessBlock -- ERROR: Failed to find masternode to pay\n");
        return false;
    }

    LogPrintf("CMasternodePayments::ProcessBlock -- Masternode found : %s\n", mnInfo.outpoint.ToStringShort());


    CScript payee = GetScriptForDestination(mnInfo.pubKeyCollateralAddress.GetID());

    CMasternodePaymentVote voteNew(activeMasternode.outpoint, nBlockHeight, payee);

    CTxDestination address1;
    ExtractDestination(payee, address1);
    CBitcoinAddress address2(address1);

    LogPrintf("CMasternodePayments::ProcessBlock -- vote: payee=%s, nBlockHeight=%d\n", address2.ToString(), nBlockHeight);

    // SIGN MESSAGE TO NETWORK WITH OUR MASTERNODE KEYS

    LogPrintf("CMasternodePayments::ProcessBlock -- Signing winner\n");
    if (voteNew.Sign()) {
        LogPrintf("CMasternodePayments::ProcessBlock -- AddPaymentWinner()\n");

        if (AddOrUpdatePaymentVote(voteNew)) {
            voteNew.Relay(connman);
            return true;
        }
    }

    return false;
}

void CMasternodePayments::CheckBlockVotes(int nBlockHeight)
{
    if (!masternodeSync.IsWinnersListSynced()) return;

}

void CMasternodePaymentVote::Relay(CConnman& connman) const
{
    // Do not relay until fully synced
    if(!masternodeSync.IsSynced()) {
        LogPrint("mnpayments", "CMasternodePayments::Relay -- won't relay until fully synced\n");
        return;
    }

    CInv inv(MSG_MASTERNODE_PAYMENT_VOTE, GetHash());
    connman.RelayInv(inv);
}

bool CMasternodePaymentVote::CheckSignature(const CPubKey& pubKeyMasternode, int nValidationHeight, int &nDos) const
{
    // do not ban by default
    nDos = 0;
    std::string strError = "";

	std::string strMessage = masternodeOutpoint.ToStringShort() +
			boost::lexical_cast<std::string>(nBlockHeight) +
            ScriptToAsmStr(payee);

	if (!CMessageSigner::VerifyMessage(pubKeyMasternode, vchSig, strMessage, strError)) {
		// Only ban for future block vote when we are already synced.
        // Otherwise it could be the case when MN which signed this vote is using another key now
        // and we have no idea about the old one.
		if(masternodeSync.IsMasternodeListSynced() && nBlockHeight > nValidationHeight) {
			nDos = 20;
        }
        return error("CMasternodePaymentVote::CheckSignature -- Got bad Masternode payment signature, masternode=%s, error: %s",
				masternodeOutpoint.ToStringShort(), strError);
	}

    return true;
}

std::string CMasternodePaymentVote::ToString() const
{
    std::ostringstream info;

    info << masternodeOutpoint.ToStringShort() <<
            ", " << nBlockHeight <<
            ", " << ScriptToAsmStr(payee) <<
            ", " << (int)vchSig.size();

    return info.str();
}

// Send only votes for future blocks, node should request every other missing payment block individually
void CMasternodePayments::Sync(CNode* pnode, CConnman& connman) const
{
    LOCK(cs_mapMasternodeBlocks);

    if(!masternodeSync.IsWinnersListSynced()) return;

    int nInvCount = 0;

    for(int h = nCachedBlockHeight; h < nCachedBlockHeight + 20; h++) {
        const auto it = mapMasternodeBlocks.find(h);
        if(it != mapMasternodeBlocks.end()) {
            for (const auto& payee : it->second.vecPayees) {
                std::vector<uint256> vecVoteHashes = payee.GetVoteHashes();
                for (const auto& hash : vecVoteHashes) {
                    if(!HasVerifiedPaymentVote(hash)) continue;
                    pnode->PushInventory(CInv(MSG_MASTERNODE_PAYMENT_VOTE, hash));
                    nInvCount++;
                }
            }
        }
    }

    LogPrintf("CMasternodePayments::Sync -- Sent %d votes to peer=%d\n", nInvCount, pnode->id);
    CNetMsgMaker msgMaker(pnode->GetSendVersion());
    connman.PushMessage(pnode, msgMaker.Make(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_MNW, nInvCount));
}

// Request low data/unknown payment blocks in batches directly from some node instead of/after preliminary Sync.
void CMasternodePayments::RequestLowDataPaymentBlocks(CNode* pnode, CConnman& connman) const
{
    if(!masternodeSync.IsMasternodeListSynced()) return;

    CNetMsgMaker msgMaker(pnode->GetSendVersion());
    LOCK2(cs_main, cs_mapMasternodeBlocks);

    std::vector<CInv> vToFetch;
    int nLimit = GetStorageLimit();

    const CBlockIndex *pindex = chainActive.Tip();

    while(nCachedBlockHeight - pindex->nHeight < nLimit) {
        const auto it = mapMasternodeBlocks.find(pindex->nHeight);
        if(it == mapMasternodeBlocks.end()) {
            // We have no idea about this block height, let's ask
            vToFetch.push_back(CInv(MSG_MASTERNODE_PAYMENT_BLOCK, pindex->GetBlockHash()));
            // We should not violate GETDATA rules
            if(vToFetch.size() == MAX_INV_SZ) {
                LogPrintf("CMasternodePayments::RequestLowDataPaymentBlocks -- asking peer=%d for %d blocks\n", pnode->id, MAX_INV_SZ);
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::GETDATA, vToFetch));
                // Start filling new batch
                vToFetch.clear();
            }
        }
        if(!pindex->pprev) break;
        pindex = pindex->pprev;
    }

    auto it = mapMasternodeBlocks.begin();

    while(it != mapMasternodeBlocks.end()) {
        int nTotalVotes = 0;
        bool fFound = false;
        for (const auto& payee : it->second.vecPayees) {
            if(payee.GetVoteCount() >= MNPAYMENTS_SIGNATURES_REQUIRED) {
                fFound = true;
                break;
            }
            nTotalVotes += payee.GetVoteCount();
        }
        // A clear winner (MNPAYMENTS_SIGNATURES_REQUIRED+ votes) was found
        // or no clear winner was found but there are at least avg number of votes
  
		if(!sporkManager.IsSporkActive(SPORK_21_MASTERNODE_ORDER_ENABLE))
		{       
			if(fFound || nTotalVotes >= (MNPAYMENTS_SIGNATURES_TOTAL + MNPAYMENTS_SIGNATURES_REQUIRED)/2) {
	            // so just move to the next block
	            ++it;
	            continue;
	        }
		}
		else
		{  
			if(fFound) {
	            // so just move to the next block
	            ++it;
	            continue;
	        }
		}
        // DEBUG
        DBG (
            // Let's see why this failed
            BOOST_FOREACH(CMasternodePayee& payee, it->second.vecPayees) {
                CTxDestination address1;
                ExtractDestination(payee.GetPayee(), address1);
                CBitcoinAddress address2(address1);
                printf("payee %s votes %d\n", address2.ToString().c_str(), payee.GetVoteCount());
            }
            printf("block %d votes total %d\n", it->first, nTotalVotes);
        )
        // END DEBUG
        // Low data block found, let's try to sync it
        uint256 hash;
        if(GetBlockHash(hash, it->first)) {
            vToFetch.push_back(CInv(MSG_MASTERNODE_PAYMENT_BLOCK, hash));
        }
        // We should not violate GETDATA rules
        if(vToFetch.size() == MAX_INV_SZ) {
            LogPrintf("CMasternodePayments::RequestLowDataPaymentBlocks -- asking peer=%d for %d payment blocks\n", pnode->id, MAX_INV_SZ);
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::GETDATA, vToFetch));
            // Start filling new batch
            vToFetch.clear();
        }
        ++it;
    }
    // Ask for the rest of it
    if(!vToFetch.empty()) {
        LogPrintf("CMasternodePayments::RequestLowDataPaymentBlocks -- asking peer=%d for %d payment blocks\n", pnode->id, vToFetch.size());
        connman.PushMessage(pnode, msgMaker.Make(NetMsgType::GETDATA, vToFetch));
    }
}

std::string CMasternodePayments::ToString() const
{
    std::ostringstream info;

	if(!sporkManager.IsSporkActive(SPORK_21_MASTERNODE_ORDER_ENABLE))
    	info << "Votes: " << (int)mapMasternodePaymentVotes.size() << ", Blocks: " << (int)mapMasternodeBlocks.size();
	info << " Blocks: " << (int)mapMasternodeBlocks.size();

    return info.str();
}

bool CMasternodePayments::IsEnoughData() const
{

	if(!sporkManager.IsSporkActive(SPORK_21_MASTERNODE_ORDER_ENABLE))
	{
	    float nAverageVotes = (MNPAYMENTS_SIGNATURES_TOTAL + MNPAYMENTS_SIGNATURES_REQUIRED) / 2;
	    int nStorageLimit = GetStorageLimit();
	    return GetBlockCount() > nStorageLimit && GetVoteCount() > nStorageLimit * nAverageVotes;
	}
	else
	{
	    int nStorageLimit = GetStorageLimit();
	    return GetBlockCount() > nStorageLimit;
	}
}

int CMasternodePayments::GetStorageLimit() const
{
    return std::max(int(mnodeman.size() * nStorageCoeff), nMinBlocksToStore);
}

void CMasternodePayments::UpdatedBlockTip(const CBlockIndex *pindex, CConnman& connman)
{
    if(!pindex) return;

    nCachedBlockHeight = pindex->nHeight;
    LogPrint("mnpayments", "CMasternodePayments::UpdatedBlockTip -- nCachedBlockHeight=%d\n", nCachedBlockHeight);

    int nFutureBlock = nCachedBlockHeight + 10;

    CheckBlockVotes(nFutureBlock - 1);
	//from body CheckBlockVotes
	if (!masternodeSync.IsWinnersListSynced()) return;
    ProcessBlock(nFutureBlock, connman);
}
/**
*   Create Evolution Payments
*
*   - Create the correct payment structure for a given evolution
*/
void CMasternodePayments::CreateEvolution( CMutableTransaction& txNewRet, int nBlockHeight, CAmount blockEvolution, std::vector<CTxOut>& voutSuperblockRet )
{	
    // make sure it's empty, just in case
    voutSuperblockRet.clear();
			
	CScript payeeEvo;
	CPubKey pb;

	CBitcoinAddress address( evolutionManager.getEvolution(nBlockHeight)  );

	//--.
	payeeEvo = GetScriptForDestination( address.Get() );
	
	CTxOut txout = CTxOut(  blockEvolution, payeeEvo );
	
	txNewRet.vout.push_back( txout );
	
	voutSuperblockRet.push_back( txout );
}	
