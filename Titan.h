#ifndef TITAN_H_INCLUDED
#define TITAN_H_INCLUDED

#pragma once

#include <iostream>
#include <deque>
#include <windows.h> 
#include "card.h"
#include "cards.h"
#include "deck.h"

enum {
    BLACK             = 0,
    DARKBLUE          = FOREGROUND_BLUE,
    DARKGREEN         = FOREGROUND_GREEN,
    DARKCYAN          = FOREGROUND_GREEN | FOREGROUND_BLUE,
    DARKRED           = FOREGROUND_RED,
    DARKMAGENTA       = FOREGROUND_RED | FOREGROUND_BLUE,
    DARKYELLOW        = FOREGROUND_RED | FOREGROUND_GREEN,
    DARKGRAY          = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    GRAY              = FOREGROUND_INTENSITY,
    BLUE              = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
    GREEN             = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
    CYAN              = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
    RED               = FOREGROUND_INTENSITY | FOREGROUND_RED,
    MAGENTA           = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
    YELLOW            = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
    WHITE             = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
 };


class Process;
extern Process*					g_pkProc;
extern unsigned					g_Iter;
extern unsigned					g_CalcIter;
extern unsigned					g_ClusterSize;
extern bool						g_CalcMode;
extern Decks*					g_pkDecks;
extern Cards*					g_pkAllCards;
extern unsigned					g_TitanZeroDeckMode;
extern std::vector<Deck*>		g_EnemyDecks;

extern unsigned					guiThreads;
extern unsigned					guiIter;
extern unsigned					guiGlobalLimit;
extern unsigned					guiDeckSize;
extern unsigned					guiCardLimitY;
extern unsigned					guiComLimitY;
extern unsigned					guiDomLimitY;
extern unsigned					guiIdLimit;
extern unsigned					guiReCalcIter;
extern unsigned					guiMaxList;
extern unsigned					guiOutList;
extern unsigned					guiOutIdMax;

class cTDeck
{
public:	
	unsigned				m_uiBaseID;
	unsigned				m_uiWins;
	unsigned				m_uiDraws;
	unsigned				m_uiLosses;
	float					m_fWinPct;
	Card*					m_pkCards[10];
	Card*					m_pkCom;
	Card*					m_pkDom;
	Card*					m_pkFortress[2][2];	
public:
	cTDeck()
	{				
		m_uiBaseID = 0;
		m_fWinPct = 0;
		m_uiWins=0;
		m_uiDraws=0;
		m_uiLosses=0;		
		m_pkCom = NULL;
		m_pkDom = NULL;
		m_pkFortress[0][0] = NULL;
		m_pkFortress[0][1] = NULL;
		m_pkFortress[1][0] = NULL;
		m_pkFortress[1][1] = NULL;
		for (unsigned i = 0; i < 10; i++)
			m_pkCards[i] = NULL;
	};
	~cTDeck()
	{				
		m_uiBaseID = 0;
		m_fWinPct = 0;
		m_uiWins=0;
		m_uiDraws=0;
		m_uiLosses=0;		
		m_pkCom = NULL;
		m_pkDom = NULL;
		m_pkFortress[0][0] = NULL;
		m_pkFortress[0][1] = NULL;
		m_pkFortress[1][0] = NULL;
		m_pkFortress[1][1] = NULL;
		for (unsigned i = 0; i < 10; i++)
			m_pkCards[i] = NULL;
	};	
	inline void ResetData()
	{
		m_fWinPct = 0;
		m_uiWins=0;
		m_uiDraws=0;
		m_uiLosses=0;	
	};
	inline unsigned GetCardsSize()
	{
		for (unsigned i = 0; i < 10; i++)
			if (m_pkCards[i] == NULL)
				return i;
		return 10;
	};
	inline cTDeck& operator=(const cTDeck& other)
	{											
		m_uiWins=other.m_uiWins;
		m_uiDraws=other.m_uiDraws;
		m_uiLosses=other.m_uiLosses;		
		m_fWinPct=other.m_fWinPct;	
		m_pkCom=other.m_pkCom;
		m_pkDom=other.m_pkDom;
		m_pkFortress[0][0]=other.m_pkFortress[0][0];
		m_pkFortress[0][1]=other.m_pkFortress[0][1];
		m_pkFortress[1][0]=other.m_pkFortress[1][0];
		m_pkFortress[1][1]=other.m_pkFortress[1][1];
		m_uiBaseID=other.m_uiBaseID;		
		for (unsigned i = 0; i < 10; i++)
			m_pkCards[i] = other.m_pkCards[i];
		return *this;
	};
	unsigned GetNumCopys(Card* pkCard)
	{		
		unsigned uiNum = 0;
		for (unsigned i = 0; i < GetCardsSize(); i++)
			if (m_pkCards[i] == pkCard) { uiNum++; };
		return uiNum;
	};
	void CapEnemyDeck(unsigned uiEnemyCap);
	std::string GetDeckString();
	bool TiTanSim(unsigned uiMode,unsigned uiNumIterations);
	
};

