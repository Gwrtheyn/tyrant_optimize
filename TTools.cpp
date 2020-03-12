#include "titan.h"

#include "read.h"

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

std::deque<cTDeck>					g_Buffer;
boost::posix_time::ptime			gtStartTime;
boost::posix_time::ptime			gUpdateTime;

void InitTimes()
{
	gtStartTime = boost::posix_time::microsec_clock::local_time();
	gUpdateTime = boost::posix_time::microsec_clock::local_time();	
};

void OutputCard(Card* pkCard,bool bSHowCopys)
{
	if (pkCard == NULL)
		return;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	switch (pkCard->m_rarity)
	{
		case 3:
			SetConsoleTextAttribute(hConsole, DARKYELLOW);							
			std::cout << "[E]";
		break;
		case 4:
			SetConsoleTextAttribute(hConsole, MAGENTA);							
			std::cout << "[L]";
		break;
		case 5:
			SetConsoleTextAttribute(hConsole, CYAN);							
			std::cout << "[V]";
		break;
		case 6:
			SetConsoleTextAttribute(hConsole, BLUE);							
			std::cout << "[M]";
		break;	
		default:			
		break;
	};
	SetConsoleTextAttribute(hConsole, WHITE);
	switch (pkCard->m_faction)
	{
		case Faction::imperial:
			SetConsoleTextAttribute(hConsole, DARKBLUE);    
		break;
		case Faction::raider:
			SetConsoleTextAttribute(hConsole, DARKYELLOW);    
		break;
		case Faction::bloodthirsty:
			SetConsoleTextAttribute(hConsole, DARKRED);    
		break;
		case Faction::xeno:
			SetConsoleTextAttribute(hConsole, GRAY);    
		break;
		case Faction::righteous:
			SetConsoleTextAttribute(hConsole, BLUE);    
		break;
		case Faction::progenitor:
			SetConsoleTextAttribute(hConsole, CYAN);    
		break;
		default:			
		break;
	};	
	std::stringstream TempStr("");	
	int iSize = 22 - pkCard->m_name.size();	
	TempStr << pkCard->m_name;		
	std::cout << TempStr.str();	
	for (int i = 0; i < iSize; i++)
	{
		std::cout << " ";
	};
	SetConsoleTextAttribute(hConsole, WHITE);		
};

struct { bool operator()(cTDeck a, cTDeck b) const {return a.m_fWinPct > b.m_fWinPct;}} wsort;

