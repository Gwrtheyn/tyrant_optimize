#include "TOutPut.h"
#include "titan.h"

#include "read.h"
#include "sim.h"

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
#include "TTools.h"


std::stringstream					g_StrBuffer("");	
std::stringstream					g_StrBufferHtml("");	
std::stringstream					g_StrBufferWiki("");	
unsigned							g_ConsoleSize=0;
const std::string faction_names_short[Faction::num_factions] = { "", "I.", "D.", "B.", "X.", "T.", "P." };

void TxC(unsigned uiColor)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);	
	switch (uiColor)
	{				
		case BLACK: SetConsoleTextAttribute(hConsole, BLACK); g_StrBufferHtml << "<d style=\"color:rgb(12,12,12)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(12,12,12)\">"; break;
		case DARKBLUE: SetConsoleTextAttribute(hConsole, DARKBLUE); g_StrBufferHtml << "<d style=\"color:rgb(0,55,218)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(0,55,218)\">"; break;
		case DARKGREEN: SetConsoleTextAttribute(hConsole, DARKGREEN); g_StrBufferHtml << "<d style=\"color:rgb(19,161,14)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(19,161,14)\">"; break;
		case DARKCYAN: SetConsoleTextAttribute(hConsole, DARKCYAN); g_StrBufferHtml << "<d style=\"color:rgb(58,150,221)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(58,150,221)\">"; break;
		case DARKRED: SetConsoleTextAttribute(hConsole, DARKRED); g_StrBufferHtml << "<d style=\"color:rgb(197,15,31)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(197,15,31)\">"; break;
		case DARKMAGENTA: SetConsoleTextAttribute(hConsole, DARKMAGENTA); g_StrBufferHtml << "<d style=\"color:rgb(136,23,152)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(136,23,152)\">"; break;
		case DARKYELLOW: SetConsoleTextAttribute(hConsole, DARKYELLOW); g_StrBufferHtml << "<d style=\"color:rgb(193,156,0)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(193,156,0)\">"; break;
		case DARKGRAY: SetConsoleTextAttribute(hConsole, DARKGRAY);g_StrBufferHtml << "<d style=\"color:rgb(204,204,204)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(204,204,204)\">"; break;
		case GRAY: SetConsoleTextAttribute(hConsole, GRAY); g_StrBufferHtml << "<d style=\"color:rgb(118,118,118)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(118,118,118)\">"; break;
		case BLUE: SetConsoleTextAttribute(hConsole, BLUE); g_StrBufferHtml << "<d style=\"color:rgb(59,120,255)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(59,120,255)\">";   break;
		case GREEN: SetConsoleTextAttribute(hConsole, GREEN); g_StrBufferHtml << "<d style=\"color:rgb(22,198,12)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(22,198,12)\">"; break;
		case CYAN: SetConsoleTextAttribute(hConsole, CYAN); g_StrBufferHtml << "<d style=\"color:rgb(97,214,214)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(97,214,214)\">";  break;
		case RED: SetConsoleTextAttribute(hConsole, RED);	g_StrBufferHtml << "<d style=\"color:rgb(231,72,86)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(231,72,86)\">";  break;
		case MAGENTA: SetConsoleTextAttribute(hConsole, MAGENTA); g_StrBufferHtml << "<d style=\"color:rgb(180,0,158)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(180,0,158)\">";  break;
		case YELLOW: SetConsoleTextAttribute(hConsole, YELLOW); g_StrBufferHtml << "<d style=\"color:rgb(249,241,156)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(249,241,156)\">";  break;
		case WHITE: SetConsoleTextAttribute(hConsole, WHITE); g_StrBufferHtml << "<d style=\"color:rgb(242,242,242)\">\n"; 
			g_StrBufferWiki << "<span style=\"color:rgb(242,242,242)\">";  break;
		default: break;
	};	
};

