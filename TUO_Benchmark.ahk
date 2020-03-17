#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
#SingleInstance off
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

VersionOfStarter := "v1.0.0"

MaxCardsSections := 100

IniFileName := "data\TUOBenchmark.ini"
IniSection := "onLoad"

IniRead, IniThreads, %IniFileName%, %IniSection%, Threads, 8
IniRead, IniIter, %IniFileName%, %IniSection%, Iter, 256
IniRead, IniGlobalLimit, %IniFileName%, %IniSection%, GlobalLimit, 16
IniRead, IniDeckSize, %IniFileName%, %IniSection%, DeckSize, 8

IniRead, IniCardLimitY, %IniFileName%, %IniSection%, CardLimitY, 8
IniRead, IniComLimitY, %IniFileName%, %IniSection%, ComLimitY, 8
IniRead, IniDomLimitY, %IniFileName%, %IniSection%, DomLimitY, 8
IniRead, IniIdLimit, %IniFileName%, %IniSection%, IdLimit, 4

IniRead, IniReCalcIter, %IniFileName%, %IniSection%, ReCalcIter, 1024
IniRead, IniMaxList, %IniFileName%, %IniSection%, MaxList, 1024
IniRead, IniOutList, %IniFileName%, %IniSection%, OutList, 25
IniRead, IniOutIdMax, %IniFileName%, %IniSection%, OutIdMax, 2

Menu, MyMenu, Add, ownedcards.txt, MenuOwnedcards
Menu, MyMenu, Add, customdecks.txt, MenuCustomdecks
Menu, MyMenu, Add, cardabbrs.txt, MenuCardabbrs
Menu, MyMenu, Add, Update XMLs, MenuUpdate
Menu, MyMenu, Add, Update XMLs (DEV), MenuUpdateDev
Menu, MyMenu, Add, Help, MenuHelp
Menu, MyMenu, Add, Web, MenuWeb
Gui, Menu, MyMenu

Gui, Add, Text, r1, Threads:
Gui, Add, Text, r1, Iter:
Gui, Add, Text, r1, GlobalLimit:
Gui, Add, Text, r1, DeckSize:
Gui, Add, Edit, vThreads ym w40 r1, %IniThreads%
Gui, Add, Edit, vIter w40 r1, %IniIter%
Gui, Add, Edit, vGlobalLimit w40 r1, %IniGlobalLimit%
Gui, Add, Edit, vDeckSize w40 r1, %IniDeckSize%

Gui, Add, Text,ym r1, CardLimitY:
Gui, Add, Text, r1, ComLimitY:
Gui, Add, Text, r1, DomLimitY:
Gui, Add, Text, r1, IdLimit:
Gui, Add, Edit, vCardLimitY ym w40 r1, %IniCardLimitY%
Gui, Add, Edit, vComLimitY w40 r1, %IniComLimitY%
Gui, Add, Edit, vDomLimitY w40 r1, %IniDomLimitY%
Gui, Add, Edit, vIdLimit w40 r1, %IniIdLimit%

Gui, Add, Text,ym r1, ReCalcIter:
Gui, Add, Text, r1, MaxList:
Gui, Add, Text, r1, OutList:
Gui, Add, Text, r1, OutIdMax:
Gui, Add, Text, r1, Factions:
Gui, Add, Text, r1, Mode:
Gui, Add, Edit, vReCalcIter ym w40 r1, %IniReCalcIter%
Gui, Add, Edit, vMaxList w40 r1, %IniMaxList%
Gui, Add, Edit, vOutList w40 r1, %IniOutList%
Gui, Add, Edit, vOutIdMax w40 r1, %IniOutIdMax%

Gui, Add, DDL, vFactions w160 section, AllFactions||Imperial|Raider|Bloodthirsty|Xeno|Righteous|Progenitor
Gui, Add, DDL, vBenchmark w160 section, Benchmark_Cards||Benchmark_Com|Benchmark_Dom|Benchmark_Fortress|GetBestCards|Gauntlet

Gui, Add, Button, default r2 w100 x100 y+25 section, Simulate
Gui, Add, Button, r2 w100 ys xs+200, Exit
Gui, Show,, Simple TUO TUOBenchmark
return

ButtonSimulate:
Gui, Submit
execString = tuo.exe "Cyrus, Medic" "Mission #1" TBMD %Threads% %Iter% %GlobalLimit% %DeckSize% %CardLimitY% %ComLimitY% %DomLimitY% %IdLimit% %ReCalcIter% %MaxList% %OutList% %OutIdMax% %Factions% %Benchmark%

Run, cmd.exe /c title TUOptimizeOutput && echo %execString% && %execString% & pause
Gui, Show
return

MenuHelp:
Gui, Submit
selTUO := "tuo" . (x86 ? "-x86" : "") . (openmp ? "-openmp" : "") . (debug ? "-debug" : (time ? "-time" : "")) . ".exe"
Run, cmd.exe /c title TUOptimizeOutput && echo %selTUO% && %selTUO% & pause
Gui, Show
return

MenuWeb:
Gui, Submit
Run https://github.com/APN-Pucky/tyrant_optimize/releases
Gui, Show
return

