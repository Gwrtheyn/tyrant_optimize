#include "titan.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/optional.hpp>
#include <boost/range/join.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/timer/timer.hpp>
#include <boost/tokenizer.hpp>
#include "read.h"
#include "tyrant_optimize.h"
#include "TOutPut.h"
#include "TTools.h"



Process*				g_pkProc = NULL;
ProcessData*			g_ProcessData = NULL;
unsigned				g_Iter = 0;
unsigned				g_CalcIter = 0;
bool					g_CalcMode = false;
Decks*					g_pkDecks;
Cards*					g_pkAllCards;
cTDeckCore				g_DeckCore;
unsigned				g_TitanZeroDeckMode = 0;
std::vector<Deck*>		g_EnemyDecks;
std::vector<Deck*>		g_EnemyDecksBackup;
std::vector<Deck*>		g_BenchmarkDecks;

unsigned					guiThreads = 14;
unsigned					guiIter = 32;
unsigned					guiGlobalLimit = 16;
unsigned					guiDeckSize = 8;
unsigned					guiCardLimitY = 8;
unsigned					guiComLimitY = 8;
unsigned					guiDomLimitY = 8;
unsigned					guiIdLimit = 2;
unsigned					guiReCalcIter = 1024;
unsigned					guiMaxList = 1024;
unsigned					guiOutList = 25;
unsigned					guiOutIdMax = 1;

Faction						gFactionLock = Faction::allfactions;


void cTDeck::CapEnemyDeck(unsigned uiEnemyCap)
{
	g_ProcessData->m_enemy_decks_.clear();
	for (unsigned i = 0; i < g_EnemyDecksBackup.size(); i++)
	{
		Deck* pkBackupDeck = g_EnemyDecksBackup[i];
		if (pkBackupDeck != NULL)
		{
			Deck* pkBackupDeckClone = pkBackupDeck->clone();
			if (pkBackupDeckClone != NULL)
			{
				unsigned uiSize = uiEnemyCap;
				if (pkBackupDeck->cards.size() < uiEnemyCap)
					uiSize = pkBackupDeck->cards.size();
				pkBackupDeckClone->cards.clear();
				for (unsigned j = 0; j < uiSize; j++)
					pkBackupDeckClone->cards.push_back(pkBackupDeck->cards[j]);
				pkBackupDeckClone->deck_size = uiSize;
				g_ProcessData->m_enemy_decks_.push_back(pkBackupDeckClone);
			};
		};
	};
	g_pkProc = g_ProcessData->GetNewProcess();
};

std::string cTDeck::GetDeckString()
{
	std::string sDeckStr("");
	sDeckStr = m_pkCom->m_name;
	sDeckStr += "," + m_pkDom->m_name;
	for (unsigned i = 0; i < GetCardsSize(); i++) { if (m_pkCards[i]) sDeckStr += "," + m_pkCards[i]->m_name; };
	return sDeckStr;
}

bool cTDeck::TiTanSim(unsigned uiMode,unsigned uiNumIterations)
{
	if (uiNumIterations == 0)
		return false;
	g_Iter += uiNumIterations;
	if (g_CalcMode == true)
	{
		m_uiWins = 1;
		m_uiDraws = 0;
		m_uiLosses = 0;
		m_fWinPct = 100;
		return true;
	};
	if (GetCardsSize() == 0)
		return false;
	std::string sDeckStr(GetDeckString());
	Deck* pkDeck = tfind_deck(*g_pkDecks, *g_pkAllCards, sDeckStr);
	if (pkDeck == nullptr)
	{
		std::cout << "Error: TiTanSim Failed " << sDeckStr << ".\n";
		return false;
	};	
	std::string sFortressStr("");
	if (m_pkFortress[uiMode][0] != NULL)
		sFortressStr = m_pkFortress[uiMode][0]->m_name;
	if (m_pkFortress[uiMode][1] != NULL)
		sFortressStr += "," + m_pkFortress[uiMode][1]->m_name;	
	Deck* pkD1 = g_pkProc->your_decks[0];
	pkD1->fortress_cards.clear();
	pkD1->add_forts(sFortressStr);
	pkD1->commander = pkDeck->commander;
	pkD1->alpha_dominion = pkDeck->alpha_dominion;
	pkD1->cards = pkDeck->cards;
	pkD1->deck_size = pkDeck->deck_size;
	std::string best_deck = pkD1->hash();
	EvaluatedResults zero_results = { EvaluatedResults::first_type(g_pkProc->enemy_decks.size()), 0 };
	std::unordered_map<std::string, EvaluatedResults> evaluated_decks{ { best_deck, zero_results } };
	EvaluatedResults& results = g_pkProc->evaluate(uiNumIterations, evaluated_decks.begin()->second);
	if (results.first.size() == 0)
		return false;
	m_uiWins = 0;
	m_uiDraws = 0;
	m_uiLosses = 0;
	for (unsigned index(0); index < results.first.size(); ++index)
	{
		m_uiWins += results.first[index].wins;
		m_uiDraws += results.first[index].draws;
		m_uiLosses += results.first[index].losses;
	};
	unsigned uiRounds = m_uiWins + m_uiDraws + m_uiLosses;
	if (uiRounds == 0)uiRounds = 1;
	m_fWinPct = (m_uiWins / (float)uiRounds) * 100;	
	for (unsigned i = 0; i < pkDeck->deck_size; i++)
	{
		Card* pkCards = (Card*)pkDeck->cards[i];
		if (pkCards != NULL)
		{
			cTCardStats* pkStats = &g_DeckCore.m_pkClusterMap[pkCards];
			if (pkStats)
			{				
				pkStats->m_uiWins += m_uiWins;
				pkStats->m_uiDraws += m_uiDraws;
				pkStats->m_uiLosses += m_uiLosses;
			};
		};
	};
	SimOutput(this);
	return true;
};