std::string TTUIntToStr(unsigned uiNumber)
{
	std::stringstream ss;
	ss << uiNumber;
	return ss.str();	
};

std::string TTFloatToStr(float fNumber)
{
	std::stringstream ss;
	unsigned uNum = fNumber * 100;	
	ss << (float)uNum / 100.0f;
	return ss.str();	
};

std::string Txi(unsigned uiNumber)
{
	std::stringstream ss;
	ss << uiNumber;
	return ss.str();	
};
std::string Txf(float fNumber)
{
	std::stringstream ss;
	unsigned uNum = fNumber * 100;	
	ss << (float)uNum / 100.0f;
	return ss.str();
};

void TxI()
{
	std::cout << std::fixed << std::setprecision(2); g_ConsoleSize=0;
	TxC(WHITE);
	g_StrBuffer.str("");
	g_StrBufferHtml.str("");
	g_StrBufferWiki.str("");
	g_StrBufferHtml << "<!DOCTYPE html><html><body><font face = \"Consolas\" size = \"1\"><body style=\"background-color:rgb(0,0,0) \"><table>\n";		
	g_StrBufferWiki << "<font face=\"Consolas\" size=\"1\">\n\n";
};

void TxOS(std::string pkzFileName)
{		
	boost::filesystem::create_directory(boost::filesystem::path("Results"));		
	g_StrBufferHtml << "\n</p>\n";
	g_StrBufferHtml << "</body></html>\n";
	std::stringstream filestr; filestr << "Results/" << pkzFileName << ".html"; std::ofstream myfile(filestr.str());
	if (myfile.is_open()) { myfile << g_StrBufferHtml.str();myfile.close(); }; g_ConsoleSize=0;	
	TxC(WHITE);	
	g_StrBufferHtml.str("");

	std::stringstream wikifilestr; wikifilestr << "Results/" << pkzFileName << ".wiki"; std::ofstream wikimyfile(wikifilestr.str());
	if (wikimyfile.is_open()) { wikimyfile << g_StrBufferWiki.str();wikimyfile.close(); }; g_ConsoleSize=0;	
	TxC(WHITE);	
	g_StrBufferWiki.str("");
};
void TxFS(std::string pkzFileName)
{		
	boost::filesystem::create_directory(boost::filesystem::path("Results"));			
	std::stringstream filestr; filestr << "Results/" << pkzFileName << ".txt"; std::ofstream myfile(filestr.str());
	if (myfile.is_open()) { myfile << g_StrBuffer.str();myfile.close(); }; g_ConsoleSize=0;	
	TxC(WHITE);
	g_StrBuffer.str("");	
};
void TxO(unsigned uiColor, std::string pkzString)
{
	if (uiColor > 0)
		TxC(uiColor);
	g_ConsoleSize += pkzString.size();
	g_StrBufferHtml << pkzString;
	g_StrBufferWiki << pkzString << "</span> ";
	std::cout << pkzString;	
};

void TxF(std::string pkzString)
{	
	g_StrBuffer << pkzString;
};

void TxN()
{
	g_StrBufferHtml << "\n</p>\n";
	g_StrBufferWiki << "\n\n";
	std::cout << "\n";
	g_ConsoleSize = 0;
};



void TxTab(unsigned uiSize,std::string pkzFill)
{	
	if (pkzFill != "")
	{
		for (unsigned i = g_ConsoleSize; i < uiSize; i++)
		{

			std::cout << pkzFill;
			g_StrBufferHtml << pkzFill;
			g_StrBufferWiki << pkzFill;
			g_ConsoleSize++;
		};
	}else
	{
		for (unsigned i = g_ConsoleSize; i < uiSize; i++)
		{
			std::cout << " ";			
			g_StrBufferHtml << "&nbsp;";
			g_StrBufferWiki << "&nbsp;";
			g_ConsoleSize++;
		};	
	};
};

