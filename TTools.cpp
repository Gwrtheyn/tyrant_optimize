#include "titan.h"

#include "read.h"
#include "sim.h"
#include "TOutPut.h"

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

#include <iostream>
#include <fstream>
using namespace std;

std::deque<cTDeck>					g_Buffer;
boost::posix_time::ptime			gtStartTime;
boost::posix_time::ptime			gUpdateTime;



float TTCalcBaseScore(Cards& all_cards, Card* pkCard, unsigned uiInstance);

void UpdateScore(Cards& all_cards)
{	
	std::deque<cTCardStats*>			pkstats;
	pkstats.clear();

	for (Card* card : all_cards.all_cards)
	{
		if ((card->m_fusion_level == 2) && (card->m_level == 6) && (card == card->m_top_level_card))
		{
			cTCardStats* pkStats = g_DeckCore.AddCardToMap(card);
			if (pkStats)
			{
				pkStats->m_fScore = (unsigned)TTCalcBaseScore(all_cards,(Card*)card,1);
				pkstats.push_back(pkStats);				
			};
		};
	};
	struct { bool operator()(cTCardStats* a, cTCardStats* b) const {return a->m_fScore > b->m_fScore;}} bScoresort;
    std::sort(pkstats.begin(),pkstats.end(),bScoresort);		
	TxI();
	
	{
		TxO(WHITE, "A quick List for Tyrant Unleashed."); TxN();
		TxO(WHITE, "Sorted by Stats."); TxN();
		TxO(WHITE, "for easy search of Cards."); TxN(); TxN();
		TxO(DARKYELLOW, "[E]=Epic ");TxO(MAGENTA,"[L]=Legend ");TxO(CYAN,"[V]=Vindi ");TxO(BLUE,"[M]=Mythic "); TxN();
		TxO(DARKBLUE,"I.=Imperial ");TxO(DARKYELLOW,"D.=raiDer ");TxO(DARKRED,"B.=Bloodthirsty ");TxO(GRAY,"X.=Xeno ");TxO(BLUE,"T.=righTeous ");TxO(CYAN,"P.=Progenitor ");TxN();		
		TxO(BLUE,"P_=Trigger Play ");TxO(MAGENTA,"A_=Trigger Attacked ");TxO(RED,"D_=Trigger Play Death ");TxO(MAGENTA, "[S]=Summon");TxN();TxN();
	};
	unsigned iCount = 1;
	for (unsigned j = 1; j < pkstats.size(); j++)
	{
		if (pkstats[j]->m_uiInstance < 2)
		{
			unsigned uiMax = pkstats.size()/2;			
			if (guiOutList > 0)
				uiMax = guiOutList;
			uiMax = 500;			
			if (iCount <= uiMax)
			{											
				const Card* card = pkstats[j]->m_pkCard;
				if (card != NULL)
				{									
					TxO(WHITE,Txi(iCount));//TxO(MAGENTA," ");
					//TxO(GREEN,"[");TxO(GREEN,Txf(pkstats[j]->m_fScore));TxO(GREEN,"]");
					TxTab(5,"");
					//std::cout << card->m_set;
					TzCard(card,0);		
					TxF(card->m_name);TxF("#4\n");
					
					TxN();					
				}
			};
			iCount++;
		};
	};	
	TxOS("ScoreList");	
	TxFS("ScoreList");
};

float GetSkillsValue(unsigned uiSkill,float fFlurry)
{
	switch (uiSkill)
	{ 
	case Skill::summon:
	case Skill::flurry:
		return 0;
		break;	
	case Skill::protect:	
	case Skill::enrage:
	case Skill::berserk:
	case Skill::fortify:
	case Skill::legion:
	case Skill::drain:
	case Skill::hunt:
	case Skill::enfeeble:
	case Skill::mortar:
	case Skill::mimic:
	case Skill::mend:				
	case Skill::siege:
	case Skill::strike:	
	case Skill::weaken:
	case Skill::heal:	
	case Skill::rally:	
	case Skill::entrap:
	case Skill::rush:
	case Skill::rupture:	
	case Skill::leech:	
	case Skill::sunder:
		return 1*fFlurry;
		break;	
	case Skill::evade:		
	case Skill::payback:		
	case Skill::revenge:		
	case Skill::inhibit:			
		return 25;
		break;		
	case Skill::jam:
		return 120*fFlurry;
	case Skill::overload:			
	case Skill::evolve:
		return 120;
	default:
		return 1;
		break;
	};
	return 0;
};

