#include <map>
#include "Logger.h"
#include "Room.h"
#include "ClientHandler.h"
#include "Version.h"
#include "Configure.h"
#include "GameCmd.h"
#include "IProcess.h"
#include "AllocSvrConnect.h"

static Room * Instance = NULL;

Room::Room()
{
	this->status = 1;
    this->monitor_time = Configure::getInstance()->monitor_time;
    this->keeplive_time = Configure::getInstance()->keeplive_time;
	this->max_user_count =  Configure::getInstance()->max_user < MAX_ROOM_TABLE*GAME_PLAYER ? Configure::getInstance()->max_user   : MAX_ROOM_TABLE*GAME_PLAYER; 
	this->max_table_count =  Configure::getInstance()->max_table < MAX_ROOM_TABLE   ? Configure::getInstance()->max_table  : MAX_ROOM_TABLE;
}

Room * Room::getInstance()
{
	if(Instance==NULL)
		Instance  = new Room();
	return Instance;
}

int Room::init()
{
    this->timer.init(this);
	this->startHeartTimer();
	max_count = 0;
	for(int i=0;i<MAX_ROOM_TABLE ;++i)
	{
		tables[i].id = i;
		tables[i].init();
		players[4*i].tid =  -1; 
		players[4*i].seat_id = i*4;
		players[4*i].status =STATUS_PLAYER_LOGOUT;
		players[4*i+1].tid = -1;
		players[4*i+1].seat_id = i*4+1;
		players[4*i+1].status =STATUS_PLAYER_LOGOUT;
		players[4*i+2].tid = -1;
		players[4*i+2].seat_id = i*4+2;
		players[4*i+2].status =STATUS_PLAYER_LOGOUT;
		players[4*i+3].tid = -1;
		players[4*i+3].seat_id = i*4+3;
		players[4*i+3].status =STATUS_PLAYER_LOGOUT;
	}
	return 0;
}

static Table* myGetAvailableTable(int uid,Table* tables,int max_table_count)
{
	_LOG_WARN_("[WARING] %s\n", "myGetAvailableTable");
	for(int i=0;i<max_table_count ;++i)
	{
		if(tables[i].isNotFull() && !tables[i].isUserInTab(uid))
			return &tables[i];
	}

	for(int i=0;i<max_table_count ;++i)
	{
		if(tables[i].isEmpty())
			return &tables[i];
	}
	return NULL;
}

Table* Room::getAvailableTable()
{
	for(int i = 0; i < max_table_count ;++i)
	{
		if((tables[i].isNotFull() || tables[i].isEmpty()) && !tables[i].isLock())//空桌子未锁定可以用
			return &tables[i];
	}
	return NULL;
}

Table* Room::getAvailableTable(int table_id)
{
	for(int i = 0; i < max_table_count ;++i)
	{
		if(!tables[i].isLock() && tables[i].isEmpty())//空桌子未锁定可以用
		{
			return &tables[i];
		}
	}
	return NULL;
}

Table* Room::getAvailableTable(Player* player)
{
	if(player==NULL)
		return NULL;

	return myGetAvailableTable(player->id,this->tables,this->max_table_count);
}

Table* Room::getTable(int table_id)
{
	if(table_id<0 || table_id>this->max_table_count )
		return NULL;

	return &tables[table_id];

}

Table* Room::getChangeTable(int changeTid)
{
	for(int i=0;i<max_table_count ;++i)
	{
		if(i != changeTid)
		{
			if(tables[i].isNotFull() || tables[i].isEmpty())//空桌子未预定可以用
				return &tables[i];
		}
	}
	return NULL;
}

Player* Room::getAvailablePlayer()
{
	for(int i=0;i<max_user_count;++i)
	{
		if(players[i].status == STATUS_PLAYER_LOGOUT)
			return &players[i];
	}
	return NULL;

}

Player* Room::getPlayer(int seat_id)
{
	if(seat_id<0 || seat_id>max_user_count)
		return NULL;

	return &players[seat_id];
}

