#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "Table.h"
#include "Logger.h"
#include "IProcess.h"
#include "Configure.h"
#include "AllocSvrConnect.h"
#include "CoinConf.h"
#include "HallManager.h"
#include "Util.h"
#include "GameApp.h"
#include "StrFunc.h"
#include "hiredis.h"
#include "RedisLogic.h"
#include "GameUtil.h"

#ifdef TEXAS
static std::string game_name = "TexasHundred";
#endif

#ifdef SHOWHAND
static std::string game_name = "ShowhandHundred";
#endif

bool compare_nocase (Player* first,Player* second)
{
	return (first)->m_lMoney > (second)->m_lMoney;
}

bool compare_reward (JackPotPlayer* first,JackPotPlayer* second)
{
	return (first)->m_lRewardCoin > (second)->m_lRewardCoin;
}

Table::Table():m_nStatus(-1)
{	 
	reloadCfg();
}
Table::~Table()
{
}

void Table::init()
{
	timer.init(this);
	id = Configure::getInstance()->m_nServerId << 16;
	this->m_nStatus = STATUS_TABLE_IDLE;
	memset(player_array, 0, sizeof(player_array));
	m_nType = Configure::getInstance()->m_nLevel;
	PlayerList.clear();
	BankerList.clear();
	memset(m_lTabBetArray, 0, sizeof(m_lTabBetArray));
	memset(m_lRealBetArray, 0, sizeof(m_lRealBetArray));
	m_bHasBet = false;
	m_lTabBetLimit = 0;

	maxwinner = NULL;
	m_nLimitChatCoin = 0;
	m_nOneChatCost = 0;
	m_lBankerWinScoreCount = 0;
	this->startIdleTimer(Configure::getInstance()->idletime);
	this->setStatusTime(time(NULL));
	memset(m_CardResult, 0, sizeof(m_CardResult));
	m_nRecordLast=0;
	memset(m_GameRecordArrary, 0, sizeof(m_GameRecordArrary));
	m_nDespatchNum = 0;

	bankeruid = 0;
	bankernum = 0;
	m_nLimitCoin = 0;
	bankersid = -1;
	betVector.clear();
	reloadCfg();
	JackPotList.clear();
	m_tRewardTime = 0;
	memset(m_lGetJackPotArea, 0, sizeof(m_lGetJackPotArea));
	receive_push_card_type.clear();

	for (int i = 0; i < MAX_SEAT_NUM; ++i)
	{
		m_SeatidArray[i] = NULL;
	}
	m_lJackPot = 0;
}

void Table::reset()
{
	memset(m_lTabBetArray, 0, sizeof(m_lTabBetArray));
	memset(m_lRealBetArray, 0, sizeof(m_lRealBetArray));
	m_bHasBet = false;
	m_lBankerWinScoreCount = 0;
	for(int i = 0; i < GAME_PLAYER; ++i)
	{
		Player* player = player_array[i];
		if(player)
			player->reset();
	}
	memset(m_CardResult, 0, sizeof(m_CardResult));
	betVector.clear();
	maxwinner = NULL;
	m_nDespatchNum = 0;
	memset(m_lGetJackPotArea, 0, sizeof(m_lGetJackPotArea));
	m_lJackPot = 0;
	receive_push_card_type.clear();
	m_lTabBetLimit = 0;

}

void Table::setStartTime(time_t t)
{
	StartTime = t;
	bool bRet = Server()->HSETi(StrFormatA("%s:%d", game_name.c_str(), Configure::getInstance()->m_nServerId).c_str(),
		"starttime", t);
	if (!bRet)
	{
		LOGGER(E_LOG_ERROR) << "save time to redis failed!";
	}
}

Player* Table::isUserInTab(int uid)
{
	for(int i = 0; i < GAME_PLAYER; ++i)
	{
		Player* player = player_array[i];
		if(player && player->id == uid)
			return player;
	}
	return NULL;
}

bool Table::isAllRobot()
{
	for(int i = 0; i < GAME_PLAYER; ++i)
	{
		if(player_array[i] && player_array[i]->source != 30 && player_array[i]->m_lBetArray[0] > 0)
		{
			return false;
		}
	}
	return true;
}

Player* Table::getPlayer(int uid)
{
	for(int i = 0; i < GAME_PLAYER; ++i)
	{
		Player* player = player_array[i];
		if(player && player->id == uid)
			return player;
	}
    return NULL;
}

void Table::setToPlayerList(Player* player)
{
	if(player == NULL)
		return ;
	if (PlayerList.size() <= 0)
	{
		PlayerList.push_back(player);
		return;
	}
	list<Player*>::iterator it;
	it = PlayerList.begin();
	while(it != PlayerList.end())
	{
		Player* other = *it;
		if(other)
		{
			if(other->m_lMoney <= player->m_lMoney)
			{
				PlayerList.insert(it, player);
				return ;
			}
		}
		it++;
	}
	PlayerList.push_back(player);
}

void Table::setToBankerList(Player* player)
{
	if(player == NULL)
		return ;
	//if (BankerList.size() <= 0)
	{
		BankerList.push_back(player);
		return;
	}
// 	list<Player*>::iterator it;
// 	it = BankerList.begin();
// 	while(it != BankerList.end())
// 	{
// 		Player* other = *it;
// 		if(other)
// 		{
// 			if(other->m_lMoney <= player->m_lMoney)
// 			{
// 				BankerList.insert(it, player);
// 				return ;
// 			}
// 		}
// 		it++;
// 	}
// 	BankerList.push_back(player);
}

void Table::CancleBanker(Player* player)
{
	if(player == NULL)
		return ;
	Player* noer = NULL;
	if(player->isbankerlist)
	{
		list<Player*>::iterator it;
		it = BankerList.begin();
		while(it != BankerList.end())
		{
			noer = *it;
			if(noer->id == player->id)
			{
				BankerList.erase(it);
				break;
			}
			it++;
		}
	}
	player->isbankerlist = false;
}

void Table::HandleBankeList()
{
	list<Player*>::iterator it;
	it = BankerList.begin();
	while(it != BankerList.end())
	{
		Player* other = *it;
		if(other)
		{
			if(other->m_lMoney < Configure::getInstance()->bankerlimit)
			{
				other->isbankerlist = false;
				IProcess::NoteBankerLeave(this, other);
				BankerList.erase(it++);
			}
			else
				it++;
		}
		else
			BankerList.erase(it++);
	}

	//上庄列表排序
	//BankerList.sort(compare_nocase);
}

void Table::setBetLimit(Player* banker)
{
	if (banker == NULL)
	{
		LOGGER(E_LOG_ERROR) << "banker == NULL!";
		return;
	}
	static const int factor = 40;
	m_lTabBetLimit = banker->m_lMoney / factor;
	LOGGER(E_LOG_INFO) << "table area bet limit = " << m_lTabBetLimit;

}