void TzRare(const Card* pkCard)
{
	if (pkCard == NULL) return;
	if ((pkCard->m_set!=2000)&&(pkCard->m_set!=9500))
		TxO(DARKRED,"{F2P}");	
	switch (pkCard->m_rarity)
	{
		case 3: TxO(DARKYELLOW,"[E]"); break;
		case 4: TxO(MAGENTA,"[L]"); break;
		case 5: TxO(CYAN,"[V]"); break;
		case 6: TxO(BLUE,"[M]"); break;
		default: break;
	};	
};

void TzFName(const Card* pkCard)
{
	if (pkCard == NULL) return;
	switch (pkCard->m_faction)
	{
		case Faction::imperial:		TxO(DARKBLUE,"I.");		TxO(DARKBLUE,pkCard->m_name); break;
		case Faction::raider:		TxO(DARKYELLOW,"D.");	TxO(DARKYELLOW,pkCard->m_name); break;
		case Faction::bloodthirsty: TxO(DARKRED,"B.");		TxO(DARKRED,pkCard->m_name);break;
		case Faction::xeno:			TxO(GRAY,"X.");			TxO(GRAY,pkCard->m_name); break;
		case Faction::righteous:	TxO(BLUE,"T.");			TxO(BLUE,pkCard->m_name); break;
		case Faction::progenitor:	TxO(CYAN,"P.");			TxO(CYAN,pkCard->m_name); break;
		default: break;
	};	
};
void TzStats(const Card* pkCard)
{
	TxTab(38,"");
	if (pkCard == NULL) return;
	TxO(RED," A:");		TxO(RED,Txi(pkCard->m_attack));
	TxO(GREEN," H:");	TxO(GREEN,Txi(pkCard->m_health));
	TxO(WHITE," D:");	TxO(WHITE,Txi(pkCard->m_delay));
};

unsigned					g_SkillNum = 0;

void TzSkill(const Card* pkCard,const SkillSpec* pkSkill,unsigned uiType,std::deque<Card*>	&lSummonHelper)
{
	if (pkSkill == NULL)
		return;	
	TxTab(55+(g_SkillNum * 20),"");
	g_SkillNum++;	
	TxO(YELLOW," (");
	switch (uiType)
	{	
		case 1: TxO(BLUE,"P_"); break;
		case 2: TxO(MAGENTA,"A_"); break;
		case 3: TxO(RED,"D_"); break;
		default: break;
	};
	if (pkSkill->id == Skill::summon)
	{
		unsigned summon_card_id(pkCard->m_skill_value[Skill::summon]);
		if (summon_card_id)
		{
			TxO(MAGENTA, "[S]");
			Card* pkSummon = (Card*)g_pkAllCards->by_id(summon_card_id);
			if (pkSummon)
			{
				TzFName(pkSummon);
				lSummonHelper.push_back(pkSummon);//TzCard(pkSummon, 1);
			};
		};
	}else
	{
		if ((pkSkill->id >= Skill::enfeeble) && (pkSkill->id <= Skill::weaken))// Activation (harmful):
			TxO(DARKRED, skill_names[pkSkill->id]);
		else if ((pkSkill->id >= Skill::evolve) && (pkSkill->id <= Skill::rush))// Activation (helpful):
			TxO(GREEN, skill_names[pkSkill->id]);
		else if ((pkSkill->id == Skill::mimic))// Activation (unclassified/polymorphic):
			TxO(DARKCYAN, skill_names[pkSkill->id]);
		else if ((pkSkill->id >= Skill::armor) && (pkSkill->id <= Skill::barrier))// Defensive:
			TxO(GRAY, skill_names[pkSkill->id]);
		else if ((pkSkill->id >= Skill::coalition) && (pkSkill->id <= Skill::mark))// Combat-Modifier:
			TxO(CYAN, skill_names[pkSkill->id]);
		else if ((pkSkill->id >= Skill::berserk) && (pkSkill->id <= Skill::poison)) // Damage-Dependent:
			TxO(DARKGREEN, skill_names[pkSkill->id]);
		else if ((pkSkill->id >= Skill::inhibit) && (pkSkill->id <= Skill::disease))// Instant-Debuff:
			TxO(YELLOW, skill_names[pkSkill->id]);
		else if ((pkSkill->id >= Skill::allegiance) && (pkSkill->id <= Skill::enhance)) // Triggered:
			TxO(BLUE, skill_names[pkSkill->id]);		
		TxO(MAGENTA, (pkSkill->s == Skill::no_skill ? "" : std::string(":") + skill_names[pkSkill->s]));
		TxO(MAGENTA, (pkSkill->s2 == Skill::no_skill ? "" : std::string(":") + skill_names[pkSkill->s2]));
		TxO(RED, " ");
		TxO(WHITE, (pkSkill->all ? "All " : pkSkill->n == 0 ? "" : std::string(" ") + to_string(pkSkill->n)));
		TxO(WHITE, (pkSkill->y == allfactions ? "" : std::string("") + faction_names_short[pkSkill->y]));
		if (pkSkill->id == Skill::jam)
			TxO(WHITE, (pkSkill->x == 0 ? "" : std::string("") + to_string(pkSkill->x)));
		else
			TxO(WHITE, (pkSkill->x == 0 ? "1" : std::string("") + to_string(pkSkill->x)));
		TxO(WHITE, (pkSkill->c == 0 ? "" : std::string(" every ") + to_string(pkSkill->c)));
	};
	TxO(YELLOW,")");	
};

