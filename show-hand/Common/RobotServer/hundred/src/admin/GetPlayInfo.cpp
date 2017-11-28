#include "GetPlayInfo.h"
#include "ClientHandler.h"
#include "Logger.h"
#include "Configure.h"
#include "Player.h"
#include "GameServerConnect.h"
#include "PlayerManager.h"
#include "HallHandler.h"
#include "Util.h"
#include <json/json.h>
#include "NamePool.h"
#include "RobotUtil.h"
#include <string>
using namespace std;

GetPlayInfo::GetPlayInfo()
{
	this->name = "GetPlayInfo";
}

GetPlayInfo::~GetPlayInfo()
{

} 

int GetPlayInfo::doRequest(CDLSocketHandler* client, InputPacket* pPacket, Context* pt )
{

	ClientHandler* clientHandler = reinterpret_cast <ClientHandler*> (client);
	_NOTUSED(clientHandler);
	_NOTUSED(pPacket);
	_NOTUSED(pt);
	return 0;
}

int GetPlayInfo::doResponse(CDLSocketHandler* clientHandler, InputPacket* inputPacket,Context* pt)  
{
	_NOTUSED(clientHandler);
	int cmd = inputPacket->GetCmdType();
	int retcode = inputPacket->ReadShort();
	string retmsg = inputPacket->ReadString();
	if(retcode < 0)
	{
		LOGGER(E_LOG_WARNING) << "GetPlayInfo Error retcode[" <<retcode<< "] retmsg[" <<retmsg.c_str()<< "]";
	//	_LOG_ERROR_("GetPlayInfo Error retcode[%d] retmsg[%s]\n", retcode, retmsg.c_str());
		return 0;
	}
	int tid = inputPacket->ReadInt();
	short status = inputPacket->ReadShort();
//	_LOG_INFO_("==>[GetPlayInfo] [0x%04x] tid=[%d] status=[%d]\n", cmd, tid, status);
	short countPlayer = inputPacket->ReadShort();
	short banklistcount = inputPacket->ReadShort();
	short robotNotInBanklistnum = inputPacket->ReadShort();
	short robotInBanklistnum = inputPacket->ReadShort();
	short winRobotnum = inputPacket->ReadShort();
//	_LOG_DEBUG_("[GetPlayInfo] countPlayer[%d] banklistcount[%d] robotNotInBanklistnum[%d] robotInBanklistnum[%d] winRobotnum[%d] bankernum[%d] normalrobotnum[%d] switchbetter[%d]\n",
//		countPlayer, banklistcount, robotNotInBanklistnum,robotInBanklistnum, winRobotnum, PlayerManager::getInstance()->cfg.bankernum, PlayerManager::getInstance()->cfg.normalrobotnum, PlayerManager::getInstance()->cfg.switchbetter);
	if(Configure::getInstance()->m_nLevel == 2)
	{
		if(robotNotInBanklistnum < PlayerManager::getInstance()->cfg.normalrobotnum)
		{
			productBetRobot(tid);
			productBetRobot(tid);
			productBetRobot(tid);
		}
	}

	if(Configure::getInstance()->m_nLevel == 1)
	{
		if(PlayerManager::getInstance()->cfg.switchbanker == 1 && robotInBanklistnum < PlayerManager::getInstance()->cfg.bankernum)
		{
			productBankerRobot(tid);
		}
	}

	return 0;
}

