#include "AllocSvrConnect.h"
#include "Logger.h"
#include "Configure.h"
#include "BaseClientHandler.h"
#include "GameCmd.h"
#include "ProtocolServerId.h"
#include "Room.h"
#include "GameApp.h"

static AllocSvrConnect* instance = NULL;

AllocSvrConnect* AllocSvrConnect::getInstance()
{
	if(instance==NULL)
	{
		instance = new AllocSvrConnect();
	}
	return instance;
}

AllocSvrConnect::AllocSvrConnect()
{
	reportTimer = new ReportTimer();	
	reportTimer->init(this);
}

AllocSvrConnect::~AllocSvrConnect()
{
	_LOG_ERROR_( "[AllocSvrConnect::~AllocSvrConnect] AllocSvrConnect was delete\n" );
}

//上报服务器Info
int AllocSvrConnect::reportSeverInfo()
{
	Room* room = Room::getInstance();
	OutputPacket ReportPkg;
	ReportPkg.Begin(GMSERVER_REPORT);
	ReportPkg.WriteShort(Configure::getInstance()->server_id); 
	ReportPkg.WriteString(Configure::getInstance()->m_sListenIp);
	ReportPkg.WriteShort(Configure::getInstance()->m_nPort);

	ReportPkg.WriteShort(Configure::getInstance()->level); 

	ReportPkg.WriteByte(room->getStatus()); 
	ReportPkg.WriteByte(Configure::getInstance()->server_priority); 
	
	ReportPkg.WriteInt(room->getMaxUsers());
	ReportPkg.WriteShort(room->getMaxTables()); 
	ReportPkg.WriteInt(room->getCurrUsers());
	Table* tableArray[512] = {0};
	int currTable = 0;
	for(int i=0; i < room->getMaxTables() ;++i)
	{
		Table *table = &room->tables[i];
		if(!table->isEmpty())
		{
			int tid = (Configure::getInstance()->server_id << 16) | table->id;
			tableArray[currTable++] = table;
		}
	}
	ReportPkg.WriteShort(currTable);
	for(int j = 0; j < currTable; j++)
	{
		Table *table = tableArray[j];
		int tid = (Configure::getInstance()->server_id << 16) | table->id;
		ReportPkg.WriteInt(tid);
		ReportPkg.WriteShort(table->m_nType);
		ReportPkg.WriteShort(table->m_nCountPlayer);
		if (table->m_nCountPlayer > 0)
		{
			for(int i = 0; i < table->m_bMaxNum; ++i)
			{
				if(table->player_array[i])
				{
					ReportPkg.WriteInt64(table->player_array[i]->m_lMoney);
					break;
				}
			}
		}
		else 
			ReportPkg.WriteInt64(0);
	}
	ReportPkg.End();
	return Send(&ReportPkg,false);
}

//更新服务器Info：在线人数
int AllocSvrConnect::updateSeverInfo()
{
	Room* room = Room::getInstance();
	OutputPacket ReportPkg;
	ReportPkg.Begin(GMSERVER_UPDATE);
	ReportPkg.WriteShort(Configure::getInstance()->server_id); 
	ReportPkg.WriteShort(Configure::getInstance()->level); 
	ReportPkg.WriteInt(room->getMaxUsers());
	ReportPkg.WriteShort(room->getMaxTables()); 
	ReportPkg.WriteInt(room->getCurrUsers());
	ReportPkg.WriteShort(room->getCurrTables());     
	ReportPkg.WriteShort(room->getIPhoneUsers()); 
	ReportPkg.WriteShort(room->getAndroidUsers()); 
	ReportPkg.WriteShort(room->getIPadUsers()); 
	ReportPkg.WriteShort(room->getRobotUsers());
	

	ReportPkg.End();

	//MySqlConnect* connect = MySqlConnect::getInstance();
	//connect->Send( &ReportPkg );
	return Send(&ReportPkg,false);


}

//当桌子上面人数有变化的时候就通知Alloc更新当前桌子的信息
int AllocSvrConnect::updateTableUserCount(Table* table)
{
	OutputPacket ReportPkg;
	ReportPkg.Begin(GMSERVER_SET_TABLEUSER);
	ReportPkg.WriteShort(Configure::getInstance()->server_id);
	int tid = Configure::getInstance()->server_id<< 16 | table->id;
	ReportPkg.WriteInt(tid); 
	ReportPkg.WriteShort(table->m_nType); 
	ReportPkg.WriteShort(table->m_nCountPlayer); 
	if(table->m_nCountPlayer > 0)
	{
		for(int i = 0; i < table->m_bMaxNum; ++i)
		{
			if(table->player_array[i])
			{
				ReportPkg.WriteInt64(table->player_array[i]->m_lMoney);
				break;
			}
		}
	}
	else
		ReportPkg.WriteInt64(0);
	ReportPkg.End();
	return Send(&ReportPkg,false);
}