void TzCard(const Card* pkCard,unsigned uiInstance)
{
	if (pkCard == NULL)
		return;		
	TzRare(pkCard);
	TzFName(pkCard);
	if (uiInstance < 5)
	{
		TzStats(pkCard);
		g_SkillNum = 0;		
		std::deque<Card*> SummonHelper;
		SummonHelper.clear();
		for (auto& skill : pkCard->m_skills) { TzSkill(pkCard, &skill, 0, SummonHelper); };
		for (auto& skill : pkCard->m_skills_on_play) { TzSkill(pkCard, &skill, 1, SummonHelper); };
		for (auto& skill : pkCard->m_skills_on_attacked) { TzSkill(pkCard, &skill, 2, SummonHelper); };
		for (auto& skill : pkCard->m_skills_on_death) { TzSkill(pkCard, &skill, 3, SummonHelper); };

		for (unsigned i = 0; i < SummonHelper.size(); i++)
		{
			TxN();
			TxTab(16+(uiInstance*20),"");
			TzCard(SummonHelper[i],uiInstance+1);
		};		
	};
};

struct { bool operator()(cTDeck a, cTDeck b) const {return a.m_fWinPct > b.m_fWinPct;}} SimOutputSort;
void SimOutput(cTDeck* pkctDeck)
{			
	if (g_CalcMode) { return; };	
	g_Buffer.push_back(*pkctDeck);
	boost::posix_time::ptime CurTime = boost::posix_time::microsec_clock::local_time();
	boost::posix_time::time_duration msdiff = CurTime - gtStartTime;
	boost::posix_time::time_duration updatetime = CurTime - gUpdateTime;
	if ((updatetime.total_milliseconds() > 250) || (g_Buffer.size() > 20))
	{
		std::sort(g_Buffer.begin(), g_Buffer.end(), SimOutputSort);		
		cTDeck* rDC = &g_Buffer[0];		
		gUpdateTime = boost::posix_time::microsec_clock::local_time();
		float Diff = msdiff.total_milliseconds() / 1000.0;// / 60.0;
		float pctdone = ((float)g_Iter) / g_CalcIter;
		float invpctdone = 0;
		if (g_Iter > 0) { invpctdone = 1.0 / pctdone; };
		int esttime = (Diff * (invpctdone)-Diff);
		if (esttime < 0) { esttime = 0; };		
		std::string sExtra0 = "";
		if (pctdone < 0.1) sExtra0 = "0";	
		sExtra0 = "";
		if (rDC->m_fWinPct < 10.0)
			sExtra0 = "0";				
		TxO(GREEN, sExtra0); TxO(GREEN, TTFloatToStr(pctdone * 100)); TxO(GREEN, "% ");		
		if (esttime > 120) { TxO(YELLOW,TTFloatToStr((esttime / (float)60)) ); TxO(YELLOW, " Min "); }
		else { TxO(YELLOW,TTFloatToStr(esttime )); TxO(YELLOW, " Sec "); };				
		sExtra0 = "";
		if (rDC->m_fWinPct < 10.0)
			sExtra0 = "0";		
		TxO(RED, sExtra0); TxO(RED, TTFloatToStr(rDC->m_fWinPct)); TxO(RED, "%  ");
		TxTab(25,"");		
		TzFName(rDC->m_pkCom);TxO(WHITE, ", ");
		TxTab(55,"");
		TzFName(rDC->m_pkDom);
		for (unsigned i = 0; i < rDC->GetCardsSize(); i++)
		{
			TxTab(80+(i*30),"");
			TxO(WHITE, ", ");
			TzFName(rDC->m_pkCards[i]);			
		};
		TxN();
		g_StrBufferHtml.str("");
		g_Buffer.clear();
	};	
};












