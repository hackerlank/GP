#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "GameLogic.h"

//////////////////////////////////////////////////////////////////////////
//扑克数据
BYTE GameLogic::m_cbCardData[FULL_COUNT]=
{
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
	0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
	0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
};

//构造函数
GameLogic::GameLogic()
{
}

//析构函数
GameLogic::~GameLogic()
{
}

VOID GameLogic::LeftCardList(BYTE cbCardBuffer[],BYTE cbBufferCount,BYTE cbHasUseCardData[],BYTE cbUseCardCount)
{
    BYTE allCard[FULL_COUNT];
    memcpy(allCard,m_cbCardData,sizeof(m_cbCardData[0])*FULL_COUNT);
    for (int i = 0; i<cbUseCardCount; i++)
    {
        BYTE cardvalue = cbHasUseCardData[i];
        int lie = cardvalue&0xf0;
        int index = (lie>>4)*13+(cardvalue&0x0f)-1;
        allCard[index]=0;
    }
    int j=0;
    for (int i = 0; i<FULL_COUNT; i++)
    {
        if (allCard[i]!=0 && j<cbBufferCount)
        {
            cbCardBuffer[j++] = allCard[i];
        }
    }
}
VOID GameLogic::RandLeftCardList(BYTE cbCardBuffer[], BYTE cbBufferCount,BYTE cbInputCardBuffer[],BYTE cbInputCardCount)
{
//    BYTE bCardCount = CountArray(m_cbCardData);
    BYTE cbCardData[cbInputCardCount];
    memcpy(cbCardData,cbInputCardBuffer,cbInputCardCount);
    
    //混乱扑克
    BYTE cbRandCount=0,cbPosition=0;
    do
    {
        cbPosition=rand()%(cbInputCardCount-cbRandCount);
        cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
        cbCardData[cbPosition]=cbCardData[cbInputCardCount-cbRandCount];
    } while (cbRandCount<cbBufferCount);
    
    return;
}

//混乱扑克
VOID GameLogic::RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount)
{
	//测试代码
	//memcpy(cbCardBuffer,m_cbCardData,cbBufferCount);

	//混乱准备
	BYTE bCardCount = CountArray(m_cbCardData);
	BYTE cbCardData[CountArray(m_cbCardData)];
	memcpy(cbCardData,m_cbCardData,sizeof(m_cbCardData));

	//混乱扑克
	BYTE cbRandCount=0,cbPosition=0;
	do
	{
		cbPosition=rand()%(bCardCount-cbRandCount);
		cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
		cbCardData[cbPosition]=cbCardData[bCardCount-cbRandCount];
	} while (cbRandCount<cbBufferCount);

	return;
}

//获取类型
BYTE GameLogic::GetCardType(BYTE cbCardData[], BYTE cbCardCount)
{
	//数据校验
	assert(cbCardCount == MAX_CENTERCOUNT);
	if(cbCardCount !=MAX_CENTERCOUNT) return 0;

	//变量定义
	bool cbSameColor=true,bLineCard=true;
	BYTE cbFirstColor=GetCardColor(cbCardData[0]);
	BYTE cbFirstValue=GetCardLogicValue(cbCardData[0]);

	//牌形分析
	for (BYTE i=1;i<cbCardCount;i++)
	{
		//数据分析
		if (GetCardColor(cbCardData[i])!=cbFirstColor) cbSameColor=false;
		if (cbFirstValue!=(GetCardLogicValue(cbCardData[i])+i)) bLineCard=false;

		//结束判断
		if ((cbSameColor==false)&&(bLineCard==false)) break;
	}

	//最小同花顺
	if((bLineCard == false)&&(cbFirstValue == 14))
	{
		BYTE i=1;
		for (i=1;i<cbCardCount;i++)
		{
			BYTE cbLogicValue=GetCardLogicValue(cbCardData[i]);
			if ((cbFirstValue!=(cbLogicValue+i+8))) break;
		}
		if( i == cbCardCount)
			bLineCard =true;
	}

	//皇家同花顺
	if ((cbSameColor==true)&&(bLineCard==true)&&(GetCardLogicValue(cbCardData[1]) ==13 )) 
		return CT_KING_TONG_HUA_SHUN;

	//顺子类型
	if ((cbSameColor==false)&&(bLineCard==true)) 
		return CT_SHUN_ZI;

	//同花类型
	if ((cbSameColor==true)&&(bLineCard==false)) 
		return CT_TONG_HUA;

	//同花顺类型
	if ((cbSameColor==true)&&(bLineCard==true))
		return CT_TONG_HUA_SHUN;

	//扑克分析
	tagAnalyseResult AnalyseResult;
	AnalysebCardData(cbCardData,cbCardCount,AnalyseResult);

	//类型判断
	if (AnalyseResult.cbFourCount==1) 
		return CT_TIE_ZHI;
	if (AnalyseResult.cbLONGCount==2) 
		return CT_TWO_LONG;
	if ((AnalyseResult.cbLONGCount==1)&&(AnalyseResult.cbThreeCount==1))
		return CT_HU_LU;
	if ((AnalyseResult.cbThreeCount==1)&&(AnalyseResult.cbLONGCount==0))
		return CT_THREE_TIAO;
	if ((AnalyseResult.cbLONGCount==1)&&(AnalyseResult.cbSignedCount==3)) 
		return CT_ONE_LONG;

	return CT_SINGLE;
}