void SimOutput(cTDeck* pkctDeck)
{			
	if (g_CalcMode) { return; };	
	g_Buffer.push_back(*pkctDeck);
	boost::posix_time::ptime CurTime = boost::posix_time::microsec_clock::local_time();
	boost::posix_time::time_duration msdiff = CurTime - gtStartTime;
	boost::posix_time::time_duration updatetime = CurTime - gUpdateTime;
	if ((updatetime.total_milliseconds() > 250) || (g_Buffer.size() > 100))
	{
		std::sort(g_Buffer.begin(), g_Buffer.end(), wsort);		
		cTDeck* rDC = &g_Buffer[0];		
		gUpdateTime = boost::posix_time::microsec_clock::local_time();
		float Diff = msdiff.total_milliseconds() / 1000.0;// / 60.0;
		float pctdone = ((float)g_Iter) / g_CalcIter;
		float invpctdone = 0;
		if (g_Iter > 0) { invpctdone = 1.0 / pctdone; };
		int esttime = (Diff * (invpctdone)-Diff);
		if (esttime < 0) { esttime = 0; };
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		std::string sExtra0 = "";
		if (pctdone < 0.1) sExtra0 = "0";
		SetConsoleTextAttribute(hConsole, GREEN);
		std::cout << std::fixed << std::setprecision(2) << sExtra0 << (pctdone * 100) << "% ";
		SetConsoleTextAttribute(hConsole, YELLOW);
		if (esttime > 120) std::cout << (esttime / (float)60) << " Min ";
		else std::cout << esttime << " Sec ";
		SetConsoleTextAttribute(hConsole, RED);
		sExtra0 = "";
		if (rDC->m_fWinPct < 10.0)
			sExtra0 = "0";
		std::cout << sExtra0 << rDC->m_fWinPct << "% ";
		SetConsoleTextAttribute(hConsole, WHITE);
		OutputCard(rDC->m_pkCom, false); std::cout << ",";
		OutputCard(rDC->m_pkDom, false);
		unsigned uiSize = rDC->GetCardsSize();
		for (unsigned i = 0; i < uiSize; i++)
		{
			std::cout << ",";
			OutputCard(rDC->m_pkCards[i], false);
		};
		SetConsoleTextAttribute(hConsole, YELLOW);						
		std::cout << "\n";
		g_Buffer.clear();
	};
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

void TuBenchmarkCardsOut(std::string pkzFileName,unsigned uiMode)
{
	std::deque<cTCardStats*>			pkstats;
	pkstats.clear();
	for (auto it = g_DeckCore.m_BenchmarkStack.begin(); it != g_DeckCore.m_BenchmarkStack.end(); ++it)
	{
		cTCardStats* pkStatsBase = &g_DeckCore.m_BenchmarkBase[it->first];
		if (pkStatsBase)
		{
			unsigned uiRoundsBase = pkStatsBase->m_uiWins + pkStatsBase->m_uiDraws + pkStatsBase->m_uiLosses;
			if (uiRoundsBase == 0)uiRoundsBase = 1;
			float BaseWinPct = (pkStatsBase->m_uiWins / (float)uiRoundsBase) * 100;

			unsigned uiRoundsStack = it->second.m_uiWins + it->second.m_uiDraws + it->second.m_uiLosses;
			if (uiRoundsStack == 0)uiRoundsStack = 1;
			float StackWinPct = (it->second.m_uiWins / (float)uiRoundsStack) * 100;

			it->second.m_fWinPct = StackWinPct - BaseWinPct;
			pkstats.push_back(&it->second);
		};
	};

	std::cout << "\n";
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, GREEN);
	struct { bool operator()(cTCardStats* a, cTCardStats* b) const { return a->m_fWinPct > b->m_fWinPct; } } bScoresort;
	std::sort(pkstats.begin(), pkstats.end(), bScoresort);	
	std::stringstream filestr;
	filestr << "data/" << pkzFileName;
	std::ofstream myfile(filestr.str());
	if (myfile.is_open())
	{
		for (unsigned j = 0; j < pkstats.size(); j++)
		{
			if (uiMode == 0)
			{
				if (j == 0)
				{
					std::cout << "S-Class: \n";
					myfile << "S-Class: \n";
				}
				else
					if (j == 10)
					{
						std::cout << "A-Class: \n";
						myfile << "A-Class: \n";
					}
					else
						if (j == 30)
						{
							std::cout << "B-Class: \n";
							myfile << "B-Class: \n";
						}
						else
							if (j == 60)
							{
								std::cout << "C-Class: \n";
								myfile << "C-Class: \n";
							}
							else
								if (j == 100)
								{
									std::cout << "D-Class: \n";
									myfile << "D-Class: \n";
								};
			}else
			if (uiMode == 2)
			{
				if (j == 0)
				{
					std::cout << "S-Class: \n";
					myfile << "S-Class: \n";
				}
				else
					if (j == 4)
					{
						std::cout << "A-Class: \n";
						myfile << "A-Class: \n";
					}
					else
						if (j == 10)
						{
							std::cout << "B-Class: \n";
							myfile << "B-Class: \n";
						}
						else
							if (j == 20)
							{
								std::cout << "C-Class: \n";
								myfile << "C-Class: \n";
							}
							else
								if (j == 30)
								{
									std::cout << "D-Class: \n";
									myfile << "D-Class: \n";
								};
			};
			if (uiMode == 3)
			{
				float fBM = (pkstats[j]->m_fWinPct);
				SetConsoleTextAttribute(hConsole, CYAN);
				std::cout << "    -" << pkstats[j]->m_pkCard->m_name << "," << pkstats[j]->m_pkCard->m_name;
				myfile << "    -" << pkstats[j]->m_pkCard->m_name << "," << pkstats[j]->m_pkCard->m_name;
				SetConsoleTextAttribute(hConsole, GREEN);
				std::cout << std::fixed << std::setprecision(2) << " [" << fBM << "%]\n";
				myfile << std::fixed << std::setprecision(2) << " [" << fBM << "%]\n";
			}else
			if (j < 150)
			{
				float fBM = 10000 + (pkstats[j]->m_fWinPct * 100.0f);
				SetConsoleTextAttribute(hConsole, CYAN);
				std::cout << "    -" << pkstats[j]->m_pkCard->m_name;
				myfile << "    -" << pkstats[j]->m_pkCard->m_name;
				SetConsoleTextAttribute(hConsole, GREEN);
				std::cout << std::fixed << std::setprecision(0) << "(" << fBM << ")\n";
				myfile << std::fixed << std::setprecision(0) << "(" << fBM << ")\n";
			};
		};
		myfile.close();
	};
	SetConsoleTextAttribute(hConsole, WHITE);
	std::cout << "\n";
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
				if (card->m_type == CardType::commander)
					g_DeckCore.m_pkCommander.push_back(card);
				else if (card->m_category == CardCategory::dominion_alpha)
					g_DeckCore.m_pkDominion.push_back(card);
				else if (card->m_category == CardCategory::normal)
					g_DeckCore.m_pkCards.push_back(card);
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

void OutPutDeck(Deck* pkDeck)
{	
	OutputCard((Card*)pkDeck->commander, false); std::cout << ",";
	OutputCard((Card*)pkDeck->alpha_dominion, false);
	unsigned uiSize = pkDeck->cards.size();
	for (unsigned i = 0; i < uiSize; i++)
	{
		std::cout << ",";
		OutputCard((Card*)pkDeck->cards[i], false);
	};	
	std::cout << "\n";
};

void cTDeckCore::OutPutDeck(cTDeck& TempDeck)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, GREEN);
	std::string sExtra0 = "";
	if (TempDeck.m_fWinPct < 10.0)
		sExtra0 = "0";
	std::cout << std::fixed << std::setprecision(2) << sExtra0 << TempDeck.m_fWinPct << "% ";
	SetConsoleTextAttribute(hConsole, WHITE);
	OutputCard(TempDeck.m_pkCom, false); std::cout << ", ";
	OutputCard(TempDeck.m_pkDom, false);
	unsigned uiSize = TempDeck.GetCardsSize();
	for (unsigned i = 0; i < uiSize; i++)
	{
		std::cout << ", ";
		OutputCard(TempDeck.m_pkCards[i], false);
	};
	std::cout << "\n";
};


