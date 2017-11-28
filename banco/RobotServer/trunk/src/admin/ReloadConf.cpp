#include "ReloadConf.h"
#include "Logger.h"
#include "Configure.h"
#include "GameCmd.h"
#include "ProcessManager.h"

REGISTER_PROCESS(ADMIN_RELOAD_CONF, ReloadConf);

ReloadConf::ReloadConf()
{
	this->name = "ReloadConf";
}

ReloadConf::~ReloadConf()
{

} 

int ReloadConf::doRequest(CDLSocketHandler* socketHandler, InputPacket* inputPacket, Context* pt) 
{
	int cmd = inputPacket->GetCmdType() ;
	LOGGER(E_LOG_INFO) << "==>[ReloadConf] cmd = " << cmd;
	Configure::getInstance()->LoadConfig(0, NULL, true);

	OutputPacket OutPkg;
	OutPkg.Begin(cmd);
	OutPkg.WriteShort(0); 
	OutPkg.WriteString("Ok"); 
	OutPkg.End();
	this->send(socketHandler, &OutPkg);
	return 0;
}

int ReloadConf::doResponse(CDLSocketHandler* socketHandler, InputPacket* inputPacket, Context* pt) 
{
	_NOTUSED(socketHandler);
	_NOTUSED(inputPacket);
	return 0;
}
