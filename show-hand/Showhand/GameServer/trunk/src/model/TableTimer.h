#ifndef _TableTimer_H
#define _TableTimer_H

#include <time.h>
#include "CDL_Timer_Handler.h"

#define BET_COIN_TIMER	20
#define TABLE_START_TIMER 21
#define TABLE_KICK_TIMER 22
#define TABLE_SEND_TIMER 23

class Table;
class Player;
class TableTimer; 
class CTimer_Handler:public CCTimer
{
	public:
		virtual int ProcessOnTimerOut();
		void SetTimeEventObj(TableTimer * obj, int timeid, int uid = 0);
		void StartTimer(long interval)
		{
			CCTimer::StartTimer(interval*1000);					
		}  
		void StartMinTimer(long interval)
		{
			CCTimer::StartTimer(interval);					
		}  
	private:
		int timeid;
		int uid;
		TableTimer * handler;
};

class TableTimer
{
	public:
		TableTimer(){} ;
		virtual ~TableTimer() {};	
		void init(Table* table);
	//������ʱ����
	public:
		void stopAllTimer();
		void startBetCoinTimer(int uid,int timeout);
		void stopBetCoinTimer();
		void startTableStartTimer(int timeout);
		void stopTableStartTimer();
		void startKickTimer(int timeout);
		void stopKickTimer();
		void startSendCardTimer(int timeout);
		void stopSendCardTimer();
	//����֪ͨ����
	public:
		int sendBetTimeOut(Player* player, Table* table, Player* timeoutplayer,Player* nextplayer);
		int sendOpenTimeOut(Player* player, Table* table, Player* timeoutplayer,Player* nextplayer);

	private:
		Table* table;
		int ProcessOnTimerOut(int Timerid, int uid);
		//�����û���ע��ʱ
		CTimer_Handler m_BetTimer;
		//�������ӿ�ʼ��Ϸ�Ķ�ʱ��
		CTimer_Handler m_TableStartTimer;
		//�߳��û���ʱ��
		CTimer_Handler m_TableKickTimer;
	//��ʱ�ص�����
	private:
		int BetTimeOut(int uid);
		int TableGameStartTimeOut();
		int TableKickTimeOut();
		int SendCardTimeOut();

		friend class CTimer_Handler;

		
};

#endif