struct { bool operator()(cTDeck a, cTDeck b) const { return a.m_fWinPct > b.m_fWinPct; } } wsort;

unsigned cTDeckCore::GetBaseIds(cTDeck* pkDeck)
{
	if (pkDeck == NULL)
		return 0;
	if (pkDeck->m_uiBaseID == 0)
	{
		cTCardStats* pkStatsA = &g_DeckCore.m_pkClusterMap[pkDeck->m_pkCards[0]];
		cTCardStats* pkStatsB = &g_DeckCore.m_pkClusterMap[pkDeck->m_pkCards[1]];
		if ((pkStatsA!=NULL)&&(pkStatsB!=NULL))
			pkDeck->m_uiBaseID = pkStatsA->m_uiMapId + ( pkStatsB->m_uiMapId * 9999999);
	};
	return pkDeck->m_uiBaseID;
};

bool cTDeckCore::LoadGauntlet(std::string pkzGauntlet, DeckStrategy::DeckStrategy strategy, unsigned uiMode)
{
	g_EnemyDecks.clear();
	g_EnemyDecksBackup.clear();	
	TuReadCards();
	auto&& enemy_deck_list_parsed2 = parse_deck_list(pkzGauntlet, *g_pkDecks);
	for (auto deck_parsed : enemy_deck_list_parsed2)
	{
		Deck* lenemy_deck = tfind_deck(*g_pkDecks, *g_pkAllCards, deck_parsed.first);
		if (lenemy_deck)
		{
			lenemy_deck->strategy = strategy;
			//lenemy_deck->fortress_cards.clear();
			Deck* lClone = lenemy_deck->clone();
			if (lClone != NULL)
			{
				if (uiMode == 2)
				{
					g_EnemyDecks.push_back(lClone);
					g_EnemyDecksBackup.push_back(lClone);
				}else
				{
					g_BenchmarkDecks.push_back(lClone);
				};
			};
		}
	};
	return true;
};

void cTDeckCore::FilterSort(unsigned uiId, unsigned uiCom, unsigned uiDom, unsigned uiCard, unsigned uiCardGlobal,unsigned uiMax)
{	
	std::map<unsigned, unsigned>	m_IdMap;
	std::map<Card*, unsigned>		m_PhaseMap[15];
	std::map<Card*, unsigned>		m_GlobalMap;
	unsigned uiPhaseSize = 0;
	m_Phase[m_PageA].clear();	
	unsigned lMax = m_Phase[m_PageB].size();
	if (uiMax > 0)
		lMax = uiMax;
	std::sort(m_Phase[m_PageB].begin(), m_Phase[m_PageB].end(), wsort);		
	for (unsigned i = 0; i < m_Phase[m_PageB].size(); i++)
	{
		cTDeck* pkDeck = &m_Phase[m_PageB][i];
		if (pkDeck != NULL)
		{	
			Card* pkCom = pkDeck->m_pkCom;
			Card* pkDom = pkDeck->m_pkDom;
			if ((pkCom != NULL) && (pkDom != NULL))
			{
				unsigned uiBaseID = GetBaseIds(pkDeck);
				bool bAdd = true;
				if ((uiId > 0) && (m_IdMap[uiBaseID] >= uiId))
					bAdd = false;
				if ((uiCom > 0) && (m_PhaseMap[11][pkCom] >= uiCom))
					bAdd = false;
				if ((uiDom > 0) && (m_PhaseMap[12][pkDom] >= uiDom))
					bAdd = false;
				if (uiCard > 0)
				{
					for (unsigned j = 0; j < pkDeck->GetCardsSize(); j++)
					{
						Card* pkCard = pkDeck->m_pkCards[j];
						if (pkCard != NULL)
						{
							if ((uiCard > 0) && (m_PhaseMap[j][pkCard] >= uiCard))
								bAdd = false;
							if ((uiCardGlobal > 0) && (m_GlobalMap[pkCard] >= uiCardGlobal))
								bAdd = false;
						};
					};
				};				
				if ((bAdd) && (uiPhaseSize < lMax))
				{
					m_IdMap[uiBaseID]++;
					m_PhaseMap[11][pkCom]++;
					m_PhaseMap[12][pkDom]++;
					for (unsigned j = 0; j < pkDeck->GetCardsSize(); j++)
					{
						Card* pkCard = pkDeck->m_pkCards[j];
						if (pkCard != NULL)
						{
							m_PhaseMap[j][pkCard]++;
							m_GlobalMap[pkCard]++;
						};
					};
					//std::cout << (uiPhaseSize+1) << " ";
					//OutPutDeck(*pkDeck);
					uiPhaseSize++;
					m_Phase[m_PageA].push_back(*pkDeck);
				};
			};
		};
		
	};
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, GREEN);
	std::cout << "FilterSort: " << m_Phase[m_PageB].size() << " -> " << m_Phase[m_PageA].size() <<"\n";
	m_Phase[m_PageB].clear();
	m_PageA = 1 - m_PageA;
	m_PageB = 1 - m_PageB;	
};

