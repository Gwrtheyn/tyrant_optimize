#ifndef TOUTPUT_H_INCLUDED
#define TOUTPUT_H_INCLUDED

#pragma once

#include <iostream>
#include <deque>
#include <windows.h> 
#include "card.h"
#include "cards.h"
#include "deck.h"
#include "titan.h"

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



void TxI(); //Init
void TxO(unsigned uiColor, std::string pkzString); //console+html out
void TxOS(std::string pkzFileName); // save html
void TxF(std::string pkzString); //txt out
void TxFS(std::string pkzFileName);// save txt
void TxN();//newline
void TxTab(unsigned uiSize,std::string pkzFill);
void TxC(unsigned uiColor); //set color
void TzCard(const Card* pkCard, unsigned uiInstance);
std::string Txi(unsigned uiNumber);//inttostr
std::string Txf(float fNumber);//floatostr
void TuBenchmarkCardsOut(std::string pkzFileName, unsigned uiMode);
void SimOutput(cTDeck* pkctDeck);

#endif //TOUTPUT_H_INCLUDED