float GetFlurryMod(Card* pkCard)
{
	for (auto& skill : pkCard->m_skills) 
	{ 
		if (skill.id == Skill::flurry)
		{
			float fFlurry = 1;
			if (skill.x > 1)
				fFlurry = skill.x;
			if (skill.c > 0)
				fFlurry = fFlurry / (float)skill.c;
			return 1+fFlurry;				
		};
	};		
	return 1;
};

unsigned HasSummon(Card* pkCard)
{
	for (auto& skill : pkCard->m_skills) 
	{ 
		if (skill.id == Skill::summon)
			return 1;
	};	
	for (auto& skill : pkCard->m_skills_on_play) 
	{ 
		if (skill.id == Skill::summon)
			return 2;
	};	
	for (auto& skill : pkCard->m_skills_on_attacked) 
	{ 
		if (skill.id == Skill::summon)
			return 3;
	};	
	for (auto& skill : pkCard->m_skills_on_death) 
	{ 
		if (skill.id == Skill::summon)
			return 4;
	};		
	return 0;
};


float fHandleSkillScore(Cards& all_cards, const Card* pkCard,const SkillSpec* pkSkill,unsigned uiType,float fFlurry,float fDelay)
{
	float fmod = 1.0f;
	if (pkSkill->x > 1.0f)
		fmod = pkSkill->x;
	if (pkSkill->all)
		fmod = fmod * 3.0f;
	else if (pkSkill->n != 0) fmod = fmod * pkSkill->n;
	if (pkSkill->y != allfactions)
		fmod = fmod * 0.5f;
	float fScore = GetSkillsValue(pkSkill->id, fFlurry) * fDelay * fmod;
	if (uiType == 1)
		fScore = fScore * 0.3f;
	if (uiType == 3)
		fScore = fScore * 0.2f;
	fScore = fScore * fFlurry * fDelay;		
	return fScore;
};

float TTCalcBaseScore(Cards& all_cards, Card* pkCard, unsigned uiInstance)
{
	if (pkCard == NULL)
		return 0;	
	float fScore = pkCard->m_attack + pkCard->m_health;
	if (pkCard->m_type == CardType::CardType::structure)
		fScore = pkCard->m_health*1.5f;
	if (uiInstance < 5)
	{
		cTCardStats* pkStats = g_DeckCore.AddCardToMap(pkCard);
		if (pkStats)
		{
			float fFlurry = GetFlurryMod(pkCard);	
			float fDelay = 0;
			switch (pkCard->m_delay)
			{
			case 0:
				fDelay = 1.1f;
				break;
			case 1:
				fDelay = 1.0f;
				break;
			case 2:
				fDelay = 0.9f;
				break;
			case 3:
				fDelay = 0.8f;
				break;
			case 4:
				fDelay = 0.7f;
				break;
			default:
				break;
			};
			//float fDelay = (10.0f - pkCard->m_delay) / 10.0f;			
			std::vector<SkillSpec>  lSkills;
			lSkills.clear();
			float fSkill = fScore*fFlurry*fDelay;
			for (auto& skill : pkCard->m_skills) { fSkill+=fHandleSkillScore(all_cards,pkCard,&skill,0,fFlurry,fDelay); };
			for (auto& skill : pkCard->m_skills_on_play) { fSkill+=fHandleSkillScore(all_cards,pkCard,&skill,1,fFlurry,fDelay); };
			for (auto& skill : pkCard->m_skills_on_attacked) { fSkill+=fHandleSkillScore(all_cards,pkCard,&skill,2,fFlurry,fDelay); };
			for (auto& skill : pkCard->m_skills_on_death) { fSkill+=fHandleSkillScore(all_cards,pkCard,&skill,3,fFlurry,fDelay); };						
			pkStats->m_fScore = fSkill;
			unsigned uiSummon = HasSummon(pkCard);			
			if (uiSummon != 0)
			{
				unsigned summon_card_id(pkCard->m_skill_value[Skill::summon]);
				if (summon_card_id)
				{
					Card* pkSummon = (Card*)g_pkAllCards->by_id(summon_card_id);
					if (pkSummon)
					{
						float fRet = TTCalcBaseScore(all_cards, pkSummon, uiInstance + 1);						
						if (uiSummon == 1)
							pkStats->m_fScore += fRet * fDelay * 0.9f;
						else if (uiSummon == 2)
							pkStats->m_fScore += fRet * 1.0f;
						else if (uiSummon == 4)
							pkStats->m_fScore = (fRet + pkStats->m_fScore) / 2.0f;						
					};
				};
			};			
			return pkStats->m_fScore;
		};
	};
	return 0;	
};
//std::cout << " \n";
	