void cTDeckCore::DoBuildID(unsigned uiMode,unsigned uiIter, cTDeck& TempDeck)
{
	m_Phase[m_PageA].clear();
	int iLast01 = -1;
	while (GetNextCard(&TempDeck, iLast01))
	{
		TempDeck.m_pkCards[0] = g_DeckCore.m_pkCards[iLast01];
		int iLast02 = -1;
		while (GetNextCard(&TempDeck, iLast02))
		{
			TempDeck.m_pkCards[1] = g_DeckCore.m_pkCards[iLast02];
			if (TempDeck.TiTanSim(uiMode,uiIter))
				m_Phase[m_PageA].push_back(TempDeck);
		};
	};
	m_PageA = 1 - m_PageA;
	m_PageB = 1 - m_PageB;
	//std::sort(m_Phase[m_PageB].begin(), m_Phase[m_PageB].end(), wsort);	
};

void cTDeckCore::DoCalcCardSlot(unsigned uiMode,unsigned uiIter, cTDeck& TempDeck, unsigned uiSlot)
{	
	m_Phase[m_PageA].clear();
	{
		for (unsigned i = 0; i < m_Phase[m_PageB].size(); i++)
		{			
			TempDeck = m_Phase[m_PageB][i];
			TempDeck.m_pkCards[uiSlot] = NULL;
			if (TempDeck.TiTanSim(uiMode,uiIter))
			{
				m_Phase[m_PageA].push_back(TempDeck);
			};
		};
	}	
	for (unsigned i = 0; i < m_Phase[m_PageB].size(); i++)
	{
		TempDeck = m_Phase[m_PageB][i];			
		int iLast01 = -1;
		while (GetNextCard(&TempDeck, iLast01))
		{			
			TempDeck.m_pkCards[uiSlot] = g_DeckCore.m_pkCards[iLast01];
			if (TempDeck.TiTanSim(uiMode,uiIter))
			{
				m_Phase[m_PageA].push_back(TempDeck);
			};
		};
	};
	m_PageA = 1 - m_PageA;
	m_PageB = 1 - m_PageB;
	std::sort(m_Phase[m_PageB].begin(), m_Phase[m_PageB].end(), wsort);	
};

void cTDeckCore::DoReCalc(unsigned uiMode,unsigned uiIter, cTDeck& TempDeck)
{	
	m_Phase[m_PageA].clear();	
	for (unsigned i = 0; i < m_Phase[m_PageB].size(); i++)
	{
		TempDeck = m_Phase[m_PageB][i];
		if (TempDeck.TiTanSim(uiMode,uiIter))
		{
			m_Phase[m_PageA].push_back(TempDeck);
		};
	};
	m_PageA = 1 - m_PageA;
	m_PageB = 1 - m_PageB;
	std::sort(m_Phase[m_PageB].begin(), m_Phase[m_PageB].end(), wsort);	
};