MenuUpdate:
MsgBox, 0, Update started, Updating XML files.`nPlease wait at least one minute. A new window should open soon.`nThis Window will auto close in 5 seconds. , 5
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/fusion_recipes_cj2.xml, data\fusion_recipes_cj2.xml
had_error := false
if ErrorLevel
{
    MsgBox, Error downloading fusion_recipes_cj2.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/missions.xml, data\missions.xml
if ErrorLevel
{
    MsgBox, Error downloading missions.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/skills_set.xml, data\skills_set.xml
if ErrorLevel
{
    MsgBox, Error downloading skills_set.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile.tyrantonline.com/assets/levels.xml, data\levels.xml
if ErrorLevel
{
    MsgBox, Error downloading levels.xml.
    had_error := true
}
Loop, %MaxCardsSections%
{
	URL = http://mobile.tyrantonline.com/assets/cards_section_%A_Index%.xml
	CardsFile = data\cards_section_%A_Index%.xml
	if (!DownloadCards(URL, CardsFile))
		break
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/APN-Pucky/tyrant_optimize/merged/data/raids.xml, data\raids.xml
if ErrorLevel
{
    MsgBox, Error downloading raids.xml.
    had_error := true
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/APN-Pucky/tyrant_optimize/merged/data/bges.txt, data\bges.txt
if ErrorLevel
{
    MsgBox, Error downloading bges.txt.
    had_error := true
}
if !had_error
    MsgBox, 0, Update finished, xml files successfully updated.`nThis Window will auto close in 2 seconds., 2
Gui, Show
return

MenuUpdateDev:
MsgBox, 0, Update started, Updating XML (DEV) files.`nPlease wait at least one minute. A new window should open soon.`nThis Window will auto close in 5 seconds. , 5
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/fusion_recipes_cj2.xml, data\fusion_recipes_cj2.xml
had_error := false
if ErrorLevel
{
    MsgBox, Error downloading fusion_recipes_cj2.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/missions.xml, data\missions.xml
if ErrorLevel
{
    MsgBox, Error downloading missions.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/skills_set.xml, data\skills_set.xml
if ErrorLevel
{
    MsgBox, Error downloading skills_set.xml.
    had_error := true
}
UrlDownloadToFile, *0 http://mobile-dev.tyrantonline.com/assets/levels.xml, data\levels.xml
if ErrorLevel
{
    MsgBox, Error downloading levels.xml.
    had_error := true
}
Loop, %MaxCardsSections%
{
	URL = http://mobile-dev.tyrantonline.com/assets/cards_section_%A_Index%.xml
	CardsFile = data\cards_section_%A_Index%.xml
	if (!DownloadCards(URL, CardsFile))
		break
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/APN-Pucky/tyrant_optimize/merged/data/raids.xml, data\raids.xml
if ErrorLevel
{
    MsgBox, Error downloading raids.xml.
    had_error := true
}
UrlDownloadToFile, *0 https://raw.githubusercontent.com/APN-Pucky/tyrant_optimize/merged/data/bges.txt, data\bges.txt
if ErrorLevel
{
    MsgBox, Error downloading bges.txt.
    had_error := true
}
if !had_error
    MsgBox, 0, Update finished, xml files successfully updated.`nThis Window will auto close in 2 seconds., 2
Gui, Show
return


MenuOwnedcards:
Gui, Submit
Run, Notepad.exe data\ownedcards.txt
Gui, Show
return

MenuCustomdecks:
Gui, Submit
Run, Notepad.exe data\customdecks.txt
Gui, Show
return

MenuCardabbrs:
Gui, Submit
Run, Notepad.exe data\cardabbrs.txt
Gui, Show
return

GuiClose:
ButtonExit:
Gui, Submit


IniWrite, %Threads%, %IniFileName%, %IniSection%, Threads
IniWrite, %Iter%, %IniFileName%, %IniSection%, Iter
IniWrite, %GlobalLimit%, %IniFileName%, %IniSection%, GlobalLimit
IniWrite, %DeckSize%, %IniFileName%, %IniSection%, DeckSize

IniWrite, %CardLimitY%, %IniFileName%, %IniSection%, CardLimitY
IniWrite, %ComLimitY%, %IniFileName%, %IniSection%, ComLimitY
IniWrite, %DomLimitY%, %IniFileName%, %IniSection%, DomLimitY
IniWrite, %IdLimit%, %IniFileName%, %IniSection%, IdLimit

IniWrite, %ReCalcIter%, %IniFileName%, %IniSection%, ReCalcIter
IniWrite, %MaxList%, %IniFileName%, %IniSection%, MaxList
IniWrite, %OutList%, %IniFileName%, %IniSection%, OutList
IniWrite, %OutIdMax%, %IniFileName%, %IniSection%, OutIdMax

while true
{
  IfWinExist, TUOptimizeOutput
      WinClose ; use the window found above
  else
      break
}
ExitApp

DownloadCards(url,file) {
	UrlDownloadToFile, *0 %url%, %file%
	if ErrorLevel
	{
		MsgBox, Error downloading %file%.
		had_error := true
	}
	else
	{
		FileReadLine,VAR1,%file%,3
		If InStr(VAR1, "File Not Found")
		{
			FileDelete, %file%
			return 0
		}
	}
	return 1
}
