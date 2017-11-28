#include <map>

#include "ProcessFactory.h"
#include "GameCmd.h"
#include "HeartProc.h"
#include "HallRegistProc.h"
#include "AllocProcess.h"
#include "SetServerInfoProc.h"
#include "UpdateMemberProc.h"
#include "UpdateTableUserCountProc.h"
#include "GsTransToUserServer.h"
#include "UserLoginProc.h"
#include "UserLogoutProc.h"
#include "KickOutProc.h"
#include "SetTableStatusProc.h"
#include "ChangeTabProc.h"
#include "CreatePrivateProc.h"
#include "GetPrivateListProc.h"
#include "FindPrivateRoomProc.h"
#include "GetAllServer.h"
#include "GetServerInfoProc.h"
#include "SetServerStatProc.h"
#include "QuickMatchProc.h"
#include "GetMemberProc.h"
#include "PhpReChargeProc.h"
#include "ReloadConf.h"
#include "TrumptProc.h"
#include "USTrumptProc.h"
#include "FeedBackPushProc.h"
#include "LevelMatchProc.h"


static std::map<int ,IProcess*>  * processMap = new  std::map<int ,IProcess*> ();

IProcess* ProcessFactory::getProcesser(int cmd)
{
	std::map<int ,IProcess*>* pMap = processMap;

	std::map<int ,IProcess*>::iterator it;
	it = pMap->find(cmd);
	if(it==pMap->end())
	{
		IProcess* p = createProcesser(cmd);
		(*pMap)[cmd] = p;
		return p;
	}else{
		return it->second;
	}
}

IProcess* ProcessFactory::createProcesser(int cmd)
{
	IProcess* processer = NULL;
	switch(cmd)
	{
		case HALL_REGIST:
			processer = new HallRegistProc();break;
		case HALL_HEART_BEAT:
		case USER_HEART_BEAT:
			processer = new HeartProc();break;
		case HALL_USER_ALLOC:
			processer = new AllocProcess();break;
		case HALL_USER_LOGIN:
			processer = new UserLoginProc();break;
		case HALL_USER_LOGOUT:
			processer = new UserLogoutProc();break;
		case HALL_MSG_CHANGETAB:
			processer = new ChangeTabProc(); break;
		case HALL_GET_MEMBERS:
			processer = new GetMemberProc(); break;

		case HALL_QUICK_MATCH:
			processer = new QuickMatchProc(); break;
		case HALL_LEVEL_MATCH:
			processer = new LevelMatchProc(); break;
		//=====================私人房===================================//
		case CLIENT_CREATE_PRIVATE:
			processer = new CreatePrivateProc(); break;
		case CLIENT_GET_PRIVATELIST:
			processer = new GetPrivateListProc(); break;
		case CLIENT_FIND_PRIVATE:
			processer = new FindPrivateRoomProc(); break;
		

		//=====================游戏服务器上报===================================//
		case GMSERVER_REPORT:
			processer = new SetServerInfoProc();break;
		case GMSERVER_SET_TABLEUSER:
			processer = new UpdateTableUserCountProc();break;
		case GMSERVER_UPDATE:
			processer = new UpdateMemberProc();break;
		case GMSERVER_SET_TABLE_STATUS:
			processer = new SetTableStatusProc();break;
		case GMSERVER_ENTER:
		case GMSERVER_LEAVE:
		case GMSERVER_USER_STATUS:
			processer = new GsTransToUserServer();break;

		case SERVER_CMD_KICK_OUT:
			processer = new KickOutProc();break;

		//==================admin===================================//
		case GMSERVER_ALL:
			processer = new GetAllServer();break;
		case GMSERVER_GET_INFO:
			processer = new GetServerInfoProc();break;
		case GMSERVER_SET_STATUS:
			processer = new SetServerStatProc();break;
		case ADMIN_CMD_RELOAD_ALLOC:
			processer = new ReloadConf();break;

		case PHP_CMD_RECHARGE:
			processer = new PhpReChargeProc();break;
		case FEEDBACK_CMD_PUSH:
			processer = new FeedBackPushProc();break;

		case CMD_TRUMPT:
			processer = new TrumptProc();break;
		case CLIENT_CMD_STRUMPT:
			processer = new USTrumptProc();break;
		default:
			processer = NULL;break;
	}
	return processer;
}

 