void cTDeckCore::DoCalcComDom(unsigned uiMode,unsigned uiIter, cTDeck& TempDeck,unsigned uiType)
{
	m_Phase[m_PageA].clear();
	for (unsigned i = 0; i < m_Phase[m_PageB].size(); i++)
		m_Phase[m_PageA].push_back(m_Phase[m_PageB][i]);
	if (uiType == 0)
	{
		for (unsigned i = 0; i < m_Phase[m_PageB].size(); i++)
		{
			TempDeck = m_Phase[m_PageB][i];
			for (unsigned j = 1; j < m_pkCommander.size(); j++)
			{
				TempDeck.m_pkCom = m_pkCommander[j];
				if (TempDeck.TiTanSim(uiMode,uiIter))
				{
					m_Phase[m_PageA].push_back(TempDeck);
				};
			};
		};
	}else
	{
		for (unsigned i = 0; i < m_Phase[m_PageB].size(); i++)
		{
			TempDeck = m_Phase[m_PageB][i];
			for (unsigned j = 1; j < m_pkDominion.size(); j++)
			{
				TempDeck.m_pkDom = m_pkDominion[j];
				if (TempDeck.TiTanSim(uiMode,uiIter))
				{
					m_Phase[m_PageA].push_back(TempDeck);
				};
			};
		};
	};
	m_PageA = 1 - m_PageA;
	m_PageB = 1 - m_PageB;
	std::sort(m_Phase[m_PageB].begin(), m_Phase[m_PageB].end(), wsort);	
};

void cTDeckCore::TBuildDeckMode(std::string pkzGauntlet, unsigned uiMode, Cards& all_cards)
{	
	Deck* pkD1 = g_pkProc->your_decks[0];
	if (uiMode == 0)
	{
		gamemode = surge;
		optimization_mode = OptimizationMode::war;
		pkD1->strategy = DeckStrategy::ordered;
		//pkD1->fortress_cards.clear();
		LoadGauntlet(pkzGauntlet, DeckStrategy::random, 2);
	}
	else if (uiMode == 1)
	{
		gamemode = fight;
		optimization_mode = OptimizationMode::war_defense;
		pkD1->strategy = DeckStrategy::random;
		//pkD1->fortress_cards.clear();
		LoadGauntlet(pkzGauntlet, DeckStrategy::ordered, 2);
	};
	if ((m_pkCommander.size()==0) || (m_pkDominion.size()==0)|| (m_pkCards.size()==0))
		return;

	unsigned uiIter = guiIter;
	unsigned luiId = guiIdLimit;
	unsigned luiCom = guiComLimitY;
	unsigned luiDom = guiDomLimitY;
	unsigned luiCard = guiCardLimitY;
	unsigned luiCardGlobal = guiGlobalLimit;
	unsigned luiMax = guiMaxList;
	
	cTDeck TempDeck;
	TempDeck.m_pkCom = m_pkCommander[0];
	TempDeck.m_pkDom = m_pkDominion[0];
	for (unsigned k = 0; k < 10; k++)
		TempDeck.m_pkCards[k] = NULL;
	TempDeck.CapEnemyDeck(2);	
	DoBuildID(uiMode,uiIter,TempDeck);
	FilterSort(luiId, 0, 0, luiCard, luiCardGlobal, luiMax*8);		
	TempDeck.CapEnemyDeck(3);	
	DoCalcCardSlot(uiMode,uiIter, TempDeck, 2);		
	FilterSort(luiId, 0, 0, luiCard, luiCardGlobal, luiMax);	
	DoCalcComDom(uiMode,uiIter, TempDeck, 0);
	FilterSort(luiId, luiCom, 0, luiCard, luiCardGlobal, luiMax);	
	DoCalcComDom(uiMode,uiIter, TempDeck, 1);
	FilterSort(luiId, luiCom, luiDom, luiCard, luiCardGlobal, luiMax);	
	for (unsigned k = 3; k < guiDeckSize; k++)
	{
		TempDeck.CapEnemyDeck(k+1);		
		DoCalcCardSlot(uiMode,uiIter, TempDeck, k);
		FilterSort(luiId, luiCom, luiDom, luiCard, luiCardGlobal, luiMax);
	}/**/
	FilterSort(luiId, luiCom, luiDom, luiCard, luiCardGlobal, luiMax);	
	TempDeck.CapEnemyDeck(10);
	DoReCalc(uiMode,guiReCalcIter, TempDeck);
	FilterSort(guiOutIdMax, luiCom, luiDom, luiCard, luiCardGlobal, guiOutList);	
	m_SimData[uiMode].clear();
	unsigned uiSize = m_Phase[m_PageB].size();
	for (unsigned i = 0; i < uiSize; i++)
	{
		m_SimData[uiMode].push_back(m_Phase[m_PageB][i]);
	};
	std::cout << "\n";
};