void InitTimes()
{
	gtStartTime = boost::posix_time::microsec_clock::local_time();
	gUpdateTime = boost::posix_time::microsec_clock::local_time();	
};


Deck* tfind_deck(Decks& decks, const Cards& all_cards, std::string deck_name)
{
	if (deck_name == "")
		return NULL;
	Deck* deck = decks.find_deck_by_name(deck_name);
	if (deck != nullptr)
	{
		deck->resolve();
		return(deck);
	}
	decks.decks.emplace_back(Deck{all_cards});
	deck = &decks.decks.back();
	deck->set(deck_name);
	deck->resolve();
	return(deck);
}

Card* FReadCard(std::string sData, Cards& all_cards)
{
	trim(sData);	
	if (is_line_empty_or_commented(sData)) return NULL;
	unsigned card_id{ 0 };
	unsigned card_num{ 1 };
	char num_sign{ 0 };
	char mark{ 0 };
	parse_card_spec(all_cards, sData, card_id, card_num, num_sign, mark);
	Card * pkOut= (Card *)all_cards.by_id(card_id);
	if (pkOut)
	{		
		//if (pkOut->m_uiCopys < card_num)
			//pkOut->m_uiCopys = card_num;		
		cTCardStats* pkStats = g_DeckCore.AddCardToMap(pkOut);
		if (pkStats)
		{
			if (pkStats->m_uiCopys < card_num)
				pkStats->m_uiCopys= card_num;			
			return pkOut;
		}
	};
	return NULL;
};


std::string OffForts[6] = { "Lightning Cannon","Corrosive Spore", "Death Factory", "Inspiring Altar", "Darkspire", "Medical Center" };
std::string DefFort[5] = { "Tesla Coil", "Minefield", "Foreboding Archway", "Illuminary Blockade", "Forcefield" };

void TuReadCards()
{
	std::string filename = "data/ownedcards.txt";
	if (boost::filesystem::exists(filename))
	{
		g_DeckCore.m_pkCommander.clear();
		g_DeckCore.m_pkDominion.clear();
		g_DeckCore.m_pkCards.clear();	
		std::ifstream owned_file{ filename };
		if (!owned_file.good()) return;
		while (owned_file && !owned_file.eof())
		{
			std::string card_spec;
			getline(owned_file, card_spec);			
			std::stringstream  card_spec_stream(card_spec);
			std::string  word;
			card_spec_stream >> word;			
			Card* card = FReadCard(card_spec, *g_pkAllCards);
			if (card != NULL)
			{
				if (card->m_category == CardCategory::dominion_alpha)
					g_DeckCore.m_pkDominion.push_back(card);
				else
				{
					if ((gFactionLock == Faction::allfactions) || (card->m_faction == gFactionLock))
					{
						if (card->m_type == CardType::commander)
							g_DeckCore.m_pkCommander.push_back(card);
						else if (card->m_category == CardCategory::normal)
							g_DeckCore.m_pkCards.push_back(card);
					}
					else
					{
						if (gFactionLock == Faction::progenitor)
							if (card->m_type == CardType::commander)
								g_DeckCore.m_pkCommander.push_back(card);						
					};
				};
				
				
			};			
		};
		g_DeckCore.m_pkFortress[0].clear();	
		for (unsigned i = 0; i < 6; i++)
		{
			Card* card = FReadCard(OffForts[i], *g_pkAllCards);
			if (card != NULL)
			{
				g_DeckCore.m_pkFortress[0].push_back(card);
			};
		};
		g_DeckCore.m_pkFortress[1].clear();
		for (unsigned i = 0; i < 5; i++)
		{
			Card* card = FReadCard(DefFort[i], *g_pkAllCards);
			if (card != NULL)
			{
				g_DeckCore.m_pkFortress[1].push_back(card);
			};
		};
	};
};
