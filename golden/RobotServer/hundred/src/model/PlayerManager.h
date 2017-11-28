#ifndef _PlayerManager_H_
#define _PlayerManager_H_

#include "CDL_Timer_Handler.h"
#include "Player.h"
#include <map>
#include "Util.h"
#include "RobotUtil.h"

typedef struct _Cfg
{
	short type;
	int bankerwincount;			//���f������Ӯ�����
	int betwincount;			//��ע������Ӯ�����
	char starttime[16];			//��ʼʱ��
	char endtime[16];			//����ʱ��
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
		//��ȡ�����˵�id
	public:
		int getRobotUid();
		int delRobotUid(int robotuid);
		int getPlayerSize();
	protected:
		std::map<int , Player*> playerMap;
		
		//������ʹ��id��
	public:
		Robot_id m_idpool[MAX_ROBOT_NUMBER];

	public:
		Cfg cfg;
		bool addbetrobot;
};
#endif