void cTDeckCore::TBenchmarkMode(std::string pkzGauntlet, unsigned uiMode, Cards& all_cards)
{	
	Deck* pkD1 = g_pkProc->your_decks[0];
	g_BenchmarkDecks.clear();
	LoadGauntlet(pkzGauntlet, DeckStrategy::random, 0);	
	LoadGauntlet(pkzGauntlet, DeckStrategy::ordered, 1);
	if (uiMode == 0)
	{
		gamemode = surge;
		optimization_mode = OptimizationMode::war;
		pkD1->strategy = DeckStrategy::ordered;
		//pkD1->fortress_cards.clear();
		LoadGauntlet(pkzGauntlet, DeckStrategy::random, 2);
	}
	else if (uiMode == 1)
	{
		gamemode = fight;
		optimization_mode = OptimizationMode::war_defense;
		pkD1->strategy = DeckStrategy::random;
		//pkD1->fortress_cards.clear();
		LoadGauntlet(pkzGauntlet, DeckStrategy::ordered, 2);
	};
	if ((m_pkCommander.size()==0) || (m_pkDominion.size()==0)|| (m_pkCards.size()==0))
		return;	
	unsigned uiIter = guiIter;
	cTDeck TempDeck;
	TempDeck.CapEnemyDeck(10);
	TempDeck.m_pkCom = m_pkCommander[0];
	TempDeck.m_pkDom = m_pkDominion[0];
	for (unsigned k = 0; k < 10; k++)
		TempDeck.m_pkCards[k] = NULL;		
	m_Phase[m_PageA].clear();
	for (unsigned uia = 0; uia < m_pkCards.size(); uia++)
	{
		Card* pkCard = m_pkCards[uia];
		if (pkCard != NULL)
		{
			cTCardStats* pkStats = &m_BenchmarkStack[pkCard];
			if (pkStats)
			{
				cTDeck BaseDeck;
				cTDeck StackDeck;
				StackDeck.ResetData();
				for (unsigned i = 0; i < g_BenchmarkDecks.size(); i++)
				{
					Deck* pkTestDeck = g_BenchmarkDecks[i];
					if (pkTestDeck != NULL)
					{
						TempDeck.m_pkCom = (Card*)pkTestDeck->commander;
						TempDeck.m_pkDom = (Card*)pkTestDeck->alpha_dominion;
						for (unsigned k = 0; k < pkTestDeck->cards.size(); k++)
							TempDeck.m_pkCards[k] = (Card*)pkTestDeck->cards[k];
						if (TempDeck.TiTanSim(uiMode,uiIter))
						{
							cTCardStats* pkStatsBase = &m_BenchmarkBase[pkCard];
							if (pkStatsBase)
							{
								pkStatsBase->m_pkCard = pkCard;
								pkStatsBase->m_uiWins += TempDeck.m_uiWins;
								pkStatsBase->m_uiDraws += TempDeck.m_uiDraws;
								pkStatsBase->m_uiLosses += TempDeck.m_uiLosses;
							};
							BaseDeck = TempDeck;
							for (unsigned uic = 0; uic < pkTestDeck->cards.size(); uic++)
							{
								TempDeck.ResetData();
								TempDeck.m_pkCom = (Card*)pkTestDeck->commander;
								TempDeck.m_pkDom = (Card*)pkTestDeck->alpha_dominion;
								for (unsigned k = 0; k < pkTestDeck->cards.size(); k++)
									TempDeck.m_pkCards[k] = (Card*)pkTestDeck->cards[k];
								TempDeck.m_pkCards[uic] = pkCard;
								if (TempDeck.TiTanSim(uiMode,uiIter))
								{
									pkStats->m_pkCard = pkCard;
									pkStats->m_uiWins += TempDeck.m_uiWins;
									pkStats->m_uiDraws += TempDeck.m_uiDraws;
									pkStats->m_uiLosses += TempDeck.m_uiLosses;
								};
							};
						};
					};
				};				
			};
		};
	};		
	std::cout << "\n";
};