int AllocSvrConnect::updateTableStatus(Table* table)
{
	OutputPacket ReportPkg;
	ReportPkg.Begin(GMSERVER_SET_TABLE_STATUS);
	ReportPkg.WriteShort(Configure::getInstance()->server_id);
	int tid = Configure::getInstance()->server_id<< 16 | table->id;
	ReportPkg.WriteInt(tid); 
	ReportPkg.WriteShort(table->m_nType); 
	ReportPkg.WriteShort(table->m_nStatus); 
	ReportPkg.End();
	return Send(&ReportPkg,false);
}

//User离开上报
int AllocSvrConnect::userLeaveGame(Player* player, int uid)
{
	OutputPacket ReportPkg;
	ReportPkg.Begin(GMSERVER_LEAVE);
	ReportPkg.WriteInt(player ? player->id : uid); 
	ReportPkg.WriteShort(Configure::getInstance()->server_id); 
	ReportPkg.WriteShort(Configure::getInstance()->level);
	ReportPkg.WriteInt(player ? player->m_nWin:0);
	ReportPkg.WriteInt(player ? player->m_nLose:0);
	ReportPkg.WriteInt(player ? player->m_nRunAway:0);
	ReportPkg.WriteInt(player ? player->m_nTie:0);
	ReportPkg.End();
	return Send(&ReportPkg,false);
}
//User进入上报
int AllocSvrConnect::userEnterGame(Player* player)
{
	OutputPacket ReportPkg;
	ReportPkg.Begin(GMSERVER_ENTER);
	ReportPkg.WriteInt(player->id); 
	ReportPkg.WriteShort(Configure::getInstance()->server_id); 
	ReportPkg.WriteShort(Configure::getInstance()->level); 
	ReportPkg.WriteInt(player->m_nWin);
	ReportPkg.WriteInt(player->m_nLose);
	ReportPkg.WriteInt(player->m_nRunAway);
	ReportPkg.WriteInt(player->m_nTie);
	ReportPkg.End();
	return Send(&ReportPkg,false);
}
//User状态上报
int AllocSvrConnect::userUpdateStatus(Player* player,short nm_nStatus)
{
	OutputPacket ReportPkg;
	ReportPkg.Begin(GMSERVER_USER_STATUS);
	ReportPkg.WriteInt(player->id); 
	ReportPkg.WriteShort(nm_nStatus); 
	int tid = Configure::getInstance()->server_id<< 16 | player->tid; 
	ReportPkg.WriteInt(tid); 
	ReportPkg.WriteShort(Configure::getInstance()->server_id); 
	ReportPkg.WriteShort(Configure::getInstance()->level);

	ReportPkg.End();
	return Send(&ReportPkg,false);
}

//大厅返回Report 设置结果是否成功 
int AllocSvrConnect::processReportResponse(InputPacket* pPacket)
{
	_LOG_DEBUG_("\n");
	_LOG_DEBUG_( "==>[process Report Response] CMD=[0x%04x] \n", pPacket->GetCmdType());

	short errorno = pPacket->ReadShort();
	std::string errormsg = pPacket->ReadString();

	_LOG_DEBUG_( "[DATA Recv ] errorno=[%d] \n",errorno );
	_LOG_DEBUG_( "[DATA Recv ] errormsg=[%s] \n",errormsg.c_str() );

	return 0;
}

//大厅返回Update 设置结果是否成功 
int AllocSvrConnect::processUpdateResponse(InputPacket* pPacket)
{
	_LOG_DEBUG_( "==>[process Update Response] CMD=[0x%04x] \n", pPacket->GetCmdType());

	short errorno = pPacket->ReadShort();
	std::string errormsg = pPacket->ReadString();

	_LOG_DEBUG_( "[DATA Recv ] errorno=[%d] \n",errorno );
	_LOG_DEBUG_( "[DATA Recv ] errormsg=[%s] \n",errormsg.c_str() );

	if(errorno==-1)
	{
		this->reportSeverInfo();
	}

	return 0;
}

