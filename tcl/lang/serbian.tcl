### Srpski.tcl: 
# Srpski meni za SCID vs PC.
# Autor: Miljan Ranèiæ (Mujagi77).
# Prevod uraðen uz podršku ChatGPT-a.

# Dodavanje srpskog jezika u SCID

addLanguage Y Serbian 2 cp1250

proc setLanguage_Y {} {

# File menu:
menuText Y File "Fajl" 0
menuText Y FileNew "Novi..." 0 {Kreiraj novu Scid bazu podataka}
menuText Y FileOpen "Otvori..." 0 {Otvori postojeæu Scid bazu podataka}
menuText Y FileClose "Zatvori" 0 {Zatvori aktivnu Scid bazu podataka}
menuText Y FileFinder "Tragaè" 0 {Pokreni Fajl Tragaè}
menuText Y FileSavePgn "Saèuvaj Pgn..." 0 {}
menuText Y FileOpenBaseAsTree "Otvori bazu kao stablo" 13 {Otvori bazu i koristi je u Tree prozoru}
menuText Y FileOpenRecentBaseAsTree "Otvori nedavnu bazu kao stablo" 0 {Otvori nedavnu bazu i koristi je u Tree prozoru}
menuText Y FileBookmarks "Markeri" 0 {Meni sa markerima (preèica: Ctrl+B)}
menuText Y FileBookmarksAdd "Dodaj u markere" 0 \
  {Dodaj u markere tekuæu partiju iz baze i poziciju}
menuText Y FileBookmarksFile "Arhiviraj marker" 0 \
  {Arhiviraj marker za tekuæu partiju i poziciju}
menuText Y FileBookmarksEdit "Izmeni markere..." 0 \
  {Izmeni menije markera}
menuText Y FileBookmarksList "Prikaži foldere kao jedinstvenu listu" 0 \
  {Prikaži foldere markera kao jedinstvenu listu, bez podmenija}
menuText Y FileBookmarksSub "Prikaži foldere kao podmenije" 0 \
  {Prikaži foldere markera kao podmenije, ne jedinstvenu listu}
menuText Y FileReadOnly "Samo za èitanje..." 0 \
  {Spreèi izmene u tekuæoj bazi}
menuText Y FileSwitch "Prebaci bazu" 0 \
  {Prebaci na drugu otvorenu bazu} 
menuText Y FileExit "Izaði" 1 {Izlazak iz Scid-a}

# Edit menu:
menuText Y Edit "Izmene" 0
menuText Y EditAdd "Dodaj varijantu" 0 {Dodaj varijantu poteza u partiji}
menuText Y EditPasteVar "Nalepi varijantu" 0
menuText Y EditDelete "Obriši varijantu" 0 {Obriši varijantu za ovaj potez}
menuText Y EditDeleteComment "Obriši komentar" 0
menuText Y EditFirst "Postavi prvu varijantu" 5 \
  {Postavi varijantu kao prvu u listi}
menuText Y EditMain "Postavi na glavnu liniju" 21 \
  {Postavi varijantu na glavnu liniju}
menuText Y EditTrial "Probaj varijantu" 0 \
  {Pokreni/zaustavi mod za testiranje ideje na tabli}
menuText Y EditStrip "Ukloni" 2 {Ukloni komentare ili varijante iz ove partije}
menuText Y EditUndo "Poništi" 0 {Poništi poslednju izmenu partije}
menuText Y EditRedo "Ponovi" 0 {Ponovi poslednju poništenu izmenu}
menuText Y EditStripComments "Komentare" 0 \
  {Ukloni sve komentare i napomene iz ove partije}
menuText Y EditStripVars "Varijante" 0 {Ukloni sve varijante iz ove partije}
menuText Y EditStripBegin "Poteze od poèetka" 1 \
  {Ukloni poteze od poèetka partije} 
menuText Y EditStripEnd "Poteze do kraja" 0 \
  {Ukloni poteze do kraja partije}
menuText Y EditReset "Isprazni Clipbase" 0 \
  {Resetuje clipbase da bude potpuno prazan}
menuText Y EditCopy "Kopiraj partiju u Clipbase" 0 \
  {Kopiraj ovu partiju u clipbase partije}
menuText Y EditPaste "Prenesi poslednju Clipbase partiju" 0 \
  {Prenesi aktivnu clipbase partiju ovde}
menuText Y EditPastePGN "Nalepi PGN iz klipborda..." 10 \
  {Tumaèi tekst iz klipborda kao PGN partiju i nalepi ovde}
menuText Y EditSetup "Namesti poèetnu poziciju..." 0 \
  {Postavi poèetnu poziciju za ovu partiju}
menuText Y EditCopyBoard "Kopiraj poziciju" 6 \
  {Kopiraj trenutnu poziciju u FEN notaciji na klipbord}
menuText Y EditCopyPGN "Kopiraj PGN" 0 {}
menuText Y EditPasteBoard "Prenesi poèetnu poziciju" 9 \
  {Postavi poèetnu poziciju na osnovu trenutno izabranog teksta (klipbord)}

# Game menu:
menuText Y Game "Partija" 0
menuText Y GameNew "Nova partija" 0 \
  {Pokreni novu partiju, sa gubljenjem svih izmena}
menuText Y GameFirst "Uèitaj prvu partiju" 5 {Uèitaj prvu filtriranu partiju}
menuText Y GamePrev "Uèitaj prethodnu partiju" 5 {Uèitaj prethodnu filtriranu partiju}
menuText Y GameReload "Osveži trenutnu partiju" 3 \
  {Osveži ovu partiju, sa gubljenjem svih izmena}
menuText Y GameNext "Uèitaj sledeæu partiju" 5 {Uèitaj sledeæu filtriranu partiju}
menuText Y GameLast "Uèitaj poslednju partiju" 8 {Uèitaj poslednju filtriranu partiju}
menuText Y GameRandom "Uèitaj nasumiènu partiju" 8 {Uèitaj nasumiènu filtriranu partiju}
menuText Y GameNumber "Uèitaj partiju broj..." 5 \
  {Uèitaj partiju, unoseæi njen broj}
menuText Y GameReplace "Saèuvaj: Zameni partiju..." 6 \
  {Saèuvaj ovu partiju preko stare partije u bazi}
menuText Y GameAdd "Saèuvaj: Dodaj novu partiju..." 6 \
  {Saèuvaj ovu partiju kao novu u bazi}
menuText Y GameInfo "Postavi informacije o partiji" 9
menuText Y GameBrowse "Pregled partije" 0
menuText Y GameList "Prikaži sve partije" 0
menuText Y GameDelete "Obriši partiju" 0
menuText Y GameDeepest "Identifikuj otvaranje" 0 \
  {Idi na poslednju poziciju partije koja odgovara knjizi otvaranja}
menuText Y GameGotoMove "Idi na potez broj..." 5 \
  {Idi na odreðeni broj poteza u tekuæoj partiji}
menuText Y GameNovelty "Pronaði novost..." 8 \
  {Pronaði prvi potez ove partije koji nije igran ranije}

# Search Menu:
menuText Y Search "Pretraži" 3
menuText Y SearchReset "Resetuj filter" 0 {Resetuj filter da bi sve partije bile ukljuèene}
menuText Y SearchNegate "Invertuj filter" 0 {Invertuj filter da ukljuèi samo iskljuèene partije}
menuText Y SearchEnd "Filter do poslednjeg poteza" 0 {Primeni filter do poslednjeg poteza partije}
menuText Y SearchCurrent "Tekuæa pozicija..." 0 {Pretraži prema tekuæoj poziciji na tabli}
menuText Y SearchHeader "Zaglavlje..." 0 {Pretraži prema informacijama iz zaglavlja (igraè, turnir, itd.)}
menuText Y SearchMaterial "Materijal/Pozicija..." 0 {Pretraži prema materijalu i poziciji}
menuText Y SearchMoves "Potezi" 0 {Pretraži prema potezima partije}
menuText Y SearchUsing "Pomoæu Search fajla..." 0 {Pretraži pomoæu Search fajla}

# Windows menu:
menuText Y Windows "Prozori" 1
menuText Y WindowsGameinfo "Info o partiji" 0 {Prikaži/sakrij panel sa informacijama o partiji}
menuText Y WindowsComment "Editor komentara" 0 {Otvori/zatvori editor komentara}
menuText Y WindowsGList "Lista partija" 0 {Otvori/zatvori listu partija}
menuText Y WindowsPGN "PGN prozor" 0 {Otvori/zatvori PGN prozor (sa podacima o partiji)}
menuText Y WindowsCross "Tabela turnira" 0 {Prikaži tabelu turnira za ovu partiju}
menuText Y WindowsPList "Tragaè igraèa" 2 {Otvori/zatvori tragaè igraèa}
menuText Y WindowsTmt "Turnir tragaè" 2 {Otvori/zatvori turnir tragaè}
menuText Y WindowsSwitcher "Biraè baza" 0 \
  {Otvori/zatvori biraè baza}
menuText Y WindowsMaint "Prozor održavanja" 0 \
  {Otvori/zatvori prozor održavanja}
menuText Y WindowsECO "ECO prikazivaè" 0 {Otvori/zatvori ECO prikazivaè}
menuText Y WindowsStats "Statistièki prozor" 0 \
  {Otvori/zatvori prozor statistika filtera}
menuText Y WindowsTree "Stablo varijanata" 0 {Otvori/zatvori stablo varijanata}
menuText Y WindowsTB "Tabela završnica" 1 \
  {Otvori/zatvori tabelu završnica}
menuText Y WindowsBook "Book prozor" 0 {Otvori/zatvori Book prozor}
menuText Y WindowsCorrChess "Korespondencijski prozor" 0 {Otvori/zatvori korespondencijski prozor}

# Tools menu:
menuText Y Tools "Alati" 0
menuText Y ToolsAnalysis "Program za analizu..." 0 \
  {Pokreni/zaustavi šahovski program za analizu}
menuText Y ToolsEmail "Email menadžer" 0 \
  {Otvori/zatvori menadžer za korespondencijske partije preko email-a}
menuText Y ToolsFilterGraph "Grafikon filtera" 7 \
  {Otvori/zatvori prozor grafikona filtera}
menuText Y ToolsAbsFilterGraph "Apsolutni grafikon filtera" 7 \
  {Otvori/zatvori prozor grafikona filtera za apsolutne vrednosti}
menuText Y ToolsOpReport "Izveštaj o otvaranjima" 0 \
  {Napravi izveštaj o otvaranjima za trenutnu poziciju}
menuText Y ToolsTracker "Prati figure" 0 {Otvori prozor za praæenje figura}
menuText Y ToolsTraining "Trening" 0 {Alati za trening (taktike, otvaranja,...)}
menuText Y ToolsComp "Turnir" 2 {Turnir šahovskih motora}
menuText Y ToolsTacticalGame "Taktièka partija" 0 {Igraj partiju sa taktièkim izazovima}
menuText Y ToolsSeriousGame "Ozbiljna partija" 0 {Igraj standardnu partiju}
menuText Y ToolsTrainTactics "Taktike" 0 {Reši taktièke zadatke}
# ====== TODO To be translated ======
menuText Y ToolsTrainOpening "Opening Trainer" 0 {Rep training}
menuText Y ToolsTrainCalvar "Izraèunavanje varijanti" 0 {Trening izraèunavanja varijanti}
menuText Y ToolsTrainFindBestMove "Pronaði najbolji potez" 0 {Pronaði najbolji potez}
menuText Y ToolsTrainFics "Internet" 0 {Igraj na freechess.org}
menuText Y ToolsBookTuning "Podešavanje knjige otvaranja" 0 {Podešavanje knjige otvaranja}
menuText Y ToolsMaint "Održavanje" 0 {Alati za održavanje Scid baze podataka}
menuText Y ToolsMaintWin "Prozor održavanja" 0 \
  {Otvori/zatvori prozor održavanja Scid baze podataka}
menuText Y ToolsMaintExtra "Dodatni tagovi" 0 {Upravljanje dodatnim tagovima}
menuText Y ToolsMaintCompact "Komprimuj bazu..." 0 \
  {Uèini fajlove baze kompaktnijim, uklanjajuæi obrisane partije i nekorišæena imena}
menuText Y ToolsMaintClass "ECO klasifikacija partija..." 2 \
  {Klasifikuj sve partije po otvaranju}
menuText Y ToolsMaintSort "Sortiraj bazu..." 0 \
  {Sortiraj sve partije u bazi podataka}
menuText Y ToolsMaintDelete "Izbriši duplirane partije..." 0 \
  {Naði duplirane partije i oznaèi ih za brisanje}
menuText Y ToolsMaintTwin "Provera dupliranih partija" 0 \
  {Otvori/osveži prozor za proveru dupliranih partija}
menuText Y ToolsMaintNameEditor "Editor imena" 0 \
  {Otvori/zatvori editor imena}
menuText Y ToolsMaintNamePlayer "Provera pravopisa imena igraèa..." 11 \
  {Proveri pravopis imena igraèa pomoæu spellcheck fajla}
menuText Y ToolsMaintNameEvent "Provera pravopisa imena turnira..." 11 \
  {Proveri pravopis imena turnira pomoæu spellcheck fajla}
menuText Y ToolsMaintNameSite "Provera pravopisa imena mesta..." 11 \
  {Proveri pravopis imena mesta pomoæu spellcheck fajla}
menuText Y ToolsMaintNameRound "Provera pravopisa kola..." 11 \
  {Proveri pravopis imena kola pomoæu spellcheck fajla}
menuText Y ToolsMaintFixBase "Popravi bazu" 0 {Pokušaj popraviti ošteæenu bazu}
menuText Y ToolsConnectHardware "Poveži hardver" 0 {Poveži eksterni hardver}
menuText Y ToolsConnectHardwareConfigure "Konfiguriši..." 0 {Konfiguriši eksterni hardver i konekciju}
menuText Y ToolsConnectHardwareNovagCitrineConnect "Poveži Novag Citrine" 0 {Poveži Novag Citrine}
menuText Y ToolsConnectHardwareInputEngineConnect "Poveži Input Engine" 0 {Poveži Input Engine (npr. DGT)}
menuText Y ToolsPInfo "Informacije o igraèu" 0 {Otvori/osveži informacije o igraèu}
menuText Y ToolsPlayerReport "Izveštaj o igraèu" 3 {Generiši izveštaj o igraèu}
menuText Y ToolsRating "Grafikon rejtinga" 0 \
  {Prikaži grafikon istorije rejtinga igraèa u tekuæoj partiji}
menuText Y ToolsScore "Grafikon rezultata" 0 {Prikaži grafikon rezultata}
menuText Y ToolsExpCurrent "Izvezi tekuæu partiju" 8 \
  {Saèuvaj tekuæu partiju u tekstualni fajl}
menuText Y ToolsExpCurrentPGN "Izvezi partiju u PGN fajl..." 17 \
  {Saèuvaj tekuæu partiju u PGN fajl}
menuText Y ToolsExpCurrentHTML "Izvezi partiju u HTML fajl..." 17 \
  {Saèuvaj tekuæu partiju u HTML fajl}
menuText Y ToolsExpCurrentHTMLJS "Izvezi partiju u HTML i JavaScript fajl..." 15 \
  {Saèuvaj tekuæu partiju u HTML i JavaScript fajl}
menuText Y ToolsExpCurrentLaTeX "Izvezi partiju u LaTeX fajl..." 17 \
  {Saèuvaj tekuæu partiju u LaTeX fajl}
menuText Y ToolsExpFilter "Izvezi sve filtrirane partije" 1 \
  {Saèuvaj sve filtrirane partije u tekstualni fajl}
menuText Y ToolsExpFilterPGN "Izvezi filtrirane partije u PGN fajl..." 28 \
  {Saèuvaj sve filtrirane partije u PGN fajl}
menuText Y ToolsExpFilterHTML "Izvezi filtrirane partije u HTML fajl..." 28 \
  {Saèuvaj sve filtrirane partije u HTML fajl}
menuText Y ToolsExpFilterHTMLJS "Izvezi filtrirane partije u HTML i JavaScript fajl..." 17 \
  {Saèuvaj sve filtrirane partije u HTML i JavaScript fajl}
menuText Y ToolsExpFilterLaTeX "Izvezi filtrirane partije u LaTeX fajl..." 28 \
  {Saèuvaj sve filtrirane partije u LaTeX fajl}
menuText Y ToolsExpFilterEPD "Izvezi filtrirane partije u EPD/FEN" 17 \
  {Saèuvaj sve filtrirane partije u EPD fajl}
menuText Y ToolsExpFilterGames "Izvezi listu partija u tekst..." 19 \
  {Ispiši formatiranu listu partija u tekstualni fajl}
menuText Y ToolsImportOne "Uvezi jednu PGN partiju..." 0 \
  {Uvezi jednu partiju iz PGN fajla}
menuText Y ToolsImportFile "Uvezi više PGN partija..." 6 \
  {Uvezi više partija iz PGN fajla}
menuText Y ToolsStartEngine1 "Pokreni motor 1" 0 {Pokreni motor 1}
menuText Y ToolsStartEngine2 "Pokreni motor 2" 0 {Pokreni motor 2}
menuText Y ToolsScreenshot "Snimak table" 0
menuText Y Play "Igraj" 0
menuText Y CorrespondenceChess "Korespondencijski šah" 0 {Funkcije za eMail i Xfcc korespondencijski šah}
menuText Y CCConfigure "Konfiguriši..." 0 {Konfiguriši spoljne alate i opštu postavku}
menuText Y CCConfigRelay "Konfiguriši posmatranje..." 10 {Konfiguriši partije za posmatranje}
menuText Y CCOpenDB "Otvori bazu..." 0 {Otvori podrazumevanu korespondencijsku bazu}
menuText Y CCRetrieve "Preuzmi partije" 0 {Preuzmi partije preko eksternog (Xfcc) pomoænika}
menuText Y CCInbox "Obradi Inbox" 0 {Obradi sve fajlove u scid Inbox}
menuText Y CCSend "Pošalji potez" 0 {Pošalji svoj potez putem eMail-a ili eksternog pomoænika}
menuText Y CCResign "Predaj partiju" 0 {Predaj partiju (ne putem eMail-a)}
menuText Y CCClaimDraw "Zatraži remi" 0 {Pošalji potez i zatraži remi (ne putem eMail-a)}
menuText Y CCOfferDraw "Ponudi remi" 0 {Pošalji potez i ponudi remi (ne putem eMail-a)}
menuText Y CCAcceptDraw "Prihvati remi" 0 {Prihvati ponudu za remi (ne putem eMail-a)}
menuText Y CCNewMailGame "Nova eMail partija..." 0 {Pokreni novu eMail partiju}
menuText Y CCMailMove "Pošalji potez..." 0 {Pošalji potez putem eMail-a protivniku}
menuText Y CCGamePage "Stranica partije..." 0 {Pozovi partiju preko web pretraživaèa}
menuText Y CCEditCopy "Kopiraj listu partija u Clipbase" 0 {Kopiraj partije kao CSV listu u clipbase}


# Options menu:
menuText Y Options "Opcije" 0
menuText Y OptionsBoard "Tabla" 0 {Opcije izgleda šahovske table}
menuText Y OptionsColour "Boja teksta" 0 {Podrazumevana boja teksta u widgetu}
menuText Y OptionsBackColour "Pozadina" 0 {Podrazumevana boja pozadine widgeta}
menuText Y OptionsBackColourGlobal "Globalno" 0 {Globalna boja pozadine}
menuText Y OptionsMainLineColour "Glavna linija strelica" 0 {Boja strelica glavne linije}
menuText Y OptionsVarLineColour "Varijante strelica" 0 {Boja strelica varijanti}
menuText Y OptionsEngineLineColour "Strelice motora" 0 {Boja strelica motora}
# ====== TODO To be translated ======
menuText Y OptionsArrows "Arrows" 0 {Arrow options}
# ====== TODO To be translated ======
menuText Y OptionsArrowsWidth "Arrow Width" 5 {Thickness of board arrows}
# ====== TODO To be translated ======
menuText Y OptionsArrowsLength "Arrow Length" 6 {Length of board arrows}
# ====== TODO To be translated ======
menuText Y OptionsArrowsEngineColour "Engine arrows" 0 {Engine arrow options}
# ====== TODO To be translated ======
menuText Y OptionsArrowsEngineFixed "Fixed colour" 0 {Best engine arrow in the engine colour, other ranks in grey}
# ====== TODO To be translated ======
menuText Y OptionsArrowsEngineRelative "Traffic Light (relative)" 0 {Colour the engine arrows by score drop to the best line: green, yellow, orange, red}
# ====== TODO To be translated ======
menuText Y OptionsArrowsEngineAbsolute "Traffic Light (absolute)" 0 {Colour the engine arrows by absolute score, like the MultiPV colour bar}
# ====== TODO To be translated ======
menuText Y OptionsArrowsEngineChange "Change Colour..." 0 {Choose the engine arrow colour}
# ====== TODO To be translated ======
menuText Y OptionsArrowsEngineLabelsOff "Off" 0 {No numbers on the engine arrows}
# ====== TODO To be translated ======
menuText Y OptionsArrowsEngineLabelsPV "Rank" 0 {Show the MultiPV rank on each engine arrow}
# ====== TODO To be translated ======
menuText Y OptionsArrowsEngineLabelsDiff "Score Diff" 0 {Show the score difference to the best line on each engine arrow}
# ====== TODO To be translated ======
menuText Y OptionsArrowsEngineLabelsScore "Absolute" 0 {Show the engine score in tenths of a pawn on each engine arrow}
# ====== TODO To be translated ======
menuText Y OptionsArrowsEngineThick "Thicker arrows for better lines" 0 {Draw better engine lines with thicker arrows}
# ====== TODO To be translated ======
menuText Y OptionsRowColour "Redovi" 0 {Podrazumevana boja redova u stablu/knjizi}
menuText Y OptionsHighlightColour "Isticanje" 0 {Podrazumevana boja isticanja teksta}
menuText Y OptionsSwitcherColour "Biraè baza" 0 {Boja biraèa baza}
menuText Y OptionsProgressColour "Traka napretka" 0 {Boja trake napretka}
menuText Y OptionsScoreColour "Grafikon rezultata" 0 {}
menuText Y OptionsScoreBarColour "Traka trenutnog rezultata" 0 {}
menuText Y OptionsNames "Imena mojih igraèa..." 0 {Uredi imena svojih igraèa}
menuText Y OptionsExport "Izvoz" 0 {Promeni opcije tekstualnog izvoza}
menuText Y OptionsFonts "Fontovi" 0 {Promeni fontove}
menuText Y OptionsFontsRegular "Obièan" 0 {Promeni obièan font}
menuText Y OptionsFontsMenu "Meni" 0 {Promeni font menija}
menuText Y OptionsFontsSmall "Mali" 0 {Promeni mali font}
menuText Y OptionsFontsFixed "Neproporcionalni" 0 {Promeni neproporcionalni font}
menuText Y OptionsEvalbar "Traka rezultata" 0 {Opcije trake rezultata}
menuText Y OptionsGInfo "Informacije o partiji" 0 {Opcije prikaza informacija o partiji}
menuText Y OptionsFics "FICS" 0
menuText Y OptionsFicsAuto "Automatska promocija dame" 0
menuText Y OptionsFicsClk "Pamæenje vremena na satu" 0
menuText Y OptionsFicsColour "Boja teksta" 0
menuText Y OptionsFicsSize "Velièina table" 0
menuText Y OptionsFicsButtons "Dugmad korisnika" 0
menuText Y OptionsFicsCommands "Inicijalne komande" 0
menuText Y OptionsFicsNoRes "Bez rezultata" 0
menuText Y OptionsFicsNoReq "Bez zahteva" 0
menuText Y OptionsFicsPremove "Dozvoli Premove" 0
menuText Y OptionsEvalMain "Glavna tabla" 0
menuText Y OptionsEvalBrowser "Preglednici partija" 0
menuText Y OptionsEvalFics "Fics table" 0
menuText Y OptionsEvalTime "Vreme evaluacije" 0 {Vreme za brzu analizu}
menuText Y OptionsEvalScale "Skala" 0 {Skala trake rezultata}
menuText Y OptionsEvalTicks "Prikaži oznake" 0
menuText Y OptionsEvalLog "Logaritamska evaluacija" 0
menuText Y OptionsLanguage "Jezik" 0 {Izaberi jezik}
menuText Y OptionsMovesTranslatePieces "Prevedi figure" 0 {Prevedi poèetno slovo figura}
menuText Y OptionsMovesHighlightLastMove "Istakni poslednji potez" 0
menuText Y OptionsMovesHighlightLastMoveDisplay "Prikaži" 0 {Prikaži isticanje poslednjeg poteza}
menuText Y OptionsMovesHighlightLastMoveWidth "Debljina" 0 {Debljina linije isticanja}
menuText Y OptionsMovesHighlightLastMoveColor "Izaberi boju" 0 {Boja linije isticanja}
menuText Y OptionsMovesColorSelected "Boja izabranog polja" 0 {Boja izabranog poteza}
menuText Y OptionsMoves "Potezi" 0 {Opcije unosa poteza}
menuText Y OptionsMovesAsk "Pitaj pre zamene poteza" 0 {Pitaj da li postojeæi potez zameniti novim}
menuText Y OptionsMovesAnimate "Animacija poteza" 1 {Podesi trajanje animacije poteza}
menuText Y OptionsMovesDelay "Odlaganje izmeðu poteza..." 1 {Podesi vreme odlaganja pri automatskom pregledanju poteza}
menuText Y OptionsMovesCoord "Unošenje poteza preko koordinata" 0 {Prihvata unos poteza preko koordinata ("g1f3")}
menuText Y OptionsMovesSuggest "Prikaži predložene poteze" 0
menuText Y OptionsShowVarPopup "Prikaži prozor varijanti" 0
menuText Y OptionsMovesSpace "Dodaj razmake posle broja poteza" 0
menuText Y OptionsMovesKey "Dopunjavanje poteza" 0 {Ukljuèi/iskljuèi automatsko dopunjavanje poteza tastaturom}
menuText Y OptionsMovesShowVarArrows "Prikaži strelice varijanti" 0
menuText Y OptionsNumbers "Format brojeva" 0 {Izaberi format brojeva}
menuText Y OptionsStartup "Pokretanje" 3 {Izaberi prozore koji se otvaraju pri startu}
menuText Y OptionsTheme "Tema" 0 {Promeni izgled interfejsa}
menuText Y OptionsWindows "Prozori" 0 {Opcije prozora}
menuText Y OptionsWindowsIconify "Automatsko minimiziranje" 5 {Minimiziraj sve prozore kada je glavni prozor minimiziran}
menuText Y OptionsWindowsRaise "Automatsko podizanje" 0 {Podigni odreðene prozore kad god su prekriveni}
menuText Y OptionsSounds "Zvuci..." 2 {Konfiguriši zvuke za poteze}
menuText Y OptionsWindowsDock "Prikljuèi prozore" 0
menuText Y OptionsWindowsSaveLayout "Saèuvaj raspored" 0
menuText Y OptionsWindowsRestoreLayout "Vrati raspored" 0
menuText Y OptionsWindowsShowGameInfo "Prikaži informacije o partiji" 0
menuText Y OptionsWindowsAutoLoadLayout "Automatski uèitaj prvi raspored" 0
menuText Y OptionsWindowsAutoResize "Automatska promena velièine table" 0
menuText Y OptionsWindowsFullScreen "Ceo ekran" 0
menuText Y OptionsToolbar "Toolbar glavnog prozora" 12 {Prikaži/sakrij toolbar glavnog prozora}
menuText Y OptionsECO "Uèitaj ECO fajl..." 7 {Uèitaj ECO klasifikacioni fajl}
menuText Y OptionsSpell "Uèitaj fajl za proveru pravopisa..." 13 {Uèitaj fajl za proveru pravopisa}
menuText Y OptionsTable "Direktorijum baza završnica..." 0 {Izaberi direktorijum baze završnica; sve baze u njemu æe biti korišæene}
menuText Y OptionsRecent "Nedavni fajlovi..." 0 {Promeni broj prikazanih nedavnih fajlova u Fajl meniju}
menuText Y OptionsBooksDir "Direktorijum knjiga otvaranja..." 0 {Postavi direktorijum knjiga otvaranja}
menuText Y OptionsTacticsBasesDir "Direktorijum baza taktika..." 0 {Postavi direktorijum baza taktika (trening)}
menuText Y OptionsInformant "Konfiguriši Informant vrednosti" 0
menuText Y OptionsSave "Saèuvaj opcije" 0 "Saèuvaj sve opcije u fajl $::optionsFile"
menuText Y OptionsSaveBases "Saèuvaj baze kao podrazumevane" 0
menuText Y OptionsAutoSave "Automatski saèuvaj opcije pri izlasku" 0 {Automatski saèuvaj sve opcije pri izlasku iz Scid-a}

# Help menu:
menuText Y Help "Pomoæ" 2
menuText Y HelpContents "Sadržaj" 0 {Prikaži stranicu sa sadržajem pomoæi}
menuText Y HelpIndex "Sadržaj" 0 {Prikaži glavnu stranu pomoæi}
menuText Y HelpGuide "Brzi vodiè" 0 {Prikaži brzi vodiè}
menuText Y HelpHints "Saveti" 0 {Prikaži savete}
menuText Y HelpContact "Kontakt" 0 {Prikaži informacije o kontaktu}
menuText Y HelpTip "Dnevni savet" 0 {Prikaži korisni Scid savet dana}
menuText Y HelpStartup "Startup prozor" 0 {Prikaži startup prozor}
menuText Y HelpAbout "O programu Scid" 0 {Informacije o programu Scid}

# Game info box popup menu:
menuText Y GInfoHideNext "Sakrij sledeæi potez" 0
menuText Y GInfoShow "Igra na potezu" 0
menuText Y GInfoCoords "Prikaži koordinate" 0
menuText Y GInfoMaterial "Prikaži vrednosti materijala" 0
menuText Y GInfoFEN "Prikaži FEN" 8
menuText Y GInfoMarks "Prikaži obojena polja i strelice" 5
menuText Y GInfoWrap "Prelomi dugaèke linije" 0
menuText Y GInfoFullComment "Prikaži ceo komentar" 10
menuText Y GInfoPhotos "Prikaži fotografije" 5
menuText Y GInfoTBNothing "Baza završnica: ništa" 16
menuText Y GInfoTBResult "Baza završnica: samo rezultati" 16
menuText Y GInfoTBAll "Baza završnica: rezultati i najbolji potezi" 16
menuText Y GInfoDelete "Obriši/vrati ovu partiju" 0
menuText Y GInfoMark "Oznaèi/ukloni oznaku partije" 0
menuText Y GInfoMenuBar "Traka menija" 0
menuText Y GInfoToolBar "Traka alatki" 0
menuText Y GInfoButtonBar "Traka dugmadi" 0
menuText Y GInfoStatusBar "Traka statusa" 0
menuText Y GInfoEvalBar "Traka rezultata" 0
translate Y FlipBoard "Okreni tablu"
translate Y RaiseWindows "Podigni prozore"
translate Y AutoPlay "Automatska reprodukcija"
translate Y TrialMode "Probni režim"
translate Y KeepOpen "Ostavi otvoreno posle pokretanja"
# ====== TODO To be translated ======
translate Y BigIcons {Big Icons}

# General buttons:
translate Y Apply "Primeni"
translate Y Back "Nazad"
translate Y Browse "Pretraži"
translate Y Cancel "Poništi"
translate Y Continue "Nastavi"
translate Y Clear "Oèisti"
translate Y Close "Zatvori"
translate Y Contents "Sadržaj"
translate Y Defaults "Podrazumevano"
translate Y Delete "Obriši"
translate Y Graph "Grafikon"
translate Y Help "Pomoæ"
translate Y Import "Uvoz"
translate Y Index "Indeks"
translate Y LoadGame "Uèitaj partiju"
translate Y BrowseGame "Pregled partije"
translate Y MergeGame "Spoji partiju"
translate Y MergeGames "Spoji partije"
translate Y Preview "Pregled"
translate Y Revert "Vrati"
translate Y Save "Saèuvaj"
translate Y DontSave "Ne saèuvaj"
translate Y Search "Traži"
translate Y Stop "Zaustavi"
translate Y Store "Saèuvaj"
translate Y Update "Osveži"
translate Y ChangeOrient "Promeni orijentaciju prozora"
translate Y ShowIcons "Prikaži ikone"
translate Y ConfirmCopy "Potvrdi kopiranje"
translate Y None "Ništa"
translate Y First "Prvu"
translate Y Current "Trenutna"
translate Y Last "Poslednju"
translate Y Font "Font"
translate Y Change "Promeni"
translate Y Random "Nasumièno"

# General messages:
translate Y game "partija"
translate Y games "partije"
translate Y move "potez"
translate Y moves "potezi"
translate Y all "sve"
translate Y Yes "Da"
translate Y No "Ne"
translate Y Both "Oba"
translate Y King "Kralj"
translate Y Queen "Dama"
translate Y Rook "Top"
translate Y Bishop "Lovac"
translate Y Knight "Skakaè"
translate Y Pawn "Pešak"
translate Y White "Beli"
translate Y Black "Crni"
translate Y Player "Igraè"
translate Y Rating "Rejting"
translate Y RatingDiff "Razlika u rejtingu (Beli - Crni)"
translate Y AverageRating "Proseèan rejting"
translate Y Event "Turnir"
translate Y Site "Mesto"
translate Y Country "Zemlja"
translate Y IgnoreColors "Ignoriši boje"
translate Y MatchEnd "Samo završni rezultat"
translate Y Date "Datum"
translate Y EventDate "Datum turnira"
translate Y Decade "Decenija"
translate Y Year "Godina"
translate Y Month "Mesec"
translate Y Months "Januar Februar Mart April Maj Jun Jul Avgust Septembar Oktobar Novembar Decembar"
translate Y Days "Ned Pon Uto Sre Èet Pet Sub"
translate Y YearToToday "Godina do danas"
translate Y Result "Rezultat"
translate Y Round "Kolo"
translate Y Length "Dužina"
translate Y ECOCode "ECO kôd"
translate Y ECO "ECO"
translate Y Deleted "Izbrisano"
translate Y SearchResults "Rezultati pretrage"
translate Y OpeningTheDatabase "Otvaranje baze"
translate Y Database "Baza"
translate Y Filter "Filter"
translate Y Reset "Resetuj"
translate Y IgnoreCase "Ignoriši velika/mala slova"
translate Y noGames "ni jedna partija"
translate Y allGames "sve partije"
translate Y empty "prazno"
translate Y clipbase "clipbase"
translate Y score "skor"
translate Y Start "Poèetna"
translate Y StartPos "Poèetna pozicija"
translate Y Total "Ukupno"
translate Y readonly "samo za èitanje"
translate Y altered "izmenjeno"
translate Y tagsDescript "Dodatne oznake (npr. Annotator \"Anand\")"
translate Y prevTags "Koristi prethodne oznake"
translate Y value "Vrednost"

# Standard error messages:
translate Y ErrNotOpen "Ovo nije otvorena baza."
translate Y ErrReadOnly "Ova baza je samo za èitanje; nije moguæe izvršiti izmene."
translate Y ErrSearchInterrupted "Pretraga je prekinuta; rezultati su nepotpuni."

# Game information:
translate Y twin "udvojena"
translate Y deleted "izbrisana"
translate Y comment "komentar"
translate Y hidden "sakrivena"
translate Y LastMove "Poslednji potez"
translate Y NextMove "Sledeæi potez"
translate Y GameStart "Poèetak partije"
translate Y LineStart "Poèetak serije poteza"
translate Y GameEnd "Kraj partije"
translate Y LineEnd "Kraj serije poteza"

# Player information:
translate Y PInfoAll "svih partija"
translate Y PInfoFilter "filtriranih partija"
translate Y PInfoAgainst "Rezultati protiv"
translate Y PInfoMostWhite "Najèešæe igrana otvaranja sa belim figurama"
translate Y PInfoMostBlack "Najèešæe igrana otvaranja sa crnim figurama"
translate Y PInfoRating "Istorija rejtinga"
translate Y PInfoBio "Biografija"
translate Y PInfoEditRatings "Izmeni rejting"
translate Y PinfoEditName "Izmeni ime"
translate Y PinfoLookupName "Pronaði ime"

# Tablebase information:
translate Y Draw "Remi"
translate Y stalemate "pat"
translate Y checkmate "mat"
translate Y withAllMoves "sa svim potezima"
translate Y withAllButOneMove "sa svim osim jednog poteza"
translate Y with "sa"
translate Y only "samo"
translate Y lose "gubi"
translate Y loses "gubi"
translate Y allOthersLose "svi ostali gube"
translate Y matesIn "matira u"
translate Y longest "najduži"
translate Y WinningMoves "Pobednièki potezi"
translate Y DrawingMoves "Potezi za remi"
translate Y LosingMoves "Potezi koji vode do gubitka"
translate Y UnknownMoves "Potezi nepoznatog ishoda"

# Tip of the day:
translate Y Tip "Tip"
translate Y TipAtStartup "Tip pri pokretanju"

# Tree window menus:
menuText Y TreeFile "Fajl" 0
menuText Y TreeFileFillWithBase "Popuni keš bazom" 0 {Popuni keš fajl svim partijama iz tekuæe baze}
menuText Y TreeFileFillWithGame "Popuni keš partijom" 0 {Popuni keš fajl tekuæom partijom iz baze}
menuText Y TreeFileCacheInfo "Informacije o kešu" 0 {Prikaži informacije o upotrebi keša}
menuText Y TreeFileSave "Saèuvaj keš fajl" 0 {Saèuvaj cache (.stc) fajl stabla}
menuText Y TreeFileFill "Ispuni keš fajl" 0 {Ispuni keš fajl èestim pozicijama otvaranja}
menuText Y TreeFileBest "Lista najboljih partija" 0 {Prikaži listu najboljih partija stabla}
menuText Y TreeFileGraph "Prozor grafika" 0 {Prikaži grafik za ovu granu stabla}
menuText Y TreeFileCopy "Kopiraj tekst stabla u clipboard" 0 {Kopiraj statistike stabla u clipboard}
menuText Y TreeFileClose "Zatvori stablo varijanata" 0 {Zatvori prozor stabla}

menuText Y TreeMask "Maska" 0
menuText Y TreeMaskNew "Nova" 0 {Nova maska}
menuText Y TreeMaskOpen "Otvori" 0 {Otvori masku}
menuText Y TreeMaskOpenRecent "Otvori nedavno" 0 {Otvori nedavno korišæenu masku}
menuText Y TreeMaskSave "Saèuvaj" 0 {Saèuvaj masku}
menuText Y TreeMaskClose "Zatvori" 0 {Zatvori masku}
menuText Y TreeMaskFillWithLine "Popuni linijom" 0 {Popuni masku sa svim prethodnim potezima}
menuText Y TreeMaskFillWithGame "Popuni partijom" 0 {Popuni masku partijom}
menuText Y TreeMaskFillWithBase "Popuni bazom" 0 {Popuni masku svim partijama iz baze}
menuText Y TreeMaskInfo "Informacije" 0 {Prikaži statistiku za tekuæu masku}
menuText Y TreeMaskDisplay "Prikaži masku" 0 {Prikaži podatke maske u obliku stabla}
menuText Y TreeMaskSearch "Traži" 0 {Pretraži trenutnu masku}

menuText Y TreeSort "Sortiraj" 0
menuText Y TreeSortAlpha "Abecedno" 0
menuText Y TreeSortECO "E©O kôd" 0
menuText Y TreeSortFreq "Uèestalost" 0
menuText Y TreeSortScore "Skor" 0

menuText Y TreeOpt "Opcije" 0
menuText Y TreeOptSlowmode "Spori režim" 0 {Spori režim za osvežavanje stabla (visoka taènost)}
menuText Y TreeOptFastmode "Brzi režim" 0 {Brzi režim za osvežavanje (bez transpozicija poteza)}
menuText Y TreeOptFastAndSlowmode "Brzi i spori režim" 0 {Prvo brzi, zatim spori režim za osvežavanje}
# ====== TODO To be translated ======
menuText Y TreeOptMultiCore "Use Multiple CPU Cores" 4 {Search the tree on several CPU cores}
menuText Y TreeOptStartStop "Automatsko osvežavanje" 0 {Ukljuèi/iskljuèi automatsko osvežavanje prozora stabla}
menuText Y TreeOptLock "Zakljuèaj" 0 {Zakljuèaj/otkljuèaj stablo za tekuæu bazu}
menuText Y TreeOptTraining "Trening" 0 {Ukljuèi/iskljuèi režim treninga stabla}
menuText Y TreeOptShort "Kratki prikaz" 0 {Ne prikazuje ELO informacije}
menuText Y TreeOptAutosave "Automatski saèuvaj keš" 0 {Automatski saèuvaj keš fajl pri zatvaranju prozora stabla}
menuText Y TreeOptAutomask "Automatsko uèitavanje maske" 0 "Automatski uèitaj najnoviju masku sa otvorenim stablom."
menuText Y TreeOptCacheSize "Velièina keša" 0 {Podesi velièinu keša}
menuText Y TreeOptShowBar "Prikaži traku napretka" 0 "Prikaži traku napretka stabla."
menuText Y TreeOptShowFrame "Prikaži dugmad" 0 "Prikaži traku dugmadi stabla."
menuText Y TreeOptSmallFont "Koristi prilagoðeni font" 0 "Koristi font_Small za listu partija."
menuText Y TreeOptSortBest "Sortiraj najbolje partije" 0 "Sortiraj najbolje partije po ELO rejtingu"

menuText Y TreeHelp "Pomoæ" 0
menuText Y TreeHelpTree "Pomoæ za stablo" 0
menuText Y TreeHelpIndex "Sadržaj pomoæi" 0

translate Y SaveCache {Saèuvaj keš}
translate Y Training {Trening}
translate Y LockTree {Zakljuèaj stablo}
translate Y TreeLocked {zakljuèano}
translate Y TreeBest {Najbolje}
translate Y TreeBestGames {Najbolje partije stabla}
translate Y TreeAdjust {Prilagodi filter}
translate Y TreeTitleRow {    Potez      Uèestalost    Skor Remiji ProsekELO Perf ProsekGod ECO}
translate Y TreeTitleRowShort {    Potez      Uèestalost    Skor Remiji}
translate Y TreeTotal: {UKUPNO:}
translate Y DoYouWantToSaveFirst {Da li želite prvo da saèuvate}
translate Y AddToMask {Dodaj u masku}
translate Y RemoveFromMask {Ukloni iz maske}
translate Y AddThisMoveToMask {Dodaj potez u masku}
translate Y SearchMask {Traži u maski}
translate Y DisplayMask {Prikaži masku}
translate Y MaskShowMarkers {Prikaži markere}
translate Y MaskShowToolTips {Prikaži pomoæne informacije}
translate Y Nag {Nag kod}
translate Y Marker {Marker}
translate Y Include {Ukljuèi}
translate Y Exclude {Iskljuèi}
translate Y MainLine {Glavna linija}
translate Y Bookmark {Oznaka}
translate Y NewLine {Nova linija}
translate Y ToBeVerified {Za proveru}
translate Y ToTrain {Za trening}
translate Y Dubious {Sumnjivo}
translate Y ToRemove {Za uklanjanje}
translate Y NoMarker {Bez markera}
translate Y ColorMarker {Boja}
translate Y WhiteMark {Bela}
translate Y GreenMark {Zelena}
translate Y YellowMark {Žuta}
translate Y BlueMark {Plava}
translate Y RedMark {Crvena}
translate Y CommentMove {Komentar poteza}
translate Y CommentPosition {Komentar pozicije}
translate Y AddMoveToMaskFirst {Prvo dodaj potez u masku}
translate Y OpenAMaskFileFirst {Prvo otvori fajl maske}
translate Y positions {pozicije}
translate Y Moves {Potezi}

# Finder window:
menuText Y FinderFile "Fajl" 0
menuText Y FinderFileSubdirs "Pogledaj u poddirektorijumima" 0
menuText Y FinderFileClose "Zatvori Fajl tragaè" 0
menuText Y FinderSort "Sortiraj" 0
menuText Y FinderSortType "Tip" 0
menuText Y FinderSortSize "Velièina" 0
menuText Y FinderSortMod "Modifikovano" 0
menuText Y FinderSortName "Ime" 0
menuText Y FinderSortPath "Staza" 0
menuText Y FinderTypes "Tipovi" 0
menuText Y FinderTypesScid "Scid baze" 0
menuText Y FinderTypesOld "Scid baze u starom formatu" 0
menuText Y FinderTypesPGN "PGN fajlovi" 0
menuText Y FinderTypesEPD "EPD (book) fajlovi" 0
menuText Y FinderHelp "Pomoæ" 0
menuText Y FinderHelpFinder "Pomoæ - Fajl tragaè" 0
menuText Y FinderHelpIndex "Sadržaj pomoæi" 0
translate Y FileFinder {Fajl tragaè}
translate Y FinderDir {Direktorijum}
translate Y FinderDirs {Direktorijumi}
translate Y FinderFiles {Fajlovi}
translate Y FinderUpDir {Gore}
translate Y FinderCtxOpen {Otvori}
translate Y FinderCtxBackup {Rezerva}
translate Y FinderCtxCopy {Kopiraj}
translate Y FinderCtxMove {Premesti}
translate Y FinderCtxDelete {Obriši}
translate Y FinderCtxRename {Preimenuj}
translate Y FinderDirOpen {Otvori direktorijum}

# Player finder:
menuText Y PListFile "Fajl" 0
menuText Y PListFileUpdate "Osveži" 0
menuText Y PListFileClose "Zatvori tragaè igraèa" 0
menuText Y PListSort "Sortiraj" 0
menuText Y PListSortName "Ime" 0
menuText Y PListSortElo "Elo" 0
menuText Y PListSortGames "Partije" 0
menuText Y PListSortOldest "Najstariji" 0
menuText Y PListSortNewest "Najnoviji" 0
menuText Y PListSortCountry "Zemlja" 0

# Tournament finder:
menuText Y TmtFile "Fajl" 0
menuText Y TmtFileUpdate "Osveži" 0
menuText Y TmtFileClose "Zatvori tragaè turnira" 0
menuText Y TmtSort "Sortiraj" 0
menuText Y TmtSortDate "Datum" 0
menuText Y TmtSortPlayers "Igraèi" 0
menuText Y TmtSortGames "Partije" 0
menuText Y TmtSortElo "Elo" 0
menuText Y TmtSortSite "Mesto" 0
menuText Y TmtSortEvent "Turnir" 1
menuText Y TmtSortWinner "Pobednik" 0
translate Y TmtLimit "Ogranièenje liste"
translate Y TmtMeanElo "Najniži Elo"
translate Y TmtNone "Nije pronaðen odgovarajuæi turnir."

# Graph windows:
menuText Y GraphFile "Fajl" 0
menuText Y GraphFileColor "Saèuvaj kao Postscript u boji..." 25
menuText Y GraphFileGrey "Saèuvaj kao Postscript u sivim nivoima..." 25
# ====== TODO To be translated ======
menuText Y GraphFilePreview "Add Preview Scores to Game" 0
menuText Y GraphFileClose "Zatvori prozor" 6
menuText Y GraphOptions "Opcije" 0
menuText Y GraphOptionsWhite "Beli" 0
menuText Y GraphOptionsBlack "Crni" 0
menuText Y GraphOptionsDots "Prikaži taèkice" 0
menuText Y GraphOptionsSpelling "Elo iz fajla za pravopis" 0
menuText Y GraphOptionsBar "Istakni tekuæi potez" 0
# ====== TODO To be translated ======
menuText Y GraphOptionsLine "Score Current Move" 0
menuText Y GraphOptionsBoth "Oba" 1
menuText Y GraphOptionsPInfo "Informacije o igraèu" 0
translate Y GraphFilterTitle "Grafik filtera: uèestalost po 1000 partija"
translate Y GraphAbsFilterTitle "Grafik filtera: uèestalost partija"
translate Y ConfigureFilter "Konfiguriši X osu"
translate Y FilterEstimate "Procena"
translate Y TitleFilterGraph "Scid: Grafik filtera"

# Analysis window:
translate Y AddVariation {Dodaj varijantu}
translate Y AddAllVariations {Dodaj sve varijante}
translate Y AddMove {Dodaj potez}
translate Y Annotate {Dodaj napomenu}
translate Y ShowAnalysisBoard {Prikaži tablu za analizu}
translate Y ShowInfo {Prikaži informacije engine-a}
translate Y FinishGame {Završi partiju}
translate Y StopEngine {Zaustavi engine}
translate Y StartEngine {Pokreni engine}
translate Y ExcludeMove {Iskljuèi potez}
translate Y LockEngine {Zakljuèaj engine na tekuæoj poziciji}
translate Y AnalysisCommand {Komanda analize}
translate Y PreviousChoices {Prethodni izbori}
translate Y AnnotateTime {Postavi vreme izmeðu poteza u sekundama}
translate Y AnnotateWhich {Dodaj varijante}
translate Y AnnotateAll {Za poteze oba igraèa}
translate Y AnnotateAllMoves {Komentariši sve poteze}
translate Y AnnotateWhite {Samo za bele poteze}
translate Y AnnotateBlack {Samo za crne poteze}
translate Y AnnotateNotBest {Kada potez partije nije najbolji potez}
translate Y AnnotateBlundersOnly {Kada je potez oèigledna greška}
translate Y BlundersNotBest {Greške/Ne najbolji potez}
translate Y AnnotateTitle {Konfiguriši napomene}
translate Y AnnotateMissedMates {Propušteni ili kraæi matovi}
translate Y AnnotateEnd {Na kraju partije}
translate Y BlundersThreshold {Prag grešaka}
translate Y ScoreFormat {Format skora}
translate Y CutOff {Preseci}
translate Y LowPriority {Niska prioritet CPU-a}
translate Y LogEngines {Velièina log-a}
translate Y LogName {Dodaj ime}
translate Y MaxPly {Maksimalni broj polja}
translate Y ClickHereToSeeMoves {Klikni ovde da vidiš poteze}
translate Y ConfigureInformant {Konfiguriši Informant}
translate Y Informant!? {Zanimljiv potez}
translate Y Informant? {Loš potez}
translate Y Informant?? {Greška}
translate Y Informant?! {Sumnjiv potez}
translate Y Informant+= {Beli ima malu prednost}
translate Y Informant+/- {Beli ima umerenu prednost}
translate Y Informant+- {Beli ima odluèujuæu prednost}
translate Y Informant++- {Partija se smatra dobijenom}
translate Y Book {Otvaranja}
translate Y OtherBook {Drugi potezi}
translate Y Alphabetical {Abecedno}
translate Y TwoBooks {Dve knjige}

# Analysis Engine open dialog:
translate Y EngineList {Lista analiza engine-a}
translate Y EngineKey {Taster}
translate Y EngineType {Tip}
translate Y EngineName {Ime}
translate Y EngineCmd {Komanda}
translate Y EngineArgs {Parametri}
translate Y EngineDir {Direktorijum}
translate Y EngineElo {Elo}
translate Y EngineTime {Vreme}
translate Y EngineNew {Novo}
translate Y EngineEdit {Izmeni}
translate Y EngineRequired {Polja oznaèena podebljano su obavezna; ostala su opciona}

# Stats window menus:
menuText Y StatsFile "Fajl" 0
menuText Y StatsFilePrint "Štampaj u fajl..." 0
menuText Y StatsFileClose "Zatvori prozor" 0
menuText Y StatsOpt "Opcije" 0

# PGN window menus:
menuText Y PgnFile "Fajl" 0
menuText Y PgnFileCopy "Kopiraj partiju u clipboard" 0
menuText Y PgnFilePrint "Štampaj u fajl..." 1
menuText Y PgnFileClose "Zatvori PGN prozor" 0
menuText Y PgnOpt "Prikaz" 0
menuText Y PgnOptColor "Prikaz u boji" 0
menuText Y PgnOptShort "Kratko (3-linije) zaglavlje" 0
menuText Y PgnOptSymbols "Simbolièki komentari" 0
menuText Y PgnOptIndentC "Uvuci pasus kod komentara" 0
menuText Y PgnOptIndentV "Uvuci pasus kod varijanata" 16
menuText Y PgnOptColumn "Pasus stil (jedan potez po liniji)" 1
menuText Y PgnOptSpace "Blanko posle broja poteza" 0
menuText Y PgnOptStripMarks "Ukloni kodove obojenih polja/pilova" 1
menuText Y PgnOptChess "Šahovske figure" 6
menuText Y PgnOptScrollbar "Skrol bar" 6
menuText Y PgnOptBoldMainLine "Podebljani tekst za glavnu liniju poteza" 4
menuText Y PgnColor "Boje" 0
menuText Y PgnColorHeader "Zaglavlje..." 0
menuText Y PgnColorAnno "Napomene..." 0
menuText Y PgnColorComments "Komentari..." 0
menuText Y PgnColorVars "Varijante..." 0
menuText Y PgnColorBackground "Pozadina..." 0
menuText Y PgnColorMain "Glavna linija..." 0
menuText Y PgnColorCurrent "Pozadina trenutnog poteza..." 1
menuText Y PgnColorNextMove "Pozadina sledeæeg poteza..." 0
menuText Y PgnHelp "Pomoæ" 1
menuText Y PgnHelpPgn "PGN pomoæ" 0
menuText Y PgnHelpIndex "Sadržaj" 0
translate Y PgnWindowTitle {Notacija partije - partija %u}

# Crosstable window menus:
menuText Y CrosstabFile "Fajl" 0
menuText Y CrosstabFileText "Štampaj u Tekst fajl..." 10
menuText Y CrosstabFileHtml "Štampaj u HTML fajl..." 10
menuText Y CrosstabFileLaTeX "Štampaj u LaTeX fajl..." 10
menuText Y CrosstabFileClose "Zatvori tabelu turnira" 0
menuText Y CrosstabEdit "Izmena" 0
menuText Y CrosstabEditEvent "Turnir" 0
menuText Y CrosstabEditSite "Mesto" 0
menuText Y CrosstabEditDate "Datum" 0
menuText Y CrosstabOpt "Prikaz" 0
menuText Y CrosstabOptColorPlain "Obièan tekst" 0
menuText Y CrosstabOptColorHyper "Hipertekst" 0
menuText Y CrosstabOptTieWin "Tie-break po pobedama" 1
menuText Y CrosstabOptTieHead "Tie-break po meðusobnom susretu" 1
menuText Y CrosstabOptThreeWin "3 poena za pobedu" 1
menuText Y CrosstabOptAges "Starost u godinama" 10
menuText Y CrosstabOptNats "Narodnost" 0
menuText Y CrosstabOptTallies "Pobede/Ner. Rezultati/Remi" 0
menuText Y CrosstabOptRatings "Rejtinzi" 0
menuText Y CrosstabOptTitles "Titule" 0
menuText Y CrosstabOptBreaks "Tie-break rezultati" 4
menuText Y CrosstabOptDeleted "Ukljuèi izbrisane partije" 8
menuText Y CrosstabOptColors "Boje (samo za švajcarski sistem)" 0
menuText Y CrosstabOptColorRows "Oboji redove" 0
menuText Y CrosstabOptRowsColor "Boja reda" 0 {Podrazumevana boja reda tabele}
menuText Y CrosstabOptColumnNumbers "Numerisane kolone (samo za tabelu Svako sa svakim)" 2
menuText Y CrosstabOptGroup "Grupiši rezultate" 0
menuText Y CrosstabSort "Sortiraj" 0
menuText Y CrosstabSortName "Ime" 0
menuText Y CrosstabSortRating "Rejting" 0
menuText Y CrosstabSortScore "Rezultat" 0
menuText Y CrosstabSortCountry "Zemlja" 0
menuText Y CrosstabType "Format" 0
menuText Y CrosstabTypeAll "Svako sa svakim" 0
menuText Y CrosstabTypeSwiss "Švajcarski sistem" 0
menuText Y CrosstabTypeKnockout "Nokaut" 0
menuText Y CrosstabTypeAuto "Automatski" 0
menuText Y CrosstabHelp "Pomoæ" 0
menuText Y CrosstabHelpCross "Pomoæ - Tabela turnira" 0
menuText Y CrosstabHelpIndex "Sadržaj pomoæi" 0
translate Y SetFilter {Postavi filter}
translate Y AddToFilter {Dodaj u filter}
translate Y Swiss {Švajcarski sistem}
translate Y Category {Kategorija}

# Opening report window menus:
menuText Y OprepFile "Fajl" 0
menuText Y OprepFileText "Štampaj u Tekst fajl..." 10
menuText Y OprepFileHtml "Štampaj u HTML fajl..." 10
menuText Y OprepFileLaTeX "Štampaj u LaTeX fajl..." 10
menuText Y OprepFileOptions "Opcije..." 0
menuText Y OprepFileClose "Zatvori izveštaj o otvaranjima" 0
menuText Y OprepFavorites "Omiljeno" 1
menuText Y OprepFavoritesAdd "Dodaj izveštaj..." 0
menuText Y OprepFavoritesEdit "Izmeni omiljene izveštaje..." 0
menuText Y OprepFavoritesGenerate "Generiši izveštaje..." 0
menuText Y OprepHelp "Pomoæ" 0
menuText Y OprepHelpReport "Pomoæ - Izveštaj o otvaranjima" 0
menuText Y OprepHelpIndex "Sadržaj pomoæi" 0

# Header search:
translate Y HeaderSearch {Pretraga zaglavlja}
translate Y EndSideToMove {Igraè na potezu na kraju partije}
translate Y GamesWithNoECO {Partije bez E©O-a}
translate Y GameLength {Dužina partije}
translate Y FindGamesWith {Pronaði partije sa}
translate Y StdStart {standardnim poèetkom}
translate Y Promotions {sa promocijama}
translate Y UnderPromo {Ispod promocija}
translate Y Comments {Komentari}
translate Y Variations {Varijante}
translate Y Annotations {Napomene}
translate Y DeleteFlag {Znak za brisanje}
translate Y WhiteOpFlag {Otvaranje belog}
translate Y BlackOpFlag {Otvaranje crnog}
translate Y MiddlegameFlag {Središnjica}
translate Y EndgameFlag {Završnica}
translate Y NoveltyFlag {Novost}
translate Y PawnFlag {Pešaèka struktura}
translate Y TacticsFlag {Taktika}
translate Y QsideFlag {Igra na daminom krilu}
translate Y KsideFlag {Igra na kraljevom krilu}
translate Y BrilliancyFlag {Brilijantnost}
translate Y BlunderFlag {Greška}
translate Y UserFlag {Korisnik}
translate Y PgnContains {PGN sadrži tekst}
translate Y PreComment {Komentar pre partije}
translate Y PostComment {Komentar na kraju partije}

# Game list window:
translate Y GlistNumber {Broj}
translate Y GlistWhite {Beli}
translate Y GlistBlack {Crni}
translate Y GlistWElo {B-Elo}
translate Y GlistBElo {C-Elo}
translate Y GlistEvent {Turnir}
translate Y GlistSite {Mesto}
translate Y GlistRound {Kolo}
translate Y GlistDate {Datum}
translate Y GlistYear {Godina}
translate Y GlistEventDate {Datum turnira}
translate Y GlistResult {Rezultat}
translate Y GlistLength {Dužina}
translate Y GlistCountry {Zemlja}
translate Y GlistECO {E©O}
translate Y GlistOpening {Potezi}
translate Y GlistEndMaterial {Materijal na kraju partije}
translate Y GlistDeleted {Izbrisano}
translate Y GlistFlags {Oznaka}
translate Y GlistVariations {Varijante}
translate Y GlistComments {Komentari}
translate Y GlistAnnos {Napomene}
translate Y GlistStart {Poèetak}
translate Y GlistGameNumber {Broj partije}
translate Y GlistFindText {Naði tekst}
translate Y GlistMoveField {Potez}
translate Y GlistEditField {Konfiguriši}
translate Y GlistAddField {Dodaj}
translate Y GlistDeleteField {Obriši}
translate Y GlistColor {Boja}
translate Y GlistSort {Sortiraj bazu}
translate Y GlistRemoveThisGameFromFilter {Ukloni ovu partiju iz filtera}
translate Y GlistRemoveGameAndAboveFromFilter {Ukloni partiju (i sve iznad nje)}
translate Y GlistRemoveGameAndBelowFromFilter {Ukloni partiju (i sve ispod nje)}
translate Y GlistDeleteGame {(O)brisi/vrati ovu partiju}
translate Y GlistDeleteAllGames {Obriši sve partije u filteru}
translate Y GlistUndeleteAllGames {Vrati sve partije u filteru}
translate Y GlistAlignL {Poravnaj levo}
translate Y GlistAlignR {Poravnaj desno}
translate Y GlistAlignC {Poravnaj centrirano}
translate Y GlistMoveFieldUp {Pomeri gore}
translate Y GlistMoveFieldDown {Pomeri dole}
translate Y GlistMoveFieldFirst {Pomeri na poèetak}
translate Y GlistMoveFieldLast {Pomeri na kraj}
translate Y GlistMoveFieldN {Pomeri na N}

# Maintenance window:
translate Y DatabaseName {Ime baze:}
translate Y TypeIcon {Ikona}
translate Y NumOfGames {Partija:}
translate Y NumDeletedGames {Obrisanih partija:}
translate Y NumFilterGames {Partija u filteru:}
translate Y YearRange {Raspon godina:}
translate Y RatingRange {Raspon rejtinga:}
translate Y Description {Opis}
translate Y Flag {Oznaka}
translate Y CustomFlags {Prilagoðene oznake}
translate Y DeleteCurrent {Obriši tekuæu partiju}
translate Y DeleteFilter {Obriši filtrirane partije}
translate Y DeleteAll {Obriši sve partije}
translate Y UndeleteCurrent {Obnovi tekuæu partiju}
translate Y UndeleteFilter {Obnovi filtrirane partije}
translate Y UndeleteAll {Obnovi sve partije}
translate Y DeleteTwins {Obriši udvojene partije}
translate Y MarkCurrent {Oznaèi tekuæu partiju}
translate Y MarkFilter {Oznaèi filtrirane partije}
translate Y MarkAll {Oznaèi sve partije}
translate Y UnmarkCurrent {Ukloni oznaku sa tekuæe partije}
translate Y UnmarkFilter {Ukloni oznake sa filtriranih partija}
translate Y UnmarkAll {Ukloni sve oznake}
translate Y Spellchecking {Provera pravopisa}
translate Y MakeCorrections {Izvrši ispravke}
translate Y Ambiguous {Dvosmisleno}
translate Y Surnames {Prezimena}
translate Y Players {Igraèi}
translate Y Events {Turniri}
translate Y Sites {Mesta}
translate Y Rounds {Kola}
translate Y DatabaseOps {Operacije sa bazom}
translate Y ReclassifyGames {Klasifikuj partije po E©O}
translate Y CompactDatabase {Komprimuj bazu}
translate Y SortDatabase {Sortiraj bazu}
translate Y AddEloRatings {Dodaj Elo rejtinge}
translate Y AutoloadGame {Automatsko uèitavanje partije} 
translate Y StripTags {Ukloni dodatne oznake}
translate Y StripTag {Ukloni oznaku}
translate Y AddTag {Dodaj oznaku}
translate Y CheckGames {Proveri partije}
translate Y Cleaner {Èistaè}
translate Y CleanerHelp {
Scid Èistaè primenjuje sve odabrane akcije održavanja na tekuæu bazu.

Trenutna podešavanja u dijalozima za E©O klasifikaciju i brisanje udvojenih partija æe se primeniti ako ste izabrali te funkcije.
}
translate Y CleanerConfirm {
Kada je Èistaè pokrenut, ne može se prekinuti!

Ovo može potrajati na velikoj bazi, zavisno od izabranih funkcija i njihovih trenutnih podešavanja.

Da li ste sigurni da želite da zapoènete odabrane funkcije održavanja?
}
translate Y TwinCheckUndelete {Pritisnite "u" da obnovite obe partije}
translate Y TwinCheckprevPair {Prethodni par}
translate Y TwinChecknextPair {Sledeæi par}
translate Y TwinChecker {Scid: Provera udvojenih partija}
translate Y TwinCheckTournament {Partije na turniru:}
translate Y TwinCheckNoTwin {Nema udvojenih partija}
translate Y TwinCheckNoTwinfound {Nije pronaðena udvojena partija za ovu partiju.\nDa biste prikazali udvojene partije u ovom prozoru, prvo morate koristiti funkciju "Obriši udvojene partije...".}
translate Y TwinCheckTag {Deljenje oznaka...}
translate Y TwinCheckFound1 {Scid pronašao $result udvojenih partija}
translate Y TwinCheckFound2 { i postavio njihove oznake za brisanje}
translate Y TwinCheckNoDelete {U ovoj bazi nema partija za brisanje.}
translate Y TwinCriteria1 {Podešavanja za pronalaženje udvojenih partija mogu uzrokovati da partije sa sliènim potezima budu pogrešno oznaèene kao udvojene.}
translate Y TwinCriteria2 {Preporuèuje se da ako odaberete "Ne" za "isti potezi", odaberete "Da" za boje, turnir, mesto, kolo, godinu i mesec.\nDa li želite da nastavite i obrišete udvojene partije?}
translate Y TwinCriteria3 {Preporuèuje se da odaberete "Da" za bar dve od opcija "isti turnir", "isto kolo" i "ista godina".\nDa li želite da nastavite i obrišete udvojene partije?}
translate Y TwinCriteriaConfirm {Scid: Potvrda podešavanja za udvojene partije}
translate Y TwinChangeTag "Promeni sledeæe oznake partija:\n\n"
translate Y AllocRatingDescription "Ova komanda koristi trenutni fajl provere pravopisa da doda Elo rejtinge igraèima u bazi. Ako igraè nema trenutni rejting, ali je njegov rejting u vreme partije naveden u fajlu provere pravopisa, taj rejting æe biti dodat."
translate Y RatingOverride "Zameniti postojeæe nenulte rejtinge?"
translate Y AddRatings "Dodaj rejtinge za:"
translate Y AddedRatings {Scid je dodao $r Elo rejtinga u $g partija.}
translate Y NewSubmenu "Novi podmeni"

# Comment editor:
translate Y AnnotationSymbols {Simboli napomena:}
translate Y Comment {Komentari:}
translate Y InsertMark {Ubaci oznaku}
translate Y InsertMarkHelp {
Ubaci/ukloni oznaku: Izaberite boju, tip, polje.
Ubaci/ukloni strelicu: Desni klik na dve polja.
}

# Dugmad za NAG u ureðivaèu komentara:
translate Y GoodMove {Dobar potez}
translate Y PoorMove {Loš potez}
translate Y ExcellentMove {Odlièan potez}
translate Y Blunder {Greška}
translate Y InterestingMove {Zanimljiv potez}
translate Y DubiousMove {Sumnjiv potez}
translate Y WhiteDecisiveAdvantage {Beli ima odluèujuæu prednost}
translate Y BlackDecisiveAdvantage {Crni ima odluèujuæu prednost}
translate Y WhiteClearAdvantage {Beli ima jasnu prednost}
translate Y BlackClearAdvantage {Crni ima jasnu prednost}
translate Y WhiteSlightAdvantage {Beli ima malu prednost}
translate Y BlackSlightAdvantage {Crni ima malu prednost}
translate Y Equality {Ravnopravno}
translate Y Unclear {Nejasno}
translate Y Diagram {Dijagram}

# Board search:
translate Y BoardSearch {Pretraga po poziciji}
translate Y FilterOperation {Operacije na tekuæem filteru:}
translate Y FilterAnd {I (Ogranièi filter)}
translate Y FilterOr {ILI (Dodaj u filter)}
translate Y FilterIgnore {IGNORIŠI (Resetuj filter)}
translate Y SearchType {Tip pretrage:}
translate Y SearchBoardExact {Ista pozicija (sve figure na istim poljima)}
translate Y SearchBoardPawns {Pešaci (isti materijal, svi pešaci na istim poljima)}
translate Y SearchBoardFiles {Linije (isti materijal, svi pešaci na istim linijama)}
translate Y SearchBoardAny {Materijal (isti materijal, pešaci i figure bilo gde)}
translate Y SearchInRefDatabase {Pretraži u bazi}
translate Y LookInVars {Pogledaj u varijantama}
translate Y CQLLabel {CQL skript}
translate Y CQLComments {Dozvoli komentare}
translate Y CQLStrip {Ukloni komentare iz partije}

# Material search:
translate Y MaterialSearch {Pretraga po materijalu}
translate Y Material {Materijal}
translate Y Patterns {Pozicije}
translate Y Zero {Ništa}
translate Y Any {Bilo koje}
translate Y CurrentBoard {Trenutna pozicija}
translate Y CommonEndings {Tipiène završnice}
translate Y CommonPatterns {Tipiène pozicije}
translate Y MaterialDiff {Razlika u materijalu}
translate Y squares {Polja}
translate Y SameColor {Ista boja}
translate Y OppColor {Suprotna boja}
translate Y Either {Oba}
translate Y MoveNumberRange {Raspon broja poteza}
translate Y MatchForAtLeast {Naði za najmanje}
translate Y HalfMoves {Polupoteza}

# Tipiène završnice u pretrazi po materijalu:
translate Y EndingPawns {Pešaèke završnice}
translate Y EndingRookVsPawns {Top protiv pešaka}
translate Y EndingRookPawnVsRook {Top i 1 pešak protiv topa}
translate Y EndingRookPawnsVsRook {Top i pešaci protiv topa}
translate Y EndingRooks {Završnice Top protiv Top}
translate Y EndingRooksPassedA {Završnice Top protiv Top sa prošlim a-pešakom}
translate Y EndingRooksDouble {Dvostruke završnice Topa}
translate Y EndingBishops {Završnice Lovac protiv Lovac}
translate Y EndingBishopVsKnight {Završnice Lovac protiv Skakaè}
translate Y EndingKnights {Završnice Skakaè protiv Skakaè}
translate Y EndingQueens {Završnice Dama protiv Dame}
translate Y EndingQueenPawnVsQueen {Dama i 1 pešak protiv Dame}
translate Y BishopPairVsKnightPair {Dva lovca protiv dva skakaèa u srednjoj igri}

# Tipiène pozicije u pretrazi po materijalu:
translate Y PatternWhiteIQP {Beli izolovani pešak}
translate Y PatternWhiteIQPBreakE6 {Beli izolovani pešak: d4-d5 probija e6}
translate Y PatternWhiteIQPBreakC6 {Beli izolovani pešak: d4-d5 probija c6}
translate Y PatternBlackIQP {Crni izolovani pešak}
translate Y PatternWhiteBlackIQP {Beli IQP protiv crnog IQP}
translate Y PatternCoupleC3D4 {Beli c3+d4 izolovana pešaèka dupla}
translate Y PatternHangingC5D5 {Crni visi pešaci na c5 i d5}
translate Y PatternMaroczy {Maroczy centar (pešaci na c4 i e4)}
translate Y PatternRookSacC3 {Žrtvovanje topa na c3}
translate Y PatternKc1Kg8 {O-O-O protiv O-O (Kc1 protiv Kg8)}
translate Y PatternKg1Kc8 {O-O protiv O-O-O (Kg1 protiv Kc8)}
translate Y PatternLightFian {Fianèeti svetlih polja (Lovac-g2 protiv Lovac-b7)}
translate Y PatternDarkFian {Fianèeti tamnih polja (Lovac-b2 protiv Lovac-g7)}
translate Y PatternFourFian {Èetiri fianèeta (Lovci na b2,g2,b7,g7)}

# Game saving:
translate Y Today {Danas}
translate Y ClassifyGame {Klasifikuj partiju}

# Setup position:
translate Y EmptyBoard {Isprazni tablu}
translate Y InitialBoard {Poèetna pozicija}
translate Y SideToMove {Na potezu je}
translate Y MoveNumber {Broj poteza}
translate Y Castling {Rokada}
translate Y EnPassantFile {En Passant linija}
translate Y ClearFen {Oèisti FEN}
translate Y PasteFen {Prenesi FEN}
translate Y SaveAndContinue {Saèuvaj i nastavi}
translate Y DiscardChangesAndContinue {Odbaci promene i nastavi}
translate Y GoBack {Vrati se nazad}

# Replace move dialog:
translate Y ReplaceMove {Zameni potez}
translate Y AddNewVar {Dodaj novu varijantu}
translate Y NewMainLine {Nova glavna linija}
translate Y ReplaceMoveMessage {Ovde veæ postoji potez.

Možete ga zameniti, odbacujuæi sve poteze posle njega, ili dodajte svoj potez kao novu varijantu.

(Možete izbeæi pojavljivanje ove poruke ubuduæe ako iskljuèite opciju "Pitaj pre zamene poteza" u meniju Opcije:Potezi.)}

# Make database read-only dialog:
translate Y ReadOnlyDialog {Ako omoguæite samo èitanje iz ove baze, neæe biti dozvoljene izmene.
Nijedna partija neæe biti saèuvana ili zamenjena, i nijedan znak za brisanje ne može biti izmenjen.
Bilo kakvo sortiranje ili rezultati E©O klasifikacije æe biti privremeni.

Možete lako ponovo omoguæiti pisanje po bazi, ako je zatvorite i ponovo otvorite.

Da li zaista želite da ova baza bude samo za èitanje?}

# Exit dialog:
translate Y ExitDialog {Da li zaista želite da napustite Scid?}
translate Y ClearGameDialog {Ova partija je izmenjena.\nDa li želite da je saèuvate?}
translate Y ExitUnsaved {Sledeæe baze sadrže nespremljene izmene partija. Ako sada izaðete, te izmene æe biti izgubljene.}
translate Y ChangeTextDialog {Omoguæavanje boje teksta globalno zahteva restart.}
translate Y ChangeDockDialog {Promena Dock opcija zahteva restart.}
translate Y FontSelectDialog {Ovo je neki primer teksta\nDvokliknite iznad da promenite font, zatim izaberite 'ok'\nAaBbCcDdEeFfGgHhIiJjKkLlMm 0123456789. +=-}
translate Y ExitNow {Da li želite da izaðete sada?}

# Import window:
translate Y PasteCurrentGame {Prenesi tekuæu partiju}
translate Y ImportHelp1 {Unesi ili prenesi partiju u PGN formatu u gornji okvir.}
translate Y ImportHelp2 {Bilo koja greška u uvozu partije æe biti prikazana ovde.}
translate Y OverwriteExistingMoves {Da li želite da zamenite postojeæe poteze?}

# ECO Browser:
translate Y ECOAllSections {Sve E©O sekcije}
translate Y ECOSection {E©O sekcija}
translate Y ECOSummary {Pregled za}
translate Y ECOFrequency {Uèestalost subkodova za}

# Opening Report:
translate Y OprepTitle {Izveštaj o otvaranjima}
translate Y OprepReport {Izveštaj}
translate Y OprepGenerated {Generisano}
translate Y OprepStatsHist {Statistika i istorija}
translate Y OprepStats {Statistika}
translate Y OprepStatAll {Sve partije iz izveštaja}
translate Y OprepStatBoth {Oba sa rejtingom}
translate Y OprepStatSince {Od}
translate Y OprepOldest {Najstarije partije}
translate Y OprepNewest {Najnovije partije}
translate Y OprepPopular {Trenutna popularnost}
translate Y OprepFreqAll {Uèestalost za sve godine:}
translate Y OprepFreq1 {Tokom 1 poslednje godine:}
translate Y OprepFreq5 {Tokom 5 poslednjih godina:}
translate Y OprepFreq10 {Tokom 10 poslednjih godina:}
translate Y OprepEvery {jednom svakih %u partija}
translate Y OprepUp {više za %u%s tokom svih godina}
translate Y OprepDown {manje za %u%s tokom svih godina}
translate Y OprepSame {bez promena tokom svih godina}
translate Y OprepMostFrequent {Najèešæi igraèi}
translate Y OprepMostFrequentOpponents {Najèešæi protivnici}
translate Y OprepRatingsPerf {Rejtinzi i umešnost}
translate Y OprepAvgPerf {Proseèni rejtinzi i umešnost}
translate Y OprepWRating {Rejting belog}
translate Y OprepBRating {Rejting crnog}
translate Y OprepWPerf {Umešnost belog}
translate Y OprepBPerf {Umešnost crnog}
translate Y OprepHighRating {Partije sa najveæim proseènim rejtingom}
translate Y OprepTrends {Rezultatski trendovi}
translate Y OprepResults {Dužine rezultata i uèestalosti}
translate Y OprepLength {Dužina partije}
translate Y OprepFrequency {Uèestalost}
translate Y OprepWWins {Pobede belog:}
translate Y OprepBWins {Pobede crnog:}
translate Y OprepDraws {Remiji:}
translate Y OprepWholeDB {Cela baza}
translate Y OprepShortest {Najkraæa pobeda}
translate Y OprepMovesThemes {Potezi i teme}
translate Y OprepMoveOrders {Redosledi poteza koji su dosegli poziciju iz izveštaja}
translate Y OprepMoveOrdersOne {Samo je jedan redosled poteza koji je dostigao ovu poziciju:}
translate Y OprepMoveOrdersAll {Bilo je %u redosleda poteza koji su dostigli ovu poziciju:}
translate Y OprepMoveOrdersMany {Bilo je %u redosleda poteza koji su dostigli ovu poziciju. Prvih %u su:}
translate Y OprepMovesFrom {Potezi iz pozicije iz izveštaja}
translate Y OprepMostFrequentEcoCodes {Najèešæi E©O kodovi}
translate Y OprepThemes {Pozicione teme}
translate Y OprepThemeDescription {Uèestalost tema u prvih %u poteza svake partije}
translate Y OprepThemeSameCastling {Rokade sa iste strane}
translate Y OprepThemeOppCastling {Rokade sa suprotne strane}
translate Y OprepThemeNoCastling {Obe strane bez rokada}
translate Y OprepThemeKPawnStorm {Prodor pešaka na kraljevom krilu}
translate Y OprepThemeQueenswap {Zamena dama}
translate Y OprepThemeWIQP {Beli izolovani dama pešak}
translate Y OprepThemeBIQP {Crni izolovani dama pešak}
translate Y OprepThemeWP567 {Beli pešak na 5., 6. ili 7. redu}
translate Y OprepThemeBP234 {Crni pešak na 2., 3. ili 4. redu}
translate Y OprepThemeOpenCDE {Otvorena c, d ili e linija}
translate Y OprepTheme1BishopPair {Jedna strana ima par lovaca}
translate Y OprepEndgames {Završnice}
translate Y OprepReportGames {Partije iz izveštaja}
translate Y OprepAllGames {Sve partije}
translate Y OprepEndClass {Materijal na kraju svake partije}
translate Y OprepTheoryTable {Tabela teorije}
translate Y OprepTableComment {Generisano iz %u najbolje rejtiranih partija.}
translate Y OprepExtraMoves {Posebno notirani potezi u tabeli teorije}
translate Y OprepMaxTableGames {Maksimum partija u tabeli teorije}
translate Y OprepMaxGames {Maksimalan broj partija za statistiku}
translate Y OprepViewHTML {Prikaži HTML}
translate Y OprepViewLaTeX {Prikaži LaTeX}

# Player Report:
translate Y PReportTitle {Izveštaj o igraèu}
translate Y PReportColorWhite {sa belim figurama}
translate Y PReportColorBlack {sa crnim figurama}
translate Y PReportBeginning {Poèetak sa}
translate Y PReportMoves {posle %s}
translate Y PReportOpenings {Otvaranja}
translate Y PReportClipbase {Oèisti privremenu bazu i kopiraj odgovarajuæe partije u nju}

# Piece Tracker window:
translate Y TrackerSelectSingle {Levi taster miša bira ovu figuru.}
translate Y TrackerSelectPair {Levi taster miša bira ovu figuru; desni taster bira i njen par.}
translate Y TrackerSelectPawn {Levi taster miša bira ovog pešaka; desni taster bira svih 8 pešaka.}
translate Y TrackerStat {Statistika}
translate Y TrackerGames {% partija sa potezom na polje}
translate Y TrackerTime {% vremena na svakom polju}
translate Y TrackerMoves {Potezi}
translate Y TrackerMovesStart {Unesite broj poteza od kojeg praæenje poèinje.}
translate Y TrackerMovesStop {Unesite broj poteza na kojem praæenje prestaje.}

# Game selection dialogs:
translate Y SelectAllGames {Sve partije u bazi}
translate Y SelectFilterGames {Samo partije u filteru}
translate Y SelectTournamentGames {Samo partije u tekuæem turniru}
translate Y SelectOlderGames {Samo starije partije}

# Delete Twins window:
translate Y TwinsNote {Da bi bile udvojene, dve partije moraju da imaju bar iste dva igraèa i kriterijume koje niže možete podesiti. Kada se par udvojenih partija otkrije, kraæa partija se briše. Savet: Najbolje je proveriti pravopis u bazi pre brisanja udvojenih partija, jer to unapreðuje detekciju.}
translate Y TwinsCriteria {Kriterijumi: Udvojene partije moraju imati...}
translate Y TwinsWhich {Koje partije da proverim?}
translate Y TwinsColors {Iste boje igraèa?}
translate Y TwinsEvent {Isti turnir?}
translate Y TwinsSite {Isto mesto?}
translate Y TwinsRound {Isto kolo?}
translate Y TwinsYear {Ista godina?}
translate Y TwinsMonth {Isti mesec?}
translate Y TwinsDay {Isti dan?}
translate Y TwinsResult {Isti rezultat?}
translate Y TwinsECO {Isti E©O kôd?}
translate Y TwinsMoves {Isti potezi?}
translate Y TwinsPlayers {Uporeðivanje imena igraèa:}
translate Y TwinsPlayersExact {Potpuna podudarnost}
translate Y TwinsPlayersPrefix {Samo prva 4 slova}
translate Y TwinsWhen {Kod brisanja udvojenih partija}
translate Y TwinsSkipShort {Ignoriši sve partije kraæe od 5 poteza?}
translate Y TwinsUndelete {Obnovi prvo sve partije?}
translate Y TwinsSetFilter {Postavi filter na sve obrisane udvojene partije?}
translate Y TwinsComments {Uvek saèuvaj partije sa komentarima?}
translate Y TwinsVars {Uvek saèuvaj partije sa varijantama?}
translate Y TwinsDeleteWhich {Koju partiju obrisati?}
translate Y TwinsDeleteShorter {Kraæa partija}
translate Y TwinsDeleteOlder {Partija sa manjim brojem}
translate Y TwinsDeleteNewer {Partija sa veæim brojem}
translate Y TwinsDelete {Obriši partije}

# Name editor window:
translate Y NameEditType {Tip imena za izmenu}
translate Y NameEditSelect {Partije za izmenu}
translate Y NameEditReplace {Zameni}
translate Y NameEditWith {sa}
translate Y NameEditMatches {Podudara se: Pritisni Ctrl+1 do Ctrl+9 za izbor}
translate Y MatchPattern {Šablon podudaranja}

# Classify window:
translate Y Classify {Klasifikuj}
translate Y ClassifyWhich {Klasifikuj partije po E©O}
translate Y ClassifyAll {Sve partije (prebriši stare E©O kôdove)}
translate Y ClassifyYear {Sve partije igrane poslednje godine}
translate Y ClassifyMonth {Sve partije igrane poslednjeg meseca}
translate Y ClassifyNew {Samo partije koje su još bez E©O kôda}
translate Y ClassifyCodes {Koristi E©O kôdove}
translate Y ClassifyBasic {Samo osnovne kôdove ("B12", ...)}
translate Y ClassifyExtended {Prošireni Scid ("B12j", ...)}

# Compaction:
translate Y NameFile {Fajl imena}
translate Y GameFile {Fajl partija}
translate Y Names {Imena}
translate Y Unused {Nekorišæena}
translate Y SizeKb {Velièina (kb)}
translate Y CurrentState {Trenutno stanje}
translate Y AfterCompaction {Posle komprimacije}
translate Y CompactNames {Komprimuj fajl imena}
translate Y CompactGames {Komprimuj fajl partija}
translate Y NoUnusedNames {Nema nekorišæenih imena, fajl imena je veæ potpuno komprimovan.}
translate Y NoUnusedGames {Fajl partija je veæ potpuno komprimovan.}
translate Y NameFileCompacted {Fajl imena za "[file tail [sc_base filename]]" je komprimovan.}
translate Y GameFileCompacted {Fajl partija za "[file tail [sc_base filename]]" je komprimovan.}

# Sorting:
translate Y SortCriteria {Kriterijumi}
translate Y AddCriteria {Dodaj kriterijum}
translate Y CommonSorts {Uobièajena sortiranja}
translate Y Sort {Sortiraj}

# Exporting:
translate Y AddToExistingFile {Dodaj partije u postojeæi fajl?}
translate Y ExportComments {Izvezi komentare?}
translate Y ExportVariations {Izvezi varijante?}
translate Y IndentComments {Uvuci pasus kod komentara?}
translate Y IndentVariations {Uvuci pasus kod varijanata?}
translate Y InsertNewlines {Ubaci novi red na svakih 80 karaktera?}
translate Y ExportColumnStyle {Pasus stil (jedan potez po liniji)?}
translate Y ExportSymbolStyle {Stil simbolièkih napomena:}
translate Y ExportStripMarks {Ukloni kodove polja/strelica iz komentara?}
translate Y ExportFlags {Izvezi Scid oznake}

# Goto game/move dialogs:
translate Y LoadGameNumber {Unesi broj partije za uèitavanje:}
translate Y GotoMoveNumber {Idi na potez broj:}

# Copy games dialog:
translate Y CopyGames {Kopiraj partije}
translate Y CopyConfirm {
 Da li zaista želite da kopirate
 [::utils::thousands $nGamesToCopy] filtriranih partija
 iz baze "$fromName"
 u bazu "$targetName"?
}
translate Y CopyErr {Ne mogu da kopiram partije}
translate Y CopyErrSource {izvorna baza}
translate Y CopyErrTarget {odredišna baza}
translate Y CopyErrNoGames {nema partija u filteru}
translate Y CopyErrReadOnly {je samo za èitanje}
translate Y CopyErrNotOpen {nije otvorena}

# Colors:
translate Y LightSquares {Svetla polja}
translate Y DarkSquares {Tamna polja}
translate Y SelectedSquares {Izabrana polja}
translate Y SelectedOutline {Kontura izabranog polja}
translate Y SuggestedSquares {Predložena polja}
translate Y Grid {Mreža}
translate Y Previous {Prethodni}
translate Y WhitePieces {Bele figure}
translate Y BlackPieces {Crne figure}
translate Y WhiteBorder {Ivica belih figura}
translate Y BlackBorder {Ivica crnih figura}
translate Y ArrowMain {Glavna strelica}
translate Y ArrowVar {Varijantne strelice}
translate Y ShowArrows {Prikaži strelice}

# Novelty window:
translate Y FindNovelty {Pronaði novost}
translate Y Novelty {Novost}
translate Y NoveltyInterrupt {Praæenje novosti prekinuto}
translate Y NoveltyNone {Nijedna novost nije pronaðena u ovoj partiji}
translate Y NoveltyHelp {
Scid æe pronaæi prvi potez tekuæe partije koji dostiže poziciju koja nije pronaðena u izabranoj bazi ili u E©O knjizi otvaranja.
}

# Sounds configuration:
translate Y SoundsFolder {Folder sa zvucima}
translate Y SoundsAnnounceOptions {Opcije za najavu poteza}
translate Y SoundsAnnounceNew {Najavi nove poteze èim se odigraju}
translate Y SoundsAnnounceForward {Najavi poteze pri pomeranju napred za jedan potez}
translate Y SoundsAnnounceBack {Najavi poteze pri vraæanju ili pomeranju nazad za jedan potez}

# Upgrading databases:
translate Y Upgrading {Unapreðivanje}
translate Y ConfirmOpenNew {
Ovo je baza u starom formatu (Scid 2), koju Scid 3 ne može da otvori, veæ je kreirana verzija u novom formatu (Scid 3).

Da li želite da otvorite verziju baze u novom formatu?
}
translate Y ConfirmUpgrade {
Ovo je baza u starom formatu (Scid 2). Verzija baze u novom formatu mora biti kreirana pre korišæenja u Scid-u 3.

Unapreðivanjem æe se kreirati nova verzija baze; originalni fajlovi neæe biti uklonjeni.

Ovo može potrajati, ali se radi samo jednom. Možete prekinuti ako bude predugo trajalo.

Da li želite da sada unapredite ovu bazu?
}

# Recent files options:
translate Y RecentFilesMenu {Broj poslednjih fajlova u meniju Fajl}
translate Y RecentFilesExtra {Broj poslednjih fajlova u dodatnom podmeniju}

# My Player Names options:
translate Y MyPlayerNamesDescription {
Unesite listu omiljenih imena igraèa ispod, jedno ime po liniji. Dozvoljeni su džokeri (npr. "?" za bilo koje jedno slovo, "*" za bilo koji niz karaktera).

Svaki put kada se uèita partija sa igraèem sa liste, tabla u glavnom prozoru æe se rotirati ako je potrebno da se partija vidi iz perspektive tog igraèa.
}
translate Y showblunderexists {prikaži da postoji greška}
translate Y showblundervalue {prikaži vrednost greške}
translate Y showscore {prikaži rezultat}
translate Y coachgame {trenerska partija}
translate Y configurecoachgame {konfiguriši trenersku partiju}
translate Y configuregame {konfiguriši partiju}
translate Y Phalanxengine {Phalanx motor}
translate Y Coachengine {Trenerski motor}
translate Y difficulty {težina}
translate Y hard {teško}
translate Y easy {lako}
translate Y Playwith {Igraj sa}
translate Y white {belim}
translate Y black {crnim}
translate Y both {obojicom}
translate Y Play {Igraj}
translate Y Noblunder {Bez greške}
translate Y blunder {Greška}
translate Y Noinfo {-- Nema informacija --}
translate Y moveblunderthreshold {potez je greška ako gubitak je veæi od}
translate Y limitanalysis {ogranièi vreme analize motora}
translate Y seconds {sekundi}
translate Y Abort {Prekini}
translate Y Quit {Izlaz}
translate Y Resume {Nastavi}
translate Y Restart {Ponovo}
translate Y OutOfOpening {Van otvaranja}
translate Y NotFollowedLine {Niste pratili liniju}
translate Y DoYouWantContinue {Da li želite da nastavite?}
translate Y CoachIsWatching {Trener posmatra}
translate Y Ponder {Permanentno razmišljanje}
translate Y LimitELO {Ogranièi ELO snagu}
translate Y DubiousMovePlayedTakeBack {Sumnjiv potez, da li želite da vratite?}
translate Y WeakMovePlayedTakeBack {Slab potez, da li želite da vratite?}
translate Y BadMovePlayedTakeBack {Loš potez, da li želite da vratite?}
translate Y Iresign {Predajem se}
translate Y yourmoveisnotgood {Vaš potez nije dobar}
translate Y EndOfVar {Kraj varijante}
translate Y Openingtrainer {Trener otvaranja}
translate Y DisplayCM {Prikaži kandidatske poteze}
translate Y DisplayCMValue {Prikaži vrednost kandidatskih poteza}
translate Y DisplayOpeningStats {Prikaži statistiku}
translate Y ShowReport {Prikaži izveštaj}
translate Y NumberOfGoodMovesPlayed {broj dobrih poteza}
translate Y NumberOfDubiousMovesPlayed {broj sumnjivih poteza}
translate Y NumberOfTimesPositionEncountered {broj puta pozicija odigrana}
translate Y PlayerBestMove {Dozvoli samo najbolje poteze}
translate Y OpponentBestMove {Protivnik igra najbolje poteze}
# ====== TODO To be translated ======
translate Y RemoveGames {Don't repeat a game}
translate Y OnlyFlaggedLines {Samo oznaèene linije}
translate Y resetStats {Resetuj statistiku}
translate Y Movesloaded {Uèitani potezi}
translate Y PositionsNotPlayed {Pozicije koje nisu odigrane}
translate Y PositionsPlayed {Pozicije koje su odigrane}
translate Y Success {Uspeh}
translate Y DubiousMoves {Sumnjivi potezi}
# ====== TODO To be translated ======
translate Y NumberOfMovesPlayedNotInRepertoire {moves played not in repertoire}
# ====== TODO To be translated ======
translate Y Repertoiretrainingconfiguration {Opening Trainer Configuration}
# ====== TODO To be translated ======
translate Y Loadingrepertoire {Loading repertoire}
# ====== TODO To be translated ======
translate Y Repertoirenotfound {Repertoire not found}
# ====== TODO To be translated ======
translate Y NoRepertoireFound {No Repertoire Database found}
# ====== TODO To be translated ======
translate Y Movenotinrepertoire {Move not in repertoire}
# ====== TODO To be translated ======
translate Y PositionsInRepertoire {Positions in Repertoire}
# ====== TODO To be translated ======
translate Y OutOfRepertoire {Out of Repertoire}
# ====== TODO To be translated ======
translate Y NextGame {Next Game}
translate Y ConfigureTactics {Konfiguriši taktike}
translate Y ResetScores {Resetuj rezultate}
translate Y LoadingBase {Uèitavanje baze}
translate Y Tactics {Taktike}
translate Y ShowSolution {Prikaži rešenje}
translate Y Next {Sledeæe}
translate Y ResettingScore {Resetovanje rezultata}
translate Y LoadingGame {Uèitavanje partije}
translate Y MateFound {Mat pronaðen}
translate Y BestSolutionNotFound {Najbolje rešenje NIJE pronaðeno!}
translate Y MateNotFound {Mat nije pronaðen}
translate Y ShorterMateExists {Postoji kraæi mat}
translate Y ScorePlayed {Odigran rezultat}
translate Y Expected {oèekivano}
translate Y ChooseTrainingBase {Izaberite bazu za trening}
translate Y Thinking {Razmišljanje}
translate Y AnalyzeDone {Analiza završena}
translate Y WinWonGame {Pobednièki rezultat}
translate Y Lines {Linije}
translate Y ConfigureUCIengine {Konfiguriši UCI motor}
translate Y SpecificOpening {Specifièno otvaranje}
translate Y ImportECO {Uvezi E©O}
translate Y StartNewGame {Poèni novu partiju}
translate Y FixedLevel {Fiksni nivo}
translate Y Opening {Otvaranje}
translate Y RandomLevel {Nasumièni nivo}
translate Y StartFromCurrentPosition {Poèni sa trenutne pozicije}
translate Y FixedDepth {Fiksna dubina}
translate Y Nodes {Èvorovi}
translate Y Depth {Dubina}
translate Y Time {Vreme}
translate Y Auto {Auto}
translate Y Score {Rezultat}
translate Y {Score Combo} {Rezultat Kombinacija}
translate Y {Time Combo} {Vreme Kombinacija}
translate Y SecondsPerMove {Sekundi po potezu}
translate Y DepthPerMove {Dubina po potezu}
translate Y MoveControl {Kontrola poteza}
translate Y TimeLabel {Vreme po potezu}
translate Y AddVars {Dodaj varijante}
translate Y AddScores {Dodaj rezultat}
translate Y Engine {Motor}
translate Y TimeMode {Naèin vremena}
translate Y TimeBonus {Vreme + bonus}
translate Y TimeMin {min}
translate Y TimeSec {sek}
translate Y AllExercisesDone {Svi zadaci završeni}
translate Y MoveOutOfBook {Potez van knjige}
translate Y LastBookMove {Poslednji potez iz knjige}
translate Y AnnotateSeveralGames {Oznaèi više partija}
translate Y FindOpeningErrors {Pronaði greške u otvaranju}
translate Y MarkTacticalExercises {Oznaèi taktièke zadatke}
translate Y UseBook {Koristi knjigu otvaranja}
translate Y MultiPV {Više varijacija}
translate Y Hash {Hash memorija}
translate Y OwnBook {Koristi motornu knjigu otvaranja}
translate Y BookFile {Knjiga otvaranja}
translate Y AnnotateVariations {Oznaèi varijante}
# ====== TODO To be translated ======
translate Y AnnotateBackward {Analyze backwards}
# ====== TODO To be translated ======
translate Y AnnotateQuickPasses {Right-click: quick score graph}
translate Y ShortAnnotations {Kratke napomene}
translate Y addAnnotatorTag {Dodaj oznaku autora napomene}
translate Y AddScoreToShortAnnotations {Dodaj rezultat kratkim napomenama}
translate Y Export {Izvoz}
translate Y BookPartiallyLoaded {Knjiga delimièno uèitana}
translate Y AddLine {Dodaj liniju}
translate Y RemLine {Ukloni liniju}
translate Y Calvar {Proraèun varijacija}
translate Y ConfigureCalvar {Konfiguracija}
translate Y Reti {Reti}
translate Y English {Engleski}
translate Y d4Nf6Miscellaneous {1.d4 Nf6 Ostalo}
translate Y Trompowsky {Trompovski}
translate Y Budapest {Budimpešta}
translate Y OldIndian {Stara Indijska}
translate Y BenkoGambit {Benko gambit}
translate Y ModernBenoni {Moderni Benoni}
translate Y DutchDefence {Holandska odbrana}
translate Y Scandinavian {Skandinavska odbrana}
translate Y AlekhineDefence {Alehinova odbrana}
translate Y Pirc {Pirc}
translate Y CaroKann {Caro-Kan}
translate Y CaroKannAdvance {Caro-Kan napredovanje}
translate Y Sicilian {Sicilijanska}
translate Y SicilianAlapin {Sicilijanska Alapin}
translate Y SicilianClosed {Sicilijanska zatvorena}
translate Y SicilianRauzer {Sicilijanska Rauzer}
translate Y SicilianDragon {Sicilijanska Zmaj}
translate Y SicilianScheveningen {Sicilijanska Ševeningen}
translate Y SicilianNajdorf {Sicilijanska Najdorf}
translate Y OpenGame {Otvorena partija}
translate Y Vienna {Beèka}
translate Y KingsGambit {Kraljev gambit}
translate Y RussianGame {Ruska partija}
translate Y ItalianTwoKnights {Italijanska/Dva lovca}
translate Y Spanish {Španska}
translate Y SpanishExchange {Španska razmena}
translate Y SpanishOpen {Španska otvorena}
translate Y SpanishClosed {Španska zatvorena}
translate Y FrenchDefence {Francuska odbrana}
translate Y FrenchAdvance {Francuski napad}
translate Y FrenchTarrasch {Francuska Taras}
translate Y FrenchWinawer {Francuska Vinaver}
translate Y FrenchExchange {Francuska razmena}
translate Y QueensPawn {Damski pešak}
translate Y Slav {Slavska odbrana}
translate Y QGA {QGA}
translate Y QGD {QGD}
translate Y QGDExchange {QGD razmena}
translate Y SemiSlav {Polu-slavska}
translate Y QGDwithBg5 {QGD sa Bg5}
translate Y QGDOrthodox {QGD Ortodoksna}
translate Y Grunfeld {Grunfeld}
translate Y GrunfeldExchange {Grunfeld razmena}
translate Y GrunfeldRussian {Grunfeld Ruska}
translate Y Catalan {Katalanska}
translate Y CatalanOpen {Katalanska otvorena}
translate Y CatalanClosed {Katalanska zatvorena}
translate Y QueensIndian {Indijska dama}
translate Y NimzoIndian {Nimco-Indijska}
translate Y NimzoIndianClassical {Nimco-Indijska klasièna}
translate Y NimzoIndianRubinstein {Nimco-Indijska Rubinštajn}
translate Y KingsIndian {Kraljevska Indijska}
translate Y KingsIndianSamisch {Kraljevska Indijska Samisch}
translate Y KingsIndianMainLine {Kraljevska Indijska Glavna linija}

# FICS
translate Y ConfigureFics {Konfiguriši FICS}
translate Y FICSLogin {Prijava}
translate Y FICSGuest {Prijava kao gost}
translate Y FICSServerPort {Port servera}
translate Y FICSServerAddress {IP adresa}
translate Y FICSRefresh {Osveži}
translate Y FICSTimeseal {Timeseal}
translate Y FICSTimesealPort {Port za Timeseal}
translate Y FICSSilence {Filter konzole}
translate Y FICSOffers {Ponude}
translate Y FICSMakeOffer {Napravite ponudu}
translate Y FICSGames {Partije}
translate Y FICSFindOpponent {Pronaði protivnika}
translate Y FICSTakeback {Povuci potez}
translate Y FICSTakeback2 {Povuci 2 poteza}
translate Y FICSInitTime {Vreme (min)}
translate Y FICSIncrement {Inkrement (sek)}
translate Y FICSRatedGame {Partija sa rejtingom}
translate Y FICSAutoColour {Automatski}
translate Y FICSManualConfirm {Potvrdi ruèno}
translate Y FICSFilterFormula {Filtriraj formulom}
translate Y FICSIssueSeek {Objavi poziv}
translate Y FICSAccept {Prihvati}
translate Y FICSDecline {Odbij}
translate Y FICSColour {Boja}
translate Y FICSSend {Pošalji}
translate Y FICSConnect {Poveži se}
translate Y FICSShouts {Oglasi}
translate Y FICSTells {Poruke}
translate Y FICSOpponent {Informacije o protivniku}
translate Y FICSInfo {Informacije}
translate Y FICSDraw {Ponudi remi}
translate Y FICSRematch {Revanš}
translate Y FICSQuit {Izaði iz FICS-a}
translate Y FICSCensor {Cenzura}
# ====== TODO To be translated ======
translate Y Tournament Tournament
# ====== TODO To be translated ======
translate Y TourConfigure {Configure Tournament}
# ====== TODO To be translated ======
translate Y TourEngineNum {Number of Engines}
# ====== TODO To be translated ======
translate Y TourName {Tournament Name}
# ====== TODO To be translated ======
translate Y TourRounds {Number of Rounds}
# ====== TODO To be translated ======
translate Y TourControl {Time Control is per}
# ====== TODO To be translated ======
translate Y TourPer {Time per Move}
# ====== TODO To be translated ======
translate Y TourPer2 {Time per game (seconds)}
# ====== TODO To be translated ======
translate Y TourClocks {Show Clocks}
# ====== TODO To be translated ======
translate Y TourAnimate {Animate Moves}
# ====== TODO To be translated ======
translate Y TourComments {Engine Scores as Comments}
# ====== TODO To be translated ======
translate Y TourShed {Game Scheduling}
# ====== TODO To be translated ======
translate Y TourPause {Pause}
# ====== TODO To be translated ======
translate Y TourEnd {End Comp}
# ====== TODO To be translated ======
translate Y TourWinner {Winner(s)}
# ====== TODO To be translated ======
translate Y TourGame {Game}
# ====== TODO To be translated ======
translate Y TourMove {Move}
# ====== TODO To be translated ======
translate Y TourBase {Base}
# ====== TODO To be translated ======
translate Y TourInc {Inc}
# ====== TODO To be translated ======
translate Y TourSecs {secs}

translate Y CCDlgConfigureWindowTitle {Konfiguriši poštanski šah}
translate Y CCDlgCGeneraloptions {Opšte opcije}
translate Y CCDlgDefaultDB {Podrazumevana baza:}
translate Y CCDlgInbox {Prijemno (putanja):}
translate Y CCDlgOutbox {Odlazno (putanja):}
translate Y CCDlgXfcc {XFCC konfiguracija:}
translate Y CCDlgExternalProtocol {Spoljašnji protokol handler (npr. Xfcc)}
translate Y CCDlgFetchTool {Alat za preuzimanje:}
translate Y CCDlgSendTool {Alat za slanje:}
translate Y CCDlgEmailCommunication {eMail komunikacija}
translate Y CCDlgMailPrg {Mail program:}
translate Y CCDlgBCCAddr {(B)CC adresa:}
translate Y CCDlgMailerMode {Mod:}
translate Y CCDlgThunderbirdEg {npr. Thunderbird, Mozilla Mail, Icedove...}
translate Y CCDlgMailUrlEg {npr. Evolution}
translate Y CCDlgClawsEg {npr. Sylpheed Claws}
translate Y CCDlgmailxEg {npr. mailx, mutt, nail...}
translate Y CCDlgAttachementPar {Parametar za prilog:}
translate Y CCDlgInternalXfcc {Koristi internu Xfcc podršku}
translate Y CCDlgConfirmXfcc {Potvrdi poteze}
translate Y CCDlgSubjectPar {Parametar naslova:}
translate Y CCDlgDeleteBoxes {Isprazni prijemno/odlazno}
translate Y CCDlgDeleteBoxesText {Da li zaista želite da ispraznite prijemne i odlazne foldere za poštanski šah? Ovo zahteva novu sinhronizaciju da bi se prikazalo trenutno stanje partija.}
translate Y CCDlgConfirmMove {Potvrdi potez}
translate Y CCDlgConfirmMoveText {Ako potvrdite, sledeæi potez i komentar biæe poslati serveru:}
translate Y CCDlgDBGameToLong {Nekonzistentna glavna linija}
translate Y CCDlgDBGameToLongError {Glavna linija u vašoj bazi je duža od partije u prijemnom. Ako prijemni sadrži aktuelne partije, tj. odmah posle sinhronizacije, neki potezi su greškom dodati u glavnu liniju baze. U tom sluèaju skratite glavnu liniju na (maksimalno) potez\n}
translate Y CCDlgStartEmail {Pokreni novu eMail partiju}
translate Y CCDlgYourName {Vaše ime:}
translate Y CCDlgYourMail {Vaša eMail adresa:}
translate Y CCDlgOpponentName {Ime protivnika:}
translate Y CCDlgOpponentMail {eMail adresa protivnika:}
translate Y CCDlgGameID {ID partije (jedinstven):}
translate Y CCDlgTitNoOutbox {Scid: Odlazni poštanski šah}
translate Y CCDlgTitNoInbox {Scid: Prijemni poštanski šah}
translate Y CCDlgTitNoGames {Scid: Nema poštanskih partija}
translate Y CCErrInboxDir {Direktorijum prijemnog:}
translate Y CCErrOutboxDir {Direktorijum odlaznog:}
translate Y CCErrDirNotUsable {ne postoji ili nije dostupan!\nProverite i ispravite podešavanja.}
translate Y CCErrNoGames {ne sadrži partije!\nPrvo ih preuzmite.}
translate Y CCDlgTitNoCCDB {Scid: Nema poštanske baze}
translate Y CCErrNoCCDB {Nijedna baza tipa 'Poštanski' nije otvorena. Otvorite bazu pre korišæenja funkcija poštanskog šaha.}
translate Y CCFetchBtn {Preuzmi partije sa servera i obradi prijemni}
translate Y CCPrevBtn {Idi na prethodnu partiju}
translate Y CCNextBtn {Idi na sledeæu partiju}
translate Y CCSendBtn {Pošalji potez}
translate Y CCEmptyBtn {Isprazni prijemni i odlazni}
translate Y CCHelpBtn {Pomoæ za ikone i indikatore statusa.\nZa opštu pomoæ pritisnite F1!}
translate Y CCDlgServerName {Ime servera:}
translate Y CCDlgLoginName  {Korisnièko ime:}
translate Y CCDlgPassword   {Lozinka:}
translate Y CCDlgURL        {XFCC-URL:}
translate Y CCDlgRatingType {Tip rejtinga:}
translate Y CCDlgDuplicateGame {Nejedinstven ID partije}
translate Y CCDlgDuplicateGameError {Ova partija postoji više puta u vašoj bazi. Obrišite sve duplikate i kompresujte fajl partija (Fajl/Održavanje/Komprimuj bazu).}
translate Y CCDlgSortOption {Sortiranje:}
translate Y CCDlgListOnlyOwnMove {Samo partije gde je moj potez}
translate Y CCOrderClassicTxt {Mesto, Turnir, Kolo, Rezultat, Beli, Crni}
translate Y CCOrderMyTimeTxt {Moj sat}
translate Y CCOrderTimePerMoveTxt {Vreme po potezu do sledeæe kontrole vremena}
translate Y CCOrderStartDate {Datum poèetka}
translate Y CCOrderOppTimeTxt {Sat protivnika}
translate Y CCDlgConfigRelay {Konfiguriši ICCF posmatranje}
translate Y CCDlgConfigRelayHelp {Otiðite na stranicu partije na http://www.iccf-webchess.com i prikažite partiju koja se posmatra. Ako vidite šahovsku tablu, kopirajte URL iz pregledaèa u listu ispod. Samo jedan URL po liniji!\nPrimer: http://www.iccf-webchess.com/MakeAMove.aspx?id=266452}

translate Y ExtHWConfigConnection {Konfiguriši eksterni hardver}
translate Y ExtHWPort {Port}
translate Y ExtHWEngineCmd {Komanda za engine}
translate Y ExtHWEngineParam {Parametar engine-a}
translate Y ExtHWShowButton {Prikaži dugme}
translate Y ExtHWHardware {Hardver}
translate Y ExtHWNovag {Novag Citrine}
translate Y ExtHWInputEngine {Input Engine}
translate Y ExtHWNoBoard {Nema table}
translate Y IEConsole {Konzola Input Engine-a}
translate Y IESending {Potezi poslati za}
translate Y IESynchronise {Sinhronizuj}
translate Y IERotate  {Rotiraj}
translate Y IEUnableToStart {Ne mogu da pokrenem Input Engine:}
translate Y DoneWithPosition {Gotovo sa pozicijom}
translate Y Board {Tabla}
translate Y showGameInfo {Prikaži informacije o partiji}
translate Y autoResizeBoard {Automatsko podešavanje velièine table}
translate Y DockTop {Premesti na vrh}
translate Y DockBottom {Premesti na dno}
translate Y DockLeft {Premesti levo}
translate Y DockRight {Premesti desno}
translate Y Undock {Odvoji}
translate Y ChangeIcon {Promeni ikonu}
translate Y More {Više}

# Drag & Drop
translate Y CannotOpenUri {Ne mogu da otvorim sledeæi URI:}
translate Y InvalidUri {Sadržaj koji je prevuèen nije validna lista URI-ja.}
translate Y UriRejected {Sledeæi fajlovi su odbijeni:}
translate Y UriRejectedDetail {Samo navedeni tipovi fajlova mogu biti obraðeni:}
translate Y EmptyUriList {Lista URI-ja je prazna.}
translate Y SelectionOwnerDidntRespond {Isteklo vreme tokom prevlaèenja: vlasnik selekcije nije odgovorio.}
translate Y PasteAnal {Prenesi analizu}
translate Y SortOpcodes {Sortiraj Opcodes}
translate Y AddPosition {Dodaj poziciju}
translate Y FindPos {Pronaði najdublju poziciju u partiji}
translate Y AnalPosition {Analiziraj pozicije}
translate Y StripOpcodes {Ukloni Opcodes}
translate Y NoMoves {Nema poteza iz ove pozicije}
translate Y CountBestMoves {Prebroj najbolje poteze}

}
# end of serbian.tcl – prevod na srpski jezik
