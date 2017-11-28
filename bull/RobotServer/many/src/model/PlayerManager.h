#ifndef _PlayerManager_H_
#define _PlayerManager_H_

#include "CDL_Timer_Handler.h"
#include "Player.h"
#include <map>
#include <deque>
#include "Util.h"
#include "RobotUtil.h"

#define BETNUM 5

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
	
	public:
		int getRobotUid();
		int delRobotUid(int robotuid);
		int getPlayerSize();

		void calculateAreaBetLimit(int bankeruid);	//����������������Ƽ���
		void updateAreaBetLimit(int64_t bankermoney, int bettype, int rate);			//�����ĳ������ע������Ȼ��߳�������ʱ���¼�����������
		void ResetBetArray();
		bool CheckAreaBetLimit(int bettype, int64_t betcoin);

		int SavePlayerBetRecord(int uid, int bettype, int64_t betmoney, int pc);
		void ClearBetRecord();

	protected:
		std::map<int , Player*> playerMap;
		struct BetArea
		{
			int bettype;
			int continuePlayCount;
			int64_t betmoney;
		};
		std::map<int, std::deque<BetArea> > playerBetRecord;	//���ǰ5����ע��¼
	
	public:
		Robot_id m_idpool[MAX_ROBOT_NUMBER];

		int64_t areaTotalBetLimit[BETNUM];	//�������������ע�ܶ�����
		int64_t areaTotalBetArray[BETNUM];	//��ǰ���������������ע���

	public:
		Cfg cfg;
		bool addbetrobot;

	private:
		 
};
#endif