//最大牌型
BYTE GameLogic::FiveFromSeven(BYTE cbHandCardData[],BYTE cbHandCardCount,BYTE cbCenterCardData[],BYTE cbCenterCardCount,BYTE cbLastCardData[],BYTE cbLastCardCount)
{
	//临时变量
	BYTE cbTempCardData[MAX_COUNT+MAX_CENTERCOUNT],cbTempLastCardData[5];
	ZeroMemory(cbTempCardData,sizeof(cbTempCardData));
	ZeroMemory(cbTempLastCardData,sizeof(cbTempLastCardData));

	//拷贝数据
	memcpy(cbTempCardData,cbHandCardData,sizeof(BYTE)*MAX_COUNT);
	memcpy(&cbTempCardData[2],cbCenterCardData,sizeof(BYTE)*MAX_CENTERCOUNT);

	//排列扑克
	SortCardList(cbTempCardData,CountArray(cbTempCardData));

	memcpy(cbLastCardData,cbTempCardData,sizeof(BYTE)*MAX_CENTERCOUNT);
	BYTE cbCardKind = GetCardType(cbLastCardData,sizeof(BYTE)*MAX_CENTERCOUNT);
	BYTE cbTempCardKind = 0;

	//组合算法
	for (int i=0;i<3;i++)
	{
		for (int j= i+1;j<4;j++)
		{
			for (int k = j+1;k<5;k++)
			{
				for (int l =k+1;l<6;l++)
				{
					for (int m=l+1;m<7;m++)
					{
						//获取数据
						cbTempLastCardData[0]=cbTempCardData[i];
						cbTempLastCardData[1]=cbTempCardData[j];
						cbTempLastCardData[2]=cbTempCardData[k];
						cbTempLastCardData[3]=cbTempCardData[l];
						cbTempLastCardData[4]=cbTempCardData[m];

						//获取牌型
						cbTempCardKind = GetCardType(cbTempLastCardData,CountArray(cbTempLastCardData));

						//牌型大小
						if(CompareCard(cbTempLastCardData,cbLastCardData,CountArray(cbTempLastCardData))==2)
						{
							cbCardKind = cbTempCardKind;
							memcpy(cbLastCardData,cbTempLastCardData,sizeof(BYTE)*CountArray(cbTempLastCardData));
						}
					}
				}
			}
		}
	}

	return cbCardKind;
}