void Table::rotateBanker()
{
	Player* prebanker = NULL;
	if(this->bankersid >=0)
		prebanker = this->player_array[this->bankersid];
	if(prebanker)
	{
		prebanker->isbankerlist = false;
		prebanker->m_nLastBetTime = time(NULL);
	}
	bankersid = -1;
	bankeruid = 0;//做庄的uid
	bankernum = 0;//连续做庄的次数

	list<Player*>::iterator it;
	it = BankerList.begin();
	_LOG_DEBUG_("bankersize:%d\n", BankerList.size());
	int i = 0;
	while(it != BankerList.end())
	{
		Player* player = *it;
		if(player)
			_LOG_DEBUG_("index:%d player[%d] seatid[%d]\n", i, player->id, player->m_nSeatID);
		it++;
		i++;
	}

	
	it = BankerList.begin();

	while (it != BankerList.end())
	{
		Player* banker = *it;
		if(banker && prebanker)
		{
			if(!banker->isonline)
			{
				BankerList.erase(it++);
				continue;
			}
			if(banker->id == prebanker->id)
			{
				_LOG_ERROR_("bankerlist id[%d] m_nSeatID[%d] is prebanker id[%d] m_nSeatID[%d] \n", banker->id, banker->m_nSeatID, prebanker->id, prebanker->m_nSeatID);
				BankerList.erase(it++);
				continue;
			}
		}

		if(banker)
		{
			bankersid = banker->m_nSeatID;
			bankeruid = banker->id;//做庄的uid
			bankernum = 0;//连续做庄的次数
			//setBetLimit();

			time_t t;       
			time(&t);               
			char time_str[32]={0};
			struct tm* tp= localtime(&t);
			strftime(time_str,32,"%Y%m%d%H%M%S",tp);
			char gameId[64] ={0};
			//Player* banker = NULL;
			//if(this->bankersid >=0)
			//	banker = this->player_array[this->bankersid];
			sprintf(gameId, "%s|%d|%02d", time_str, banker ? banker->id: 0, this->bankernum + 1);
			this->setGameID(gameId);
			BankerList.erase(it++);

			IProcess::rotateBankerInfo(this, banker, prebanker);
			if (banker->m_seatid != 0)
			{
				this->LeaveSeat(banker->m_seatid);
				this->NotifyPlayerSeatInfo();
			}

			return;
		}
		BankerList.erase(it++);
	}
	
	if(BankerList.size() == 0)
	{	
		IProcess::rotateBankerInfo(this, NULL, prebanker);
		return ;
	}

}

int Table::playerComming(Player* player)
{
	if(player == NULL)
		return -1;

	for(int i = 0; i < GAME_PLAYER; ++i)
	{
		if(player_array[i] == NULL)
		{
			player_array[i] = player;
			player->m_nStatus = STATUS_PLAYER_LOGIN;
			setToPlayerList(player);
			break;
		}
	}
	player->tid = this->id;
	player->enter();
	++this->m_nCountPlayer;
	AllocSvrConnect::getInstance()->updateTableUserCount(this);
	return 0;
}

void Table::setSeatNULL(Player* player)
{
	for(int i = 0; i < GAME_PLAYER; ++i)
	{
		if(player_array[i] == player)
			player_array[i] = NULL;
	}

	list<Player*>::iterator it;
	it = PlayerList.begin();
	while(it != PlayerList.end())
	{
		Player* other = *it;
		if(other)
		{
			if(other->id == player->id)
			{
				PlayerList.erase(it);
				break ;
			}
		}
		it++;
	}

	if(player->isbankerlist)
	{
		it = BankerList.begin();
		while(it != BankerList.end())
		{
			Player* other = *it;
			if(other)
			{
				if(other->id == player->id)
				{
					BankerList.erase(it);
					break ;
				}
			}
			it++;
		}
	}
}

void Table::playerLeave(int uid)
{
	Player*  player = this->getPlayer(uid);
	if(player)
	{
		this->playerLeave(player);
	}
}

void Table::playerLeave(Player* player)
{
	if(player == NULL)
		return;
	_LOG_WARN_("Player[%d] Leave\n", player->id);
	
	this->setSeatNULL(player);

	if (player->m_seatid != 0)
	{
		this->LeaveSeat(player->m_seatid);
		this->NotifyPlayerSeatInfo();
	}

	
	player->leave();
	player->init();
	//当前用户减一
	--this->m_nCountPlayer;
	
	AllocSvrConnect::getInstance()->updateTableUserCount(this);
}

int Table::playerBetCoin(Player* player, short bettype, int64_t betmoney)
{
	if(player == NULL)
		return -1;

	if(bettype <= 0 || bettype > 4)
		return -1;

	player->m_lMoney -= betmoney;
	player->m_lBetArray[bettype] += betmoney;
	player->m_lBetArray[0] += betmoney;
	this->m_lTabBetArray[bettype] += betmoney;
	this->m_lTabBetArray[0] += betmoney;
	player->m_lReturnScore += betmoney;
	player->m_nLastBetTime = time(NULL);
	if(player->source != E_MSG_SOURCE_ROBOT)
	{
		this->m_lRealBetArray[bettype] += betmoney;
		SaveBetInfoToRedis(player, bettype);
		ULOGGER(E_LOG_INFO, player->id) << "bettype = " << bettype << "tabel bet = " << this->m_lTabBetArray[bettype];
		m_bHasBet = true;
	}
	return 0;
}

int Table::GetSpecialType()
{
	int num = 0;
	static int dwRandCount=0;
	if(dwRandCount > 10000000)
		dwRandCount = 0;
	srand((unsigned)time(NULL)+dwRandCount++);
	int rnd = rand()%100;
	if(rnd >= 0 && rnd < Configure::getInstance()->specialnum0)
		return 0;
	if(rnd >= Configure::getInstance()->specialnum0 && rnd < Configure::getInstance()->specialnum1)
		return CT_ONE_LONG;
	if(rnd >= Configure::getInstance()->specialnum1 && rnd < Configure::getInstance()->specialnum2)
		return CT_TWO_LONG;
	if(rnd >= Configure::getInstance()->specialnum2 && rnd < Configure::getInstance()->specialnum3)
		return CT_THREE_TIAO;
	if(rnd >= Configure::getInstance()->specialnum3 && rnd < Configure::getInstance()->specialnum4)
		return CT_SHUN_ZI;
	if(rnd >= Configure::getInstance()->specialnum4 && rnd < Configure::getInstance()->specialnum5)
		return CT_TONG_HUA;
	if(rnd >= Configure::getInstance()->specialnum5 && rnd < Configure::getInstance()->specialnum6)
		return CT_HU_LU;
	return 0;
}

void Table::SaveBetInfoToRedis(Player * player, int bettype)
{
	// 0 表示玩家下注总数
	// 1 天 2 地 3 玄 4 黄
	redisReply* reply = Server()->CommandV("hset %s:%d:%d %d %lld",
		game_name.c_str(),
		Configure::getInstance()->m_nServerId,
		bettype,
		player->id,
		player->m_lBetArray[bettype]);
	if (reply)
	{
		freeReplyObject(reply);
	}
	else
	{
		LOGGER(E_LOG_ERROR) << "save to redis failed!bettype = " << bettype << " betmoney = " << player->m_lBetArray[bettype];
	}
}

void Table::ClearBetInfo()
{
	for (int i = 1; i < BETNUM; i++)
	{
		redisReply* reply = Server()->CommandV("del %s:%d:%d",
			game_name.c_str(),
			Configure::getInstance()->m_nServerId,
			i);
		if (reply)
		{
			freeReplyObject(reply);
		}
		else
		{
			LOGGER(E_LOG_ERROR) << "save to redis failed!bettype = " << i;
		}
	}
}

void Table::ReceivePushCardType(const std::vector<int>& cardtypes)
{
	receive_push_card_type = cardtypes;
}

struct PosKV
{
	int key;
	int value;
};

static void sortPKV(PosKV arr[], int count)
{
	for (int i = 0; i < count - 1; i++)
	{
		for (int j = 0; j < count - 1 - i; j++)
		{
			if (arr[j].value < arr[j+1].value)
			{
				PosKV tmp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = tmp;
			}
		}
	}
}