void cTDeckCore::TBenchmarkComDomMode(std::string pkzGauntlet, unsigned uiMode, Cards& all_cards,bool bDom)
{	
	Deck* pkD1 = g_pkProc->your_decks[0];
	g_BenchmarkDecks.clear();
	LoadGauntlet(pkzGauntlet, DeckStrategy::random, 0);	
	LoadGauntlet(pkzGauntlet, DeckStrategy::ordered, 1);
	if (uiMode == 0)
	{
		gamemode = surge;
		optimization_mode = OptimizationMode::war;
		pkD1->strategy = DeckStrategy::ordered;
		//pkD1->fortress_cards.clear();
		LoadGauntlet(pkzGauntlet, DeckStrategy::random, 2);
	}
	else if (uiMode == 1)
	{
		gamemode = fight;
		optimization_mode = OptimizationMode::war_defense;
		pkD1->strategy = DeckStrategy::random;
		//pkD1->fortress_cards.clear();
		LoadGauntlet(pkzGauntlet, DeckStrategy::ordered, 2);
	};
	if ((m_pkCommander.size()==0) || (m_pkDominion.size()==0)|| (m_pkCards.size()==0))
		return;	
	unsigned uiIter = guiIter;
	cTDeck TempDeck;
	TempDeck.CapEnemyDeck(10);
	TempDeck.m_pkCom = m_pkCommander[0];
	TempDeck.m_pkDom = m_pkDominion[0];
	for (unsigned k = 0; k < 10; k++)
		TempDeck.m_pkCards[k] = NULL;		
	m_Phase[m_PageA].clear();

	unsigned uiListSize = m_pkCommander.size();
	if (bDom==true)
		uiListSize = m_pkDominion.size();
	for (unsigned uia = 0; uia < uiListSize; uia++)
	{
		Card* pkCard = NULL;
		if (bDom==true)
			pkCard = m_pkDominion[uia];
		else
			pkCard = m_pkCommander[uia];
		if (pkCard != NULL)
		{
			cTCardStats* pkStats = &m_BenchmarkStack[pkCard];
			if (pkStats)
			{
				cTDeck BaseDeck;
				cTDeck StackDeck;
				StackDeck.ResetData();
				for (unsigned i = 0; i < g_BenchmarkDecks.size(); i++)
				{
					Deck* pkTestDeck = g_BenchmarkDecks[i];
					if (pkTestDeck != NULL)
					{
						TempDeck.m_pkCom = (Card*)pkTestDeck->commander;
						TempDeck.m_pkDom = (Card*)pkTestDeck->alpha_dominion;
						for (unsigned k = 0; k < pkTestDeck->cards.size(); k++)
							TempDeck.m_pkCards[k] = (Card*)pkTestDeck->cards[k];
						if (TempDeck.TiTanSim(uiMode,uiIter))
						{
							cTCardStats* pkStatsBase = &m_BenchmarkBase[pkCard];
							if (pkStatsBase)
							{
								pkStatsBase->m_pkCard = pkCard;
								pkStatsBase->m_uiWins += TempDeck.m_uiWins;
								pkStatsBase->m_uiDraws += TempDeck.m_uiDraws;
								pkStatsBase->m_uiLosses += TempDeck.m_uiLosses;
							};
							BaseDeck = TempDeck;							
							TempDeck.ResetData();
							
							TempDeck.m_pkCom = (Card*)pkTestDeck->commander;
							TempDeck.m_pkDom = (Card*)pkTestDeck->alpha_dominion;
							if (bDom==true)
								TempDeck.m_pkDom = (Card*)pkCard;
							else
								TempDeck.m_pkCom = (Card*)pkCard;
							for (unsigned k = 0; k < pkTestDeck->cards.size(); k++)
								TempDeck.m_pkCards[k] = (Card*)pkTestDeck->cards[k];							
							if (TempDeck.TiTanSim(uiMode,uiIter))
							{
								pkStats->m_pkCard = pkCard;
								pkStats->m_uiWins += TempDeck.m_uiWins;
								pkStats->m_uiDraws += TempDeck.m_uiDraws;
								pkStats->m_uiLosses += TempDeck.m_uiLosses;
							};							
						};
					};
				};				
			};
		};
	};		
	std::cout << "\n";
};