//查找最大
bool GameLogic::SelectMaxUser(BYTE bCardData[GAME_PLAYER][MAX_CENTERCOUNT],BYTE playernum,UserWinList &EndResult,const LONG lAddScore[])
{
	//清理数据
	ZeroMemory(&EndResult,sizeof(EndResult));

	//First数据
	WORD wWinnerID;
	WORD i = 0;
	for (i=0;i<playernum;i++)
	{
		if(bCardData[i][0]!=0)
		{
			wWinnerID=i;
			break;
		}
	}
	
	//过滤全零
	if(i==playernum)return false;

	//printf("before winnerid:%d\n", wWinnerID);

	//查找最大用户
	WORD wTemp = wWinnerID;
	for(i=1;i<playernum;i++)
	{
		WORD j=(i+wTemp)%playernum;
		if(bCardData[j][0]==0)continue;
		if(CompareCard(bCardData[j],bCardData[wWinnerID],MAX_CENTERCOUNT)>1)
		{
			wWinnerID=j;
		}
	}

	//查找相同数据
	EndResult.wWinerList[EndResult.bSameCount++] = wWinnerID;
	for(i=0;i<playernum;i++)
	{
		if(i==wWinnerID || bCardData[i][0]==0)continue;
		if(CompareCard(bCardData[i],bCardData[wWinnerID],MAX_CENTERCOUNT)==FALSE)
		{
			EndResult.wWinerList[EndResult.bSameCount++] = i;
		}
	}
	//printf(" SelectMaxUser EndResult.bSameCount:%d wWinnerID:%d\n", EndResult.bSameCount, wWinnerID);

	//从小到大
	if(EndResult.bSameCount>1 && lAddScore!=NULL)
	{
		int iSameCount=(int)EndResult.bSameCount;
		while((iSameCount--)>0)
		{
			LONG lTempSocre = lAddScore[EndResult.wWinerList[iSameCount]];
			for(int i=iSameCount-1;i>=0;i--)
			{
				assert(lAddScore[EndResult.wWinerList[i]]>0);
				if(lTempSocre < lAddScore[EndResult.wWinerList[i]])
				{
					lTempSocre = lAddScore[EndResult.wWinerList[i]];
					WORD wTemp = EndResult.wWinerList[iSameCount];
					EndResult.wWinerList[iSameCount] = EndResult.wWinerList[i];
					EndResult.wWinerList[i] = wTemp;
				}
			}
		}
	}

	return true;
}

//排列扑克
void GameLogic::SortCardList(BYTE cbCardData[], BYTE cbCardCount)
{
	//转换数值
	BYTE cbLogicValue[FULL_COUNT];
	for (BYTE i=0;i<cbCardCount;i++) 
		cbLogicValue[i]=GetCardLogicValue(cbCardData[i]);	

	//排序操作
	bool bSorted=true;
	BYTE cbTempData,bLast=cbCardCount-1;
	do
	{
		bSorted=true;
		for (BYTE i=0;i<bLast;i++)
		{
			if ((cbLogicValue[i]<cbLogicValue[i+1])||
				((cbLogicValue[i]==cbLogicValue[i+1])&&(cbCardData[i]<cbCardData[i+1])))
			{
				//交换位置
				cbTempData=cbCardData[i];
				cbCardData[i]=cbCardData[i+1];
				cbCardData[i+1]=cbTempData;
				cbTempData=cbLogicValue[i];
				cbLogicValue[i]=cbLogicValue[i+1];
				cbLogicValue[i+1]=cbTempData;
				bSorted=false;
			}	
		}
		bLast--;
	} while(bSorted==false);

	return;
}

//逻辑数值
BYTE GameLogic::GetCardLogicValue(BYTE cbCardData)
{
	//扑克属性
	BYTE bCardColor=GetCardColor(cbCardData);
	BYTE bCardValue=GetCardValue(cbCardData);

	//转换数值
	return (bCardValue==1)?(bCardValue+13):bCardValue;
}