bool Table::SetResult(int64_t bankerwincount, int64_t userbankerwincount)
{
	//m_GameLogic.RandCardList(m_bTableCardArray[0], sizeof(m_bTableCardArray)/sizeof(m_bTableCardArray[0][0]));
	if (receive_push_card_type.empty() || receive_push_card_type.size() < BETNUM) //未收到php推送的牌型
	{
		m_GameLogic.RandSpecialCard(m_bTableCardArray[0], sizeof(m_bTableCardArray) / sizeof(m_bTableCardArray[0][0]), GetSpecialType());
		short maxindex = rand() % 5;
		BYTE tempCardList[5] = { 0 };
		memcpy(tempCardList, m_bTableCardArray[maxindex], 5);
		memcpy(m_bTableCardArray[maxindex], m_bTableCardArray[0], 5);
		memcpy(m_bTableCardArray[0], tempCardList, 5);
		printf(" SetResult  maxindex:%d \n", maxindex);
	}
	else
	{
		//m_GameLogic.RandSpecialCardList(m_bTableCardArray[0], sizeof(m_bTableCardArray) / sizeof(m_bTableCardArray[0][0]), receive_push_card_type);
		m_GameLogic.RandCardList(m_bTableCardArray[0], sizeof(m_bTableCardArray) / sizeof(m_bTableCardArray[0][0]));
		BYTE tmp[5][5];
		memcpy(tmp, m_bTableCardArray, sizeof(m_bTableCardArray) / sizeof(m_bTableCardArray[0][0]));
		// sort from big -> small
		bool swaped = true;
		int j = 0;
		BYTE tempCard[5] = { 0 };
		while (swaped)
		{
			swaped = false;
			j++;
			for (int i = 0; i < 5 - j; i++)
			{
				m_GameLogic.SortCardList(tmp[i], 5);
				m_GameLogic.SortCardList(tmp[i + 1], 5);
				int cmp = m_GameLogic.CompareCard(tmp[i], tmp[i + 1], 5);
				if (cmp == 1)
				{
					memcpy(tempCard, tmp[i], 5);
					memcpy(tmp[i], tmp[i + 1], 5);
					memcpy(tmp[i + 1], tempCard, 5);
					swaped = true;
				}
			}
		}
		if (receive_push_card_type[0] == 0) //庄家对所有闲家均输
		{
			BYTE tempCard[5] = { 0 };
			memcpy(tempCard, tmp[4], 5);
			memcpy(tmp[4], tmp[0], 5);
			memcpy(tmp[0], tempCard, 5);
			memcpy(m_bTableCardArray, tmp, sizeof(tmp) / sizeof(tmp[0][0]));
		}
		else if (receive_push_card_type[0] == 2) // 庄家对所有闲家均赢
		{
			memcpy(m_bTableCardArray, tmp, sizeof(tmp) / sizeof(tmp[0][0]));
		}
		else
		{
			PosKV kv[5];
			for (int i = 0; i < 5; i++)
			{
				PosKV pkv;
				pkv.key = i;
				pkv.value = receive_push_card_type[i];
				kv[i] = pkv;
			}
			sortPKV(kv, 5);
			for (int i = 0; i < 5; i++)
			{
				int key = kv[i].key;
				memcpy(m_bTableCardArray[key], tmp[i], 5);
			}
		}
	}
	
	int i, j;
	for(i = 0; i < 5; i++)
	{
		BYTE cbTempCardData[5];
		memcpy(cbTempCardData,this->m_bTableCardArray[i],sizeof(BYTE)*5);
		m_GameLogic.SortCardList(cbTempCardData, 5);
		m_CardResult[i].type = m_GameLogic.GetCardType(cbTempCardData, 5);
		m_CardResult[i].mul = m_GameLogic.GetTimes(cbTempCardData, 5);
		short temptype = m_CardResult[i].type;
		if(temptype == CT_TIE_ZHI || temptype == CT_TONG_HUA_SHUN || temptype == CT_KING_TONG_HUA_SHUN)
			return false;
	}

	for(j = 0; j < GAME_PLAYER; ++j)
	{
		Player *player = this->player_array[j];
		if (player==NULL) continue;

		player->m_lTempScore = 0;
	}
	Player* banker = NULL;
	if(bankersid >=0)
		banker = this->player_array[bankersid];

	if(banker == NULL)
	{
		_LOG_ERROR_("SetResult: banker is NULL bankersid:%d bankeruid:%d\n", bankersid, bankeruid);
		return true;
	}

	int64_t lSystemScoreCount = 0;
	BYTE BMul = this->m_CardResult[0].mul;
	BYTE cbBankerCardData[5];
	memcpy(cbBankerCardData,this->m_bTableCardArray[0],sizeof(BYTE)*5);
	m_GameLogic.SortCardList(cbBankerCardData, 5);
	for(i = 1; i < 5; ++i)
	{
		BYTE OtherMul = this->m_CardResult[i].mul;
		short isBankerWin = 2;
		if(BMul == OtherMul)
		{
			BYTE cbTempCardData[5];
			memcpy(cbTempCardData,this->m_bTableCardArray[i],sizeof(BYTE)*5);
			m_GameLogic.SortCardList(cbTempCardData, 5);
			if(this->m_GameLogic.CompareCard(cbBankerCardData, cbTempCardData, 5)==1)
			{
				isBankerWin = 1;
			}
			else if (this->m_GameLogic.CompareCard(cbBankerCardData, cbTempCardData, 5)==0)
			{
				isBankerWin = 0;
			}
		}

		if(OtherMul > BMul)
			isBankerWin = 1;

		for(j = 0; j < GAME_PLAYER; ++j)
		{
			Player *player = this->player_array[j];
			if (player==NULL) continue;

			if (player->id == this->bankeruid) continue;
			
			if(player->m_lBetArray[i] != 0)
			{
				if(isBankerWin == 2)
				{
					banker->m_lTempScore += player->m_lBetArray[i] * BMul;
					player->m_lTempScore -= player->m_lBetArray[i] * BMul;
				}
				else if(isBankerWin == 1)
				{
					banker->m_lTempScore -= player->m_lBetArray[i] * OtherMul;
					player->m_lTempScore += player->m_lBetArray[i] * OtherMul;
				}
			}
		}
	}

	for(j = 0; j < GAME_PLAYER; ++j)
	{
		Player *player = this->player_array[j];
		if (player==NULL) continue;
		
		if(player->source == 30)
		{
			lSystemScoreCount += player->m_lTempScore;
		}
	}

	Cfg* coincfg = CoinConf::getInstance()->getCoinCfg();
	//机器人当莊
	if(banker->source == 30)
	{
		int64_t tempScore = lSystemScoreCount + bankerwincount;
		if(tempScore >= coincfg->lowerlimit && tempScore <= coincfg->upperlimit)
			return true;

		if(tempScore < coincfg->lowerlimit)
		{
			//系统赢
			if(lSystemScoreCount >= 0)
				return true;

			return false;
		}

		if(tempScore > coincfg->upperlimit)
		{
			//系统输
			if(lSystemScoreCount <= 0)
				return true;
			
			return false;
		}
	}
	//用户当莊
	else
	{
		int64_t tempScore = banker->m_lTempScore + userbankerwincount;
		int64_t tempSysScore = lSystemScoreCount + bankerwincount;
		//这里要保证系统不输
		if(tempSysScore >= coincfg->lowerlimit)
		{
			if(tempScore >= coincfg->bankerlowerlimit && tempScore <= coincfg->bankerupperlimit)
				return true;

			//说明用户已经赢了很多
			if(tempScore > coincfg->bankerupperlimit)
			{
				if(banker->m_lTempScore <= 0)
					return true;
			}

			if(tempScore < coincfg->bankerlowerlimit)
			{
				if(banker->m_lTempScore >= 0)
					return true;
			}
			//要确保系统不输
			if(m_nDespatchNum > 50)
				return true;
			else
				return false;
		}
		else
		{
			if(lSystemScoreCount >= 0)
				return true;
			return false;
		}
	}

	return true;
	
}