void cTDeckCore::TBenchmarkFortress(std::string pkzGauntlet, unsigned uiMode, Cards& all_cards)
{	
	Deck* pkD1 = g_pkProc->your_decks[0];
	g_BenchmarkDecks.clear();
	LoadGauntlet(pkzGauntlet, DeckStrategy::random, 0);	
	LoadGauntlet(pkzGauntlet, DeckStrategy::ordered, 1);
	if (uiMode == 0)
	{
		gamemode = surge;
		optimization_mode = OptimizationMode::war;
		pkD1->strategy = DeckStrategy::ordered;
		//pkD1->fortress_cards.clear();
		LoadGauntlet(pkzGauntlet, DeckStrategy::random, 2);
	}
	else if (uiMode == 1)
	{
		gamemode = fight;
		optimization_mode = OptimizationMode::war_defense;
		pkD1->strategy = DeckStrategy::random;
		//pkD1->fortress_cards.clear();
		LoadGauntlet(pkzGauntlet, DeckStrategy::ordered, 2);
	};
	if ((m_pkCommander.size()==0) || (m_pkDominion.size()==0)|| (m_pkCards.size()==0))
		return;
	
	unsigned uiIter = guiIter;		
	cTDeck TempDeck;
	TempDeck.CapEnemyDeck(10);
	
	if (uiMode == 0)
	{
		for (unsigned i(0); i < g_ProcessData->m_enemy_decks_.size(); ++i)
		{
			g_ProcessData->m_enemy_decks_[i]->fortress_cards.clear();
			g_ProcessData->m_enemy_decks_[i]->add_forts("Minefield,Minefield");
		};
	}else if (uiMode == 1)
	{
		for (unsigned i(0); i < g_ProcessData->m_enemy_decks_.size(); ++i)
		{
			g_ProcessData->m_enemy_decks_[i]->fortress_cards.clear();
			g_ProcessData->m_enemy_decks_[i]->add_forts("Death Factory,Death Factory");
		};
	};			

	TempDeck.m_pkCom = m_pkCommander[0];
	TempDeck.m_pkDom = m_pkDominion[0];
	for (unsigned k = 0; k < 10; k++)
		TempDeck.m_pkCards[k] = NULL;		
	m_Phase[m_PageA].clear();

	unsigned uiListSize = m_pkFortress[uiMode].size();	
	for (unsigned uia = 0; uia < uiListSize; uia++)
	for (unsigned uib = 0; uib < uiListSize; uib++)
	{
		Card* pkCard1=m_pkFortress[uiMode][uia];
		Card* pkCard2=m_pkFortress[uiMode][uib];
		if ((pkCard1 != NULL)&&(pkCard2 != NULL))
		{
			cTCardStats* pkStats1 = &m_BenchmarkStack[pkCard1];
			cTCardStats* pkStats2 = &m_BenchmarkStack[pkCard2];
			if ((pkStats1)&&(pkStats2))
			{
				cTDeck BaseDeck;
				cTDeck StackDeck;
				StackDeck.ResetData();
				for (unsigned i = 0; i < g_BenchmarkDecks.size(); i++)
				{
					Deck* pkTestDeck = g_BenchmarkDecks[i];
					if (pkTestDeck != NULL)
					{
						TempDeck.m_pkCom = (Card*)pkTestDeck->commander;
						TempDeck.m_pkDom = (Card*)pkTestDeck->alpha_dominion;
						for (unsigned k = 0; k < pkTestDeck->cards.size(); k++)
							TempDeck.m_pkCards[k] = (Card*)pkTestDeck->cards[k];
						TempDeck.m_pkFortress[0][0] = NULL;
						TempDeck.m_pkFortress[0][1] = NULL;
						TempDeck.m_pkFortress[1][0] = NULL;
						TempDeck.m_pkFortress[1][1] = NULL;
						if (TempDeck.TiTanSim(uiMode,uiIter))
						{
							cTCardStats* pkStatsBase1 = &m_BenchmarkBase[pkCard1];
							if (pkStatsBase1)
							{
								pkStatsBase1->m_pkCard = pkCard1;
								pkStatsBase1->m_uiWins += TempDeck.m_uiWins;
								pkStatsBase1->m_uiDraws += TempDeck.m_uiDraws;
								pkStatsBase1->m_uiLosses += TempDeck.m_uiLosses;
							};
							cTCardStats* pkStatsBase2 = &m_BenchmarkBase[pkCard2];
							if (pkStatsBase2)
							{
								pkStatsBase2->m_pkCard = pkCard2;
								pkStatsBase2->m_uiWins += TempDeck.m_uiWins;
								pkStatsBase2->m_uiDraws += TempDeck.m_uiDraws;
								pkStatsBase2->m_uiLosses += TempDeck.m_uiLosses;
							};
							BaseDeck = TempDeck;							
							TempDeck.ResetData();
							
							TempDeck.m_pkCom = (Card*)pkTestDeck->commander;
							TempDeck.m_pkDom = (Card*)pkTestDeck->alpha_dominion;
														
							TempDeck.m_pkFortress[uiMode][0] = pkCard1;
							TempDeck.m_pkFortress[uiMode][1] = pkCard2;
							
							for (unsigned k = 0; k < pkTestDeck->cards.size(); k++)
								TempDeck.m_pkCards[k] = (Card*)pkTestDeck->cards[k];							
							if (TempDeck.TiTanSim(uiMode,uiIter))
							{
								pkStats1->m_pkCard = pkCard1;
								pkStats1->m_uiWins += TempDeck.m_uiWins;
								pkStats1->m_uiDraws += TempDeck.m_uiDraws;
								pkStats1->m_uiLosses += TempDeck.m_uiLosses;

								pkStats2->m_pkCard = pkCard2;
								pkStats2->m_uiWins += TempDeck.m_uiWins;
								pkStats2->m_uiDraws += TempDeck.m_uiDraws;
								pkStats2->m_uiLosses += TempDeck.m_uiLosses;
							};							
						};
					};
				};				
			};
		};
	};		
	std::cout << "\n";
};