//对比扑克
BYTE GameLogic::CompareCard(BYTE cbFirstData[], BYTE cbNextData[], BYTE cbCardCount)
{
	//获取类型
	BYTE cbNextType=GetCardType(cbNextData,cbCardCount);
	BYTE cbFirstType=GetCardType(cbFirstData,cbCardCount);

	//类型判断
	//大
	if(cbFirstType>cbNextType)
		return 2;

	//小
	if(cbFirstType<cbNextType)
		return 1;

	//简单类型
	switch(cbFirstType)
	{
	case CT_SINGLE:			//单牌
		{
			//对比数值
			BYTE i=0;
			for (i=0;i<cbCardCount;i++)
			{
				BYTE cbNextValue=GetCardLogicValue(cbNextData[i]);
				BYTE cbFirstValue=GetCardLogicValue(cbFirstData[i]);

				//大
				if(cbFirstValue > cbNextValue)
					return 2;
				//小
				else if(cbFirstValue <cbNextValue)
					return 1;
				//等
				else
					continue;
			}
			//平
			if( i == cbCardCount)
				return 0;
			assert(FALSE);
		}
	case CT_ONE_LONG:		//对子
	case CT_TWO_LONG:		//两对
	case CT_THREE_TIAO:		//三条
	case CT_TIE_ZHI:		//铁支
	case CT_HU_LU:			//葫芦
		{
			//分析扑克
			tagAnalyseResult AnalyseResultNext;
			tagAnalyseResult AnalyseResultFirst;
			AnalysebCardData(cbNextData,cbCardCount,AnalyseResultNext);
			AnalysebCardData(cbFirstData,cbCardCount,AnalyseResultFirst);

			//四条数值
			if (AnalyseResultFirst.cbFourCount>0)
			{
				BYTE cbNextValue=AnalyseResultNext.cbFourLogicVolue[0];
				BYTE cbFirstValue=AnalyseResultFirst.cbFourLogicVolue[0];

				//比较四条
				if(cbFirstValue != cbNextValue)return (cbFirstValue > cbNextValue)?2:1;

				//比较单牌
				assert(AnalyseResultFirst.cbSignedCount==1 && AnalyseResultNext.cbSignedCount==1);
				cbFirstValue = AnalyseResultFirst.cbSignedLogicVolue[0];
				cbNextValue = AnalyseResultNext.cbSignedLogicVolue[0];
				if(cbFirstValue != cbNextValue)return (cbFirstValue > cbNextValue)?2:1;
				else return 0;
			}

			//三条数值
			if (AnalyseResultFirst.cbThreeCount>0)
			{
				BYTE cbNextValue=AnalyseResultNext.cbThreeLogicVolue[0];
				BYTE cbFirstValue=AnalyseResultFirst.cbThreeLogicVolue[0];

				//比较三条
				if(cbFirstValue != cbNextValue)return (cbFirstValue > cbNextValue)?2:1;

				//葫芦牌型
				if(CT_HU_LU == cbFirstType)
				{
					//比较对牌
					assert(AnalyseResultFirst.cbLONGCount==1 && AnalyseResultNext.cbLONGCount==1);
					cbFirstValue = AnalyseResultFirst.cbLONGLogicVolue[0];
					cbNextValue = AnalyseResultNext.cbLONGLogicVolue[0];
					if(cbFirstValue != cbNextValue)return (cbFirstValue > cbNextValue)?2:1;
					else return 0;
				}
				else //三条带单
				{
					//比较单牌
					assert(AnalyseResultFirst.cbSignedCount==2 && AnalyseResultNext.cbSignedCount==2);

					//散牌数值
					BYTE i=0;
					for (i=0;i<AnalyseResultFirst.cbSignedCount;i++)
					{
						BYTE cbNextValue=AnalyseResultNext.cbSignedLogicVolue[i];
						BYTE cbFirstValue=AnalyseResultFirst.cbSignedLogicVolue[i];
						//大
						if(cbFirstValue > cbNextValue)
							return 2;
						//小
						else if(cbFirstValue <cbNextValue)
							return 1;
						//等
						else continue;
					}
					if( i == AnalyseResultFirst.cbSignedCount)
						return 0;
					assert(FALSE);
				}
			}

			//对子数值
			BYTE i=0;
			for ( i=0;i<AnalyseResultFirst.cbLONGCount;i++)
			{
				BYTE cbNextValue=AnalyseResultNext.cbLONGLogicVolue[i];
				BYTE cbFirstValue=AnalyseResultFirst.cbLONGLogicVolue[i];
				//大
				if(cbFirstValue > cbNextValue)
					return 2;
				//小
				else if(cbFirstValue <cbNextValue)
					return 1;
				//平
				else
					continue;
			}

			//比较单牌
			assert( i == AnalyseResultFirst.cbLONGCount);
			{
				assert(AnalyseResultFirst.cbSignedCount==AnalyseResultNext.cbSignedCount
					&& AnalyseResultNext.cbSignedCount>0);
				//散牌数值
				for (i=0;i<AnalyseResultFirst.cbSignedCount;i++)
				{
					BYTE cbNextValue=AnalyseResultNext.cbSignedLogicVolue[i];
					BYTE cbFirstValue=AnalyseResultFirst.cbSignedLogicVolue[i];
					//大
					if(cbFirstValue > cbNextValue)
						return 2;
					//小
					else if(cbFirstValue <cbNextValue)
						return 1;
					//等
					else continue;
				}
				//平
				if( i == AnalyseResultFirst.cbSignedCount)
					return 0;
			}
			break;
		}
	case CT_SHUN_ZI:		//顺子
	case CT_TONG_HUA_SHUN:	//同花顺
		{
			//数值判断
			BYTE cbNextValue=GetCardLogicValue(cbNextData[0]);
			BYTE cbFirstValue=GetCardLogicValue(cbFirstData[0]);

			bool bFirstmin= (cbFirstValue ==(GetCardLogicValue(cbFirstData[1])+9));
			bool bNextmin= (cbNextValue ==(GetCardLogicValue(cbNextData[1])+9));

			//大小顺子
			if ((bFirstmin==true)&&(bNextmin == false))
			{
				return 1;
			}
			//大小顺子
			else if ((bFirstmin==false)&&(bNextmin == true))
			{
				return 2;
			}
			//等同顺子
			else
			{
				//平
				if(cbFirstValue == cbNextValue)return 0;		
				return (cbFirstValue > cbNextValue)?2:1;		
			}
		}
	case CT_TONG_HUA:		//同花
		{	
			BYTE i = 0;
			//散牌数值
			for (i=0;i<cbCardCount;i++)
			{
				BYTE cbNextValue=GetCardLogicValue(cbNextData[i]);
				BYTE cbFirstValue=GetCardLogicValue(cbFirstData[i]);

				if(cbFirstValue == cbNextValue)continue;
				return (cbFirstValue > cbNextValue)?2:1;
			}
			//平
			if( i == cbCardCount) return 0;
		}
	}

	return  0;
}