bool Table::SysSetResult(short result)
{
	m_GameLogic.RandSpecialCard(m_bTableCardArray[0], sizeof(m_bTableCardArray)/sizeof(m_bTableCardArray[0][0]), GetSpecialType());
	short maxindex = rand()%5;
	BYTE tempCardList[5] = {0};
	memcpy(tempCardList, m_bTableCardArray[maxindex], 5);
	memcpy(m_bTableCardArray[maxindex], m_bTableCardArray[0], 5);
	memcpy(m_bTableCardArray[0], tempCardList, 5);
	printf(" SysSetResult  maxindex:%d \n",maxindex);
	int i, j;
	for(i = 0; i < 5; i++)
	{
		BYTE cbTempCardData[5];
		memcpy(cbTempCardData,this->m_bTableCardArray[i],sizeof(BYTE)*5);
		m_GameLogic.SortCardList(cbTempCardData, 5);
		m_CardResult[i].type = m_GameLogic.GetCardType(cbTempCardData, 5);
		m_CardResult[i].mul = m_GameLogic.GetTimes(cbTempCardData, 5);
		short temptype = m_CardResult[i].type;
		if(temptype == CT_TIE_ZHI || temptype == CT_TONG_HUA_SHUN || temptype == CT_KING_TONG_HUA_SHUN)
			return false;
	}

	if (result < 1 || result > 6)
		return true;

	BYTE cbBankerCardData[5];
	memcpy(cbBankerCardData,this->m_bTableCardArray[0],sizeof(BYTE)*5);
	m_GameLogic.SortCardList(cbBankerCardData, 5);

	//5是庄家全赢
	if(result == 5)
	{
		for(i = 1; i < 5; ++i)
		{
			BYTE cbTempCardData[5];
			memcpy(cbTempCardData,this->m_bTableCardArray[i],sizeof(BYTE)*5);
			m_GameLogic.SortCardList(cbTempCardData, 5);
			if(this->m_GameLogic.CompareCard(cbBankerCardData, cbTempCardData, 5) == 1)
			{
				return false;
			}
		}
		if(m_CardResult[0].mul < Configure::getInstance()->bankerwintype)
			return false;
		return true;
	}

	//庄家全陪
	if(result == 6)
	{
		for(i = 1; i < 5; ++i)
		{
			BYTE cbTempCardData[5];
			memcpy(cbTempCardData,this->m_bTableCardArray[i],sizeof(BYTE)*5);
			m_GameLogic.SortCardList(cbTempCardData, 5);
			if(this->m_GameLogic.CompareCard(cbBankerCardData, cbTempCardData, 5) != 1)
			{
				return false;
			}
		}
		return true;
	}

	BYTE cbTempCardData[5];
	memcpy(cbTempCardData,this->m_bTableCardArray[result],sizeof(BYTE)*5);
	m_GameLogic.SortCardList(cbTempCardData, 5);
	if(this->m_GameLogic.CompareCard(cbBankerCardData, cbTempCardData, 5)==1)
	{
		if(m_CardResult[0].mul > Configure::getInstance()->bankerlosetype)
			return false;
		if(m_CardResult[result].mul < Configure::getInstance()->areawintype)
			return false;
		return true;
	}

	return false;
	
}

bool Table::SysSetJackPotResult(short jackpotindex, short jackcardtype)
{
	if(jackpotindex < 1 || jackpotindex > 5 || jackcardtype < 8 || jackcardtype > 10)
		return false;
	
	m_GameLogic.RandSpecialCard(m_bTableCardArray[0], sizeof(m_bTableCardArray)/sizeof(m_bTableCardArray[0][0]), jackcardtype);
	int i, j;

	//等于5说明就是下标为0的最大，所以不用处理
	if(jackpotindex != 5)
	{
		BYTE TempCardData[5];
		memcpy(TempCardData,this->m_bTableCardArray[0],sizeof(BYTE)*5);
		memcpy(this->m_bTableCardArray[0], this->m_bTableCardArray[jackpotindex], sizeof(BYTE)*5);
		memcpy(this->m_bTableCardArray[jackpotindex], TempCardData, sizeof(BYTE)*5);
		for(i = 0; i < 5; i++)
		{
			BYTE cbTempCardData[5];
			memcpy(cbTempCardData,this->m_bTableCardArray[i],sizeof(BYTE)*5);
			m_GameLogic.SortCardList(cbTempCardData, 5);
			m_CardResult[i].type = m_GameLogic.GetCardType(cbTempCardData, 5);
			m_CardResult[i].mul = m_GameLogic.GetTimes(cbTempCardData, 5);
			short temptype = m_CardResult[i].type;
			if(jackpotindex != i)
			{
				if(temptype == CT_TIE_ZHI || temptype == CT_TONG_HUA_SHUN || temptype == CT_KING_TONG_HUA_SHUN)
					return false;
			}
		}
	}
	else
	{
		for(i = 0; i < 5; i++)
		{
			BYTE cbTempCardData[5];
			memcpy(cbTempCardData,this->m_bTableCardArray[i],sizeof(BYTE)*5);
			m_GameLogic.SortCardList(cbTempCardData, 5);
			m_CardResult[i].type = m_GameLogic.GetCardType(cbTempCardData, 5);
			m_CardResult[i].mul = m_GameLogic.GetTimes(cbTempCardData, 5);
			short temptype = m_CardResult[i].type;
			if(jackpotindex != 0)
			{
				if(temptype == CT_TIE_ZHI || temptype == CT_TONG_HUA_SHUN || temptype == CT_KING_TONG_HUA_SHUN)
					return false;
			}
		}
	}
	return true;	
}

