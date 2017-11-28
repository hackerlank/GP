#ifndef _TableTimer_H
#define _TableTimer_H

#include <time.h>
#include "CDL_Timer_Handler.h"

#define IDLE_TIMER	25
#define BET_TIMER 26
#define OPEN_TIMER 27
#define SEND_BET_TIMER 28
#define PHP_PUSH_CARD_TIMER 29		//等待php推送牌型倒计时

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
		void StartMilliTimer(long interval)
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
	//操作超时函数
	public:
		void stopAllTimer();
		void startIdleTimer(int timeout);
		void stopIdleTimer();
		void startBetTimer(int timeout);
		void stopBetTimer();
		void startOpenTimer(int timeout);
		void stopOpenTimer();
		void startSendTimer(int timeout);
		void stopSendTimer();
		void startPhpPushCardTimer(int timeout);
		void stopPhpPushCardTimer();
	//发送通知函数
	public:

	private:
		Table* table;
		int ProcessOnTimerOut(int Timerid, int uid);
		CTimer_Handler m_BetTimer;
		CTimer_Handler m_SendTimer;
		CTimer_Handler m_phpTimer;
	//超时回调函数
	private:
		int IdleTimeOut();
		int BetTimeOut();
		int OpenTimeOut();
		int SendBetInfoTimeOut();
		int PhpPushCardTimeOut();

		friend class CTimer_Handler;

		
};

#endif
