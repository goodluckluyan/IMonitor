#import "stlvector.h"

struct mons__MontorStateRes
{
	int bMain;
	int iState;
};
int mons__GetMontorState(struct mons__MontorStateRes &ret);

struct mons__TmsStateRes
{
	int bRun;
	int iWorkState;
	int iState;
};
int mons__GetTMSState(struct mons__TmsStateRes &ret);

struct mons__SMSState
{
	std::string HallId;
	int bRun;
	int state;
	int position;
	std::string strSplUuid;
};
int mons__GetSMSState(struct mons__SMSState &ret);

struct mons__RaidStateRes
{
	int state;
	int ReadSpeed;
	int WriteSpeed;
	std::vector<int> diskState;
};
int mons__GetRaidtate(struct mons__RaidStateRes &ret);

struct mons__ethstate
{
	int eth;
	int type;
	int ConnectState;
	int speed;
};
int mons__GetEthState(std::vector<struct mons__ethstate> &vecret);

struct mons__SwitchStateRes
{
	int Switch1State;
	int Switch2State;
};
int mons__GetSwitchState(struct mons__SwitchStateRes &ret);

struct mons__IngestSpeedLimitRes
{
	int bEnableIngest;
	int speedLimit;
};
int mons__GetIngestSpeedLimit(struct mons__IngestSpeedLimitRes &ret);

struct mons__WorkStateRes
{
	int state;
	std::string info;
	std::string hall;
};
int mons__GetWorkState_USCORECS(struct mons__WorkStateRes &ret);