void CoreScan(Process& proc, Decks& decks, Cards& all_cards, unsigned uiMode)
{	
	opt_num_threads = guiThreads;
	std::cout << "\n";
	if (g_ProcessData == NULL)
		return;

	g_pkProc = g_ProcessData->GetProcess();
	g_pkDecks = &decks;
	g_pkAllCards = &all_cards;
	if (uiMode == 6)//TTBenchmarkFortress
	{
		g_DeckCore.m_BenchmarkBase.clear();
		g_DeckCore.m_BenchmarkStack.clear();
		g_CalcMode = true;
		g_Iter = 0;
		g_DeckCore.TBenchmarkFortress("TUTitan_Def", 0, all_cards);
		g_DeckCore.TBenchmarkFortress("TUTitan_Off", 1, all_cards);
		g_CalcMode = false;
		InitTimes();
		g_DeckCore.m_BenchmarkBase.clear();
		g_DeckCore.m_BenchmarkStack.clear();
		g_CalcIter = g_Iter + 1;
		g_Iter = 0;
		g_DeckCore.TBenchmarkFortress("TUTitan_Def", 0, all_cards);
		TuBenchmarkCardsOut("FortressOff",3);
		g_DeckCore.m_BenchmarkBase.clear();
		g_DeckCore.m_BenchmarkStack.clear();
		g_DeckCore.TBenchmarkFortress("TUTitan_Off", 1, all_cards);		
		TuBenchmarkCardsOut("FortressDef",3);
		std::cout << "\n";		
	}else	
	if ((uiMode == 4)||(uiMode == 5)) //TTBenchmark Com Dom
	{
		bool bDom = false;
		if (uiMode == 5)
			bDom = true;
		g_DeckCore.m_BenchmarkBase.clear();
		g_DeckCore.m_BenchmarkStack.clear();
		g_CalcMode = true;
		g_Iter = 0;
		g_DeckCore.TBenchmarkComDomMode("TUTitan_Def", 0, all_cards,bDom);
		g_DeckCore.TBenchmarkComDomMode("TUTitan_Off", 1, all_cards,bDom);
		g_CalcMode = false;
		InitTimes();
		g_DeckCore.m_BenchmarkBase.clear();
		g_DeckCore.m_BenchmarkStack.clear();
		g_CalcIter = g_Iter + 1;
		g_Iter = 0;
		g_DeckCore.TBenchmarkComDomMode("TUTitan_Def", 0, all_cards,bDom);
		g_DeckCore.TBenchmarkComDomMode("TUTitan_Off", 1, all_cards,bDom);		
		if (uiMode == 4)
			TuBenchmarkCardsOut("BenchmarkCommander",1);
		if (uiMode == 5)
			TuBenchmarkCardsOut("BenchmarkAlphaDominion",1);
		std::cout << "\n";		
	}else
	if (uiMode == 3)//TTBenchmark
	{
		g_DeckCore.m_BenchmarkBase.clear();
		g_DeckCore.m_BenchmarkStack.clear();
		g_CalcMode = true;
		g_Iter = 0;
		g_DeckCore.TBenchmarkMode("TUTitan_Def", 0, all_cards);
		g_DeckCore.TBenchmarkMode("TUTitan_Off", 1, all_cards);
		g_CalcMode = false;
		InitTimes();
		g_DeckCore.m_BenchmarkBase.clear();
		g_DeckCore.m_BenchmarkStack.clear();
		g_CalcIter = g_Iter + 1;
		g_Iter = 0;
		g_DeckCore.TBenchmarkMode("TUTitan_Def", 0, all_cards);
		g_DeckCore.TBenchmarkMode("TUTitan_Off", 1, all_cards);
		std::cout << "\n";	
		TuBenchmarkCardsOut("BenchmarkCards",0);
		std::cout << "\n";		
	}else
	if (uiMode==2)//TTBestCard
	{
		UpdateScore(all_cards);
	}else
	if (uiMode == 1)//TTGauntlet
	{		
		g_CalcMode = true;
		g_Iter = 0;
		g_DeckCore.TBuildDeckMode("TUTitan_Def", 0, all_cards);
		g_DeckCore.TBuildDeckMode("TUTitan_Off", 1, all_cards);
		g_CalcMode = false;
		InitTimes();
		g_CalcIter = g_Iter + 1;
		g_Iter = 0;
		g_DeckCore.TBuildDeckMode("TUTitan_Def", 0, all_cards);
		g_DeckCore.TBuildDeckMode("TUTitan_Off", 1, all_cards);
		std::cout << "\n\n";
		g_DeckCore.m_Phase[g_DeckCore.m_PageB].clear();
		for (unsigned i = 0; i < g_DeckCore.m_SimData[0].size(); i++)
		{
			g_DeckCore.m_Phase[g_DeckCore.m_PageB].push_back(g_DeckCore.m_SimData[0][i]);
		};
		//g_DeckCore.OutPutDeckList();
		std::cout << "\n";
		g_DeckCore.m_Phase[g_DeckCore.m_PageB].clear();
		for (unsigned i = 0; i < g_DeckCore.m_SimData[1].size(); i++)
		{
			g_DeckCore.m_Phase[g_DeckCore.m_PageB].push_back(g_DeckCore.m_SimData[1][i]);
		};
		//g_DeckCore.OutPutDeckList();
		std::cout << "\n";
		g_DeckCore.MakeGauntlet(all_cards);
	};
};
