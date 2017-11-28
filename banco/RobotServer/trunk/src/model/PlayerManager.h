#ifndef _PlayerManager_H_
#define _PlayerManager_H_

#include "CDL_Timer_Handler.h"
#include "Player.h"
#include <map>
#include <deque>
#include "Util.h"
#include "RobotUtil.h"

typedef struct _Cfg
{
	short type;
	int bankerwincount;			//当莊机器人赢金币数
	int betwincount;			//下注机器人赢金币数
	char starttime[16];			//开始时间
	char endtime[16];			//结束时间
	BYTE starthour;
	BYTE startmin;
	BYTE endhour;
	BYTE endmin;
	BYTE switchbanker;
	BYTE bankernum;
	BYTE switchbetter;
	int carrybankerlow;
	int carrybankerhigh;
	int carrybetterlow;
	int carrybetterhigh;
	short normalrobotnum;
	int carrynormallow;
	int carrynormalhigh;
	_Cfg():type(0), bankerwincount(0), betwincount(0),starthour(0),startmin(0),endhour(0),endmin(0)
	{}
}Cfg;


//开三个机器人服务器，每个机器人服务器能使用的机器人数量330
const int LEVEL1NUM=200;
const int LEVEL2NUM=200;
const int LEVEL3NUM=200;

class Robot_id
{
	public:
		Robot_id():uid(0), hasused(false){};
		virtual ~Robot_id(){};
		int uid;
		bool hasused;
};

class PlayerManager:public CCTimer
{
	private:
		PlayerManager();
	public:
		static PlayerManager* getInstance();
		virtual ~PlayerManager(){};
		virtual int ProcessOnTimerOut();
		int init();
	public:
		int addPlayer(int uid, Player* player);
		void  delPlayer(int uid, bool isOnclose = false);
		Player* getPlayer(int uid);	
	//获取机器人的id
	public:
		int getRobotUid();
		int delRobotUid(int robotuid);
		int getPlayerSize();
	protected:
		std::map<int , Player*> playerMap;
	//机器人使用id池
	public:
		Robot_id level1id[LEVEL1NUM];
		Robot_id level2id[LEVEL2NUM];
		Robot_id level3id[LEVEL3NUM];

	public:
		Cfg cfg;
		bool addbetrobot;

	private:
		 
};
#endif