void cTDeckCore::OutPutDeckList()
{
	if (g_CalcMode) { return; };	
	std::cout << "\n";
	cTDeck TempDeck;	

	unsigned uiPhaseSize = m_Phase[m_PageB].size();	
	for (unsigned uiBase = 0; uiBase < uiPhaseSize; uiBase++)
	{		
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, WHITE);
		std::cout << (uiBase+1) << " ";
		TempDeck = m_Phase[m_PageB][uiBase];
		OutPutDeck(TempDeck);		
	};
};


void OutputCardFile(std::ofstream &pkFile,Card* pkCard,bool bSHowCopys)
{
	if (pkCard == NULL)
		return;		
	switch (pkCard->m_rarity)
	{
		case 3:					
			pkFile << "<td style=\"color:rgb(255,255,0);\">";						
			pkFile << "[E]";			
			pkFile << "</td>\n";
		break;
		case 4:			
			pkFile << "<td style=\"color:rgb(255,0,255);\">";						
			pkFile << "[L]";			
			pkFile << "</td>\n";
		break;
		case 5:			
			pkFile << "<td style=\"color:rgb(0,255,255);\">";						
			pkFile << "[V]";			
			pkFile << "</td>\n";
		break;
		case 6:
			pkFile << "<td style=\"color:rgb(0,0,255);\">";			
			pkFile << "[M]";	
			pkFile << "</td>\n";
		break;	
			default:			
		break;
	};
	
	switch (pkCard->m_faction)
	{
		case Faction::imperial:			
			pkFile << "<td style=\"color:rgb(0,0,128);\">";			
		break;
		case Faction::raider:			
			pkFile << "<td style=\"color:rgb(128,128,0);\">";			
		break;
		case Faction::bloodthirsty:			
			pkFile << "<td style=\"color:rgb(128,0,0);\">";			
		break;
		case Faction::xeno:			
			pkFile << "<td style=\"color:rgb(128,128,128);\">";			
		break;
		case Faction::righteous:			
			pkFile << "<td style=\"color:rgb(0,0,255);\">";			
		break;
		case Faction::progenitor:			
			pkFile << "<td style=\"color:rgb(0,255,255);\">";	
		break;
			default:			
		break;
	};	
	std::stringstream TempStr("");	
	int iSize = 22 - pkCard->m_name.size();	
	TempStr << pkCard->m_name;		
	for (int i = 0; i < iSize; i++)
	{
		TempStr << " ";
	};	
	pkFile << TempStr.str();
	pkFile << "</td>\n";	
};