Player* Room::getPlayerUID(int uid)
{
	for(int i = 0; i < max_user_count; ++i)
	{
		if(players[i].id == uid)
			return &players[i];
	}

	return NULL;
}

void Room::startHeartTimer()
{
	 timer.startHeartTimer();
}

void Room::stopHeartTimer()
{
	 timer.stopHeartTimer();
}

//======================Roomtimer=============================

void RoomTimer::init(Room *room)
{
    this->room = room;
}

int RoomTimer::ProcessOnTimerOut()
{
	if(room->getStatus()==-1)
	{
		_LOG_INFO_("Svid[%d]:Room Status=[%d],Wait For Player Empty and Close.... CurrUsers=[%d]\n",Configure::getInstance()->server_id,room->getStatus(),room->getCurrUsers());
		if(room->getCurrUsers()==0)
		{
			_LOG_INFO_("Svid[%d]:Room Status=[%d],Player Empty And Close Server\n",Configure::getInstance()->server_id,room->getStatus());
			_LOG_DEBUG_("Server Exit\n");
			exit(0);
		}
	}

	return HeartTimeOut();
}

void RoomTimer::startHeartTimer()
{    
    this->StartTimer(this->room->monitor_time*1000);
}

void RoomTimer::stopHeartTimer()
{
	this->StopTimer();
}

//定时检查各个棋桌中是否有超时的，判断标准为在一定时间内没有收到心跳包
//只要是不在下棋的超时玩家都踢出去
//
int RoomTimer::HeartTimeOut()
{
	_LOG_INFO_("============Room Info============\n");
	_LOG_INFO_("Version:ShowHand[%s.%s]\n",VERSION,SUBVER);
	int curr_user_count = 0;
	int curr_table_count = 0;
	room->iphone_count = 0;
	room->android_count = 0;
	room->ipad_count = 0;
	room->robot_count = 0;
	int i = 0;
	for(i=0; i< room->getMaxUsers() ;++i)
	{
		Player * player = &room->players[i];
		if( !player->isLogout())
		{
			curr_user_count++;
			if(player->source == 1) room->android_count ++;
			else if(player->source == 2) room->iphone_count ++;
			else if(player->source == 3) room->ipad_count ++;
			
			int differtime = time(NULL) - player->getActiveTime();
            Table* table = NULL;
            if (player->tid != -1)
            {
                table = room->getTable(player->tid);
            }
            else
            {
                //player->status = STATUS_PLAYER_LOGOUT;
                player->leave();
            }

            if (differtime > 10 && table)
            {
                IProcess::NotifyPlayerNetStatus(table, player->id, 0, 1);
            }

			if ((!player->isActive()) && (differtime > room->keeplive_time))
			{
				_LOG_WARN_("KickPlayer:player[%d] ustatus[%d], Haven't recv the heart beat for [%d] second, more than keeplive_time[%d]\n",
					player->id, player->status, differtime, room->keeplive_time);
				if(table)
				{
					IProcess::serverPushLeaveInfo(table, player);
					table->playerLeave(player);
				}
			}
			/*else if((player->isActive()) && (differtime > room->keeplive_time))
			{
				Table* table = room->getTable( player->tid );
				if(table)
				{
					IProcess::serverPushLeaveInfo(table, player);
					table->playerLeave(player);
				}
			}*/
		}
	}
	for(i=0; i<room->getMaxTables(); ++i)
	{
		Table* table = &room->tables[i];
		if(!table->isEmpty())
		{
			curr_table_count++;
		}
		//是私人房则处理房间创建没有人进来的问题
		if(table->clevel == 5)
		{
			if(table->tableName[0] != '\0')
				AllocSvrConnect::getInstance()->updateTableUserCount(table);
		}
	}

	room->setCurrTables(curr_table_count);
	room->setCurrUsers(curr_user_count);
	_LOG_INFO_("curr_user_count=[%d] max_count=[%d]\n",curr_user_count,room->max_count);
	_LOG_INFO_("curr_table_count=[%d] max_count_tab=[%d]\n",curr_table_count,room->max_count_tab);
	_LOG_INFO_("============Room Info============\n");
	this->startHeartTimer();
    return 0;
}
