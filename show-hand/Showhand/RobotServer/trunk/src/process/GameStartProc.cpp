#include "GameStartProc.h"
#include "HallHandler.h"
#include "Logger.h"
#include "Options.h"
#include "GameServerConnect.h"
#include "PlayerManager.h"
#include <string>
using namespace std;

GameStartProc::GameStartProc()
{
	this->name = "GameStartProc";
}

GameStartProc::~GameStartProc()
{

} 

int GameStartProc::doRequest(CDLSocketHandler* client, InputPacket* pPacket, Context* pt )
{

	HallHandler* clientHandler = reinterpret_cast <HallHandler*> (client);
	Player* player = PlayerManager::getInstance()->getPlayer(clientHandler->uid);
	if(player == NULL)
		return 0;
	OutputPacket requestPacket;
	requestPacket.Begin(CLIENT_MSG_START_GAME, player->id);
	requestPacket.WriteInt(player->id);
	requestPacket.WriteInt(player->tid);
	requestPacket.End();
	this->send(clientHandler, &requestPacket);
	/*printf("Send SendCardProc Packet to Server\n");
	printf("Data Send: player->id=[%d]\n", player->id);
	printf("Data Send: player->id=[%d]\n", player->id);*/
	return 0;
}

int GameStartProc::doResponse(CDLSocketHandler* clientHandler, InputPacket* inputPacket,Context* pt)  
{
	HallHandler* hallHandler = reinterpret_cast <HallHandler*> (clientHandler);
	Player* player = PlayerManager::getInstance()->getPlayer(hallHandler->uid);
	int retcode = inputPacket->ReadShort();
	string retmsg = inputPacket->ReadString();
	/*_LOG_DEBUG_("Recv GameStartProc Packet From Server\n");
	_LOG_DEBUG_("Data Recv: retcode=[%d]\n",retcode);
	_LOG_DEBUG_("Data Recv: retmsg=[%s]\n",retmsg.c_str());*/
	if(retcode == -9)
		return 0;
	if(retcode < 0 || player == NULL)
	{
		return EXIT;
	}
	int uid = inputPacket->ReadInt();
	short ustatus = inputPacket->ReadShort();
	int tid = inputPacket->ReadInt();
	short tstatus = inputPacket->ReadShort();
	int startid = inputPacket->ReadInt();
	short startstatus = inputPacket->ReadShort();
	int64_t carrycoin = inputPacket->ReadInt64();
	/*_LOG_DEBUG_("Data Recv: uid=[%d]\n",uid);
	_LOG_DEBUG_("Data Recv: ustatus=[%d]\n",ustatus);
	_LOG_DEBUG_("Data Recv: tid=[%d]\n",tid);
	_LOG_DEBUG_("Data Recv: tstatus=[%d]\n",tstatus);
	_LOG_DEBUG_("Data Recv: startid=[%d]\n",startid);
	_LOG_DEBUG_("Data Recv: startstatus=[%d]\n",startstatus);
	_LOG_DEBUG_("Data Recv: carrycoin=[%ld]\n",carrycoin);
	_LOG_DEBUG_("Data Recv: player->id=[%d]\n",player->id);*/
	player->status = tstatus;
	if(tid != player->tid)
		return EXIT;
	
	if(uid == startid)
	{
		player->status = startstatus;
		player->carrycoin = carrycoin;
	}
	else
	{
		for(int i = 0; i < GAME_PLAYER; ++i)
		{
			if(player->player_array[i].id == startid)
			{
				player->player_array[i].status = startstatus;
				player->player_array[i].carrycoin = carrycoin;
				break;
			}
		}
	}
	return 0;
}