bool Table::SetLoserResult(Player* loser)
{
	if(loser == NULL)
		return false;

	m_GameLogic.RandSpecialCard(m_bTableCardArray[0], sizeof(m_bTableCardArray)/sizeof(m_bTableCardArray[0][0]), GetSpecialType());
	short maxindex = rand()%5;
	BYTE tempCardList[5] = {0};
	memcpy(tempCardList, m_bTableCardArray[maxindex], 5);
	memcpy(m_bTableCardArray[maxindex], m_bTableCardArray[0], 5);
	memcpy(m_bTableCardArray[0], tempCardList, 5);
	printf(" SetLoserResult  maxindex:%d \n",maxindex);
	int i, j;
	for(i = 0; i < 5; i++)
	{
		BYTE cbTempCardData[5];
		memcpy(cbTempCardData,this->m_bTableCardArray[i],sizeof(BYTE)*5);
		m_GameLogic.SortCardList(cbTempCardData, 5);
		m_CardResult[i].type = m_GameLogic.GetCardType(cbTempCardData, 5);
		m_CardResult[i].mul = m_GameLogic.GetTimes(cbTempCardData, 5);
		short temptype = m_CardResult[i].type;
		if(temptype == CT_TIE_ZHI || temptype == CT_TONG_HUA_SHUN || temptype == CT_KING_TONG_HUA_SHUN)
			return false;
	}

	for(j = 0; j < GAME_PLAYER; ++j)
	{
		Player *player = this->player_array[j];
		if (player==NULL) continue;

		player->m_lTempScore = 0;
	}
	Player* banker = NULL;
	if(bankersid >=0)
		banker = this->player_array[bankersid];

	if(banker == NULL)
	{
		_LOG_ERROR_("SetLoserResult: banker is NULL bankersid:%d bankeruid:%d\n", bankersid, bankeruid);
		return true;
	}

	int64_t lSystemScoreCount = 0;
	BYTE cbBankerCardData[5];
	memcpy(cbBankerCardData,this->m_bTableCardArray[0],sizeof(BYTE)*5);
	m_GameLogic.SortCardList(cbBankerCardData, 5);
	BYTE BMul = this->m_CardResult[0].mul;
	for(i = 1; i < 5; ++i)
	{
		BYTE OtherMul = this->m_CardResult[i].mul;
		short isBankerWin = 2;
		if(BMul == OtherMul)
		{
			BYTE cbTempCardData[5];
			memcpy(cbTempCardData,this->m_bTableCardArray[i],sizeof(BYTE)*5);
			m_GameLogic.SortCardList(cbTempCardData, 5);
			if(this->m_GameLogic.CompareCard(cbBankerCardData, cbTempCardData, 5)==1)
			{
				isBankerWin = 1;
			}
			else if (this->m_GameLogic.CompareCard(cbBankerCardData, cbTempCardData, 5)==0)
			{
				isBankerWin = 0;
			}
		}

		if(OtherMul > BMul)
			isBankerWin = 1;

		for(j = 0; j < GAME_PLAYER; ++j)
		{
			Player *player = this->player_array[j];
			if (player==NULL) continue;

			if (player->id == this->bankeruid) continue;
			
			if(player->m_lBetArray[i] != 0)
			{
				if(isBankerWin == 2)
				{
					banker->m_lTempScore += player->m_lBetArray[i] * BMul;
					player->m_lTempScore -= player->m_lBetArray[i] * BMul;
				}
				else if(isBankerWin == 1)
				{
					banker->m_lTempScore -= player->m_lBetArray[i] * OtherMul;
					player->m_lTempScore += player->m_lBetArray[i] * OtherMul;
				}
			}
		}
	}

	int randnum = rand()%100;

	for(j = 0; j < GAME_PLAYER; ++j)
	{
		Player *player = this->player_array[j];
		if (player==NULL) continue;
		
		if(player->id == loser->id)
		{
			if(randnum < Configure::getInstance()->randlose)
			{
				if(player->m_lTempScore < 0)
					return true;
			}
			else
				return true;
		}
	}

	return false;
	
}

void Table::GameOver()
{
	m_nDespatchNum = 0;
	int64_t bankerwincount = 0;
	int64_t playerwincount = 0;
	int64_t userbankerwincount = 0;
	CoinConf::getInstance()->getWinCount(bankerwincount, playerwincount, userbankerwincount);
// 	short resultret = CoinConf::getInstance()->getSwtichTypeResult();
// 	short jackpotindex = 0;
// 	short jackcardtype = 0;
// 	CoinConf::getInstance()->getjackpotResult(jackpotindex, jackcardtype);
// 
// 	short SysSetJackPot = 0;
// 	if(jackpotindex == 0 && resultret == 0)
// 	{
// 		if(m_lJackPot >= coincfg->jackpotswitch)
// 		{
// 			int randnum = rand()%100;
// 			if(randnum < Configure::getInstance()->randopenjack)
// 			{
// 				short openindex[5];
// 				short tempindex = 0;
// 				if(bankeruid < 1000 && bankeruid > 0)
// 					openindex[tempindex++] = 0;
// 				for(int i = 1; i < 5; ++i)
// 				{
// 					if(m_lTabBetArray[i] > 0)
// 					{
// 						//真实用户下注占比小于某个值则开奖
// 						if((m_lRealBetArray[i]*100)/m_lTabBetArray[i] < (100 - Configure::getInstance()->robotbetrate))
// 							openindex[tempindex++] = i;
// 					}
// 				}
// 
// 				if(tempindex > 1)
// 				{
// 					short tempsetindex = openindex[rand()%tempindex];
// 					printf("tempsetindex:%d tempindex:%d \n", tempsetindex, tempindex);
// 					if(tempsetindex == 0) 
// 						jackpotindex = 5;
// 					else
// 						jackpotindex = tempsetindex;
// 					jackcardtype = rand()%3 + 8;
// 					SysSetJackPot = 1;
// 				}
// 			}
// 		}
// 	}

	Player* Loser = this->getLoseUser();
	int printresult = 0;
	while(true)
	{
// 		if(resultret > 0 && resultret < 7)
// 		{
// 			if(SysSetResult(resultret))
// 			{
// 				printresult = 1;
// 				break;
// 			}
// 		}
// 		else if (Loser && Configure::getInstance()->randlose > 0 &&
// 			((Loser->id != bankeruid && Loser->m_lBetArray[0] >= Configure::getInstance()->switchbetmoney) || Loser->id == bankeruid))
// 		{
// 			if(SetLoserResult(Loser))
// 			{
// 				printresult = 2;
// 				break;
// 			}
// 		}
// 		else if (jackpotindex > 0 && jackpotindex < 6 && jackcardtype >= 8 && jackcardtype <= 10)
// 		{
// 			if(SysSetJackPotResult(jackpotindex, jackcardtype))
// 			{
// 				printresult = 3;
// 				break;
// 			}
// 		}
// 		else
// 		{
			if(SetResult(bankerwincount, userbankerwincount))
				break;
//		}
		m_nDespatchNum++;
		if(m_nDespatchNum > 100)
			break;
	}

	//_LOG_INFO_("GameOver: GameID[%s] dispatch num[%d] bet1:%ld bet2:%ld bet3:%ld bet4:%ld Loser:%d printresult:%d SysSetJackPot:%d resultret:%d bankeruid:%d\n",
	//	this->getGameID(), m_nDespatchNum, this->m_lTabBetArray[1], this->m_lTabBetArray[2], this->m_lTabBetArray[3], this->m_lTabBetArray[4], Loser ? Loser->id : 0, printresult, SysSetJackPot, resultret, bankeruid);
// 	if(SysSetJackPot == 1)
// 		printresult = 0;
	CalculateScore(printresult);
	IProcess::GameOver(this);
	ClearBetInfo();
	receive_push_card_type.clear();
}

