#include <string>
#include "AllinProc.h"
#include "HallManager.h"
#include "Logger.h"
#include "Room.h"
#include "Configure.h"
#include "ErrorMsg.h"
#include "BaseClientHandler.h"
#include "ProcessManager.h"
#include "json/json.h"
#include "GameCmd.h"

using namespace std;


AllinProc::AllinProc()
{
	this->name = "AllinProc" ;
}

AllinProc::~AllinProc()
{
}

int AllinProc::doRequest(CDLSocketHandler* clientHandler, InputPacket* pPacket,Context* pt)
{
	//_NOTUSED(pt);
	int cmd = pPacket->GetCmdType();
	short seq = pPacket->GetSeqNum();
	int uid = pPacket->ReadInt();
	int tid = pPacket->ReadInt();
	short svid = tid >> 16;
	short realTid = tid & 0x0000FFFF;
	
	_LOG_INFO_("==>[AllinProc] [0x%04x] uid[%d]\n", cmd, uid);
	_LOG_DEBUG_("[DATA Parse] tid=[%d] svid=[%d] reallTid[%d]\n", tid, svid, realTid);

	BaseClientHandler* hallhandler = reinterpret_cast<BaseClientHandler*> (clientHandler);

	Room* room = Room::getInstance();

	Table* table = room->getTable(realTid);

	if(table == NULL)
	{
		_LOG_ERROR_("[AllinProc] uid=[%d] tid=[%d] realTid=[%d] Table is NULL\n",uid, tid, realTid);
		return sendErrorMsg(hallhandler, cmd, uid, -2,ErrorMsg::getInstance()->getErrMsg(-2),seq);
	}

	Player* player = table->getPlayer(uid);
	if(player == NULL)
	{
		_LOG_ERROR_("[AllinProc] uid=[%d] tid=[%d] realTid=[%d] Your not in This Table\n",uid, tid, realTid);
		return sendErrorMsg(hallhandler, cmd, uid, -1,ErrorMsg::getInstance()->getErrMsg(-1),seq);
	}

	if( ! player->isActive())
	{
		_LOG_ERROR_("[AllinProc] uid=[%d] tid=[%d] realTid=[%d] tm_nStatus=[%d] is not active\n",uid, tid, realTid, table->m_nStatus);
		return sendErrorMsg(hallhandler, cmd, uid, -11,ErrorMsg::getInstance()->getErrMsg(-11),seq);
	}

	if(table->m_nCurrBetUid != uid)
	{
		_LOG_ERROR_("[AllinProc] uid=[%d] tid=[%d] realTid=[%d] betUid[%d] is not this uid\n",uid, tid, realTid, table->m_nCurrBetUid);
		return sendErrorMsg(hallhandler, cmd, uid, -12,ErrorMsg::getInstance()->getErrMsg(-12),seq);
	}

	if(player->m_lCarryMoney <= 0)
	{
		_LOG_ERROR_("[AllinProc] uid=[%d] tid=[%d] realTid=[%d] player->m_lCarryMoney=[%ld] countmoney[%ld]\n",uid, tid, realTid, player->m_lCarryMoney, player->betCoinList[0]);
		return sendErrorMsg(hallhandler, cmd, uid, -13,ErrorMsg::getInstance()->getErrMsg(-13),seq);
	}

	if(table->m_bIsOverTimer)
	{
		_LOG_ERROR_("[AllinProc] table is IsOverTimer uid=[%d] tid=[%d] realTid=[%d] Table Status=[%d]\n",uid, tid, realTid, table->m_nStatus);
		return sendErrorMsg(hallhandler, cmd, uid, 2,ErrorMsg::getInstance()->getErrMsg(2),seq);
	}

	int64_t betmoney = player->m_lCarryMoney;
	player->betCoinList[0] = player->betCoinList[0] + betmoney;
	player->betCoinList[table->m_bCurrRound] += betmoney;
	player->m_lCarryMoney -= betmoney;
	if(player->betCoinList[table->m_bCurrRound] > table->m_lCurrMax)
	{
		table->m_lCurrMax = player->betCoinList[table->m_bCurrRound];
		table->m_nRaseUid = player->id;
	}
	player->m_bAllin = true;
	table->m_bThisRoundHasAllin = true;
	player->setActiveTime(time(NULL));

	_LOG_INFO_("[AllinProc] Uid=[%d] GameID=[%s] CallMoney=[%ld] BetCountMoney=[%ld] currRound=[%d]\n", player->id, table->getGameID(), betmoney, player->betCoinList[0], table->m_bCurrRound);

	Json::Value data;
	data["BID"] = string(table->getGameID());
	data["Time"]=(int)(time(NULL) - table->getStartTime());
	data["currd"] = table->m_bCurrRound;
	data["allinID"] = player->id;
	data["allmoney"] = (double)betmoney;
	data["count"] = (double)player->betCoinList[0];
	if(!table->isAllRobot())
		_LOG_REPORT_(player->id, RECORD_ALL_IN, "%s", data.toStyledString().c_str());
	data["errcode"] = 0;
	_UDP_REPORT_(player->id, cmd,"%s",data.toStyledString().c_str());
	//设置下一个应该下注的用户
	Player* nextplayer = table->getNextBetPlayer(player, OP_ALLIN);
	if(nextplayer)
	{
		table->setPlayerlimitcoin();
		table->setPlayerOptype(player, OP_ALLIN);
	}
	table->setSumPool();

	player->setActiveTime(time(NULL));

	PlayerSet::iterator it = table->m_Viewers.begin();
	for(int i = 0; i < GAME_PLAYER + (int)table->m_Viewers.size(); ++i)
	{
		Player *otherplayer = NULL;
		if (i < GAME_PLAYER)
			otherplayer = table->player_array[i];
		else {
			otherplayer = *it;
			it++;
		}
		if (otherplayer)
		{
			sendAllinInfoToPlayers(otherplayer, table, player, betmoney , nextplayer,seq);
		}
	}
	//当前已经没有下一个用户下注了,此轮结束
	if(nextplayer == NULL)
	{
		table->setNextRound();	
	}
	else
	{
		table->stopBetCoinTimer();
		table->startBetCoinTimer(nextplayer->id,Configure::getInstance()->betcointime);
		nextplayer->setBetCoinTime(time(NULL));
	}

	table->privateRoomOp();

	return 0;
}