std::string GetClassStr(unsigned uiIdex, unsigned uiAStart, unsigned uiBStart, unsigned uiCStart, unsigned uiDStart)
{
	if (uiIdex == 0)
		return "S-Class:";
	else if (uiIdex == uiAStart)
		return "A-Class:";
	else if (uiIdex == uiBStart)
		return "B-Class:";
	else if (uiIdex == uiCStart)
		return "C-Class:";
	else if (uiIdex == uiDStart)
		return "D-Class: \n";	
	return "";
};

void TxClassStr(unsigned uiIdex, unsigned uiAStart, unsigned uiBStart, unsigned uiCStart, unsigned uiDStart)
{
	std::string lClassName = GetClassStr(uiIdex, uiAStart, uiBStart, uiCStart, uiDStart);
	if (lClassName != "")
	{
		TxO(WHITE, lClassName);
		TxN();
	};
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
	struct { bool operator()(cTCardStats* a, cTCardStats* b) const { return a->m_fWinPct > b->m_fWinPct; } } bScoresort;
	std::sort(pkstats.begin(), pkstats.end(), bScoresort);

	TxI();	
	{
		TxO(WHITE, "Tier List for Tyrant Unleashed."); TxN();TxN();
		TxO(DARKYELLOW, "[E]=Epic ");TxO(MAGENTA,"[L]=Legend ");TxO(CYAN,"[V]=Vindi ");TxO(BLUE,"[M]=Mythic "); TxN();
		TxO(DARKBLUE,"I.=Imperial ");TxO(DARKYELLOW,"D.=raiDer ");TxO(DARKRED,"B.=Bloodthirsty ");TxO(GRAY,"X.=Xeno ");TxO(BLUE,"T.=righTeous ");TxO(CYAN,"P.=Progenitor ");TxN();		
		TxO(BLUE,"P_=Trigger Play ");TxO(MAGENTA,"A_=Trigger Attacked ");TxO(RED,"D_=Trigger Play Death ");TxO(MAGENTA, "[S]=Summon");TxN();TxN();
	};
	for (unsigned j = 0; j < pkstats.size(); j++)
	{		
		if (uiMode != 3)
		{
			if (uiMode == 0) { TxClassStr(j, 10, 30, 60, 100); }
			else { TxClassStr(j, 4, 10, 20, 30); };
		};
		unsigned uiMax = pkstats.size()/2;
		if (guiOutList > 0)
			uiMax = guiOutList;
		if (j < uiMax)
		{
			const Card* card = pkstats[j]->m_pkCard;
			if (card != NULL)
			{					
				float fBM = 10000 + (pkstats[j]->m_fWinPct * 100.0f);
				if (uiMode==3)
				{
					fBM = pkstats[j]->m_fWinPct;
					TxTab(4, "");
					TxO(CYAN, " ["); TxO(CYAN, Txf(fBM)); TxO(CYAN, "%] ");
					TzCard(card, 0);
					TxF(pkstats[j]->m_pkCard->m_name); TxF("#4\n");
					TxN();
				}
				else
				{
					TxTab(4, "");
					TxO(CYAN, " ["); TxO(CYAN, Txi(fBM)); TxO(CYAN, "] ");
					TzCard(card, 0);
					TxF(pkstats[j]->m_pkCard->m_name); TxF("#4\n");
					TxN();
				};
			};
		};		
	};
	TxOS(pkzFileName);		
	TxFS(pkzFileName);		
};



