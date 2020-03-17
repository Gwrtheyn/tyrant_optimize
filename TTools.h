#ifndef TTOOLS_H_INCLUDED
#define TTOOLS_H_INCLUDED

#pragma once

#include <iostream>
#include <deque>
#include <windows.h> 
#include "card.h"
#include "cards.h"
#include "deck.h"

extern std::deque<cTDeck>				g_Buffer;
extern boost::posix_time::ptime			gtStartTime;
extern boost::posix_time::ptime			gUpdateTime;

void InitTimes();
Deck* tfind_deck(Decks& decks, const Cards& all_cards, std::string deck_name);
void TuReadCards();
Card* FReadCard(std::string sData, Cards& all_cards);
void UpdateScore(Cards& all_cards);

#endif //TTOOLS_H_INCLUDED
