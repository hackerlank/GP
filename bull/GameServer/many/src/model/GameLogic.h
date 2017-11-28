#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#include <vector>
#include "Typedef.h"

//////////////////////////////////////////////////////////////////////////
//宏定义

//数值掩码
#define	LOGIC_MASK_COLOR			0xF0								//花色掩码
#define	LOGIC_MASK_VALUE			0x0F								//数值掩码

//扑克类型
#define OX_VALUE0					0									//混合牌型
//#define OX_THREE_SAME				102									//三条牌型
#define OX_FOUR_SAME				103									//炸弹牛
//#define OX_FOURKING				104									//天王牌型
#define OX_FIVEKING					105									//五花牛
#define OX_FIVESMALL				106									//五小牛

#define MAX_COUNT					5									//最大数目


//////////////////////////////////////////////////////////////////////////
#define CountArray(array) ((sizeof(array))/(sizeof(array[0])))

//////////////////////////////////////////////////////////////////////////


struct Card
{
	BYTE value;
	bool used;
	int index;

	Card()
	{
		used = false;
		index = -1;
	}
};

//游戏逻辑类
class GameLogic
{
	//变量定义
public:
	static BYTE						m_cbCardListData[52];				//扑克定义
	static Card						CardList[52];

	static void initCardList();

	//函数定义
public:
	//构造函数
	GameLogic();
	//析构函数
	virtual ~GameLogic();

	//类型函数
public:
	//获取类型
	BYTE GetCardType(BYTE cbCardData[], BYTE cbCardCount, BYTE& m, BYTE &n);
	//获取类型
	BYTE GetCardType(BYTE cbCardData[], BYTE cbCardCount);
	//获取数值
	static BYTE GetCardValue(BYTE cbCardData) { return cbCardData&LOGIC_MASK_VALUE; }
	//获取花色
	static BYTE GetCardColor(BYTE cbCardData) { return cbCardData&LOGIC_MASK_COLOR; }
	//获取倍数
	BYTE GetTimes(BYTE cbCardData[], BYTE cbCardCount);
	//获取牛牛
	bool GetOxCard(BYTE cbCardData[], BYTE cbCardCount);
	//获取整数
	bool IsIntValue(BYTE cbCardData[], BYTE cbCardCount);

	//控制函数
public:
	//排列扑克
	void SortCardList(BYTE cbCardData[], BYTE cbCardCount);
	//混乱扑克
	void RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount);
	void RandCardSpecialType(BYTE cbCardBuffer[], BYTE cbBufferCount, BYTE bType);
	void RandSpecialCardList(BYTE cbCardBuffer[], BYTE cbBufferCount, BYTE ct);
	void RandCardSpecialFourType(BYTE cbCardBuffer[], BYTE cbBufferCount);
	void RandCardSpecialKingType(BYTE cbCardBuffer[], BYTE cbBufferCount);
	void RandCardSpecialSmallType(BYTE cbCardBuffer[], BYTE cbBufferCount);

	//功能函数
public:
	//逻辑数值
	static BYTE GetCardLogicValue(BYTE cbCardData);
	//对比扑克
	bool CompareCard(BYTE cbFirstData[], BYTE cbNextData[], BYTE cbCardCount, bool FirstOX, bool NextOX, bool bSpecial = false);
};

//////////////////////////////////////////////////////////////////////////

#endif