void HtmlOutDeck(std::ofstream &pkFile,cTDeck& deck)
{
	pkFile << "<tr style=\"color:white;font-size:60%;font-family:consolas; \">\n";	
	std::string sExtra0 = "";
	if (deck.m_fWinPct < 10.0)
		sExtra0 = "0";						
	pkFile << "<td style=\"color:rgb(0,128,0);\">";
	pkFile << std::fixed << std::setprecision(2) << sExtra0 << deck.m_fWinPct << "% </td>";
	OutputCardFile(pkFile,deck.m_pkCom, false);
	OutputCardFile(pkFile,deck.m_pkDom, false);
	unsigned uiSize = deck.GetCardsSize();
	for (unsigned i = 0; i < uiSize; i++)
	{
	//	std::cout << ",";
		OutputCardFile(pkFile,deck.m_pkCards[i], false);
	};			
	pkFile << "</tr>\n";	
};

void cTDeckCore::MakeHtml(Cards& all_cards)
{
	std::stringstream filestr;
	filestr << "data/" << "HtmlResults" << ".html";
	std::ofstream myfile(filestr.str());
	
	if (myfile.is_open())
	{
		//myfile << g_OutStr.str();
		myfile << "<!DOCTYPE html>\n";
		myfile << "<html>\n";
		myfile << "<body style=\"background-color:rgb(0,0,0); \">\n";
		myfile << "<pre>";
		myfile << "<table>\n";
			myfile << "<td style=\"color:rgb(255,255,255);\">";			
			myfile << "TUTitan_Off";	
			myfile << "</td>\n";
		myfile << "<table>\n";
		myfile << "<table>\n";
		
		for (unsigned uiBase = 0; uiBase < g_DeckCore.m_SimData[0].size(); uiBase++)
		{									
			cTDeck TempDeck = g_DeckCore.m_SimData[0][uiBase];			
			HtmlOutDeck(myfile,TempDeck);
		};
		myfile << "</table>\n";		
		myfile << "<table>\n";
			myfile << "<td style=\"color:rgb(255,255,255);\">";			
			myfile << "TUTitan_Def";	
			myfile << "</td>\n";
			myfile << "<table>\n";
		myfile << "<table>\n";
		myfile << "<tr style=\"color:white;font-size:60%;font-family:consolas; \">\n";					
		myfile << "</tr>\n";						
		for (unsigned uiBase = 0; uiBase < g_DeckCore.m_SimData[1].size(); uiBase++)
		{									
			cTDeck TempDeck = g_DeckCore.m_SimData[1][uiBase];			
			HtmlOutDeck(myfile,TempDeck);
		};
		myfile << "</table>\n";

		std::deque<cTCardStats*>			pkSCards;	
		pkSCards.clear();	
		for (Card* card : all_cards.all_cards)
		{
			cTCardStats* pkStats = &g_DeckCore.m_pkClusterMap[card];
			if (pkStats)
			{				
				unsigned uiRounds = pkStats->m_uiWins + pkStats->m_uiDraws + pkStats->m_uiLosses;
				if (uiRounds > 0)
				{
					pkStats->m_fWinPct = (pkStats->m_uiWins / (float)uiRounds) * 100.001f;
					pkSCards.push_back(pkStats);
				};
			};
		};
		
		struct { bool operator()(cTCardStats* a, cTCardStats* b) const { return a->m_fWinPct > b->m_fWinPct; } } CardDataSort;
		std::sort(pkSCards.begin(), pkSCards.end(), CardDataSort);		
		unsigned uiCount = 0;
		for (cTCardStats* nCard : pkSCards)
		{					
			uiCount++;
			myfile << "<table>\n";
			myfile << "<td style=\"color:rgb(0,255,0);\">";									
			myfile << std::fixed << std::setprecision(2) << uiCount << " " << nCard->m_fWinPct << "% \n";
			OutputCardFile(myfile,(Card*)nCard->m_pkCard,false);
//			myfile << "</td>\n";
			myfile << "</table>\n";
		};		
		for (unsigned uiBase = 0; uiBase < g_DeckCore.m_SimData[0].size(); uiBase++)
		{
			cTDeck TempDeck = g_DeckCore.m_SimData[0][uiBase];						
			myfile << "<table>\n";
			myfile << "<td style=\"color:rgb(255,255,255);\">";		
			myfile << "TUTitan_Off";									
			if (uiBase < 10) { myfile << "0"; };
			if (uiBase < 100) { myfile << "0"; };
			myfile << uiBase << ": ";
			//myfile << std::fixed << std::setprecision(2)<< TempDeck.m_fWinPct << "% ";			
			myfile << TempDeck.GetDeckString() << "\n";						
			myfile << "</table>\n";
		}
		myfile << "<table>\n";
		myfile << "<td style=\"color:rgb(255,255,255);\">";																
		myfile << "</table>\n";
		for (unsigned uiBase = 0; uiBase < g_DeckCore.m_SimData[1].size(); uiBase++)
		{
			cTDeck TempDeck = g_DeckCore.m_SimData[1][uiBase];						
			myfile << "<table>\n";
			myfile << "<td style=\"color:rgb(255,255,255);\">";		
			myfile << "TUTitan_Def";							
			if (uiBase < 10) { myfile << "0"; };
			if (uiBase < 100) { myfile << "0"; };
			myfile << uiBase << ": ";
			//myfile << std::fixed << std::setprecision(2)<< TempDeck.m_fWinPct << "% ";			
			myfile << TempDeck.GetDeckString() << "\n";						
			myfile << "</table>\n";
		}
		myfile << "</pre>";
		myfile << "</body>\n";
		myfile << "</html>\n";

		myfile.close();
	}
	
	
};