void Table::CalculateScore(short resultret)
{
	int i,j; 
	
	short kingtongnum = 0;
	short kingtongarray[5] = {0};
	short tongnum = 0;
	short tongarray[5] = {0};
	short sitiaonum = 0;
	short sitiaoarray[5] = {0};
	for(i = 0; i < 5; i++)
	{
		BYTE cbBankerCardData[5];
		memcpy(cbBankerCardData,this->m_bTableCardArray[i],sizeof(BYTE)*5);
		m_GameLogic.SortCardList(cbBankerCardData, 5);
		m_CardResult[i].type = m_GameLogic.GetCardType(cbBankerCardData, 5);
		m_CardResult[i].mul = m_GameLogic.GetTimes(cbBankerCardData, 5);
		if(m_CardResult[i].type == CT_KING_TONG_HUA_SHUN)
			kingtongarray[kingtongnum++] = i;
		if(m_CardResult[i].type == CT_TONG_HUA_SHUN)
			tongarray[tongnum++] = i;
		if(m_CardResult[i].type == CT_TIE_ZHI)
			sitiaoarray[sitiaonum++] = i;

		char cardbuff[64]={0};
		for(j = 0; j < 5; ++j)
		{
			sprintf(cardbuff+5*j, "0x%02x ",this->m_bTableCardArray[i][j]);
		}
		_LOG_INFO_("[CalculateScore:GameOver] GameID[%s] cardarray:[%s] cardType:%d, mul:%d \n", this->getGameID(), cardbuff, m_CardResult[i].type, m_CardResult[i].mul);
	}

	Player* banker = NULL;
	if(bankersid >=0)
		banker = this->player_array[bankersid];

	if(banker == NULL)
	{
		_LOG_ERROR_("CalculateScore: banker is NULL bankersid:%d bankeruid:%d\n", bankersid, bankeruid);
		return;
	}

	//游戏记录
	ServerGameRecord &GameRecord = m_GameRecordArrary[m_nRecordLast];

	BYTE cbBankerCardData[5];
	memcpy(cbBankerCardData,this->m_bTableCardArray[0],sizeof(BYTE)*5);
	m_GameLogic.SortCardList(cbBankerCardData, 5);

	BYTE BMul = this->m_CardResult[0].mul;
	for(i = 1; i < 5; ++i)
	{
		BYTE OtherMul = this->m_CardResult[i].mul;
		short isBankerWin = 2;
		if(BMul == OtherMul)
		{
			BYTE cbTempCardData[5];
			memcpy(cbTempCardData,this->m_bTableCardArray[i],sizeof(BYTE)*5);
			m_GameLogic.SortCardList(cbTempCardData, 5);
			if(this->m_GameLogic.CompareCard(cbBankerCardData, cbTempCardData, 5) == 1)
			{
				isBankerWin = 1;
			}
			else if (this->m_GameLogic.CompareCard(cbBankerCardData, cbTempCardData, 5) == 0)
			{
				isBankerWin = 0;
			}
		}

		if(OtherMul > BMul)
			isBankerWin = 1;

		if(isBankerWin==2)
		{
			if(i == 1)
				GameRecord.cbTian=20;
			if(i == 2)
				GameRecord.cbDi=20;
			if(i == 3)
				GameRecord.cbXuan=20;
			if(i == 4)
				GameRecord.cbHuang=20;
		}
		else if (isBankerWin==1)
		{
			if(i == 1)
				GameRecord.cbTian=21;
			if(i == 2)
				GameRecord.cbDi=21;
			if(i == 3)
				GameRecord.cbXuan=21;
			if(i == 4)
				GameRecord.cbHuang=21;
		}
		else if (isBankerWin==0)
		{
			if(i == 1)
				GameRecord.cbTian=22;
			if(i == 2)
				GameRecord.cbDi=22;
			if(i == 3)
				GameRecord.cbXuan=22;
			if(i == 4)
				GameRecord.cbHuang=22;
		}

		for(j = 0; j < GAME_PLAYER; ++j)
		{
			Player *player = this->player_array[j];
			if (player==NULL) continue;

			if (player->id == this->bankeruid) continue;
			
			if(player->m_lBetArray[i] != 0)
			{
				if(isBankerWin==2)
				{
					banker->m_lWinScore += player->m_lBetArray[i] * BMul;
					banker->m_lResultArray[i] += player->m_lBetArray[i] * BMul;
					//player->m_lWinScore -= player->m_lBetArray[i] * BMul;
					player->m_lLoseScore -= player->m_lBetArray[i] * BMul;
					player->m_lResultArray[i] -= player->m_lBetArray[i] * BMul;
					this->m_lBankerWinScoreCount += player->m_lBetArray[i] * BMul;
				}
				else if(isBankerWin==1)
				{
					banker->m_lWinScore -= player->m_lBetArray[i] * OtherMul;
					banker->m_lResultArray[i] -= player->m_lBetArray[i] * OtherMul;
					player->m_lWinScore += player->m_lBetArray[i] * OtherMul;
					player->m_lResultArray[i] += player->m_lBetArray[i] * OtherMul;
					this->m_lBankerWinScoreCount -= player->m_lBetArray[i] * OtherMul;
				}
			}
		}
	}

	if(banker->m_lWinScore > 0)
	{
		GameRecord.cbBanker=21;
	}
	else if (banker->m_lWinScore == 0)
	{
		GameRecord.cbBanker=22;
	}
	else
	{
		GameRecord.cbBanker=20;
	}
	//移动下标
	m_nRecordLast = (m_nRecordLast+1) % MAX_SCORE_HISTORY;


	int64_t lsysrobotcount = 0;
	int64_t tempjackpot = 0;
	for(j = 0; j < GAME_PLAYER; ++j)
	{
		Player *player = this->player_array[j];
		if (player==NULL) continue;
		
		player->m_lWinScore += player->m_lLoseScore;
	
		if (player->m_lWinScore > 0)
		{
			GameUtil::CalcSysWinMoney(player->m_lWinScore, player->tax, this->m_nTax);
			if (E_MSG_SOURCE_ROBOT == player->source)
			{
				int64_t lRobotWin = player->m_lWinScore + player->tax;
				lsysrobotcount += lRobotWin;
				if (!RedisLogic::AddRobotWin(Tax(), player->pid, Configure::getInstance()->m_nServerId, (int)lRobotWin))
					LOGGER(E_LOG_DEBUG) << "OperationRedis::AddRobotWin Error, pid=" << player->pid << ", server_id=" << Configure::getInstance()->m_nServerId << ", win=" << lRobotWin;
			}
			else
			{
				tempjackpot += player->tax;
				if (player->id == bankeruid)
					this->m_lBankerWinScoreCount -= player->tax;

				if (!RedisLogic::UpdateTaxRank(Tax(), player->pid, GameConfigure()->m_nServerId, GAME_ID, GameConfigure()->m_nLevel, player->tid, player->id, player->tax))
					LOGGER(E_LOG_DEBUG) << "OperationRedis::GenerateTip Error, pid=" << player->pid << ", server_id=" << Configure::getInstance()->m_nServerId << ", gameid=" << GAME_ID << ", level=" << Configure::getInstance()->m_nLevel << ", id=" << player->id << ", Tax=" << player->tax;
			}
		}
		else //机器人输钱也需要算
		{
			if (player->source == E_MSG_SOURCE_ROBOT)
			{
				lsysrobotcount += player->m_lWinScore;
			}
		}

		player->m_lMoney += player->m_lWinScore + player->m_lReturnScore;
	}

	int64_t bankerwincount = 0;
	if(banker->source != E_MSG_SOURCE_ROBOT)
	{
		bankerwincount = banker->m_lWinScore + banker->tax;
	}

// 	m_lJackPot += tempjackpot;
// 
// 	if(kingtongnum != 0 || tongnum != 0 || sitiaonum != 0)
// 	{
// 		clearJackPoter();
// 	}
// 
// 	bool hasinsertJackPot = false;
// 	short tempWinIndex = -1;
// 	int64_t tempRewad = 0;
// 	if(kingtongnum != 0)
// 	{
// 		int64_t tempRewad = m_lJackPot*5/10;
// 		m_lJackPot -= tempRewad;
// 		m_lGivePlayerJackPot = tempRewad;
// 		if(kingtongarray[0] == 0)
// 		{
// 			if(kingtongnum == 1)
// 				banker->m_lRewardCoin += tempRewad;
// 			else
// 				banker->m_lRewardCoin += tempRewad/2;
// 			m_lGetJackPotArea[0] += banker->m_lRewardCoin;
// 			tempRewad -= banker->m_lRewardCoin;
// 			this->pushJackPoter(banker);
// 			tempWinIndex = 0;
// 			hasinsertJackPot = true;
// 		}
// 		int64_t tempbetCount = 0;
// 		for(i = 0; i < kingtongnum; ++i)
// 		{
// 			if(kingtongarray[i] != 0)
// 			{
// 				tempbetCount += this->m_lTabBetArray[kingtongarray[i]];
// 			}
// 		}
// 		for(i = 1; i < 5; ++i)
// 		{
// 			bool willhand = false;
// 			for(int m = 0; m < kingtongnum; ++m)
// 			{
// 				if(kingtongarray[m] == i)
// 				{
// 					willhand = true;
// 					break;
// 				}
// 			}
// 			if(!willhand)
// 				continue;
// 			bool btmepflag = false;
// 			for(j = 0; j < GAME_PLAYER; ++j)
// 			{
// 				Player *player = this->player_array[j];
// 				if (player==NULL) continue;
// 
// 				if (player->id == this->bankeruid) continue;
// 				
// 				if(player->m_lBetArray[i] != 0)
// 				{
// 					player->m_lRewardCoin += tempRewad * player->m_lBetArray[i]/tempbetCount;
// 					m_lGetJackPotArea[i] += tempRewad * player->m_lBetArray[i]/tempbetCount;
// 					if(!hasinsertJackPot)
// 					{
// 						this->pushJackPoter(player);
// 						btmepflag = true;
// 					}
// 				}
// 			}
// 			if(btmepflag)
// 			{
// 				tempWinIndex = i;
// 				hasinsertJackPot = true;
// 			}
// 		}
// 	}
// 
// 	if(tongnum != 0)
// 	{
// 		int64_t tempRewad = m_lJackPot * 4/10;
// 		m_lJackPot -= tempRewad;
// 		m_lGivePlayerJackPot = tempRewad;
// 		if(tongarray[0] == 0)
// 		{
// 			if(tongnum == 1)
// 				banker->m_lRewardCoin += tempRewad;
// 			else
// 				banker->m_lRewardCoin += tempRewad/2;
// 			tempRewad -= banker->m_lRewardCoin;
// 			m_lGetJackPotArea[0] += banker->m_lRewardCoin;
// 			if(!hasinsertJackPot)
// 			{
// 				this->pushJackPoter(banker);
// 				hasinsertJackPot = true;
// 				tempWinIndex = 0;
// 
// 			}
// 		}
// 		int64_t tempbetCount = 0;
// 		for(i = 0; i < tongnum; ++i)
// 		{
// 			if(tongarray[i] != 0)
// 			{
// 				tempbetCount += this->m_lTabBetArray[tongarray[i]];
// 			}
// 		}
// 		for(i = 1; i < 5; ++i)
// 		{
// 			bool willhand = false;
// 			for(int m = 0; m < tongnum; ++m)
// 			{
// 				if(tongarray[m] == i)
// 				{
// 					willhand = true;
// 					break;
// 				}
// 			}
// 			if(!willhand)
// 				continue;
// 			bool btmepflag = false;
// 			for(j = 0; j < GAME_PLAYER; ++j)
// 			{
// 				Player *player = this->player_array[j];
// 				if (player==NULL) continue;
// 
// 				if (player->id == this->bankeruid) continue;
// 				
// 				if(player->m_lBetArray[i] != 0)
// 				{
// 					player->m_lRewardCoin += tempRewad * player->m_lBetArray[i]/tempbetCount;
// 					m_lGetJackPotArea[i] += tempRewad * player->m_lBetArray[i]/tempbetCount;
// 					if(!hasinsertJackPot)
// 					{
// 						this->pushJackPoter(player);
// 						btmepflag = true;
// 					}
// 				}
// 			}
// 			if(btmepflag)
// 			{
// 				tempWinIndex = i;
// 				hasinsertJackPot = true;
// 			}
// 		}
// 	}
// 
// 	if(sitiaonum != 0)
// 	{
// 		int64_t tempRewad = m_lJackPot * 3/10;
// 		m_lJackPot -= tempRewad;
// 		m_lGivePlayerJackPot = tempRewad;
// 		if(sitiaoarray[0] == 0)
// 		{
// 			if(sitiaonum == 1)
// 				banker->m_lRewardCoin += tempRewad;
// 			else
// 				banker->m_lRewardCoin += tempRewad/2;
// 			tempRewad -= banker->m_lRewardCoin;
// 			m_lGetJackPotArea[0] += banker->m_lRewardCoin;
// 			if(!hasinsertJackPot)
// 			{
// 				this->pushJackPoter(banker);
// 				hasinsertJackPot = true;
// 				tempWinIndex = 0;
// 			}
// 		}
// 		int64_t tempbetCount = 0;
// 		for(i = 0; i < sitiaonum; ++i)
// 		{
// 			if(sitiaoarray[i] != 0)
// 			{
// 				tempbetCount += this->m_lTabBetArray[sitiaoarray[i]];
// 			}
// 		}
// 		for(i = 1; i < 5; ++i)
// 		{
// 			bool willhand = false;
// 			for(int m = 0; m < sitiaonum; ++m)
// 			{
// 				if(sitiaoarray[m] == i)
// 				{
// 					willhand = true;
// 					break;
// 				}
// 			}
// 			if(!willhand)
// 				continue;
// 			bool btmepflag = false;
// 			for(j = 0; j < GAME_PLAYER; ++j)
// 			{
// 				Player *player = this->player_array[j];
// 				if (player==NULL) continue;
// 
// 				if (player->id == this->bankeruid) continue;
// 				
// 				if(player->m_lBetArray[i] != 0)
// 				{
// 					player->m_lRewardCoin += tempRewad * player->m_lBetArray[i]/tempbetCount;
// 					m_lGetJackPotArea[i] += tempRewad * player->m_lBetArray[i]/tempbetCount;
// 					if(!hasinsertJackPot)
// 					{
// 						this->pushJackPoter(player);
// 						btmepflag = true;
// 					}
// 				}
// 			}
// 			if(btmepflag)
// 			{
// 				tempWinIndex = i;
// 				hasinsertJackPot = true;
// 			}
// 		}
// 	}
	
// 	if(resultret > 0)
// 	{
// 		int notInsertRedis = 0;
// 	}
// 	else
// 	{
// 		if(lsysrobotcount != 0)
// 		{
			CoinConf::getInstance()->setWinCount(lsysrobotcount, 0, bankerwincount);
// 		}
// 	}
// 	CoinConf::getInstance()->setJackPot(m_lJackPot);
//	_LOG_INFO_("Current GameID[%s] m_lJackPot:%ld\n", this->getGameID(), m_lJackPot);

// 	if(hasinsertJackPot)
// 	{
// 		JackPotList.sort(compare_reward);
// 		m_bJackPotCard[0] = m_bTableCardArray[tempWinIndex][0];
// 		m_bJackPotCard[1] = m_bTableCardArray[tempWinIndex][1];
// 		m_bJackPotCard[2] = m_bTableCardArray[tempWinIndex][2];
// 		m_bJackPotCard[3] = m_bTableCardArray[tempWinIndex][3];
// 		m_bJackPotCard[4] = m_bTableCardArray[tempWinIndex][4];
// 		m_tRewardTime = time(NULL);
// 		for(j = 0; j < GAME_PLAYER; ++j)
// 		{
// 			Player *player = this->player_array[j];
// 			if (player==NULL) continue;
// 			
// 			if(player->m_lRewardCoin > 0)
// 				player->m_lMoney += player->m_lRewardCoin;
// 		}
// 	}
	++bankernum;
}