class cTCardStats
{
public:	
	const Card*				m_pkCard;
	unsigned				m_uiMapId;
	unsigned				m_uiWins;
	unsigned				m_uiDraws;
	unsigned				m_uiLosses;
	unsigned				m_uiCopys;
	float					m_fWinPct;
	unsigned				m_uiInstance;
	float                   m_fScore;			
	cTCardStats() :
        m_pkCard(NULL),
		m_uiMapId(0),
		m_uiWins(0),
		m_uiDraws(0),
		m_uiLosses(0),
		m_uiCopys(0),
		m_fWinPct(0),		
		m_uiInstance(0),
		m_fScore(0)
	{					
	};
	inline cTCardStats& operator=(const cTCardStats& other)
	{											
		m_pkCard=other.m_pkCard;
		m_uiMapId=other.m_uiMapId;
		m_uiWins=other.m_uiWins;
		m_uiDraws=other.m_uiDraws;
		m_uiLosses=other.m_uiLosses;
		m_uiCopys=other.m_uiCopys;
		m_fWinPct=other.m_fWinPct;	
		m_uiInstance=other.m_uiInstance;		
		m_fScore=other.m_fScore;		
		return *this;
	};
};

class cTDeckCore
{
public:
	std::deque<Card*>			m_pkCommander;
	std::deque<Card*>			m_pkDominion;
	std::deque<Card*>			m_pkCards;
	std::deque<Card*>			m_pkFortress[2];
	std::deque<cTDeck>			m_Phase[3];
	std::deque<cTDeck>			m_SimData[2];
	unsigned					m_PageA;
	unsigned					m_PageB;
	unsigned					m_uiCommanderPos;
	unsigned					m_uiDominionPos;
	unsigned					m_uiCardPos;
	unsigned							m_uiClusterCount;
	std::map<const Card*, cTCardStats>	m_pkClusterMap;
	std::map<const Card*, cTCardStats>	m_BenchmarkBase;
	std::map<const Card*, cTCardStats>	m_BenchmarkStack;
public:
	cTDeckCore()
	{		
		m_pkCommander.clear();
		m_pkDominion.clear();
		m_pkCards.clear();		
		m_pkFortress[0].clear();		
		m_pkFortress[1].clear();		
		m_Phase[0].clear();
		m_Phase[1].clear();
		m_Phase[2].clear();
		m_SimData[0].clear();
		m_SimData[1].clear();
		m_BenchmarkBase.clear();
		m_BenchmarkStack.clear();
		m_PageA=0;
		m_PageB=1;
		m_uiCommanderPos=0;
		m_uiDominionPos=0;
		m_uiCardPos=0;

		m_uiClusterCount = 0;
		m_pkClusterMap.clear();
	};	
	cTCardStats* AddCardToMap(const Card* pkCard)
	{
		cTCardStats* pkStats = &m_pkClusterMap[pkCard];
		if (pkStats)
		{
			if (pkStats->m_uiMapId == 0)
			{
				pkStats->m_uiMapId = m_uiClusterCount+1;
				pkStats->m_pkCard = pkCard;
				m_uiClusterCount++;				
			};
			return pkStats;
		};
		return NULL;
	};	
	bool GetNextCard(cTDeck* pkDeck, int& iLast)
	{
		for (unsigned uia = (unsigned)(iLast + 1); uia < m_pkCards.size(); uia++)		
		{
			Card* pkCard = m_pkCards[uia];
			if (pkCard != NULL)
			{							
				cTCardStats* pkStats = &m_pkClusterMap[pkCard];
				if (pkStats)
				{
					unsigned uiCopys = pkDeck->GetNumCopys(pkCard);
					if ((uiCopys + 1) <= pkStats->m_uiCopys)
					{
						iLast = uia;
						return  true;
					};
				};
			};
		};
		return false;
	};		
	void MakeHtml(Cards& all_cards);
	unsigned GetBaseIds(cTDeck* pkDeck);
	
	void FilterSort(unsigned uiId, unsigned uiCom, unsigned uiDom, unsigned uiCard, unsigned uiCardGlobal, unsigned uiMax);	
	bool LoadGauntlet(std::string pkzGauntlet, DeckStrategy::DeckStrategy strategy,unsigned uiMode);

	void DoBuildID(unsigned uiMode,unsigned uiIter, cTDeck& TempDeck);
	void DoReCalc(unsigned uiMode,unsigned uiIter, cTDeck& TempDeck);
	void DoCalcCardSlot(unsigned uiMode,unsigned uiIter, cTDeck& TempDeck, unsigned uiSlot);
	void DoCalcComDom(unsigned uiMode,unsigned uiIter, cTDeck& TempDeck,unsigned uiType);
	void TBuildDeckMode(std::string pkzGauntlet, unsigned uiMode, Cards& all_cards);
	void TBenchmarkMode(std::string pkzGauntlet, unsigned uiMode, Cards& all_cards);	
	void TBenchmarkComDomMode(std::string pkzGauntlet, unsigned uiMode, Cards& all_cards, bool bDom);
	void TBenchmarkFortress(std::string pkzGauntlet, unsigned uiMode, Cards& all_cards);

	void OutPutDeck(cTDeck& TempDeck);
	void OutPutDeckList();
};

extern cTDeckCore				g_DeckCore;


void InitTimes();
void TuBenchmarkCardsOut(std::string pkzFileName,unsigned uiMode);
void TuReadCards();
void SimOutput(cTDeck* pkctDeck);
void OutputCard(Card* pkCard, bool bSHowCopys);
Card* FReadCard(std::string sData, Cards& all_cards);
Deck* tfind_deck(Decks& decks, const Cards& all_cards, std::string deck_name);
void UpdateScore(Cards& all_cards);
void OutPutDeck(Deck* pkDeck);
void CoreScan(Process& proc,Decks& decks, Cards& all_cards, unsigned uiMode);

#endif //TITAN_H_INCLUDED