//分析扑克
void GameLogic::AnalysebCardData(const BYTE cbCardData[], BYTE cbCardCount, tagAnalyseResult & AnalyseResult)
{
	//设置结果
	ZeroMemory(&AnalyseResult,sizeof(AnalyseResult));

	//扑克分析
	for (BYTE i=0;i<cbCardCount;i++)
	{
		//变量定义
		BYTE cbSameCount=1;
		BYTE cbSameCardData[4]={cbCardData[i],0,0,0};
		BYTE cbLogicValue=GetCardLogicValue(cbCardData[i]);

		//获取同牌
		for (int j=i+1;j<cbCardCount;j++)
		{
			//逻辑对比
			if (GetCardLogicValue(cbCardData[j])!=cbLogicValue) break;

			//设置扑克
			cbSameCardData[cbSameCount++]=cbCardData[j];
		}

		//保存结果
		switch (cbSameCount)
		{
		case 1:		//单张
			{
				AnalyseResult.cbSignedLogicVolue[AnalyseResult.cbSignedCount]=cbLogicValue;
				memcpy(&AnalyseResult.cbSignedCardData[(AnalyseResult.cbSignedCount++)*cbSameCount],cbSameCardData,cbSameCount);
				break;
			}
		case 2:		//两张
			{
				AnalyseResult.cbLONGLogicVolue[AnalyseResult.cbLONGCount]=cbLogicValue;
				memcpy(&AnalyseResult.cbLONGCardData[(AnalyseResult.cbLONGCount++)*cbSameCount],cbSameCardData,cbSameCount);
				break;
			}
		case 3:		//三张
			{
				AnalyseResult.cbThreeLogicVolue[AnalyseResult.cbThreeCount]=cbLogicValue;
				memcpy(&AnalyseResult.cbThreeCardData[(AnalyseResult.cbThreeCount++)*cbSameCount],cbSameCardData,cbSameCount);
				break;
			}
		case 4:		//四张
			{
				AnalyseResult.cbFourLogicVolue[AnalyseResult.cbFourCount]=cbLogicValue;
				memcpy(&AnalyseResult.cbFourCardData[(AnalyseResult.cbFourCount++)*cbSameCount],cbSameCardData,cbSameCount);
				break;
			}
		}

		//设置递增
		i+=cbSameCount-1;
	}

	return;
}

