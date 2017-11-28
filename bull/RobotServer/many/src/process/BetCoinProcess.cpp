//---------------------------------------------------------------------------



#include "BetCoinProcess.h"

//---------------------------------------------------------------------------
int BetCoinProcess::doRequest(CDLSocketHandler* client, InputPacket* pPacket, Context* pt )
{
	HallHandler* clientHandler = reinterpret_cast <HallHandler*> (client);
	Player* player = PlayerManager::getInstance()->getPlayer(clientHandler->uid);
	OutputPacket packet;
	packet.Begin(CLIENT_MSG_BET_COIN, player->id);
	packet.WriteInt(player->id);
	packet.WriteInt(player->tid);
	packet.WriteShort(player->seat_id);//level
	packet.WriteShort(player->bettype);//
	packet.WriteInt64(player->betcoin);  
	packet.WriteInt(0);
	packet.WriteInt(0);
	packet.End();
	ULOGGER(E_LOG_INFO, player->id) << "bettype = " << player->bettype << " bet coin = " << player->betcoin;
	return this->send(client,&packet);
}

int BetCoinProcess::doResponse(CDLSocketHandler* client, InputPacket* inputPacket, Context* pt )
{
	int error_code = inputPacket->ReadShort();
	string error_mag = inputPacket->ReadString();

	if (error_code < 0)
	{
		LOGGER(E_LOG_ERROR) << "error code = " << error_code << " error msg = " << error_mag;
		return EXIT;
	}
	int uid = inputPacket->ReadInt();
	short status = inputPacket->ReadShort();
	int tid = inputPacket->ReadInt();
	short tstatus = inputPacket->ReadShort();
	int betid = inputPacket->ReadInt();
	short bettype = inputPacket->ReadShort();
	int64_t betmoney = inputPacket->ReadInt64();
	for (int i = 1; i < BETNUM; i++)
	{
		int64_t playerbet = inputPacket->ReadInt64();	//����Ҹ�������ע���
	}
	int64_t betermoney = inputPacket->ReadInt64();
	int64_t tabBetArray[BETNUM] = { 0 };
	for (int i = 1; i < BETNUM; i++)
	{
		tabBetArray[i] = inputPacket->ReadInt64();	//���Ӹ���������ע���
	}
	int x = inputPacket->ReadInt();
	int y = inputPacket->ReadInt();
	for (int i = 1; i < BETNUM; i++)
	{
		int64_t playerBet = inputPacket->ReadInt64();	//��¼��ң��ǻ����ˣ���������ע����
		PlayerManager::getInstance()->areaTotalBetArray[i] = tabBetArray[i] - playerBet;	//��������˸�������ע����
		LOGGER(E_LOG_DEBUG) << "bettype = " << i << " robot current total bet = " << PlayerManager::getInstance()->areaTotalBetArray[i];
	}
	ULOGGER(E_LOG_INFO, uid) << "tid = " << tid
		<< " bet type = " << bettype
		<< " bet money = " << betmoney
		;
	
	if(betid == uid)
	{
		Player* player = PlayerManager::getInstance()->getPlayer(uid);
		player->money -= betmoney;
	}
	
	return 0;

}