int GetPlayInfo::productBankerRobot(int tid)
{
	Player* player = new Player();
	if(player)
	{
		player->init();
		player->tid = tid;
		int playNumCount = Configure::getInstance()->bankerplaybase + rand()%Configure::getInstance()->bankerplayrand;
		player->willPlayCount = playNumCount;
		
		int totalnum = 50 + rand() % 1000;
        int winnum = (totalnum*(35 + rand() % 26)) / 100;
		short roll = 10 + rand() % 700;
		short vip = rand()%100 < 90 ? 1 : 0;

		std::string nickname;
		std::string headurl;
		int uid = 0;
		int sex = 0;
		RobotUtil::makeRobotInfo(Configure::getInstance()->m_headLink, sex, headurl, uid);
		nickname = NamePool::getInstance()->AllocName();
		strncpy(player->name, nickname.c_str(), sizeof(player->name));
		player->name[sizeof(player->name)-1] = '\0';

		Json::Value data;
		data["picUrl"] = headurl;
		data["sum"] = totalnum;
        data["win"] = winnum;
		data["lepaper"] = roll;
		data["tips"] = 0;
		data["sex"] = sex;
		data["source"] = 30;
		data["vip"] = vip;
		data["uid"] = uid;
        data["iconid"] = rand() % 6;

		player->id = PlayerManager::getInstance()->getRobotUid();
		if (player->id == -1)
			player->id = uid;

		player->money = 5000000 + rand() % 10000000;
		strcpy(player->json,data.toStyledString().c_str());
		HallHandler * handler = new HallHandler();
		if(CDLReactor::Instance()->Connect(handler,Configure::getInstance()->hall_ip,Configure::getInstance()->hall_port) < 0)
		{
			delete handler;
			delete player;
			LOGGER(E_LOG_WARNING) << "Connect BackServer error[" <<Configure::getInstance()->hall_ip<< ":" <<Configure::getInstance()->hall_port<< "]";
			//_LOG_ERROR_("Connect BackServer error[%s:%d]\n",Configure::getInstance()->hall_ip,Configure::getInstance()->hall_port);
			return -1;
		}
		handler->uid = player->id;
		PlayerManager::getInstance()->addPlayer(player->id, player);
		player->handler = handler;
		
		LOGGER(E_LOG_INFO) << "StartRobot[" <<player->id<< "] tid[" <<tid<< "]";
		player->login();
	}
	return 0;
}


int GetPlayInfo::productBetRobot(int tid)
{
	
	Player* player = new Player();
	if(player)
	{
		player->init();
		player->tid = tid;
		int playNumCount = Configure::getInstance()->playerplaybase + rand()%Configure::getInstance()->playerplayrand;
		player->willPlayCount = playNumCount;
		
		int totalnum = 50 + rand()%1000;
		int winnum = (totalnum*(35 + rand()%26))/100;
		short roll = 10 + rand()%700;
		short vip = rand()%100 < 60 ? 1 : 0;
		std::string nickname;
		std::string headurl;
		int uid = 0;
		int sex = 0;
		RobotUtil::makeRobotInfo(Configure::getInstance()->m_headLink, sex, headurl, uid);
		nickname = NamePool::getInstance()->AllocName();
		strncpy(player->name, nickname.c_str(), sizeof(player->name));
		player->name[sizeof(player->name)-1] = '\0';

		Json::Value data;
		data["picUrl"] = headurl;
		data["sum"] = totalnum;
        data["win"] = winnum;
		data["lepaper"] = roll;
		data["tips"] = 0;
		data["sex"] = sex;
		data["source"] = 30;
		data["vip"] = vip;
		data["uid"] = uid;
		data["iconid"] = rand() % 6;
		player->id = PlayerManager::getInstance()->getRobotUid();
		if (player->id == -1)
			player->id = uid;

		strcpy(player->json,data.toStyledString().c_str());
		player->money = 10000 + rand() % 90000;
		HallHandler * handler = new HallHandler();
		if(CDLReactor::Instance()->Connect(handler,Configure::getInstance()->hall_ip,Configure::getInstance()->hall_port) < 0)
		{
			delete handler;
			delete player;
			LOGGER(E_LOG_WARNING) << "Connect BackServer error[" <<Configure::getInstance()->hall_ip<< ":" <<Configure::getInstance()->hall_port<< "]";
			return -1;
		}
		handler->uid = player->id;
		PlayerManager::getInstance()->addPlayer(player->id, player);
		player->handler = handler;
		player->login();
	}
	return 0;
}