int AllinProc::sendAllinInfoToPlayers(Player* player, Table* table, Player* allinplayer, int64_t betmoney,Player* nextplayer, short seq)
{
	int svid = Configure::getInstance()->server_id;
	int tid = (svid << 16)|table->id;
	OutputPacket response;
	response.Begin(CLIENT_MSG_ALL_IN, player->id);
	if(player->id == allinplayer->id)
		response.SetSeqNum(seq);
	response.WriteShort(0);
	response.WriteString("");
	response.WriteInt(player->id);
	response.WriteShort(player->m_nStatus);
	response.WriteInt(tid);
	response.WriteShort(table->m_nStatus);
	response.WriteByte(table->m_bCurrRound);
	response.WriteInt(allinplayer->id);
	response.WriteInt64(betmoney);
	response.WriteInt64(allinplayer->betCoinList[table->m_bCurrRound]);
	response.WriteInt64(allinplayer->m_lCarryMoney);
	response.WriteInt64(table->m_lCurrMax);
	response.WriteInt(nextplayer ? nextplayer->id : 0);
	response.WriteInt64(player->betCoinList[table->m_bCurrRound]);
	response.WriteInt64(player->m_lCarryMoney);
	response.WriteShort(player->optype);
	response.WriteInt64(player->m_lBetLimit);
	response.WriteInt64(table->getSumPool());
	response.End();
	_LOG_DEBUG_("<==[AllinProc] Push [0x%04x] to uid=[%d]\n", CLIENT_MSG_ALL_IN, player->id);
	_LOG_DEBUG_("[Data Response] err=[0], errmsg[]\n");
	_LOG_DEBUG_("[Data Response] uid=[%d]\n",player->id);
	_LOG_DEBUG_("[Data Response] m_nStatus=[%d]\n",player->m_nStatus);
	_LOG_DEBUG_("[Data Response] m_lBetLimit=[%d]\n", player->m_lBetLimit);
	_LOG_DEBUG_("[Data Response] optype=[%d]\n", player->optype);
	_LOG_DEBUG_("[Data Response] m_bCurrRound=[%d]\n", table->m_bCurrRound);
	_LOG_DEBUG_("[Data Response] CarryMoney=[%d]\n", player->m_lCarryMoney);
	_LOG_DEBUG_("[Data Response] nextplayer=[%d]\n", nextplayer ? nextplayer->id : 0);

	if(HallManager::getInstance()->sendToHall(player->m_nHallid, &response, false) < 0)
		_LOG_ERROR_("[AllinProc] Send To Uid[%d] Error!\n", player->id);
	
	return 0;
}

REGISTER_PROCESS(CLIENT_MSG_ALL_IN, AllinProc)