void Table::pushJackPoter(Player* player)
{
	JackPotPlayer* pJackPoter = new JackPotPlayer();
	pJackPoter->id = player->id;
	pJackPoter->m_lRewardCoin = player->m_lRewardCoin;
	strncpy(pJackPoter->name, player->name, sizeof(pJackPoter->name));
	pJackPoter->name[sizeof(pJackPoter->name) - 1] = '\0';
	strncpy(pJackPoter->json, player->json, sizeof(pJackPoter->json));
	pJackPoter->json[sizeof(pJackPoter->json) - 1] = '\0';
	JackPotList.push_back(pJackPoter);
}

void Table::clearJackPoter()
{
	list<JackPotPlayer*>::iterator it;
	it = JackPotList.begin();
	while(it != JackPotList.end())
	{
		printf("id:%d\n", (*it)->id);
		delete *it;
		it++;
	}
	JackPotList.clear();
}

Player* Table::getLoseUser()
{
	if(m_vecBlacklist.size() <= 0)
		return NULL;
	int64_t maxBetCoin = 0;
	int retuid = 0;
	Player* retplayer = NULL;
	Player* banker = NULL;
	if(bankersid >=0)
		banker = this->player_array[bankersid];

	if(banker != NULL)
	{
		vector<int>::iterator it = find(m_vecBlacklist.begin(), m_vecBlacklist.end(), banker->id);
		if (it != m_vecBlacklist.end())
			return banker;
	}

	for(int j = 0; j < GAME_PLAYER; ++j)
	{
		Player *player = this->player_array[j];
		if (player==NULL) continue;
		if (player->source == 30) continue;
		if (player->m_lBetArray[0] == 0) continue;
		vector<int>::iterator it = find(m_vecBlacklist.begin(), m_vecBlacklist.end(), player->id);
		if (it != m_vecBlacklist.end() && player->m_lBetArray[0] > maxBetCoin)
		{
			maxBetCoin = player->m_lBetArray[0];
			retplayer = player;
		}
	}
	return retplayer;
}