//大厅返回SeverInfo
int AllocSvrConnect::processSeverInfoResponse(InputPacket* pPacket)
{
		_LOG_DEBUG_("\n");
	_LOG_DEBUG_( "==>[process SeverInfo Response] CMD=[0x%04x] \n", pPacket->GetCmdType());

	short errorno = pPacket->ReadShort();
	std::string errormsg = pPacket->ReadString();

	if(errorno!=0)
	{
		_LOG_DEBUG_( "[DATA Recv ] errorno=[%d] \n",errorno );
		_LOG_DEBUG_( "[DATA Recv ] errormsg=[%s] \n",errormsg.c_str() );
		return 0;
	}	
	return 0;
		
}

int AllocSvrConnect::processSetServerStatus(InputPacket* inputPacket)
{
	int cmd = inputPacket->GetCmdType();
	short subcmd =  inputPacket->GetSubCmd();
	short svid = inputPacket->ReadShort();
	char  m_nStatus = inputPacket->ReadByte();
	_LOG_DEBUG_( "==>[Set GameServer Status] CMD=[0x%04x] \n",cmd);
	_LOG_DEBUG_( "[DATA Recv ] svid=[%d]\n", svid);
	_LOG_DEBUG_( "[DATA Recv ] m_nStatus=[%d]\n", m_nStatus);

	if(Configure::getInstance()->server_id != svid)
	{
		_LOG_ERROR_( "[DATA Recv] Configure server_id[%d] svid=[%d]\n",Configure::getInstance()->server_id, svid);
		return -1;
	}

	if(subcmd==0)
	{
		Room* room = Room::getInstance();
		room->setStatus(m_nStatus);
	}
	else
		Configure::getInstance()->server_priority = m_nStatus;
	return 0;
}

int AllocSvrConnect::processCreatePrivateTable(InputPacket* pPacket)
{
	int cmd = pPacket->GetCmdType();

	int uid = pPacket->ReadInt();
	int tid = pPacket->ReadInt();
	//int MaxCount = pPacket->ReadShort();
	string tableName = pPacket->ReadString();
	short nhaspassed = pPacket->ReadShort();
	string password = pPacket->ReadString();

	short svid = tid >> 16;
	short realTid = tid & 0x0000FFFF;
	Room* room = Room::getInstance();
	_LOG_INFO_("==>[processCreatePrivateTable] [0x%04x] UID=[%d] Source=[%d]\n", cmd, uid);
	_LOG_DEBUG_("[DATA Parse] uid=[%d]\n", uid);
	_LOG_DEBUG_("[DATA Parse] tid=[%d] realTid=[%d]\n", tid, realTid);
	//_LOG_DEBUG_("[DATA Parse] MaxCount=[%d]\n", MaxCount);
	_LOG_DEBUG_("[DATA Parse] roomName=[%s]\n", tableName.c_str());
	_LOG_DEBUG_("[DATA Parse] nhaspassed=[%d]\n", nhaspassed);
	_LOG_DEBUG_("[DATA Parse] password=[%s]\n", password.c_str());

	Table* table = room->getTable(realTid);

	if(table == NULL)
	{
		_LOG_ERROR_("[processCreatePrivateTable] uid=[%d] tid=[%d] realTid=[%d] Table is NULL\n",uid, tid, realTid);
		return 0;
	}

	if(!table->isEmpty())
	{
		_LOG_ERROR_("[processCreatePrivateTable] uid=[%d] tid=[%d] realTid=[%d] Table is Not Empty CountPlayer[%d]\n",
			uid, tid, realTid, table->m_nCountPlayer);
		return 0;
	}
	
	table->m_nOwner = uid;
	//table->maxCount = MaxCount;
	table->m_bHaspasswd = (nhaspassed==1) ? true : false;
	strcpy(table->tableName,tableName.c_str());
	strncpy(table->password,password.c_str(), sizeof(table->password));
	table->password[sizeof(table->password) - 1] = '\0';
	
	return 0;
}

int AllocSvrConnect::trumptToUser(short type, const char* msg, short pid)
{
	OutputPacket ReportPkg;
	ReportPkg.Begin(GMSERVER_CMD_TRUMPT);
	ReportPkg.WriteShort(type); 
	ReportPkg.WriteShort(GAME_ID); 	//GameID 
	ReportPkg.WriteString(msg); 
	ReportPkg.WriteShort(pid);
	ReportPkg.End();
	return Send(&ReportPkg,false);
}