float GetSkillsValue(unsigned uiSkill, float fFlurry);


float dcDelayScale(Card* pkCard)
{
	switch (pkCard->m_delay)
	{
	case 0: return 1.0f; break;
	case 1: return 0.9f; break;
	case 2: return 0.8f; break;
	case 3: return 0.7f; break;
	case 4: return 0.6f; break;
	};
	return 0;
};

float CalcBaseScore(Cards& all_cards,Card* pkCard, unsigned uiInstance)
{
	float fScore = pkCard->m_attack + pkCard->m_health;
	if ((fScore > 50.0f)&&(uiInstance<5))
	{
		cTCardStats* pkStats = g_DeckCore.AddCardToMap(pkCard);
		if (pkStats)
		{
			std::vector<SkillSpec>  lSkills;
			lSkills.clear();
			for (auto& skill : pkCard->m_skills) { lSkills.push_back(skill); };
			for (auto& skill : pkCard->m_skills_on_play) { lSkills.push_back(skill); };
			for (auto& skill : pkCard->m_skills_on_attacked) { lSkills.push_back(skill); };
			for (auto& skill : pkCard->m_skills_on_death) { lSkills.push_back(skill); };
			float fFlurry = 1;
			for (unsigned i = 0; i < lSkills.size(); i++)
			{
				SkillSpec& pkSkill = lSkills[i];
				if (pkSkill.id == Skill::flurry)
				{
					fFlurry = pkSkill.x;
					if (pkSkill.x == 0)
						fFlurry = 1.0f;
					fFlurry += 1.0f;
					fFlurry = fFlurry / (float)pkSkill.c;
				};
			};
			for (unsigned i = 0; i < lSkills.size(); i++)
			{
				SkillSpec& pkSkill = lSkills[i];
				unsigned uiAllMod = 1.0f;
				if (pkSkill.all)
					uiAllMod = 3.0f;
				fScore += GetSkillsValue(pkSkill.id, fFlurry) * uiAllMod;
			};
			unsigned summon_card_id(pkCard->m_skill_value[Skill::summon]);
			if (summon_card_id)
			{
				Card* pkSummon = (Card*)all_cards.by_id(summon_card_id);
				if (pkSummon)
				{
					fScore += CalcBaseScore(all_cards, pkSummon, uiInstance + 1);
				};
			};
			pkStats->m_uiInstance = uiInstance;
			pkStats->m_fScore = fScore* dcDelayScale(pkCard);
		};
	};
	return fScore;
};
	