void Table::reloadCfg()
{
	CoinConf* coinCalc = CoinConf::getInstance();
	Cfg* coincfg = coinCalc->getCoinCfg();
	m_nTax = coincfg->tax;
	m_nLimitCoin = coincfg->limitcoin;
	m_nRetainCoin = coincfg->retaincoin;
// 	int64_t ret = coinCalc->getJackPot();
// 	if(ret > 0)
// 		m_lJackPot = ret;
	m_vecBlacklist.clear();
	coinCalc->getBlackList(m_vecBlacklist);
}

void Table::checkClearCfg()
{
	time_t t;       
    time(&t);               
    struct tm* tp= localtime(&t);
}



//==================================TableTimer=================================================
void Table::startIdleTimer(int timeout)
{
	timer.startIdleTimer(timeout);
}

void Table::stopIdleTimer()
{
	timer.stopIdleTimer();
}

void Table::startBetTimer(int timeout)
{
	timer.startBetTimer(timeout);
}

void Table::stopBetTimer()
{
	timer.stopBetTimer();
}

void Table::startOpenTimer(int timeout)
{
	timer.startOpenTimer(timeout);
}

void Table::stopOpenTimer()
{
	timer.stopOpenTimer();
}

bool Table::EnterSeat(int seatid, Player *player)
{
	if (seatid > MAX_SEAT_NUM || seatid == 0)
	{
		_LOG_ERROR_("Table::EnterSeat : Seatid=[%d] is overflow.\n", seatid);
		return false;
	}

	if (m_SeatidArray[seatid - 1] != NULL)
	{
		_LOG_ERROR_("Table::EnterSeat : Seatid=[%d] be seated uid=[%d].\n", seatid, m_SeatidArray[seatid - 1]->id);
		return false;
	}

	if (player->m_seatid != 0)
	{
		LeaveSeat(player->m_seatid);
	}


	player->m_seatid = seatid;
	m_SeatidArray[seatid - 1] = player;
	_LOG_INFO_("Table::EnterSeat : successed uid=[%d] Seatid=[%d] seated .\n", m_SeatidArray[seatid - 1]->id, seatid);
	return true;
}

bool Table::LeaveSeat(int seatid)
{
	if (seatid > MAX_SEAT_NUM || seatid == 0)
	{
		_LOG_ERROR_("Table::LeaveSeat : Seatid=[%d] is overflow.\n", seatid);
		return false;
	}

	if (m_SeatidArray[seatid - 1] != NULL)
	{
		_LOG_INFO_("Table::LeaveSeat : successed uid=[%d] Seatid=[%d] seated .\n", m_SeatidArray[seatid - 1]->id, seatid);
		m_SeatidArray[seatid - 1]->m_seatid = 0;
		m_SeatidArray[seatid - 1] = NULL;
		return true;
	}
	return false;
}

void Table::SendSeatInfo(Player *player)
{
	_LOG_INFO_("Table::SendSeatInfo : player->uid=[%d]\n", player->id);
	if (player == NULL)
	{
		_LOG_ERROR_("Table::SendSeatInfo : why player is null.\n");
		return;
	}

	OutputPacket response;
	response.Begin(CLIENT_MSG_REFRESH_SEATLIST, player->id);
	response.WriteShort(0);
	response.WriteString("");
	response.WriteInt(player->id);
	response.WriteInt((Configure::getInstance()->m_nServerId << 16) | this->id);

	response.WriteInt(MAX_SEAT_NUM); //一共多少桌位号

	for (int i = 0; i < MAX_SEAT_NUM; ++i)
	{
		if (m_SeatidArray[i] != NULL)
		{
			response.WriteInt(i + 1);							//座位ID
			response.WriteInt(m_SeatidArray[i]->id);		//玩家ID
			response.WriteString(m_SeatidArray[i]->name);	//玩家名称
			response.WriteString(m_SeatidArray[i]->headlink);//玩家头像url
			response.WriteInt64(m_SeatidArray[i]->m_lMoney);					//玩家金币
		}
		else {
			response.WriteInt(i + 1);      //座位ID
			response.WriteInt(0);		 //玩家ID
			response.WriteString("");	 //玩家名称
			response.WriteString("");	 //玩家头像url
			response.WriteInt64(0);		 //玩家金币
		}
	}

    for (int i = 0; i < MAX_SEAT_NUM; ++i)
    {
        if (m_SeatidArray[i] != NULL)
        {
            response.WriteString(m_SeatidArray[i]->json);      //player json
        }
        else
        {
            response.WriteString("");      //player json
        }
    }

	response.End();
	HallManager::getInstance()->sendToHall(player->m_nHallid, &response, false);
}

void Table::NotifyPlayerSeatInfo()
{
	LOGGER(E_LOG_DEBUG) << "Table::NotifyPlayerSeatInfo";
	for (int i = 0; i < GAME_PLAYER; ++i)
	{
		if (this->player_array[i])
		{
			SendSeatInfo(this->player_array[i]);
		}
	}
}