void TxDeckOut(cTDeck* pkDeck)
{
	if (pkDeck == NULL)
		return;	
	TxO(GREEN,"Win:");TxO(GREEN,Txf(pkDeck->m_fWinPct));TxO(GREEN,"% ");
	TxTab(15, "");
	TzRare(pkDeck->m_pkCom);
	TzFName(pkDeck->m_pkCom);TxO(WHITE,", ");
	TxF(pkDeck->m_pkCom->m_name);TxF(", ");
	TxTab(55, "");
	TzRare(pkDeck->m_pkDom);
	TzFName(pkDeck->m_pkDom);
	TxF(pkDeck->m_pkDom->m_name);
	for (unsigned i = 0; i < pkDeck->GetCardsSize(); i++)
	{
		TxO(WHITE, ", ");TxF(", ");
		TxTab(80 + (i * 30), "");
		TzRare(pkDeck->m_pkCards[i]);
		TzFName(pkDeck->m_pkCards[i]);
		TxF(pkDeck->m_pkCards[i]->m_name);
	};
	TxF("\n");
};

void cTDeckCore::MakeGauntlet(Cards& all_cards)
{
	TxI();	
	TxN();
	TxO(WHITE,"TUTitan_Off");	
	TxF("\n");	
	TxN();
	for (unsigned uiBase = 0; uiBase < g_DeckCore.m_SimData[0].size(); uiBase++)
	{
		cTDeck* TempDeck = &g_DeckCore.m_SimData[0][uiBase];
		if (TempDeck != NULL)
		{			
			TxO(MAGENTA,"TUTitan_Off");
			TxF("TUTitan_Off");
			if (uiBase < 10)  { TxF("0"); TxO(MAGENTA, "0"); };
			if (uiBase < 100) { TxF("0"); TxO(MAGENTA, "0"); };
			TxO(MAGENTA,Txi(uiBase));
			TxF(Txi(uiBase));
			TxO(MAGENTA,": ");
			TxF(": ");
			TxDeckOut(TempDeck);
			TxN();
		};
	};
	TxN();
	TxO(WHITE,"TUTitan_Def");	
	TxF("\n");	
	TxN();
	for (unsigned uiBase = 0; uiBase < g_DeckCore.m_SimData[1].size(); uiBase++)
	{
		cTDeck* TempDeck = &g_DeckCore.m_SimData[1][uiBase];
		if (TempDeck != NULL)
		{			
			TxO(MAGENTA,"TUTitan_Def");
			TxF("TUTitan_Def");
			if (uiBase < 10)  { TxF("0"); TxO(MAGENTA, "0"); };
			if (uiBase < 100) { TxF("0"); TxO(MAGENTA, "0"); };
			TxO(MAGENTA,Txi(uiBase));
			TxF(Txi(uiBase));
			TxO(MAGENTA,": ");
			TxF(": ");
			TxDeckOut(TempDeck);
			TxN();
		};
	};
	TxOS("Gauntlet");
	TxFS("Gauntlet");	
};