void UpdateScore(Cards& all_cards)
{
	for (Card* card : all_cards.all_cards)
	{
		if ((card->m_fusion_level == 2) && (card->m_level == 6) && (card == card->m_top_level_card))
		{
			CalcBaseScore(all_cards,card,1);
		};
	};
	std::deque<cTCardStats*>			pkstats;
	pkstats.clear();
	for (auto it = g_DeckCore.m_pkClusterMap.begin(); it != g_DeckCore.m_pkClusterMap.end(); ++it)
	{
		pkstats.push_back(&it->second);
	};
	std::cout << "\n";
	struct { bool operator()(cTCardStats* a, cTCardStats* b) const {return a->m_fScore > b->m_fScore;}} bScoresort;
    std::sort(pkstats.begin(),pkstats.end(),bScoresort);	
/*	unsigned iCount = 1;
	for (unsigned j = 1; j < pkstats.size(); j++)
	{
		if (pkstats[j]->m_uiInstance < 2)
		{
			if (iCount < 200)
			{
				std::cout << iCount << " " << pkstats[j]->m_pkCard->m_name << " " << pkstats[j]->m_fScore << "\n";
			};
			iCount++;
		};
	};
	std::cout << "\n";	*/
	std::stringstream filestr;
	filestr << "data/" << "Top250CardList" << ".txt";
	std::ofstream myfile(filestr.str());
	if (myfile.is_open())
	{		
		unsigned iCount = 1;
		for (unsigned j = 1; j < pkstats.size(); j++)
		{
			if (pkstats[j]->m_uiInstance < 2)
			{
				if (iCount < 250)
				{
					myfile << pkstats[j]->m_pkCard->m_name << "#4\n";
					std::cout << pkstats[j]->m_pkCard->m_name << "#4\n";
					//std::cout << iCount << " " << pkstats[j]->m_pkCard->m_name << " " << pkstats[j]->m_fScore << "\n";
				};
				iCount++;
			};
		};
		//myfile << g_OutStr.str();
		myfile.close();
	}
};



float GetSkillsValue(unsigned uiSkill,float fFlurry)
{
	switch (uiSkill)
	{
	case Skill::protect:
	case Skill::sunder:
	case Skill::enrage:
	case Skill::berserk:
	case Skill::fortify:
	case Skill::legion:
	case Skill::drain:
	case Skill::hunt:
		return 2*fFlurry;
		break;		
	case Skill::enfeeble:
	case Skill::mortar:
	case Skill::siege:
	case Skill::strike:	
	case Skill::weaken:
	case Skill::heal:	
	case Skill::rally:	
	case Skill::entrap:
	case Skill::rush:
	case Skill::rupture:	
	case Skill::leech:
	case Skill::mimic:
		return 1*fFlurry;
		break;
	case Skill::overload:		
	case Skill::evade:		
	case Skill::payback:		
	case Skill::revenge:		
	case Skill::inhibit:
		return 10;
		break;	
	default:
		return 1;
		break;
	};
	return 0;
};
