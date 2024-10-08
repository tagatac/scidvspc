//////////////////////////////////////////////////////////////////////
//
//  FILE:       tkscid.cpp
//              Scid extensions to Tcl/Tk interpreter
//
//  Part of:    Scid (Shane's Chess Information Database)
//
//  Notice:     Copyright (c) 1999-2004 Shane Hudson.  All rights reserved.
//              Copyright (c) 2006-2007 Pascal Georges
//              Copyright (c) 2009-2013 Stevenaaus
//
//////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define NOMINMAX
#endif

#include "tkscid.h"

#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <csetjmp>

#include <algorithm>
#include <utility>
#include <vector>
#include <set>
#include <unordered_set>

#include "charsetconverter.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Global variables:

static scidBaseT * db = NULL;          // current database.
static scidBaseT * clipbase = NULL;    // clipbase database.
static scidBaseT * dbList = NULL;      // array of database slots.
static int currentBase = 0;
static Position * scratchPos = NULL;   // temporary "scratch" position.
static Game * scratchGame = NULL;      // "scratch" game for searches, etc.
static PBook * ecoBook = NULL;         // eco classification pbook.
static SpellChecker * spellChecker [NUM_NAME_TYPES] = {NULL};  // Name correction.

static progressBarT progBar;

static OpTable * reports[2] = {NULL, NULL};
static const char * reportTypeName[2] = { "opening", "player" };
static const uint REPORT_OPENING = 0;
static const uint REPORT_PLAYER = 1;

static char decimalPointChar = '.';
static uint htmlDiagStyle = 0;

// Default maximum number of games in the clipbase database:
const uint CLIPBASE_MAX_GAMES = 5000000; // 5 million

// Actual current maximum number of games in the clipbase database:
static uint clipbaseMaxGames = CLIPBASE_MAX_GAMES;

// MAX_BASES is the maximum number of databases that can be open,
// including the clipbase database.
const int MAX_BASES = 9;
const int CLIPBASE_NUM = MAX_BASES - 1;

// MAX_EPD is the maximum number of EPD files that can be open.
const int MAX_EPD = 4;
static PBook * pbooks [MAX_EPD];

// Declare scid_InitTclTk, to initialise the Tcl interpreter:

int scid_InitTclTk (Tcl_Interp * interp);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// InvalidCommand():
//    Given a Tcl Interpreter, a major command name (e.g. "sc_base") and
//    a null-terminated array of minor commands, this function sets
//    the interpreter's result to a useful error message listing the
//    available subcommands.
//    Returns TCL_ERROR, so caller can simply:
//        return InvalidCommand (...);
//    instead of:
//        InvalidCommand (...);
//        return TCL_ERROR;
int
InvalidCommand (Tcl_Interp * ti, const char * majorCmd,
                const char ** minorCmds)
{
    ASSERT (majorCmd != NULL);
    Tcl_AppendResult (ti, "Invalid command: ", majorCmd,
                      " has the following minor commands:\n", NULL);
    while (*minorCmds != NULL) {
        Tcl_AppendResult (ti, "   ", *minorCmds, "\n", NULL);
        minorCmds++;
    }
    return TCL_ERROR;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Progress Bar update routine:
//
static void
updateProgressBar (Tcl_Interp * ti, uint done, uint total, bool update_idletasks_only = false)
{
    char tempStr [250];
    uint width = progBar.width;
    if (total > 0) {
        double w = (double)width * (double)done / (double)total;
        width = (int) w;
    }
    sprintf (tempStr, "%s coords %s 0 0 %u %u", progBar.canvName,
             progBar.rectName, width + 1, progBar.height + 1);
    Tcl_Eval (ti, tempStr);
    if (progBar.timeName[0] != 0) {
        int elapsed = progBar.timer.CentiSecs();
        int estimated = elapsed;
        if (done != 0) {
            // Estimated total time = elapsed * total / done, but we do
            // the calculation using double-precision floating point because
            // if total and elapsed are large, we can get overflow.
            double d = (double)elapsed * (double)total / (double)done;
            estimated = (int) d;
        }
        elapsed /= 100;
        estimated /= 100;
        sprintf (tempStr, "%s itemconfigure %s -text \"%d:%02d / %d:%02d\"",
                 progBar.canvName, progBar.timeName,
                 elapsed / 60, elapsed % 60, estimated / 60, estimated % 60);
        Tcl_Eval (ti, tempStr);
    }
    if (update_idletasks_only)
        Tcl_Eval (ti, "update idletasks");
    else
    	Tcl_Eval (ti, "update");
}


static bool
startProgressBar (void)
{
    progBar.interrupt = false;
    if (progBar.state == false) { return false; }
    progBar.state = false;
    progBar.timer.Reset();
    return true;
}

static void
restartProgressBar (Tcl_Interp * ti)
{
    progBar.timer.Reset();
    updateProgressBar (ti, 0, 1);
}

static inline bool
interruptedProgress () {
    return (progBar.interrupt);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// recalcFlagCounts:
//    Updates all precomputed stats about the database: flag counts,
//    average rating, date range, etc.
void
recalcFlagCounts (scidBaseT * basePtr)
{
    scidStatsT * stats = &(basePtr->stats);
    uint i;

    // Zero out all stats:
    for (i = 0; i < IDX_NUM_FLAGS; i++) { stats->flagCount[i] = 0; }
    stats->nRatings = 0;
    stats->sumRatings = 0;
    stats->minRating = 0;
    stats->maxRating = 0;
    stats->minDate = ZERO_DATE;
    stats->maxDate = ZERO_DATE;
    stats->nYears = 0;
    stats->sumYears = 0;
    for (i=0; i < NUM_RESULT_TYPES; i++) {
        stats->nResults[i] = 0;
    }
    for (i=0; i < 1; i++) {
        stats->ecoCount0[i].count = 0;
        stats->ecoCount0[i].results[RESULT_White] = 0;
        stats->ecoCount0[i].results[RESULT_Black] = 0;
        stats->ecoCount0[i].results[RESULT_Draw] = 0;
        stats->ecoCount0[i].results[RESULT_None] = 0;
    }
    for (i=0; i < 5; i++) {
        stats->ecoCount1[i].count = 0;
        stats->ecoCount1[i].results[RESULT_White] = 0;
        stats->ecoCount1[i].results[RESULT_Black] = 0;
        stats->ecoCount1[i].results[RESULT_Draw] = 0;
        stats->ecoCount1[i].results[RESULT_None] = 0;
    }
    for (i=0; i < 50; i++) {
        stats->ecoCount2[i].count = 0;
        stats->ecoCount2[i].results[RESULT_White] = 0;
        stats->ecoCount2[i].results[RESULT_Black] = 0;
        stats->ecoCount2[i].results[RESULT_Draw] = 0;
        stats->ecoCount2[i].results[RESULT_None] = 0;
    }
    for (i=0; i < 500; i++) {
        stats->ecoCount3[i].count = 0;
        stats->ecoCount3[i].results[RESULT_White] = 0;
        stats->ecoCount3[i].results[RESULT_Black] = 0;
        stats->ecoCount3[i].results[RESULT_Draw] = 0;
        stats->ecoCount3[i].results[RESULT_None] = 0;
    }
    for (i=0; i < 500*26; i++) {
        stats->ecoCount4[i].count = 0;
        stats->ecoCount4[i].results[RESULT_White] = 0;
        stats->ecoCount4[i].results[RESULT_Black] = 0;
        stats->ecoCount4[i].results[RESULT_Draw] = 0;
        stats->ecoCount4[i].results[RESULT_None] = 0;
    }
    // Read stats from index entry of each game:
    for (uint gnum=0; gnum < basePtr->numGames; gnum++) {
        IndexEntry * ie = basePtr->idx->FetchEntry (gnum);
        stats->nResults[ie->GetResult()]++;
        eloT elo = ie->GetWhiteElo();
        if (elo > 0) {
            stats->nRatings++;
            stats->sumRatings += elo;
            if (stats->minRating == 0) { stats->minRating = elo; }
            if (elo < stats->minRating) { stats->minRating = elo; }
            if (elo > stats->maxRating) { stats->maxRating = elo; }
            basePtr->nb->AddElo (ie->GetWhite(), elo);
        }
        elo = ie->GetBlackElo();
        if (elo > 0) {
            stats->nRatings++;
            stats->sumRatings += elo;
            if (stats->minRating == 0) { stats->minRating = elo; }
            if (elo < stats->minRating) { stats->minRating = elo; }
            if (elo > stats->maxRating) { stats->maxRating = elo; }
            basePtr->nb->AddElo (ie->GetBlack(), elo);
        }
        dateT date = ie->GetDate();
        if (gnum == 0) {
            stats->maxDate = stats->minDate = date;
        }
        if (date_GetYear(date) > 0) {
            if (date < stats->minDate) { stats->minDate = date; }
            if (date > stats->maxDate) { stats->maxDate = date; }
            stats->nYears++;
            stats->sumYears += date_GetYear (date);
            basePtr->nb->AddDate (ie->GetWhite(), date);
            basePtr->nb->AddDate (ie->GetBlack(), date);
        }

        for (uint flag = 0; flag < IDX_NUM_FLAGS; flag++) {
            bool value = ie->GetFlag (1 << flag);
            if (value) {
                stats->flagCount[flag]++;
            }
        }

        ecoT eco = ie->GetEcoCode();
        ecoStringT ecoStr;
        eco_ToExtendedString (eco, ecoStr);
        uint length = strLength (ecoStr);
        resultT result = ie->GetResult();
        if (length >= 3) {
            uint code = 0;
            stats->ecoCount0[code].count++;
            stats->ecoCount0[code].results[result]++;
            code = ecoStr[0] - 'A';
            stats->ecoCount1[code].count++;
            stats->ecoCount1[code].results[result]++;
            code = (code * 10) + (ecoStr[1] - '0');
            stats->ecoCount2[code].count++;
            stats->ecoCount2[code].results[result]++;
            code = (code * 10) + (ecoStr[2] - '0');
            stats->ecoCount3[code].count++;
            stats->ecoCount3[code].results[result]++;
            if (length >= 4) {
                code = (code * 26) + (ecoStr[3] - 'a');
                stats->ecoCount4[code].count++;
                stats->ecoCount4[code].results[result]++;
            }
        }
    }
}

void
recalcEstimatedRatings (NameBase * nb)
{
    // Update estimated ratings from spellcheck file if available:
    if (spellChecker[NAME_PLAYER] == NULL) { return; }
    for (idNumberT id=0; id < nb->GetNumNames(NAME_PLAYER); id++) {
        if (nb->GetElo(id) == 0  &&  nb->GetFrequency(NAME_PLAYER, id) > 0) {
            const char * name = nb->GetName (NAME_PLAYER, id);
            if (! strIsSurnameOnly (name)) {
                const char * text = spellChecker[NAME_PLAYER]->GetCommentExact (name);
                if (text != NULL) {
                    nb->SetElo (id, SpellChecker::GetPeakRating (text));
                }
            }
        }
    }
}

void
recalcNameFrequencies (NameBase * nb, Index * idx)
{
    for (nameT nt = NAME_FIRST; nt <= NAME_LAST; nt++) {
        nb->ZeroAllFrequencies (nt);
    }
    IndexEntry iE;
    for (uint i=0; i < idx->GetNumGames(); i++) {
        idx->ReadEntries (&iE, i, 1);
        nb->IncFrequency (NAME_PLAYER, iE.GetWhite(), 1);
        nb->IncFrequency (NAME_PLAYER, iE.GetBlack(), 1);
        nb->IncFrequency (NAME_EVENT, iE.GetEvent(), 1);
        nb->IncFrequency (NAME_SITE, iE.GetSite(), 1);
        nb->IncFrequency (NAME_ROUND, iE.GetRound(), 1);
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Standard error messages:
//
const char *
errMsgNotOpen (Tcl_Interp * ti)
{
    return translate (ti, "ErrNotOpen", "This is not an open database.");
}

const char *
errMsgReadOnly (Tcl_Interp * ti)
{
    return translate (ti, "ErrReadOnly",
                      "This database is read-only; it cannot be altered.");
}

const char *
errMsgSearchInterrupted (Tcl_Interp * ti)
{
    return translate (ti, "ErrSearchInterrupted",
                      "[Interrupted search; results are incomplete]");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Main procedure
//
int
main (int argc, char * argv[])
{
    int newArgc = argc;
    char ** newArgv = argv;

#ifdef _WIN32
#  ifdef SOURCE_TCL_FILE

    // Load scid.gui (SOURCE_TCL_FILE) by inserting it into argv[1]
    // - but i can't get is to play nice with Windows "open with" feature S.A

    newArgc++;
    newArgv = (char **) malloc (sizeof (char *) * newArgc);
    newArgv[0] = argv[0];
    for (int i = 1; i < argc; i++) { newArgv[i+1] = argv[i]; }

    // insert into newArgv[1] "PATH_TO_TKSCID.EXE\scid.gui"
    char sourceFileName [MAX_PATH];
    sourceFileName[0] = 0;
    HMODULE hModule = GetModuleHandle (NULL);
    GetModuleFileNameA (hModule, sourceFileName, MAX_PATH);
    char * end = strrchr (sourceFileName, '\\');
    if (end != NULL) { strCopy (end + 1, SOURCE_TCL_FILE); }
    newArgv[1] = sourceFileName;
#  endif  // ifdef SOURCE_TCL_FILE
#endif  // ifdef _WIN32

#ifdef TCL_ONLY
    Tcl_Main (newArgc, newArgv, scid_InitTclTk);
#else
    Tk_Main (newArgc, newArgv, scid_InitTclTk);
#endif
    exit(0);
    return 0;
}

#ifndef TCL_ONLY
#ifndef __APPLE__
extern "C" int Tkdnd_Init (Tcl_Interp*);
extern void Tk_Selection_Init (Tcl_Interp*);
#endif
#endif

int
scid_InitTclTk (Tcl_Interp * ti)
{

    if (Tcl_Init (ti) == TCL_ERROR) { return TCL_ERROR; }
#ifndef TCL_ONLY
    if (Tk_Init (ti) == TCL_ERROR) { return TCL_ERROR; }
#endif

    // Register Scid application-specific commands:
    // CREATE_CMD() is a macro to reduce the clutter of the final two args
    // to Tcl_CreateCommand().

#define CREATE_CMD(ip,name,cmd) \
 Tcl_CreateCommand ((ip), (name), (Tcl_CmdProc *)(cmd), \
 (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL)


    ////////////////////
    /// Scid-specific Tcl/Tk commands:

    CREATE_CMD (ti, "strIsPrefix", str_is_prefix);
    CREATE_CMD (ti, "strPrefixLen", str_prefix_len);
    CREATE_CMD (ti, "sc_base", sc_base);
    // CREATE_CMD (ti, "sc_book", sc_epd);  // sc_epd used to be sc_book : Pascal Georges : not used any longer, so reused
    CREATE_CMD (ti, "sc_book", sc_book);
    CREATE_CMD (ti, "sc_epd", sc_epd);
    CREATE_CMD (ti, "sc_clipbase", sc_clipbase);
    CREATE_CMD (ti, "sc_compact", sc_compact);
    CREATE_CMD (ti, "sc_eco", sc_eco);
    CREATE_CMD (ti, "sc_filter", sc_filter);
    CREATE_CMD (ti, "sc_game", sc_game);
    CREATE_CMD (ti, "sc_flags", sc_flags);
    CREATE_CMD (ti, "sc_info", sc_info);
    CREATE_CMD (ti, "sc_move", sc_move);
    CREATE_CMD (ti, "sc_name", sc_name);
    CREATE_CMD (ti, "sc_report", sc_report);
    CREATE_CMD (ti, "sc_pos", sc_pos);
    CREATE_CMD (ti, "sc_progressBar", sc_progressBar);
    CREATE_CMD (ti, "sc_search", sc_search);
    CREATE_CMD (ti, "sc_tree", sc_tree);
    CREATE_CMD (ti, "sc_var", sc_var);

    // Initialise array of EPD slots:
    for (int epdID=0; epdID < MAX_EPD; epdID++) { pbooks[epdID] = NULL; }

    // Initialise global Scid database variables:
    dbList = new scidBaseT [MAX_BASES];

    for (int base=0; base < MAX_BASES; base++) {
        db = &(dbList[base]);
        db->idx = new Index;
        db->nb = new NameBase;
        db->game = new Game;
        for (int u = 0; u < UNDO_MAX; u++)
          db->undoGame[u] = NULL;
        db->undoMax = -1;
        db->undoIndex = -1;
        db->undoCurrent = -1;
        db->undoCurrentNotAvail = false;
        db->gameNumber = -1;
        db->gameAltered = false;
        db->gfile = new GFile;
        // TODO: Bases should be able to share common buffers!!!
        db->bbuf = new ByteBuffer;
        db->bbuf->SetBufferSize (BBUF_SIZE);
        db->tbuf = new TextBuffer;
        db->tbuf->SetBufferSize (TBUF_SIZE);
        strCopy (db->fileName, "");
        strCopy (db->realFileName, "");
        db->fileMode = FMODE_Both;
        db->inUse = false;
        db->filter = new Filter(0);
        db->dbFilter = db->filter;
        db->treeFilter = NULL;
        db->numGames = 0;
        db->memoryOnly = false;
        db->duplicates = NULL;
        db->idx->SetDescription (errMsgNotOpen(ti));

        recalcFlagCounts (db);

        db->tree.moveCount = db->tree.totalCount = 0;
        db->treeCache = NULL;

        db->treeSearchTime = 0;
    }

    // Initialise the progress bar:
    progBar.state = false;
    progBar.interrupt = false;
    progBar.canvName = strDuplicate ("");
    progBar.rectName = strDuplicate ("");
    progBar.timeName = strDuplicate ("");

    // Initialise the clipbase database:
    clipbase = &(dbList[CLIPBASE_NUM]);
    clipbase->gfile->CreateMemoryOnly();
    clipbase->idx->CreateMemoryOnly();
    clipbase->idx->SetType (2);
    clipbase->idx->SetDescription ("Temporary database, not kept on disk.");
    clipbase->inUse = true;
    clipbase->memoryOnly = true;

    clipbase->treeCache = new TreeCache;
    clipbase->treeCache->SetCacheSize (SCID_TreeCacheSize);
    clipbase->backupCache = new TreeCache;
    clipbase->backupCache->SetCacheSize (SCID_BackupCacheSize);
    clipbase->backupCache->SetPolicy (TREECACHE_Oldest);

    currentBase = 0;
    scratchPos = new Position;
    scratchGame = new Game;
    db = &(dbList[currentBase]);

#ifndef TCL_ONLY
#ifndef __APPLE__
    // Drag and Drop init
    Tkdnd_Init (ti);
    Tk_Selection_Init (ti);
#endif
#endif

    return TCL_OK;
}


/////////////////////////////////////////////////////////////////////
//  MISC functions
/////////////////////////////////////////////////////////////////////

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// findEmptyBase:
//    returns a number from 0 to MAX_BASES - 1 if an empty
//    database slot exists; or returns -1 if a maximum number of bases
//    are already in use.
int
findEmptyBase (void)
{
    for (int i=0; i < MAX_BASES; i++) {
        if (! dbList[i].inUse) { return i; }
    }
    return -1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// str_is_prefix:
//    Provides a fast Tcl command "strIsPrefix" for checking if the
//    first string provided is a prefix of the second string, without
//    needing the standard slower [string match] or [string range]
//    routines.
int
str_is_prefix (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: strIsPrefix <shortStr> <longStr>");
    }

    return setBoolResult (ti, strIsPrefix (argv[1], argv[2]));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// str_prefix_len:
//    Tcl command that returns the length of the common text at the start
//    of two strings.
int
str_prefix_len (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: strPrefixLen <str> <str>");
    }

    return setUintResult (ti, strPrefix (argv[1], argv[2]));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// strGetFilterOp:
//    Converts a string value to a filter operation value.
filterOpT
strGetFilterOp (const char * str)
{
    switch (*str) {
        // AND:
        case 'A': case 'a': case '0': return FILTEROP_AND;
        // OR:
        case 'O': case 'o': case '1': return FILTEROP_OR;
        // RESET:
        case 'R': case 'r': case '2': return FILTEROP_RESET;
    }
    // Default is RESET.
    return FILTEROP_RESET;
}


/////////////////////////////////////////////////////////////////////
///  DATABASE functions

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// base_opened:
//    Returns a slot number if the named database is already
//    opened in Scid, or -1 if it is not open.
int
base_opened (const char * filename)
{
    for (int i=0; i < CLIPBASE_NUM; i++) {
        if (dbList[i].inUse  &&  strEqual (dbList[i].realFileName, filename)) {
            return i;
        }
    }

    // OK, An exact same file name was not found, but we may have compared
    // absolute path (e.g. from a File Open dialog) with a relative one
    // (e.g. from a command-line argument).
    // To check further, return true if two names have the same tail
    // (part after the last "/"), device and inode number:

    const char * tail = strLastChar (filename, '/');
    if (tail == NULL) { tail = filename; } else { tail++; }
    for (int j=0; j < CLIPBASE_NUM; j++) {
        if (! dbList[j].inUse) { continue; }
        const char * ftail = strLastChar (dbList[j].realFileName, '/');
        if (ftail == NULL) {
            ftail = dbList[j].realFileName;
        } else {
            ftail++;
        }

        if (strEqual (ftail, tail)) {
            struct stat s1;
            struct stat s2;
            if (stat (ftail, &s1) != 0) { continue; }
            if (stat (tail, &s2) != 0) { continue; }
            if (s1.st_dev == s2.st_dev  &&  s1.st_ino == s2.st_ino) {
                return j;
            }
        }
    }
    return -1;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base: database commands.
int
sc_base (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "autoload",     "check",        "close",        "count",
        "create",       "current",      "description",  "duplicates",
        "ecoStats",     "export",       "filename",     "import",
        "inUse",        "isReadOnly",   "numGames",     "open",
        "piecetrack",   "slot",         "sort",         "stats",
        "switch",       "tag",          "tournaments",  "type",
        "upgrade",      "fixCorrupted",    "sortup",    "sortdown",
        NULL
    };
    enum {
        BASE_AUTOLOAD,    BASE_CHECK,       BASE_CLOSE,       BASE_COUNT,
        BASE_CREATE,      BASE_CURRENT,     BASE_DESCRIPTION, BASE_DUPLICATES,
        BASE_ECOSTATS,    BASE_EXPORT,      BASE_FILENAME,    BASE_IMPORT,
        BASE_INUSE,       BASE_ISREADONLY,  BASE_NUMGAMES,    BASE_OPEN,
        BASE_PTRACK,      BASE_SLOT,        BASE_SORT,        BASE_STATS,
        BASE_SWITCH,      BASE_TAG,         BASE_TOURNAMENTS, BASE_TYPE,
        BASE_UPGRADE,     BASE_FIX_CORRUPTED, BASE_SORTUP,    BASE_SORTDOWN
    };
    int index = -1;

    if (argc > 1) { index = strUniqueMatch (argv[1], options); }

    switch (index) {
    case BASE_AUTOLOAD:
        return sc_base_autoload (cd, ti, argc, argv);

    case BASE_CHECK:
        return sc_base_check (cd, ti, argc, argv);

    case BASE_CLOSE:
        return sc_base_close (cd, ti, argc, argv);

    case BASE_COUNT:
        return sc_base_count (cd, ti, argc, argv);

    case BASE_CREATE:
        return sc_base_create (cd, ti, argc, argv);

    case BASE_CURRENT:
        return setIntResult (ti, currentBase + 1);

    case BASE_DESCRIPTION:
        return sc_base_description (cd, ti, argc, argv);

    case BASE_DUPLICATES:
        return sc_base_duplicates (cd, ti, argc, argv);

    case BASE_ECOSTATS:
        return sc_base_ecoStats (cd, ti, argc, argv);

    case BASE_EXPORT:
        return sc_base_export (cd, ti, argc, argv);

    case BASE_FILENAME:
        return sc_base_filename (cd, ti, argc, argv);

    case BASE_IMPORT:
        return sc_base_import (cd, ti, argc, argv);

    case BASE_INUSE:
        return sc_base_inUse (cd, ti, argc, argv);

    case BASE_ISREADONLY:
        // sc_base isReadOnly [set] [basenumber]
        if (argc < 3 ) {
	    return setBoolResult (ti, db->inUse && db->fileMode==FMODE_ReadOnly);
        } else {
            scidBaseT *basePtr = NULL;
            int baseNum;

	    if (argc == 3 && !strEqual (argv[2], "set")) {
                // Querying if base argv[2] is read-only
	        baseNum = strGetInteger (argv[2]);
		if (baseNum < 1 || baseNum > MAX_BASES) {
		    return errorResult (ti, "Invalid database number.");
		}
		basePtr = &(dbList[baseNum - 1]);

		return setBoolResult (ti, basePtr->inUse && basePtr->fileMode==FMODE_ReadOnly);
            }

            // Querying over - now we want to set readonly flag

	    if (argc == 3) {
              basePtr = db;
	    } else {
              if (argc != 4 || !strEqual (argv[2], "set")) {
		return errorResult (ti, "Usage: sc_base isReadOnly [set] [basenumber]");
              }
	      baseNum = strGetInteger (argv[3]);
	      if (baseNum < 1 || baseNum > MAX_BASES) {
	        return errorResult (ti, "Invalid database number.");
	      }
	      basePtr = &(dbList[baseNum - 1]);
            }

            if (! basePtr->inUse) {
                return errorResult (ti, errMsgNotOpen(ti));
            }
            if (basePtr->fileMode == FMODE_ReadOnly) {
                return errorResult (ti, "This database is already read-only.");
            }
            if (basePtr->idx->SetReadOnly () != OK) {
                return errorResult (ti, "Unable to make this database read-only.");
            }
            basePtr->fileMode = FMODE_ReadOnly;
            return TCL_OK;
        }

    case BASE_NUMGAMES:
        return sc_base_numGames (cd, ti, argc, argv);

    case BASE_OPEN:
        return sc_base_open (cd, ti, argc, argv);

    case BASE_PTRACK:
        return sc_base_piecetrack (cd, ti, argc, argv);

    case BASE_SLOT:
        return sc_base_slot (cd, ti, argc, argv);

    case BASE_SORT:
        return sc_base_sort (cd, ti, argc, argv);

    case BASE_STATS:
        return sc_base_stats (cd, ti, argc, argv);

    case BASE_SWITCH:
        return sc_base_switch (cd, ti, argc, argv);

    case BASE_TAG:
        return sc_base_tag (cd, ti, argc, argv);

    case BASE_TOURNAMENTS:
        return sc_base_tournaments (cd, ti, argc, argv);

    case BASE_TYPE:
        return sc_base_type (cd, ti, argc, argv);

    case BASE_UPGRADE:
        return sc_base_upgrade (cd, ti, argc, argv);

    case BASE_FIX_CORRUPTED:
        return sc_base_fix_corrupted (cd, ti, argc, argv);

    case BASE_SORTUP:
        return sc_base_sortup (cd, ti, argc, argv);

    case BASE_SORTDOWN:
        return sc_base_sortdown (cd, ti, argc, argv);

    default:
        return InvalidCommand (ti, "sc_base", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_autoload:
//   Sets or returns the autoload number of the database, which
//   is the game to load when opening the base.
int
sc_base_autoload (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc == 2) {
        return setUintResult (ti, db->idx->GetAutoLoad());
    }

    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (db->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }

    uint gnum = strGetUnsigned (argv[2]);
    db->idx->SetAutoLoad (gnum);
    db->idx->WriteHeader();
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_numGames:
//   Takes optional database number and returns number of games.
int
sc_base_numGames (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * basePtr = db;
    if (argc > 2) {
        int baseNum = strGetInteger (argv[2]);
        if (baseNum < 1 || baseNum > MAX_BASES) {
            return errorResult (ti, "Invalid database number.");
        }
        basePtr = &(dbList[baseNum - 1]);
    }
    return setUintResult (ti, basePtr->inUse ? basePtr->numGames : 0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_filename: get the name of the current database file.
//    Returns "[empty]" for an empty base, "[clipbase]" for the clipbase.
int
sc_base_filename (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * basePtr = db;
    if (argc > 2) {
        int baseNum = strGetInteger (argv[2]);
        if (baseNum < 1 || baseNum > MAX_BASES) {
            return errorResult (ti, "Invalid database number.");
        }
        basePtr = &(dbList[baseNum - 1]);
    }

    if (! basePtr->inUse) {
        Tcl_AppendResult (ti, "[", translate (ti, "empty"), "]", NULL);
    } else if (basePtr == clipbase) {
        Tcl_AppendResult (ti, "[", translate (ti, "clipbase"), "]", NULL);
    } else {
        Tcl_AppendResult (ti, basePtr->fileName, NULL);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_inUse
//  Returns 1 if the database slot is in use; 0 otherwise.
int
sc_base_inUse (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * basePtr = db;
    if (argc > 2) {
        int baseNum = strGetInteger (argv[2]);
        if (baseNum < 1 || baseNum > MAX_BASES) {
            return errorResult (ti, "Invalid database number.");
        }
        basePtr = &(dbList[baseNum - 1]);
    }

    return setBoolResult (ti, basePtr->inUse);
}


void
base_progress (void * data, uint count, uint total) {
    updateProgressBar ((Tcl_Interp *)data, count, total);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_slot: takes a database (.si3 or .pgn file) name and returns
//    the slot number it is using if it is already opened, or 0 if
//    it is not loaded yet.
int
sc_base_slot (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_base slot <filename>");
    }
    fileNameT fname;
    strCopy (fname, argv[2]);
    const char * suffix = strFileSuffix (fname);

    if (suffix == NULL  ||
        (!strEqual (suffix, INDEX_SUFFIX)
         &&  !strEqual (suffix, GZIP_SUFFIX)
         &&  !strEqual (suffix, PGN_SUFFIX))) {
        // Need to add Index file suffix:
        strAppend (fname, INDEX_SUFFIX);
    }

    return setIntResult (ti, base_opened (fname) + 1);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_open_failure: if the opening of a base fails,
// clean up db entry
void base_open_failure( int oldBaseNum ) {
  db->idx->CloseIndexFile();
  db->idx->Clear();
  db->nb->Clear();
  db->gfile->Close();
  db->inUse = false;
  db->gameNumber = -1;
  db->numGames = 0;
  strCopy (db->fileName, "<empty>");
  currentBase = oldBaseNum;
  db = &(dbList[currentBase]);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_open: takes a database name and opens the database.
//    If either the index file or game file cannot be opened for
//    reading and writing, then the database is opened read-only
//    and will not be alterable.
#include "gfile.h"
int
sc_base_open (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = 0;
    showProgress = startProgressBar();

    bool readOnly = false;  // Open database read-only.
    bool fastOpen = false;  // Fast open (no flag counts, etc)
    const char * usage = "Usage: sc_base open [-readonly] [-fast] <filename>";

    // Check options:
    const char * options[] = { "-fast", "-readonly", NULL };
    enum { OPT_FAST, OPT_READONLY };
    int baseArg = 2;
    while (baseArg+1 < argc) {
        int index = strUniqueMatch (argv[baseArg], options);
        switch (index) {
            case OPT_FAST:     fastOpen = true; break;
            case OPT_READONLY: readOnly = true; break;
            default: return errorResult (ti, usage);
        }
        baseArg++;
    }
    if (baseArg+1 != argc) { return errorResult (ti, usage); }

    const char * filename = argv[baseArg];

    // Check that this base is not already opened:
    fileNameT realFileName;
    strCopy (realFileName, filename);
    strAppend (realFileName, INDEX_SUFFIX);
    if (base_opened (realFileName) >= 0) {
        return errorResult (ti, "The database you selected is already opened.");
    }

    // Find an empty database slot to use:
    int oldBaseNum = currentBase;
    if (db->inUse) {
        int newBaseNum = findEmptyBase();
        if (newBaseNum == -1) {
            return errorResult (ti, "Too many databases are open; close one first.");
        }
        currentBase = newBaseNum;
        db = &(dbList[currentBase]);
    }

    db->idx->SetFileName (filename);
    db->nb->SetFileName (filename);

    db->memoryOnly = false;
    if (readOnly)
         db->fileMode = FMODE_ReadOnly;
    else
         db->fileMode = FMODE_Both;

    errorT err;
    err = db->idx->OpenIndexFile (db->fileMode);

    if (err == ERROR_FileOpen  &&  db->fileMode == FMODE_Both) {
        // Try opening read-only:
        db->fileMode = FMODE_ReadOnly;
        err = db->idx->OpenIndexFile (db->fileMode);
    }

    if (err != OK) {
        currentBase = oldBaseNum;
        db = &(dbList[currentBase]);
        setResult (ti, "Error opening index file");
        if (err == ERROR_FileVersion) {
            setResult (ti, "Old format Scid file, now out of date.");
        }
        if (err == ERROR_OldScidVersion) {
            setResult (ti, "Database version newer than Scid; please upgrade Scid.");
        }
        return TCL_ERROR;
    }

    if (db->nb->ReadNameFile() != OK) {
        base_open_failure(oldBaseNum);
        return errorResult (ti, "Error opening name file.");
    }

    err = db->gfile->Open (filename, db->fileMode);
    if (err == ERROR_FileOpen  &&  db->fileMode == FMODE_Both) {
        // Try opening read-only:
        db->fileMode = FMODE_ReadOnly;
        err = db->gfile->Open (filename, db->fileMode);
    }

    if (err != OK) {
        base_open_failure(oldBaseNum);
        return errorResult (ti, "Error opening game file.");
    }

    // Read entire index, showing progress every 20,000 games if applicable:
    if (showProgress) {
        db->idx->ReadEntireFile (20000, base_progress, (void *) ti);
    } else {
      db->idx->ReadEntireFile ();
    }

    if (db->idx->VerifyFile (db->nb) != OK) {
        db->idx->CloseIndexFile();
        return errorResult (ti, "Error: name corruption in index file.\nRun \"scidt -N\" on this database to fix it.");
    }

    db->numGames = db->idx->GetNumGames();

    // Compute name frequencies, flag counts, etc unless a fast open
    // was requested:
    if (! fastOpen) {
        recalcNameFrequencies (db->nb, db->idx);
        recalcFlagCounts (db);
        recalcEstimatedRatings (db->nb);
    }

    // Initialise the filter: all games match at move 1 by default.
    clearFilter (db, db->numGames);

    strCopy (db->fileName, filename);
    strCopy (db->realFileName, realFileName);
    db->inUse = true;
    db->gameNumber = -1;

    if (db->treeCache == NULL) {
        db->treeCache = new TreeCache;
        db->treeCache->SetCacheSize (SCID_TreeCacheSize);
        db->backupCache = new TreeCache;
        db->backupCache->SetCacheSize (SCID_BackupCacheSize);
        db->backupCache->SetPolicy (TREECACHE_Oldest);
    }

    db->treeCache->Clear();
    db->backupCache->Clear();

    return setIntResult (ti, currentBase + 1);
}

int
sc_createbase (Tcl_Interp * ti, const char * filename, scidBaseT * base,
               bool memoryOnly)
{
    if (base->inUse) { return TCL_ERROR; }

    base->idx->SetFileName (filename);
    base->idx->SetDescription ("");
    base->nb->Clear();
    base->nb->SetFileName (filename);
    base->fileMode = FMODE_Both;
    base->memoryOnly = false;

    if (memoryOnly) {
        base->memoryOnly = true;
        base->gfile->CreateMemoryOnly();
        base->idx->CreateMemoryOnly();
        base->idx->SetDescription (errMsgReadOnly(ti));
        base->fileMode = FMODE_ReadOnly;

    } else {
        if (base->idx->CreateIndexFile (FMODE_Both) != OK) {
            return errorResult (ti, "Error creating index file.");
        }

        base->idx->WriteHeader();
        if (base->nb->WriteNameFile() != OK) {
            return errorResult (ti, "Error creating name file.");
        }
        base->idx->ReadEntireFile();

        if (base->gfile->Create (filename, FMODE_Both) != OK) {
            return errorResult (ti, "Error creating game file.");
        }
    }

    // Initialise the filter:
    base->numGames = base->idx->GetNumGames();
    clearFilter (base, base->numGames);

    strCopy (base->fileName, filename);
    base->inUse = true;
    base->gameNumber = -1;
    if (base->treeCache == NULL) {
        base->treeCache = new TreeCache;
        base->treeCache->SetCacheSize (SCID_TreeCacheSize);
        base->backupCache = new TreeCache;
        base->backupCache->SetCacheSize (SCID_BackupCacheSize);
        base->backupCache->SetPolicy (TREECACHE_Oldest);
    }
    base->treeCache->Clear();
    base->backupCache->Clear();
    recalcFlagCounts (base);

    // Ensure an old treefile is not still around:
    if (!memoryOnly) { removeFile (base->fileName, TREEFILE_SUFFIX); }
    return TCL_OK;
}

//    Create a new empty database from a pgn(?) file

int
sc_base_create (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    // argc should be 3 or 4, e.g. "sc_base create <myfile> [<boolean>]"
    if (argc != 3  &&  argc != 4) {
        return errorResult (ti, "No file selected");
    }
    bool memoryOnly = false;
    if (argc == 4) {
        memoryOnly = strGetBoolean (argv[3]);
    }

    // Check that this base is not already opened:
    if (base_opened (argv[2]) >= 0) {
        return errorResult (ti, "The database you selected is already opened.");
    }

    // Find another slot if current slot is used:
    int newBaseNum = currentBase;
    if (db->inUse) {
        newBaseNum = findEmptyBase();
        if (newBaseNum == -1) {
            return errorResult (ti, "You have too many open databases; close one first.");
        }
    }

    scidBaseT * baseptr = &(dbList[newBaseNum]);
    if (sc_createbase (ti, argv[2], baseptr, memoryOnly) != TCL_OK) {
        return TCL_ERROR;
    }
    currentBase = newBaseNum;

    db = baseptr;

    // strCopy (db->fileName, argv[2]);
    // (in sc_base_open, this only differs by *not* having the si4 extension)

    // setting realFileName for pgn files also has the effect that they (now) can't be deleted if open.
    strCopy (db->realFileName, argv[2]);
    db->inUse = true;

    return setIntResult (ti, newBaseNum + 1);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_close:
//    Closes the current or specified database.
int
sc_base_close (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * basePtr = db;
    if (argc > 2) {
        int baseNum = strGetInteger (argv[2]);
        if (baseNum < 1 || baseNum > MAX_BASES) {
            return errorResult (ti, "Invalid database number.");
        }
        basePtr = &(dbList[baseNum - 1]);
    }

    if (!basePtr->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    sc_game_undo_reset (basePtr);

    // If the database is the clipbase, do not close it, just clear it:
    if (basePtr == clipbase) { return sc_clipbase_clear (ti); }
    basePtr->idx->CloseIndexFile();
    basePtr->idx->Clear();
    basePtr->nb->Clear();
    basePtr->gfile->Close();
    basePtr->idx->SetDescription (errMsgNotOpen(ti));

    clearFilter (basePtr, 0);

    if (basePtr->duplicates != NULL) {
        delete[] basePtr->duplicates;
        basePtr->duplicates = NULL;
    }

    basePtr->inUse = false;
    basePtr->gameNumber = -1;
    basePtr->numGames = 0;
    recalcFlagCounts (basePtr);
    strCopy (basePtr->fileName, "<empty>");
    basePtr->treeCache->Clear();
    basePtr->backupCache->Clear();
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_count:
//    Return count of free/used/total base slots.
int
sc_base_count (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = { "free", "used", "total" };
    enum { OPT_FREE, OPT_USED, OPT_TOTAL };
    int optionMode = OPT_USED;

    if (argc > 2) {
        optionMode = strUniqueMatch (argv[2], options);

        if (optionMode < OPT_FREE || argc > 3) {
            return errorResult (ti, "Usage: sc_base count [free|used|total]");
        }
    }

    if (optionMode == OPT_TOTAL) {
        return setUintResult (ti, MAX_BASES);
    }

    int numUsed = 0, numFree = 0;
    for (int i=0; i < MAX_BASES; i++) {
        if (dbList[i].inUse) { numUsed++; } else { numFree++; }
    }
    return setIntResult (ti, optionMode == OPT_USED ? numUsed : numFree);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_description:
//   Sets or gets the description for the database.
int
sc_base_description (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc < 2  ||  argc > 3) {
        return errorResult (ti, "Usage: sc_base description [<text>]");
    }
    if (argc == 2) {
        // Get description:
        Tcl_AppendResult (ti, db->idx->GetDescription(), NULL);
        return TCL_OK;
    }
    if (! db->inUse) {
        return setResult (ti, errMsgNotOpen(ti));
    }
    if (db->fileMode == FMODE_ReadOnly) {
        return setResult (ti, errMsgReadOnly(ti));
    }
    // Edit the description and return it:
    db->idx->SetDescription (argv[2]);
    db->idx->WriteHeader ();
    Tcl_AppendResult (ti, db->idx->GetDescription(), NULL);
    return TCL_OK;
}


int
sc_base_check (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = startProgressBar();
    uint update = 5000;
    uint updateStart = 5000;
    IndexEntry *ie = NULL;
    char gameNumber[16];
    bool limitToFilter = false;

    if (argc != 3) {
        return errorResult (ti, "Usage: sc_base check <bool:all_games>");
    }

    Game *g = new Game();

    if (argv[2][0] == 'f')
        limitToFilter = true;

    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    DString *ErrorBuffer = new DString;

    for (uint gameNum=0; gameNum < db->numGames; gameNum++) {

        if (showProgress) {
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, gameNum, db->numGames);
                if (interruptedProgress()) { break; }
            }
        }

        if (limitToFilter  &&  db->filter->Get(gameNum) == 0) { continue; }

        ie = db->idx->FetchEntry (gameNum);
        if (ie->GetLength() == 0) {
            sprintf( gameNumber, "%d", gameNum + 1);
            ErrorBuffer->Append ("Game ", gameNumber, ": Unable to fetch index entry.\n");
            continue;
        }

        if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(), ie->GetLength()) != OK) {
            sprintf( gameNumber, "%d", gameNum + 1);
            ErrorBuffer->Append ("Game ", gameNumber, ": Unable to read game buffer.\n");
            continue;
        }

        errorT ret = g->Decode (db->bbuf, GAME_DECODE_ALL);
        if( ret != OK){
            sprintf( gameNumber, "%d", gameNum + 1);
            ErrorBuffer->Append ("Game ", gameNumber, ": Unable to decode game.\n");
            continue;
        }
    }

    if (showProgress) { updateProgressBar (ti, 1, 1); }

    if (ErrorBuffer->Length() > 0) {
        Tcl_AppendElement (ti, ErrorBuffer->Data());
    }
    return (ErrorBuffer->Length() == 0) ? TCL_OK : TCL_ERROR;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  exportGame:
//    Called by sc_base_export() to export a single game.
static void
exportGame (Game * g, FILE * exportFile, gameFormatT format, uint pgnStyle, CharsetConverter* converter)
{
    char old_language = language;

    db->tbuf->Empty();

    g->ResetPgnStyle (pgnStyle);
    g->SetPgnFormat (format);

    // Format-specific settings:
    switch (format) {
    case PGN_FORMAT_HTML:
	g->SetHtmlStyle (htmlDiagStyle);
    case PGN_FORMAT_Latex:
        db->tbuf->NewlinesToSpaces (false);
        g->AddPgnStyle (PGN_STYLE_SHORT_HEADER);
        language = 0;
        break;
    default:
        g->AddPgnStyle (PGN_STYLE_STRIP_BRACES);
        language = 0;
        break;
    }

    g->WriteToPGN (db->tbuf);
    db->tbuf->NewLine();

#ifndef TCL_ONLY
    if (converter) {
        converter->doConversion(*db->tbuf);
    }
#endif

    db->tbuf->DumpToFile (exportFile);
    language = old_language;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_export:
//    Exports the current game or all filter games in the database
//    to a PGN, HTML or LaTeX file.
int
sc_base_export (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = startProgressBar();
    FILE * exportFile = NULL;

    bool exportFilter = false;
    bool appendToFile = false;
    bool newlines = true;
    gameFormatT outputFormat = PGN_FORMAT_Plain;
    const char * startText = "";
    const char * endText = "";
    const char * usage = "Usage: sc_base export current|filter PGN|HTML|Latex <pgn_filename> options...";
    uint pgnStyle = PGN_STYLE_TAGS;

    const char * options[] = {
        "-append", "-starttext", "-endtext", "-comments", "-variations",
        "-spaces", "-symbols", "-indentComments", "-indentVariations", "-newlines",
        "-column", "-noMarkCodes", "-scidFlags", "-convertNullMoves", "-utf8", NULL
    };
    enum {
        OPT_APPEND, OPT_STARTTEXT, OPT_ENDTEXT, OPT_COMMENTS, OPT_VARIATIONS,
        OPT_SPACES, OPT_SYMBOLS, OPT_INDENTC, OPT_INDENTV, OPT_NEWLINES,
        OPT_COLUMN, OPT_NOMARKS, OPT_SCIDFLAGS, OPT_CONVERTNULL, OPT_UTF8
    };

    if (argc < 5) { return errorResult (ti, usage); }

    if (strIsPrefix (argv[2], "current")) {
        exportFilter = false;
    } else if (strIsPrefix (argv[2], "filter")) {
        exportFilter = true;
    } else {
        return errorResult (ti, usage);
    }

    if (! Game::PgnFormatFromString (argv[3], &outputFormat)) {
        return errorResult (ti, usage);
    }

    if (exportFilter  &&  !db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    const char * exportFileName = argv[4];
    bool useUTF8 = false;

    // Check for an even number of optional parameters:
    if ((argc % 2) != 1) { return errorResult (ti, usage); }

    // Parse all optional parameters:
    for (int arg = 5; arg < argc; arg += 2) {
        const char * value = argv[arg+1];
        bool flag = strGetBoolean (value);
        int option = strUniqueMatch (argv[arg], options);

        switch (option) {
        case OPT_APPEND:
            appendToFile = flag;
            break;

        case OPT_STARTTEXT:
            startText = value;
            break;

        case OPT_ENDTEXT:
            endText = value;
            break;

        case OPT_COMMENTS:
            if (flag) { pgnStyle |= PGN_STYLE_COMMENTS; }
            break;

        case OPT_VARIATIONS:
            if (flag) { pgnStyle |= PGN_STYLE_VARS; }
            break;

        case OPT_SPACES:
            if (flag) { pgnStyle |= PGN_STYLE_MOVENUM_SPACE; }
            break;

        case OPT_SYMBOLS:
            if (flag) { pgnStyle |= PGN_STYLE_SYMBOLS; }
            break;

        case OPT_INDENTC:
            if (flag) { pgnStyle |= PGN_STYLE_INDENT_COMMENTS; }
            break;

        case OPT_INDENTV:
            if (flag) { pgnStyle |= PGN_STYLE_INDENT_VARS; }
            break;

        case OPT_NEWLINES:
            newlines = flag;
            break;

        case OPT_COLUMN:
            if (flag) { pgnStyle |= PGN_STYLE_COLUMN; }
            break;

        case OPT_NOMARKS:
            if (flag) { pgnStyle |= PGN_STYLE_STRIP_MARKS; }
            break;

        case OPT_SCIDFLAGS:
            if (flag) { pgnStyle |= PGN_STYLE_SCIDFLAGS; }
            break;

        case OPT_CONVERTNULL:
            if (flag) { pgnStyle |= PGN_STYLE_NO_NULL_MOVES; }
            break;

        case OPT_UTF8:
            if (flag) { useUTF8 = true; }
            break;

        default:
            return InvalidCommand (ti, "sc_base export", options);
        }
    }
    exportFile = fopen (exportFileName, (appendToFile ? "r+" : "w"));
    if (exportFile == NULL) {
        return errorResult (ti, "Error opening file for exporting games.");
    }

    CharsetConverter* charsetConverter = NULL;

    // Write start text or find the place in the file to append games:
    if (appendToFile) {
        if (outputFormat == PGN_FORMAT_Plain) {
            // Look whether this file is UTF-8 or Latin-1.
            unsigned char buf[3] = { 0, 0, 0 };
            fseek (exportFile, 0, SEEK_SET);
            fread (reinterpret_cast<char*>(buf), 1, 3, exportFile);
            useUTF8 = (buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF);
            fseek (exportFile, 0, SEEK_END);
        } else {
            fseek (exportFile, 0, SEEK_SET);
            const char * endMarker = "";
            if (outputFormat == PGN_FORMAT_HTML) {
                endMarker = "</body>";
            } else if (outputFormat == PGN_FORMAT_Latex) {
                endMarker = "\\end{document}";
            }
            char line [1024];
            uint pos = 0;
            while (1) {
                fgets (line, 1024, exportFile);
                if (feof (exportFile)) { break; }
                const char * s = strTrimLeft (line, " ");
                if (strIsCasePrefix (endMarker, s)) {
                    // We have seen the line to stop at, so break out
                    break;
                }
                pos = ftell (exportFile);
            }
            fseek (exportFile, pos, SEEK_SET);
        }
    } else {
        if (outputFormat == PGN_FORMAT_Plain && useUTF8) {
            // Write UTF-8 BOM like ChessBase is doing
            fputs ("\xef\xbb\xbf", exportFile);
         }
        fputs (startText, exportFile);
    }

    if (outputFormat == PGN_FORMAT_Plain || useUTF8) {
        charsetConverter = new CharsetConverter(useUTF8 ? "utf-8" : "iso8859-1");
        // This 75 from the original... but Gregor moved it slightly. WTF ?
        // Note, we now reset this if no newlines... it never gets reset elsewhere - S.A
        if (newlines)
          db->tbuf->SetWrapColumn (75);
        else
          db->tbuf->SetWrapColumn (99999);
    }

    if (!exportFilter) {
        // Only export the current game:
        exportGame (db->game, exportFile, outputFormat, pgnStyle, charsetConverter);
        fputs (endText, exportFile);
        fclose (exportFile);
        if (showProgress) { updateProgressBar (ti, 1, 1); }
        delete charsetConverter;
        return TCL_OK;
    }

    Game * g = scratchGame;
    IndexEntry * ie;
    uint updateStart, update;
    updateStart = update = 100;  // Update progress bar every 10 games
    uint numSeen = 0;
    uint numToExport = exportFilter ? db->filter->Count() : 1;

    for (uint i=0; i < db->numGames; i++) {
        if (db->filter->Get(i)) { // Export this game:
            numSeen++;
            if (showProgress) {  // Update the percentage done bar:
                update--;
                if (update == 0) {
                    update = updateStart;
                    updateProgressBar (ti, numSeen, numToExport);
                    if (interruptedProgress()) { break; }
                }
            }

            // Print the game, skipping any corrupt games:
            ie = db->idx->FetchEntry (i);
            if (ie->GetLength() == 0) { continue; }
            db->bbuf->Empty();
            if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(),
                                     ie->GetLength()) != OK) {
                continue;
            }
            if (g->Decode (db->bbuf, GAME_DECODE_ALL) != OK) {
                continue;
            }
            g->LoadStandardTags (ie, db->nb);
            exportGame (g, exportFile, outputFormat, pgnStyle, charsetConverter);
        }
    }
    fputs (endText, exportFile);
    fclose (exportFile);
    if (showProgress) { updateProgressBar (ti, 1, 1); }
    delete charsetConverter;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_import:
//    Imports games from a PGN file to the current base.
//    Returns an error message if there was any file error.
//    On success, returns a list of two elements: the number of
//    games imported, and a string containing an PGN import errors
//    or warnings.
int
sc_base_import (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_base import file|data <pgnFile|pgnData>";
    static const char * options[] = { "data", "file", NULL };
    enum { IMPORT_OPT_DATA, IMPORT_OPT_FILE };

    bool showProgress = startProgressBar();

    if (argc != 4) { return errorResult (ti, usage); }
    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    // Cannot import into a read-only database unless it is memory-only:
    if (db->fileMode == FMODE_ReadOnly  &&  !(db->memoryOnly)) {
        return errorResult (ti, errMsgReadOnly(ti));
    }

    MFile pgnFile;
    uint inputLength = 0;
    PgnParser parser;

    int index = strUniqueMatch (argv[2], options);

    if (index == IMPORT_OPT_FILE) {
        if (pgnFile.Open (argv[3], FMODE_ReadOnly) != OK) {
            return errorResult (ti, "Error opening PGN file.");
        }
        parser.Reset (&pgnFile);
        inputLength = fileSize (argv[3], "");
    } else if (index == IMPORT_OPT_DATA) {
        parser.Reset ((const char *) argv[3]);
        inputLength = strLength (argv[3]);
    } else {
        return errorResult (ti, usage);
    }

    if (inputLength < 1) { inputLength = 1; }
    parser.IgnorePreGameText();
    uint gamesSeen = 0;

    while (parser.ParseGame (scratchGame) != ERROR_NotFound) {
        if (sc_savegame (ti, scratchGame, 0, db) != TCL_OK) {
          // quick and nasty cleanup aka below
          db->gfile->FlushAll();
          pgnFile.Close();
          db->idx->WriteHeader();
          if (! db->memoryOnly) db->nb->WriteNameFile();
          recalcFlagCounts (db);
          if (! db->memoryOnly) removeFile (db->fileName, TREEFILE_SUFFIX);

          Tcl_AppendResult (ti, "Error saving game in database.\n", NULL);
          return TCL_ERROR;
        }
        // Update the progress bar:
        gamesSeen++;
        if (showProgress  &&  (gamesSeen % 100) == 0) {
            if (interruptedProgress()) { break; }
            updateProgressBar (ti, parser.BytesUsed(), inputLength);
        }
    }

    db->gfile->FlushAll();
    pgnFile.Close();

    // Now write the Index file header and the name file:
    if (db->idx->WriteHeader() != OK) {
        return errorResult (ti, "Error writing index file.");
    }
    if (! db->memoryOnly) {
        if (db->nb->WriteNameFile() != OK) {
            return errorResult (ti, "Error writing name file.");
        }
    }

    if (showProgress) { updateProgressBar (ti, 1, 1); }
    recalcFlagCounts (db);
    if (! db->memoryOnly) { removeFile (db->fileName, TREEFILE_SUFFIX); }

    appendUintElement (ti, gamesSeen);
    if (parser.ErrorCount() > 0) {
        Tcl_AppendElement (ti, parser.ErrorMessages());
    } else {
        Tcl_AppendElement (ti, "");
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_piecetrack:
//    Examines games in the filter of the current database and
//    returns a list of 64 integers indicating how frequently
//    the specified piece moves to each square.
int
sc_base_piecetrack (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = startProgressBar();

    const char * usage =
        "Usage: sc_base piecetrack [-g|-t] <minMoves> <maxMoves> <startSquare ...>";

    if (argc < 5) {
        return errorResult (ti, usage);
    }

    // Check for optional mode parameter:
    bool timeOnSquareMode = false;
    int arg = 2;
    if (argv[arg][0] == '-') {
        if (argv[arg][1] == 'g'  && strIsPrefix (argv[arg], "-games")) {
            timeOnSquareMode = false;
            arg++;
        } else if (argv[arg][1] == 't'  && strIsPrefix (argv[arg], "-time")) {
            timeOnSquareMode = true;
            arg++;
        } else {
            return errorResult (ti, usage);
        }
    }

    // Read the two move-number parameters:
    uint minPly = strGetUnsigned(argv[arg]) * 2;
    arg++;
    uint maxPly = strGetUnsigned(argv[arg]) * 2;
    arg++;

    // Convert moves to plycounts, e.g. "5-10" -> "9-20"
    if (minPly < 2) { minPly=2; }
    if (maxPly < minPly) { maxPly = minPly; }
    minPly--;

    // Parse the variable number of tracked square arguments:
    uint sqFreq[64] = {0};
    bool trackSquare[64] = { false };
    int nTrackSquares = 0;
    for (int a=arg; a < argc; a++) {
        squareT sq = strGetSquare (argv[a]);
        if (sq == NULL_SQUARE) { return errorResult (ti, usage); }
        if (!trackSquare[sq]) {
            // Seen another starting square to track.
            trackSquare[sq] = true;
            nTrackSquares++;
        }
    }

    // If current base is unused, filter is empty, or no track
    // squares specified, then just return a zero-filled list:

    if (! db->inUse  ||  db->filter->Count() == 0  ||  nTrackSquares == 0) {
        for (uint i=0; i < 64; i++) { appendUintElement (ti, 0); }
        return TCL_OK;
    }

    // Examine every filter game and track the selected pieces:

    uint updateStart = 1000;  // Update progress bar every 1000 filter games.
    uint update = updateStart;
    uint filterCount = db->filter->Count();
    uint filterSeen = 0;

    for (uint gnum = 0; gnum < db->numGames; gnum++) {
        // Skip over non-filter games:
        if (!db->filter->Get(gnum)) { continue; }

        // Update progress bar:
        if (showProgress) {
            update--;
            filterSeen++;
            if (update == 0) {
                update = updateStart;
                if (interruptedProgress()) { break; }
                updateProgressBar (ti, filterSeen, filterCount);
            }
        }

        IndexEntry * ie = db->idx->FetchEntry (gnum);

        // Skip games with non-standard start or no moves:
        if (ie->GetStartFlag()) { continue; }
        if (ie->GetLength() == 0) { continue; }

        // Skip games too short to be useful:
        if (ie->GetNumHalfMoves() < minPly) { continue; }

        // Set up piece tracking for this game:
        bool movedTo[64] = { false };
        bool track[64];
        int ntrack = nTrackSquares;
        for (uint sq=0; sq < 64; sq++) { track[sq] = trackSquare[sq]; }

        Game * g = scratchGame;
        if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(),
                                 ie->GetLength()) != OK) {
            continue;
        }
        db->bbuf->BackToStart();
        g->Clear();
        if (g->DecodeStart (db->bbuf) != OK) { continue; }

        uint plyCount = 0;
        simpleMoveT sm;

        // Process each game move until the maximum ply or end of
        // the game is reached:

        while (plyCount < maxPly) {
            if (g->DecodeNextMove (db->bbuf, &sm) != OK) { break; }
            plyCount++;
            squareT toSquare = sm.to;
            squareT fromSquare = sm.from;

            // Special hack for castling:
            if (piece_Type(sm.movingPiece) == KING) {
                if (fromSquare == E1) {
                    if (toSquare == G1  &&  track[H1]) {
                        fromSquare = H1; toSquare = F1;
                    }
                    if (toSquare == C1  &&  track[A1]) {
                        fromSquare = A1; toSquare = D1;
                    }
                }
                if (fromSquare == E8) {
                    if (toSquare == G8  &&  track[H8]) {
                        fromSquare = H8; toSquare = F8;
                    }
                    if (toSquare == C8  &&  track[A8]) {
                        fromSquare = A8; toSquare = D8;
                    }
                }
            }

            // TODO: Special hack for en-passant capture?

            if (track[toSquare]) {
                // A tracked piece has been captured:
                track[toSquare] = false;
                ntrack--;
                if (ntrack <= 0) { break; }

            } else if (track[fromSquare]) {
                // A tracked piece is moving:
                track[fromSquare] = false;
                track[toSquare] = true;
                if (plyCount >= minPly) {
                    // If not time-on-square mode, and this
                    // new target square has not been moved to
                    // already by a tracked piece in this game,
                    // increase its frequency now:
                    if (!timeOnSquareMode  && !movedTo[toSquare]) {
                        sqFreq[toSquare]++;
                    }
                    movedTo[toSquare] = true;
                }
            }

            if (timeOnSquareMode  &&  plyCount >= minPly) {
                // Time-on-square mode: find all tracked squares
                // (there are ntrack of them) and increment the
                // frequency of each.
                int nleft = ntrack;
                for (uint i=0; i < 64; i++) {
                    if (track[i]) {
                        sqFreq[i]++;
                        nleft--;
                        // We can stop early when all tracked
                        // squares have been found:
                        if (nleft <= 0) { break; }
                    }
                }
            }
        } // while (plyCount < maxPly)
    } // for (uint gnum = 0; gnum < db->numGames; gnum++)

    if (showProgress) { updateProgressBar (ti, 1, 1); }

    // Now return the 64-integer list: if in time-on-square mode,
    // the value for each square is the number of plies when a
    // tracked piece was on it, so halve it to convert to moves:

    for (uint i=0; i < 64; i++) {
        appendUintElement (ti, timeOnSquareMode ? sqFreq[i] / 2 : sqFreq[i]);
    }
    return TCL_OK;
}

//    Sorts the games in a database.

int
sc_base_sort (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = startProgressBar();

    if (argc < 3 || argc > 4) {
        return errorResult (ti, "Usage: sc_base sort <criteria> [script]");
    }
    if (db->idx->ParseSortCriteria (argv[2]) != OK) {
        return errorResult (ti, "Invalid sorting criteria.");
    }
    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (db->numGames < 2) {
        return TCL_OK;
    }

    errorT status;
    if (showProgress) {
        status = db->idx->Sort (db->nb, 5000, base_progress, (void *) ti);
    } else {
        status = db->idx->Sort (db->nb, 0, NULL, NULL);
    }

    // if (status == ERROR )
    // ERROR define seems broken with mingwx
    if (status != OK ) {
      return errorResult (ti, "Sort Failed.");
    }

    if (showProgress) { updateProgressBar (ti, 1, 1); }

    Filter *newfilter = db->filter->Clone();
    int thisgame = -1;
    uint * order = db->idx->GetEntriesHeap();
    if (order) {
      for (uint i=0; i < db->numGames; i++) {
	 newfilter->Set(i, db->filter->Get(order[i]));
	 if (order[i] == (uint) db->gameNumber)
	   thisgame = i;
      }
    }

    if (db->filter == db->dbFilter) {
      delete db->filter;
      db->filter   = newfilter;
      db->dbFilter = newfilter;
    } else {
      // todo - sort out how the tree behaves.
      delete db->filter;
      delete db->dbFilter;
      delete db->treeFilter;
      db->filter   = newfilter;
      db->dbFilter = db->filter->Clone();
      db->treeFilter = new Filter( db->numGames);
    }

    // Re-order and write the index, showing progress if applicable:
    if (argc == 4  &&  showProgress) {
        Tcl_Eval (ti, (char *) argv[3]);
        restartProgressBar (ti);
        db->idx->WriteSorted (20000, base_progress, (void *) ti);
        updateProgressBar (ti, 1, 1);
    } else {
        db->idx->WriteSorted ();
    }
    // The tree cache will now be out of date:
    db->treeCache->Clear();
    db->backupCache->Clear();
    if ((! db->memoryOnly)  &&  (db->fileMode == FMODE_Both)) {
        removeFile (db->fileName, TREEFILE_SUFFIX);
    }

    // sprintf (loadgame, "sc_game load %i", thisgame+1);
    // Tcl_Eval (ti, loadgame);

    // Current game state has not be changed, so just reassign db->gameNumber
    // Any bugs/issues ??
    db->gameNumber = thisgame;

    return TCL_OK;
}

int
sc_base_sortup (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    db->nb->SortOrder = 0;
    return TCL_OK;
}

int
sc_base_sortdown (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    db->nb->SortOrder = 1;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_stats:
//    Return statistics about the current database.
int
sc_base_stats (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * options[] = { "flags", "dates", "ratings", "results", NULL };
    enum { OPT_FLAGS, OPT_DATE, OPT_RATING, OPT_RESULTS };

    int option = -1;
    if (argc > 2) { option = strUniqueMatch (argv[2], options); }

    scidBaseT * basePtr = db;
    if (argc > 3) {
        int baseNum = strGetInteger (argv[3]);
        if (baseNum < 1 || baseNum > MAX_BASES) {
            return errorResult (ti, "Invalid database number.");
        }
        basePtr = &(dbList[baseNum - 1]);
    }

    if (option == OPT_FLAGS) {
        appendUintElement (ti, basePtr->stats.flagCount [IDX_FLAG_DELETE]);
        appendUintElement (ti, basePtr->stats.flagCount [IDX_FLAG_WHITE_OP]);
        appendUintElement (ti, basePtr->stats.flagCount [IDX_FLAG_BLACK_OP]);

    } else if (option == OPT_DATE) {
        // Date information: minimum year, maximum year, and mean year:
        appendUintElement (ti, date_GetYear (basePtr->stats.minDate));
        appendUintElement (ti, date_GetYear (basePtr->stats.maxDate));
        uint avgYear = 0;
        if (basePtr->stats.nYears > 0) {
            avgYear = basePtr->stats.sumYears / basePtr->stats.nYears;
        }
        appendUintElement (ti, avgYear);

    } else if (option == OPT_RATING) {
        // Rating information: minimum, maximum, and mean rating:
        appendUintElement (ti, basePtr->stats.minRating);
        appendUintElement (ti, basePtr->stats.maxRating);
        uint avgRating = 0;
        if (basePtr->stats.nRatings > 0) {
            avgRating = basePtr->stats.sumRatings / basePtr->stats.nRatings;
        }
        appendUintElement (ti, avgRating);

    } else if (option == OPT_RESULTS) {
        // Result frequencies: 1-0, =-=, 0-1, *
        appendUintElement (ti, basePtr->stats.nResults[RESULT_White]);
        appendUintElement (ti, basePtr->stats.nResults[RESULT_Draw]);
        appendUintElement (ti, basePtr->stats.nResults[RESULT_Black]);
        appendUintElement (ti, basePtr->stats.nResults[RESULT_None]);

    } else if (strIsPrefix ("flag:", argv[2])) {
        uint flag = IndexEntry::CharToFlag (argv[2][5]);
        appendUintElement (ti, basePtr->stats.flagCount [flag]);

    } else {
        return InvalidCommand (ti, "sc_base stats", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_ecoStats:
//    Return ECO opening code statistics about the current database.
int
sc_base_ecoStats (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_base ecoStats <ECO-prefix>");
    }
    const char * eco = argv[2];
    uint length = strLength (eco);
    bool invalid = false;
    int index = 0;
    if (length > 5) { invalid = true; }
    if (length >= 1) {
        if (eco[0] < 'A' || eco[0] > 'E') { invalid = true; }
        index = eco[0] - 'A';
    }
    if (length >= 2) {
        char ch = eco[1];
        if (ch < '0'  ||  ch > '9') { invalid = true; } else {
            index = index * 10 + ch - '0';
        }
    }
    if (length >= 3) {
        char ch = eco[2];
        if (ch < '0'  ||  ch > '9') { invalid = true; } else {
            index = index * 10 + ch - '0';
        }
    }
    if (length >= 4) {
        char ch = eco[3];
        if (ch < 'a'  ||  ch > 'z') { invalid = true; } else {
            index = index * 26 + ch - 'a';
        }
    }
    if (invalid) {
        return errorResult (ti, "Invalid ECO prefix");
    }

    ecoStatsT * result = NULL;
    switch (length) {
    case 0:
        result = &(db->stats.ecoCount0[0]);
        break;
    case 1:
        result = &(db->stats.ecoCount1[index]);
        break;
    case 2:
        result = &(db->stats.ecoCount2[index]);
        break;
    case 3:
        result = &(db->stats.ecoCount3[index]);
        break;
    case 4:
    case 5:
        result = &(db->stats.ecoCount4[index]);
        break;
    }
    ASSERT (result != NULL);
    appendUintElement (ti, result->count);
    appendUintElement (ti, result->results[RESULT_White]);
    appendUintElement (ti, result->results[RESULT_Draw]);
    appendUintElement (ti, result->results[RESULT_Black]);
    appendUintElement (ti, result->results[RESULT_None]);
    if (result->count > 0) {
        uint score = result->results[RESULT_White] * 2
            + result->results[RESULT_Draw] + result->results[RESULT_None];
        score = score * 500 / result->count;
        char str[10];
        sprintf (str, "%u.%u", score / 10, score % 10);
        Tcl_AppendElement (ti, str);
    } else {
        Tcl_AppendElement (ti, "0.0");
    }
    return TCL_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_switch:
//    Switch to a different database slot.
int
sc_base_switch (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_base switch <number>");
    }
    int baseNum = strGetInteger (argv[2]);
    if (tolower(argv[2][0]) == 'c'  &&  strIsCasePrefix (argv[2], "clipbase")) {
        baseNum = CLIPBASE_NUM + 1;
    }
    if (baseNum < 1  ||  baseNum > MAX_BASES) {
        return errorResult (ti, "sc_base switch: Invalid base number.");
    }

    // Should we do this check here ? Some places (eg file::Exit)
    // switch to unopened bases then do a 'base inUse' check
    // if (! dbList[baseNum - 1].inUse) 
    //   return errorResult (ti, "sc_base switch: This base is not open.");

    currentBase = baseNum - 1;
    db = &(dbList[currentBase]);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_type:
//    Get or set the "type" of the database: clipbase, temporary, openings,
//    tournament, etc.
//    The type can be set for a read-only or memory-only database, but the
//    change will only be temporary since the index will not be altered
//    on-disk.
int
sc_base_type (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3  &&  argc != 4) {
        return errorResult (ti, "Usage: sc_base type <number> [<type>]");
    }

    int baseNum = strGetInteger (argv[2]);
    if (baseNum < 1 || baseNum > MAX_BASES) {
        return errorResult (ti, "Invalid database number.");
    }
    scidBaseT * basePtr = &(dbList[baseNum - 1]);

    if (argc == 3) {
        return setUintResult (ti, basePtr->idx->GetType());
    }

    if (! basePtr->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    uint basetype = strGetUnsigned (argv[3]);
    basePtr->idx->SetType (basetype);
    if ((basePtr->fileMode != FMODE_ReadOnly) && (! basePtr->memoryOnly)) {
        // Update the index header on disk:
        basePtr->idx->WriteHeader();
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_duplicates:
//    Finds duplicate games and marks them deleted.
//    A pair of games are considered duplicates if the Event, Site,
//    White, Black, and Round values all match identically, and the
//    Date matches to within 2 days (that is, the same year, the same
//    month, and the days of month differ by 2 at most).
//
//    Furthermore, the moves of one game should, after truncating, be the
//    same as the moves of the other game, for them to be duplicates.
//    This is only checked after first checking that their home pawn change
//    lists are the same (for the length of the shorter change list) which
//    is an approximation but is *much* faster.

struct gNumListT {
    uint gNumber;
    uint white;
    uint black;
    gNumListT * next;
};
typedef gNumListT * gNumListPtrT;

struct dupCriteriaT {
    bool exactNames;
    bool sameColors;
    bool sameEvent;
    bool sameSite;
    bool sameRound;
    bool sameResult;
    bool sameYear;
    bool sameMonth;
    bool sameDay;
    bool sameEcoCode;
    bool sameMoves;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// hashName:
//    Returns a hash value based on the first n letters of a string.
uint
hashName (const char * name, uint n)
{
    uint h = 0;
    while (n > 0  &&  *name != 0) {
        h = (h << 7) + (*name);
        name++;
        n--;
    }
    return h;
}

bool
gamesHaveSameMoves (scidBaseT * base, IndexEntry * ieA, IndexEntry * ieB)
{
    const uint MAX_SAME_MOVES = 120;  // Only check up to 60 moves each side.
    simpleMoveT movesA [MAX_SAME_MOVES];
    simpleMoveT movesB [MAX_SAME_MOVES];

    // Start with the shorter game first:
    uint lenA = ieA->GetNumHalfMoves();
    uint lenB = ieB->GetNumHalfMoves();
    if (lenB < lenA) {  // Swap the order of the two games:
        uint temp = lenA; lenA = lenB; lenB = temp;
        IndexEntry * ie = ieA; ieA = ieB; ieB = ie;
    }

    // Now load up to MAX_SAME_MOVES of the first game:
    Game * g = scratchGame;

    if (base->gfile->ReadGame (base->bbuf, ieA->GetOffset(),
                               ieA->GetLength()) != OK) {
        return false;
    }
    base->bbuf->BackToStart();
    g->Clear();
    if (g->DecodeStart (base->bbuf) != OK) {
        return false;
    }
    simpleMoveT * smA = &(movesA[0]);
    uint countA = 0;
    while (countA < MAX_SAME_MOVES) {
        if (g->DecodeNextMove (base->bbuf, smA) != OK) { break; }
        countA++;
        smA++;
    }

    // Now read the same number of moves in the longer game, stopping when
    // a different move is found:
    if (base->gfile->ReadGame (base->bbuf, ieB->GetOffset(),
                               ieB->GetLength()) != OK) {
        return false;
    }
    base->bbuf->BackToStart();
    g->Clear();
    if (g->DecodeStart (base->bbuf) != OK) {
        return false;
    }
    smA = &(movesA[0]);
    simpleMoveT * smB = &(movesB[0]);
    uint countB = 0;
    while (countB < countA) {
        if (g->DecodeNextMove (base->bbuf, smB) != OK) { return false; }
        if (smA->from != smB->from  ||  smA->to != smB->to  ||
            smA->promote != smB->promote) { return false; }
        countB++;
        smA++;
        smB++;
    }
    // If we reach here, the games are identical for all moves in the
    // shorter game.
    return true;
}

bool
checkDuplicate (scidBaseT * base,
                IndexEntry * ie1, IndexEntry * ie2,
                gNumListT * g1, gNumListT * g2,
                dupCriteriaT * cr)
{
    if (ie1->GetDeleteFlag()  ||  ie2->GetDeleteFlag()) { return false; }
    if (cr->sameColors) {
        if (g1->white != g2->white) { return false; }
        if (g1->black != g2->black) { return false; }
    } else {
        bool colorsOK = false;
        uint w1 = g1->white;
        uint b1 = g1->black;
        uint w2 = g2->white;
        uint b2 = g2->black;
        if (w1 == w2  &&  b1 == b2) { colorsOK = true; }
        if (w1 == b2  &&  b1 == w2) { colorsOK = true; }
        if (! colorsOK) { return false; }
    }
    if (cr->sameEvent) {
        if (ie1->GetEvent() != ie2->GetEvent()) { return false; }
    }
    if (cr->sameSite) {
        if (ie1->GetSite() != ie2->GetSite()) { return false; }
    }
    if (cr->sameRound) {
        if (ie1->GetRound() != ie2->GetRound()) { return false; }
    }
    if (cr->sameYear) {
        if (ie1->GetYear() != ie2->GetYear()) { return false; }
    }
    if (cr->sameMonth) {
        if (ie1->GetMonth() != ie2->GetMonth()) { return false; }
    }
    if (cr->sameDay) {
        if (ie1->GetDay() != ie2->GetDay()) { return false; }
    }
    if (cr->sameResult) {
        if (ie1->GetResult() != ie2->GetResult()) { return false; }
    }
    if (cr->sameEcoCode) {
        ecoStringT a;
        ecoStringT b;
        eco_ToBasicString (ie1->GetEcoCode(), a);
        eco_ToBasicString (ie2->GetEcoCode(), b);
        if (a[0] != b[0]  ||  a[1] != b[1]  ||  a[2] != b[2]) { return false; }
    }

    // There are a lot of "place-holding" games in some database, that have
    // just one (usually wrong) move and a result, that are then replaced by
    // the full version of the game. Therefore, if we reach here and one
    // of the games (or both) have only one move or no moves, return true
    // as long as they have the same year, site and round:

    // Hmm..... Add some sanity checks to this 2 move skip, that whiteplayername, eventname are not "?" - S.A.

    if (ie1->GetNumHalfMoves() <= 2 || ie2->GetNumHalfMoves() <= 2) {
        if (
            strcmp(ie1->GetWhiteName(base->nb),"?") != 0 &&
            strcmp(ie1->GetEventName(base->nb),"?") != 0 &&
            ie1->GetYear() == ie2->GetYear()  &&
            ie1->GetSite() == ie2->GetSite()  &&
            ie1->GetRound() == ie2->GetRound()) {
            return true;
        }
    }

    // Now check that the games contain the same moves, up to the length
    // of the shorter game:

    if (cr->sameMoves) {
        const byte * hpData1 = ie1->GetHomePawnData();
        const byte * hpData2 = ie2->GetHomePawnData();
        if (! hpSig_Prefix (hpData1, hpData2)) { return false; }
        // Now we have to check the actual moves of the games:
        return gamesHaveSameMoves (base, ie1, ie2);
    }
    return true;
}

int
sc_base_duplicates (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = startProgressBar();

    if ((argc % 2) != 0) {
        return errorResult (ti, "Usage: sc_base duplicates [-option value ...]");
    }
    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (db->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }

    uint deletedCount = 0;
    const uint GLIST_HASH_SIZE = 32768;
    gNumListPtrT * gHashTable = new gNumListPtrT [GLIST_HASH_SIZE];
    gNumListT * gNumList = new gNumListT [db->numGames];

    dupCriteriaT criteria;
    criteria.exactNames  = false;
    criteria.sameColors  = true;
    criteria.sameEvent   = true;
    criteria.sameSite    = true;
    criteria.sameRound   = true;
    criteria.sameYear    = true;
    criteria.sameMonth   = true;
    criteria.sameDay     = false;
    criteria.sameResult  = false;
    criteria.sameEcoCode = false;
    criteria.sameMoves = true;

    bool skipShortGames = false;
    bool keepAllCommentedGames = true;
    bool keepAllGamesWithVars  = true;
    bool setFilterToDups = false;
    bool onlyFilterGames = false;
    bool copyRatings = false;

    // Deletion strategy: delete the shorter game, the game with the
    // smaller game number, or the game with the larger game number.
    enum deleteStrategyT { DELETE_SHORTER, DELETE_OLDER, DELETE_NEWER };
    deleteStrategyT deleteStrategy = DELETE_SHORTER;

    // Parse command options in pairs of arguments:

    const char * options[] = {
        "-players", "-colors", "-event", "-site", "-round", "-year",
        "-month", "-day", "-result", "-eco", "-moves", "-skipshort",
        "-comments", "-variations", "-setfilter", "-usefilter",
        "-copyratings", "-delete",
        NULL
    };
    enum {
        OPT_PLAYERS, OPT_COLORS, OPT_EVENT, OPT_SITE, OPT_ROUND, OPT_YEAR,
        OPT_MONTH, OPT_DAY, OPT_RESULT, OPT_ECO, OPT_MOVES, OPT_SKIPSHORT,
        OPT_COMMENTS, OPT_VARIATIONS, OPT_SETFILTER, OPT_USEFILTER,
        OPT_COPYRATINGS, OPT_DELETE
    };

    for (int arg = 2; arg < argc; arg += 2) {
        const char * optStr = argv[arg];
        const char * valueStr = argv[arg + 1];
        bool b = strGetBoolean (valueStr);
        int index = strUniqueMatch (optStr, options);
        switch (index) {
            case OPT_PLAYERS:     criteria.exactNames = b;   break;
            case OPT_COLORS:      criteria.sameColors = b;   break;
            case OPT_EVENT:       criteria.sameEvent = b;    break;
            case OPT_SITE:        criteria.sameSite = b;     break;
            case OPT_ROUND:       criteria.sameRound = b;    break;
            case OPT_YEAR:        criteria.sameYear = b;     break;
            case OPT_MONTH:       criteria.sameMonth = b;    break;
            case OPT_DAY:         criteria.sameDay = b;      break;
            case OPT_RESULT:      criteria.sameResult = b;   break;
            case OPT_ECO:         criteria.sameEcoCode = b;  break;
            case OPT_MOVES:       criteria.sameMoves = b;    break;
            case OPT_SKIPSHORT:   skipShortGames = b;        break;
            case OPT_COMMENTS:    keepAllCommentedGames = b; break;
            case OPT_VARIATIONS:  keepAllGamesWithVars = b;  break;
            case OPT_SETFILTER:   setFilterToDups = b;       break;
            case OPT_USEFILTER:   onlyFilterGames = b;       break;
            case OPT_COPYRATINGS: copyRatings = b;           break;
            case OPT_DELETE:
                if (strIsCasePrefix (valueStr, "shorter")) {
                    deleteStrategy = DELETE_SHORTER;
                } else if (strIsCasePrefix (valueStr, "older")) {
                    deleteStrategy = DELETE_OLDER;
                } else if (strIsCasePrefix (valueStr, "newer")) {
                    deleteStrategy = DELETE_NEWER;
                } else {
                    return errorResult (ti, "Invalid option.");
                }
                break;
            default:
                delete[] gNumList;
                return InvalidCommand (ti, "sc_base duplicates", options);
        }
    }

    // Setup duplicates array:
    if (db->duplicates == NULL) {
        db->duplicates = new uint [db->numGames];
    }
    for (uint d=0; d < db->numGames; d++) {
        db->duplicates[d] = 0;
    }

    // We use a hashtable to limit duplicate game comparisons; each game
    // is only compared to others that hash to the same value.
    // Set up the linked-list hashtable of games with same hashed names:

    for (uint h=0; h < GLIST_HASH_SIZE; h++) { gHashTable[h] = NULL; }
    for (uint i=0; i < db->numGames; i++) {
        IndexEntry * ie = db->idx->FetchEntry (i);
        if (! ie->GetDeleteFlag()  /* &&  !ie->GetStartFlag() */
            &&  (!skipShortGames  ||  ie->GetNumHalfMoves() >= 10)
            &&  (!onlyFilterGames  ||  db->filter->Get(i) > 0)) {
            uint white, black;
            if (criteria.exactNames) {
                white = ie->GetWhite();
                black = ie->GetBlack();
            } else {
                white = hashName (ie->GetWhiteName(db->nb), 4);
                black = hashName (ie->GetBlackName(db->nb), 4);
            }
            uint hash = (white + black) % GLIST_HASH_SIZE;
            gNumListT * node = &(gNumList[i]);
            node->white = white;
            node->black = black;
            node->gNumber = i;
            node->next = gHashTable[hash];
            gHashTable[hash] = node;
        }
    }

    if (setFilterToDups) { db->dbFilter->Fill (0); }
    if (showProgress) { restartProgressBar (ti); }

    // Now check each list of same-hash games for duplicates:

    for (uint hash=0; hash < GLIST_HASH_SIZE; hash++) {
        if (showProgress  &&  ((hash & 255) == 0)) {
            if (interruptedProgress()) { break; }
            updateProgressBar (ti, hash, GLIST_HASH_SIZE);
        }
        gNumListT * head = gHashTable[hash];

        while (head != NULL) {
            IndexEntry * ieHead = db->idx->FetchEntry (head->gNumber);
            gNumListT * compare = head->next;

            while (compare != NULL) {

                IndexEntry * ieComp = db->idx->FetchEntry (compare->gNumber);

                if (checkDuplicate (db, ieHead, ieComp, head, compare, &criteria)) {
                    db->duplicates[head->gNumber] = compare->gNumber + 1;
                    db->duplicates[compare->gNumber] = head->gNumber + 1;

                    // Found a duplicate! Decide which one to delete:

                    bool headImmune = false;
                    bool compImmune = false;
                    bool doDeletion = false;
                    bool copiedRatings = false;
                    gameNumberT gnumKeep, gnumDelete;
                    IndexEntry * ieDelete, * ieKeep;

                    if (keepAllCommentedGames) {
                        if (ieHead->GetCommentsFlag()) { headImmune = true; }
                        if (ieComp->GetCommentsFlag()) { compImmune = true; }
                    }
                    if (keepAllGamesWithVars) {
                        if (ieHead->GetVariationsFlag()) { headImmune = true; }
                        if (ieComp->GetVariationsFlag()) { compImmune = true; }
                    }

                    // Decide which game should get deleted:
                    bool deleteHead = false;
                    if (deleteStrategy == DELETE_OLDER) {
                        deleteHead = (head->gNumber < compare->gNumber);
                    } else if (deleteStrategy == DELETE_NEWER) {
                        deleteHead = (head->gNumber > compare->gNumber);
                    } else {
                        ASSERT (deleteStrategy == DELETE_SHORTER);
                        deleteHead = (ieHead->GetNumHalfMoves() <
                                      ieComp->GetNumHalfMoves());
                    }

                    if (deleteHead) {
                        ieDelete = ieHead;
                        ieKeep = ieComp;
                        gnumDelete = head->gNumber;
                        gnumKeep = compare->gNumber;
                        doDeletion = ! headImmune;
                    } else {
                        ieDelete = ieComp;
                        ieKeep = ieHead;
                        gnumDelete = compare->gNumber;
                        gnumKeep = head->gNumber;
                        doDeletion = ! compImmune;
                    }
                    // Delete whichever game is to be deleted:
                    if (doDeletion) {
                        deletedCount++;
                        ieDelete->SetDeleteFlag (true);
                        if (copyRatings  &&  ieKeep->GetWhiteElo() == 0) {
                            eloT elo = ieDelete->GetWhiteElo();
                            byte rtype = ieDelete->GetWhiteRatingType();
                            if (elo != 0) {
                                ieKeep->SetWhiteElo (elo);
                                ieKeep->SetWhiteRatingType (rtype);
                                copiedRatings = true;
                            }
                        }
                        if (copyRatings  &&  ieKeep->GetBlackElo() == 0) {
                            eloT elo = ieDelete->GetBlackElo();
                            byte rtype = ieDelete->GetBlackRatingType();
                            if (elo != 0) {
                                ieKeep->SetBlackElo (elo);
                                ieKeep->SetBlackRatingType (rtype);
                                copiedRatings = true;
                            }
                        }
                        db->idx->WriteEntries (ieDelete, gnumDelete, 1);
                        if (copiedRatings) {
                            db->idx->WriteEntries (ieKeep, gnumKeep, 1);
                        }
                        if (setFilterToDups) {
                            db->dbFilter->Set (gnumDelete, 1);
                        }
                    }
                }
                compare = compare->next;
            }
            head = head->next;
        }
    }
    if (setFilterToDups) {
      setMainFilter(db);
    }

    db->idx->WriteHeader();
    recalcFlagCounts (db);
    if (showProgress) { updateProgressBar (ti, 1, 1); }
    delete[] gHashTable;
    delete[] gNumList;

    return setUintResult (ti, deletedCount);
}


// Handle/Strip/Add 'extra' tags.
// Cannot be used for in-index tags such as White, ratings, ECO, EventDate or the FEN or SetUp tags.

int
sc_base_tag (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_base tag [list | [find | strip] <tagname> [filter|all] | add <tagname> <tagvalue> [filter|all]]";
    const char * options[] = {
        "find", "list", "strip", "add", NULL
    };
    enum {
        TAG_FIND, TAG_LIST, TAG_STRIP, TAG_ADD
    };

    bool showProgress = startProgressBar();
    // printf ("showProgress %u\n",showProgress);

    const char * tag = NULL;  // For find, strip, add
    const char * value = NULL;// For add
    NameBase * tagnb = NULL;  // For "list" command; tags are collected as if they were player names.

    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    int cmd = -1;
    if (argc >= 3) { cmd = strUniqueMatch (argv[2], options); }
    bool limitToFilter = false;

    switch (cmd) {
    case TAG_LIST:
        if (argc != 3) { return errorResult (ti, usage); }
        tagnb = new NameBase;
        break;
    case TAG_FIND:
    case TAG_STRIP:
        if (argc != 5) { return errorResult (ti,usage); }
        tag = argv[3];
        limitToFilter = (argv[4][0] == 'f');
        break;
    case TAG_ADD:
        if (argc != 6) { return errorResult (ti,usage); }
        tag = argv[3];
        value = argv[4];
        limitToFilter = (argv[5][0] == 'f');
        break;
    default:
        return errorResult (ti, usage);
    };

    // If stripping a tag, make sure we have a writable database:
    if (cmd == TAG_STRIP  &&  db->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }

    // Process each game in the database:
    uint updateStart = 1000;  // Update progress bar every 1000 filter games.
    uint update = updateStart;
    Game * g = scratchGame;
    uint nEditedGames = 0;

    for (uint gnum = 0; gnum < db->numGames; gnum++) {
        // Update progress bar:
        if (showProgress) {
            update--;
            if (update == 0) {
                update = updateStart;
                if (interruptedProgress()) { break; }
                updateProgressBar (ti, gnum, db->numGames);
            }
        }

        if (limitToFilter  &&  db->filter->Get(gnum) == 0) { continue; }

        IndexEntry * ie = db->idx->FetchEntry (gnum);
        if (ie->GetLength() == 0) { continue; }
        db->bbuf->Empty();
        if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(), ie->GetLength()) != OK) {
            continue;
        }
        if (g->Decode (db->bbuf, GAME_DECODE_ALL) != OK) {
            continue;
        }
        if (cmd == TAG_FIND) {
            if (g->FindExtraTag (tag) != NULL) {
                // Found the tag, so add this game to the filter:
                db->dbFilter->Set (gnum, 1);
            } else {
                db->dbFilter->Set (gnum, 0);
            }
        } else if (cmd == TAG_STRIP) {
            if (g->RemoveExtraTag (tag)) {
                // The tag was found and stripped. Re-save the game,
                // remembering to load its standard tags first:
                g->LoadStandardTags (ie, db->nb);
                if (sc_savegame (ti, g, gnum+1, db) != OK) { return TCL_ERROR; }
                nEditedGames++;
            }
        } else if (cmd == TAG_ADD) {
            g->AddPgnTag (tag,value);
            g->LoadStandardTags (ie, db->nb);
            if (sc_savegame (ti, g, gnum+1, db) != OK) { return TCL_ERROR; }
            nEditedGames++;
        } else {
            ASSERT (cmd == TAG_LIST);
            uint numtags = g->GetNumExtraTags();
            tagT * taglist = g->GetExtraTags();
            // Increment frequency for each extra tag:
            while (numtags > 0) {
                idNumberT id;
                if (tagnb->AddName (NAME_PLAYER, taglist->tag, &id) == OK) {
                    tagnb->IncFrequency (NAME_PLAYER, id, 1);
                }
                numtags--;
                taglist++;
            }
        }
    }
    if (showProgress) 
        updateProgressBar (ti, 1, 1);

    if (cmd != TAG_LIST)
        setMainFilter (db);

    // Done searching through all games.

    // If necessary, update index and name files
    if ((cmd == TAG_STRIP || cmd == TAG_ADD)  &&  nEditedGames > 0) {
        db->gfile->FlushAll();
        if (db->idx->WriteHeader() != OK) {
            return errorResult (ti, "Error writing index file.");
        }
        if (! db->memoryOnly  &&  db->nb->WriteNameFile() != OK) {
            return errorResult (ti, "Error writing name file.");
        }
    }

    if (cmd == TAG_STRIP || cmd == TAG_ADD) {
        setUintResult (ti, nEditedGames);
    }

    // If listing extra tags, generate the list now:
    if (cmd == TAG_LIST) {
        for (idNumberT id=0; id < tagnb->GetNumNames(NAME_PLAYER); id++) {
            uint freq = tagnb->GetFrequency(NAME_PLAYER, id);
            const char * name = tagnb->GetName (NAME_PLAYER, id);
            if (freq > 0  &&  !strEqual (name, "SetUp")) {
                Tcl_AppendElement (ti, name);
                appendUintElement (ti, freq);
            }
        }
        delete tagnb;
    }

    return TCL_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Tourney:
//    Class for tournament data. Used by sc_base_tournaments.
//
struct tourneyPlayerT {
    idNumberT id;
    uint elo;
    int score;
    tourneyPlayerT * next;
};

class Tourney
{
  public:
    enum { TOURNEY_HASH_SIZE = 32768 };

    idNumberT SiteID;
    idNumberT EventID;
    NameBase * NB;
    dateT     StartDate;
    dateT     EndDate; // unused
    dateT     MinDate;
    dateT     MaxDate;
    dateT     EventDate;
    uint      NumGames;
    uint      NumPlayers;
    tourneyPlayerT * PlayerList;
    uint      EloSum;
    uint      EloCount;
    gameNumberT FirstGame;
    tourneyPlayerT const * Best[2];

    Tourney(IndexEntry * ie, NameBase * nb);
    Tourney(uint eventID, uint siteID);
    ~Tourney();

    static uint HashKey (uint eventID, uint siteID) {
	return (eventID + siteID) % TOURNEY_HASH_SIZE;
    }

    void AddGame (IndexEntry * ie, gameNumberT g);
    void Dump (Tcl_DString * ds) const;
    void PrepareBestPlayers ();
    uint MeanElo() const {
	return EloCount == 0 ? 0 : uint(float(EloSum)/float(EloCount) + 0.5);
    };
    bool HasPlayer (const char * playerStr) const;

    // for unordered_set:

    uint HashKey() const { return HashKey(EventID, SiteID); }
};

// This constructor only constructs a key object.
Tourney::Tourney (uint eventID_, uint siteID_)
    :SiteID(eventID_)
    ,EventID(siteID_)
    ,NB(NULL)
    ,PlayerList(NULL)
{
}

Tourney::Tourney (IndexEntry * ie, NameBase * nb)
    :SiteID(ie->GetSite())
    ,EventID(ie->GetEvent())
    ,NB(nb)
    ,StartDate(ie->GetDate())
    ,EndDate(ie->GetDate())
    ,MinDate(date_AddMonths (StartDate, -3))
    ,MaxDate(date_AddMonths (StartDate,  3))
    ,EventDate(ie->GetEventDate())
    ,NumGames(0)
    ,NumPlayers(0)
    ,PlayerList(NULL)
    ,EloSum(0)
    ,EloCount(0)
{
    ASSERT(nb);
    Best[0] = Best[1] = NULL;
}

Tourney::~Tourney()
{
    tourneyPlayerT *p, *next;
    p = PlayerList;
    while (p != NULL) {
        next = p->next;
        free(p);
        p = next;
    }
}

void
Tourney::AddGame (IndexEntry * ie, gameNumberT g)
{
    ASSERT(NB); // fails if this is only a key

    idNumberT whiteID = ie->GetWhite();
    idNumberT blackID = ie->GetBlack();
    uint wElo = ie->GetWhiteElo();
    uint bElo = ie->GetBlackElo();
    resultT result = ie->GetResult();
    uint wScore = 1;
    uint bScore = 1;
    if (result == RESULT_White) { wScore = 2; bScore = 0; }
    if (result == RESULT_Black) { wScore = 0; bScore = 2; }

    tourneyPlayerT * p = PlayerList;
    bool playerFound = false;
    while (p != NULL) {
        if (p->id == whiteID) {
            playerFound = true;
            if (wElo > p->elo) { p->elo = wElo; }
            p->score += wScore;
            break;
        }
        p = p->next;
    }
    if (! playerFound) {
        p = new tourneyPlayerT;
        p->id = whiteID;
        p->elo = wElo;
        p->score = wScore;
        p->next = PlayerList;
        PlayerList = p;
        NumPlayers++;
    }
    p = PlayerList;
    playerFound = false;
    while (p != NULL) {
        if (p->id == blackID) {
            playerFound = true;
            if (bElo > p->elo) { p->elo = bElo; }
            p->score += bScore;
            break;
        }
        p = p->next;
    }
    if (! playerFound) {
        p = new tourneyPlayerT;
        p->id = blackID;
        p->elo = bElo;
        p->score = bScore;
        p->next = PlayerList;
        PlayerList = p;
        NumPlayers++;
    }

    if (wElo != 0) {
        EloSum += wElo;
        EloCount++;
    }
    if (bElo != 0) {
        EloSum += bElo;
        EloCount++;
    }

    dateT date = ie->GetDate();
    if (NumGames == 0  ||  date < StartDate) {
        StartDate = date;
        FirstGame = g;
    }
    NumGames++;
}

void
Tourney::PrepareBestPlayers()
{
    ASSERT(NB); // fails if this is only a key

    for (uint i=0; i < 2; i++) {
        uint bestElo = 0;
        int bestScore = -1;
	tourneyPlayerT * best = NULL;
        tourneyPlayerT * p = PlayerList;

        while (p != NULL) {
            if (p->score > bestScore || (p->score == bestScore && p->elo > bestElo)) {
                if (!(i == 1 && Best[0] == p)) {
		  best = p;
		  bestScore = p->score;
		  bestElo = p->elo;
               }
            }
            p = p->next;
        }

	if (best) {
	    best->score = bestScore;
	    Best[i] = best;
	}
    }
}

void
Tourney::Dump (Tcl_DString * ds) const
{
    ASSERT(NB); // fails if this is only a key

    char str [16];

    Tcl_DStringStartSublist (ds);
    date_DecodeToString (StartDate, str);
    strTrimDate (str);
    Tcl_DStringAppendElement (ds, str);
    Tcl_DStringAppendElement (ds, NB->GetName (NAME_SITE, SiteID));
    Tcl_DStringAppendElement (ds, NB->GetName (NAME_EVENT, EventID));
    sprintf (str, "%u", NumPlayers);
    Tcl_DStringAppendElement (ds, str);
    sprintf (str, "%u", NumGames);
    Tcl_DStringAppendElement (ds, str);
    sprintf (str, "%u", MeanElo());
    Tcl_DStringAppendElement (ds, str);
    sprintf (str, "%u", FirstGame+1);
    Tcl_DStringAppendElement (ds, str);
    // Append the name, rating and score for the top two players:
    for (uint i=0; i < 2; i++) {
	if (Best[i] != NULL) {
	    Tcl_DStringAppendElement (ds, NB->GetName (NAME_PLAYER, Best[i]->id));
	    sprintf (str, "%u", Best[i]->elo);
	    Tcl_DStringAppendElement (ds, str);
	    sprintf (str, "%d", Best[i]->score/2);
	    if (Best[i]->score & 1) { strAppend (str, ".5"); }
	    Tcl_DStringAppendElement (ds, str);
	} else {
	    Tcl_DStringAppendElement (ds, "");
	    Tcl_DStringAppendElement (ds, "0");
	    Tcl_DStringAppendElement (ds, "0");
	}
    }
    Tcl_DStringEndSublist (ds);
}

bool
Tourney::HasPlayer (const char * playerStr) const
{
    ASSERT(NB); // fails if this is only a key

    if (playerStr == NULL) { return true; }
    tourneyPlayerT *p = PlayerList;
    while (p != NULL) {
        if (strAlphaContains (NB->GetName (NAME_PLAYER, p->id), playerStr)) {
            return true;
        }
        p = p->next;
    }
    return false;
}


struct SortTourney
{
    static int
    compElo(const Tourney * lhs, const Tourney * rhs)
    {
	return rhs->MeanElo() - lhs->MeanElo();

	/* Hmmm. Why does MeanElo have a different calculation than below ?
	if (! rhs->EloSum || ! rhs->EloCount)
	  return -1;
	if (! lhs->EloSum || ! lhs->EloCount)
	  return 1;
	return rhs->EloSum / rhs->EloCount - lhs->EloSum / lhs->EloCount;
	*/

	/*
	 * Because it is not required that this function returns -1, 0, +1. For comparison
	 * the following has to be fulfilled:
	 * - return = 0 if equal
	 * - return > 0 if lhs > rhs
	 * - return < 0 if lhs < rhs
	 * This will give an ascending order. But this function is used for descending order,
	 * so it has been reversed (see "return ..." above). (GC)
	 */
    }

    static int
    compEvent (const Tourney * lhs, const Tourney * rhs)
    {
	const char * name1 = lhs->NB->GetName (NAME_EVENT, lhs->EventID);
	const char * name2 = rhs->NB->GetName (NAME_EVENT, rhs->EventID);

	int compare = strCaseCompare (name1, name2);
	if (compare == 0) { compare = strCompare (name1, name2); }
	return compare;
    }

    static int
    compSite (const Tourney * lhs, const Tourney * rhs)
    {
	const char * name1 = lhs->NB->GetName (NAME_SITE, lhs->SiteID);
	const char * name2 = rhs->NB->GetName (NAME_SITE, rhs->SiteID);

	int compare = strCaseCompare (name1, name2);
	if (compare == 0) { compare = strCompare (name1, name2); }
	return compare;
    }

    static int
    compPlayers(const Tourney * lhs, const Tourney * rhs)
    {
	return rhs->NumPlayers - lhs->NumPlayers;
    }

    static int
    compGames(const Tourney * lhs, const Tourney * rhs)
    {
	return rhs->NumGames - lhs->NumGames;
    }

    static int
    compNewest(const Tourney * lhs, const Tourney * rhs)
    {
	return rhs->StartDate - lhs->StartDate;
    }

    static int
    compWinner(const Tourney * lhs, const Tourney * rhs)
    {
        int compare = 0;
        const tourneyPlayerT * lhsBest;
        const tourneyPlayerT * rhsBest;
	const char * name1;
	const char * name2;
        // compare first winner, then second winner, alphabetically
	for (uint i=0; i < 2; i++) {
	      lhsBest = lhs->Best[i];
	      rhsBest = rhs->Best[i];
              if (!lhsBest || !rhsBest) 
                  continue;
	      name1 = lhs->NB->GetName (NAME_PLAYER, lhsBest->id);
	      name2 = rhs->NB->GetName (NAME_PLAYER, rhsBest->id);
	      compare = strCaseCompare (name1, name2);
	      if (compare == 0) 
		  compare = strCompare (name1, name2);
	      if (compare != 0)
		  return compare;
        }
        return 0;
    }

    typedef int (*Comp) (const Tourney * lhs, const Tourney *rhs);
    Comp comp;

    SortTourney (Comp comp_) :comp(comp_) {}

    // std::multiset expects this less operator, it returns whether
    // lhs is less than rhs.

    bool operator () (const Tourney *lhs, const Tourney *rhs) const
    {
	return comp(lhs, rhs) < 0;
    }
};

namespace std {
    // This class will compute the hash key for std::unordered_set.
    template<>
    struct hash<Tourney>
    {
	std::size_t operator () (Tourney const& t) const { return t.HashKey(); }
    };

    // This class provides an equality operation for std::unordered_set.
    template<>
    struct equal_to<Tourney>
    {
	// Tourney's are always unique.
	bool operator () (const Tourney&, const Tourney&) const { return false; }
    };
}

#if __cplusplus < 201103L // pre C++11
# define emplace(expr, ...) insert(Tourney(expr, ##__VA_ARGS__))
#endif

//inline static uint max(uint a, uint b) { return a > b ? a : b; }
//inline static uint min(uint a, uint b) { return a < b ? a : b; }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_tournaments:
//    Returns information on tournaments in the current database.
int
sc_base_tournaments (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_base tournaments [-<option> <value> ...]";

    uint i;
    uint maxTourneys = 100;
    uint minMeanElo = 0;
    uint maxMeanElo = 4000;
    uint minGames = 1;
    uint maxGames = 9999;
    uint minPlayers = 2;
    uint maxPlayers = 999;
    const char * country = NULL;
    const char * siteStr = NULL;
    const char * sortStr = NULL;
    const char * eventStr = NULL;
    const char * playerStr = NULL;
    dateT minDate = ZERO_DATE;
    dateT maxDate = date_EncodeFromString ("2047.12.31");

    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (db->numGames == 0) {
	return TCL_OK;
    }

    static const char * options [] = {
        "-start", "-end", "-country", "-minElo", "-maxElo",
        "-minGames", "-maxGames", "-minPlayers", "-maxPlayers",
        "-size", "-site", "-event", "-player", "-sort", NULL
    };
    enum {
        T_START, T_END, T_COUNTRY, T_MINELO, T_MAXELO,
        T_MINGAMES, T_MAXGAMES, T_MINPLAYERS, T_MAXPLAYERS,
        T_SIZE, T_SITE, T_EVENT, T_PLAYER, T_SORT
    };
    int arg = 2;
    while (arg+1 < argc) {
        const char * option = argv[arg];
        const char * value = argv[arg+1];
        arg += 2;
        int index = strUniqueMatch (option, options);
        switch (index) {
            case T_START:      minDate = date_EncodeFromString (value); break;
            case T_END:        maxDate = date_EncodeFromString (value); break;
            case T_COUNTRY:    country = value;                         break;
            case T_MINELO:     minMeanElo = strGetUnsigned (value);     break;
            case T_MAXELO:     maxMeanElo = strGetUnsigned (value);     break;
            case T_MINGAMES:   minGames = strGetUnsigned (value);       break;
            case T_MAXGAMES:   maxGames = strGetUnsigned (value);       break;
            case T_MINPLAYERS: minPlayers = strGetUnsigned (value);     break;
            case T_MAXPLAYERS: maxPlayers = strGetUnsigned (value);     break;
            case T_SIZE:       maxTourneys = strGetUnsigned (value);    break;
            case T_SITE:       siteStr = value;                         break;
            case T_EVENT:      eventStr = value;                        break;
            case T_PLAYER:     playerStr = value;                       break;
            case T_SORT:       sortStr = value;                         break;
            default:
                return InvalidCommand (ti, "sc_base tournaments", options);
        }
    }
    if (arg != argc) { return errorResult (ti, usage); }

    // std::unordered_set is based on a hash table.
    // Note that we cannot use std::ordered_set, because sorting is not
    // possible as long as not all games are assigned to the appropriate
    // tourney object.
    typedef std::unordered_set<Tourney> TourneyHash;

    std::vector<bool> useSite(db->nb->GetNumNames (NAME_SITE), true);
    TourneyHash hashTable;

    if (country) {
	// If the country is "---", ignore it:
	if (strEqual (country, "---")) {
	    country = NULL;
	} else if (country[0] != 0) {
	    // Find all sites in the selected country, if any:
	    for (i=0; i < useSite.size(); i++) {
		const char * site = db->nb->GetName (NAME_SITE, i);
		uint len = strLength (site);
		if (len > 3) { site += len - 3; }
		if (! strEqual (site, country)) { useSite[i] = false; }
	    }
	}
    }

    // Restrict search to sites containing the given site string:
    if (siteStr != NULL  &&  siteStr[0] != 0) {
        for (i=0; i < useSite.size(); i++) {
            if (! useSite[i]) { continue; }
            const char * site = db->nb->GetName (NAME_SITE, i);
            if (! strAlphaContains (site, siteStr)) {
                useSite[i] = false;
            }
        }
    }

    // Restrict search to events containing the given event string:
    std::vector<bool> useEvent;
    if (eventStr != NULL  &&  eventStr[0] != 0) {
	useEvent.resize(db->nb->GetNumNames (NAME_EVENT), true);
        for (i=0; i < useEvent.size(); i++) {
            const char * event = db->nb->GetName (NAME_EVENT, i);
            if (! strAlphaContains (event, eventStr)) {
                useEvent[i] = false;
            }
        }
    }

    // Now look through all games:
    for (i=0; i < db->numGames; i++) {
        IndexEntry * ie = db->idx->FetchEntry (i);
        dateT date = ie->GetDate();
        dateT eventDate = ie->GetEventDate();
        if (date < minDate) { continue; }
        if (date > maxDate) { continue; }
        idNumberT siteID = ie->GetSite();
        if (! useSite[siteID]) { continue; }
        idNumberT eventID = ie->GetEvent();
        if (!useEvent.empty() && !useEvent[eventID]) { continue; }

	const Tourney *thisTourney;

	if (hashTable.bucket_count() == 0) {
	    // This hash table is empty, so add a new tourney. This case needs a
	    // special handling, because hashTable.bucket() is undefined for
	    // empty tables.
	    // C++: emplace(...) gives us a pair <iterator,bool>. 'first'
	    // C++: is accessing the first member, and operator '*' gives
	    // C++: an object of class Tourney, so operator '&' will give
	    // C++: a pointer to this object.
	    thisTourney = &(*hashTable.emplace(ie, db->nb).first);
	} else {
	    uint bucketIndex = hashTable.bucket(Tourney(eventID, siteID));

	    // iterate over a bucket
	    // C++: 'auto' will deduce the correct type from assigned value. In this
	    // C++: case it will be a iterator (concrete type is TourneyHash::const_iterator).
	    auto tp = hashTable.cbegin(bucketIndex);
	    auto end = hashTable.cend(bucketIndex);

	    for ( ; tp != end; ++tp) {
		if (tp->EventID == eventID
		    &&  tp->SiteID == siteID
		    &&  ((eventDate != ZERO_DATE  &&  eventDate == tp->EventDate)
			||  (date >= tp->MinDate  &&  date <= tp->MaxDate))) {
		    break;
		}
	    }

	    if (tp == end) {
		// Have to add a new tourney:
		// C++: emplace(...) gives us a pair <iterator,bool>. 'first'
		// C++: is accessing the first member, and operator '*' gives
		// C++: an object of class Tourney, so operator '&' will give
		// C++: a pointer to this object.
		thisTourney = &(*hashTable.emplace(ie, db->nb).first);
	    } else {
		// We've found an appropriate tourney.
		// C++: operator '*' applied to this iterator gives an object of
		// C++: class Tourney, so operator '&' will give a pointer to this
		// C++: object.
		thisTourney = &(*tp);
	    }
	}

	// We need a const cast, because the set only provides const elements,
	// but AddGame() does not change the key, so it's okay.
	const_cast<Tourney *>(thisTourney)->AddGame(ie, i);
    }

    // Now gather all tourneys from the hash table into a set for sorting.
    // -------------------------------------------------------------------

    // We start with preparing all the tourney objects, so we can even
    // sort on best scores. Because we will dump the best players anyway,
    // we have to do the preparation even if we do not sort on best scores.

    for (auto tp = hashTable.cbegin(), e = hashTable.cend(); tp != e; ++tp) {
	// It's unavoidable to cast the constness away, because hash objects
	// are always const. But our operation will not change the key, so
	// it's no problem.
	const_cast<Tourney &>(*tp).PrepareBestPlayers();
    }

    auto comp = SortTourney::compNewest;

    if (!strCompare(sortStr,"Games"))   comp = SortTourney::compGames;
    if (!strCompare(sortStr,"Elo"))     comp = SortTourney::compElo;
    if (!strCompare(sortStr,"Players")) comp = SortTourney::compPlayers;
    if (!strCompare(sortStr,"Site"))    comp = SortTourney::compSite;
    if (!strCompare(sortStr,"Event"))   comp = SortTourney::compEvent;
    // Only possible because we already have prepared for best players.
    if (!strCompare(sortStr,"Winner"))	comp = SortTourney::compWinner;

    typedef std::multiset<const Tourney *, SortTourney> TourneySet;

    // Now build a set, sorted by given method.
    SortTourney sortOp(comp);	// the comparison operator
    TourneySet tset(sortOp);	// a set sorting with given comparison operator

    for (auto tp = hashTable.cbegin(), e = hashTable.cend(); tp != e; ++tp) {
	uint meanElo = tp->MeanElo();

	if ( meanElo >= minMeanElo
	&&  meanElo <= maxMeanElo
	&&  tp->NumPlayers >= minPlayers
	&&  tp->NumPlayers <= maxPlayers
	&&  tp->NumGames >= minGames
	&&  tp->NumGames <= maxGames
	&&  tp->HasPlayer (playerStr)) {
	    tset.insert(&(*tp)); // sorted insertion
	}
    }

    Tcl_DString ds;
    Tcl_DStringInit (&ds);

    maxTourneys = std::min(maxTourneys, uint(tset.size()));
    uint count = 0;

    for (auto tp = tset.cbegin(), e = tset.cend(); tp != e; ++tp) {
	(*tp)->Dump(&ds);

	// Don't return more tourney results than requested.
	if (++count == maxTourneys) {
	    break;
	}
    }

    Tcl_DStringResult (ti, &ds);
    Tcl_DStringFree (&ds);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_upgrade:
//    Upgrades an old (version 3.x, suffix .si3) Scid database
//    to version 4 (suffix .si4).
int
sc_base_upgrade (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = startProgressBar();

    if (argc != 3) {
        return errorResult (ti, "Usage: sc_base upgrade <old-database>");
    }
    const char * fname = argv[2];
    Index * oldIndex = new Index;
    Index * newIndex = new Index;

    oldIndex->SetFileName (fname);
    newIndex->SetFileName (fname);

    if (newIndex->OpenIndexFile(FMODE_ReadOnly) == OK) {
        newIndex->CloseIndexFile();
        delete oldIndex;
        delete newIndex;
        return errorResult (ti, "An upgraded version of this database already exists.");
    }

    if (oldIndex->OpenOldIndexFile (FMODE_ReadOnly) != OK) {
        delete oldIndex;
        delete newIndex;
        return errorResult (ti, "Error opening the old database.");
    }

    if (newIndex->CreateIndexFile (FMODE_WriteOnly) != OK) {
        oldIndex->CloseIndexFile();
        delete oldIndex;
        delete newIndex;
        return errorResult (ti, "Error creating the new dataabse.");
    }

    MFile * oldMFile = oldIndex->GetMFile();
    MFile * newMFile = newIndex->GetMFile();

    char buf[256];
    // First copy the header with new version number

    oldMFile->Seek(0);
    newMFile->Seek(0);

    oldMFile->ReadNBytes( buf, 8);
    newMFile->WriteNBytes(buf, 8);

    oldMFile->ReadTwoBytes();
    newMFile->WriteTwoBytes(SCID_VERSION);

    oldMFile->ReadNBytes(buf, 4 + 3 + 3 + SCID_DESC_LENGTH + 1);
    newMFile->WriteNBytes(buf, 4 + 3 + 3 + SCID_DESC_LENGTH + 1);

    // write the descriptions of the custom flags added with v4 version of base
    memset( buf, 0, 256 );
    newMFile->WriteNBytes( buf, CUSTOM_FLAG_MAX * ( CUSTOM_FLAG_DESC_LENGTH+1 ) );

    // Now copy each index entry
    uint i;
    uint numGames = oldIndex->GetNumGames();
    errorT err = OK;
    uint updateStart = 250;
    uint update = updateStart;

    for (i=0; i < numGames; i++) {
        if (showProgress) {
            update--;
            if (update == 0) {
                update = updateStart;
                if (interruptedProgress()) { break; }
                updateProgressBar (ti, i, numGames);
            }
        }

        oldMFile->ReadNBytes(buf, 4 + 2);
        newMFile->WriteNBytes(buf, 4 + 2);

        // insert one byte for Length_High and user bits
        newMFile->WriteNBytes( "\0", 1 );

        oldMFile->ReadNBytes(buf, 40 );
        newMFile->WriteNBytes(buf, 40 );

    }

    oldIndex->CloseIndexFile();
    newIndex->CloseIndexFile(true); // don't write again the header

    if (err == OK  &&  !interruptedProgress()) {
        if (err != OK) { setResult (ti, "Error writing name file"); }
    }

    delete oldIndex;
    delete newIndex;

    if (interruptedProgress()) {
        removeFile (fname, INDEX_SUFFIX);
        return errorResult (ti, "Upgrading was cancelled.");
    }

    if (err != OK) {
        removeFile (fname, INDEX_SUFFIX);
        return TCL_ERROR;
    }

    if (showProgress) { updateProgressBar (ti, 1, 1); }

    return TCL_OK;
}

inline static idNumberT
safeNameId(idNumberT id, idNumberT numEntries) { return id < numEntries ? id : 0; }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_base_fix_corrupted:
//    Tries to fix a corrupted base by rewriting its index and name
//    files. This function is very close to sc_compact_names
int
sc_base_fix_corrupted (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * filename = argv[2];

    errorT err = 0;

    Index * idx = new Index;
    NameBase * nb = new NameBase;

    idx->SetFileName (filename);
    nb->SetFileName (filename);

    if ( nb->ReadNameFile() != OK) {
        delete nb;
        delete idx;
        errorResult (ti, "Error opening name file.");
    }

    Index * idxTemp = new Index;
    idxTemp->SetFileName (filename);
    if (idxTemp->OpenIndexFile (FMODE_ReadOnly) != OK) {
        delete idxTemp;
        delete nb;
        delete idx;
        return errorResult (ti, "Error opening index file.");
    }
    recalcNameFrequencies (nb, idxTemp);
    idxTemp->CloseIndexFile();

    // Now, add only the names with nonzero frequencies to a new namebase:

    NameBase * nbNew = new NameBase;

    idNumberT * idMapping[NUM_NAME_TYPES];

    for (nameT nt = NAME_FIRST; nt <= NAME_LAST; nt++) {
        uint unused;
        nbNew->AddName (nt, "?", &unused); // must be zero
        idNumberT numNames = nb->GetNumNames (nt);
        idNumberT numEntries = NAME_MAX_ID[nt];
        idMapping[nt] = new idNumberT[numEntries];
        memset(idMapping[nt], 0, numEntries*sizeof(idNumberT));

        for (idNumberT oldID = 0; oldID < numNames; oldID++) {
            const char * name = nb->GetName (nt, oldID);
            uint frequency = nb->GetFrequency (nt, oldID);
            if (frequency > 0) {
                uint newID;
                if (nbNew->AddName (nt, name, &newID) != OK) {
                    errorResult (ti, "Error compacting namebase.");
                    break;
                }
                if (newID >= numEntries)
                    newID = 0;
                idMapping[nt][oldID] = newID;
            } else {
                idMapping[nt][oldID] = 0;
            }
        }
    }

    printf ("New: %u players, %u events, %u sites, %u rounds\n",
            nbNew->GetNumNames(NAME_PLAYER), nbNew->GetNumNames(NAME_EVENT),
            nbNew->GetNumNames(NAME_SITE), nbNew->GetNumNames(NAME_ROUND));
    Index * idxOld = new Index;
    Index * idxNew = new Index;
    char tempName[1024];
    strCopy (tempName, filename);
    strAppend (tempName, "_OLD");
    idxOld->SetFileName (tempName);
    idxNew->SetFileName (filename);

    printf ("Renaming %s%s to %s%s in case of error...\n",
            filename, INDEX_SUFFIX, tempName, INDEX_SUFFIX);
    if (renameFile (filename, tempName, INDEX_SUFFIX) != OK) {
        errorResult (ti, "Unable to rename the index file!");
    }
    printf ("Renaming %s%s to %s%s in case of error...\n",
            filename, NAMEBASE_SUFFIX, tempName, NAMEBASE_SUFFIX);
    if (renameFile (filename, tempName, NAMEBASE_SUFFIX) != OK) {
        errorResult (ti, "Unable to rename the namebase file!");
    }
    if (idxOld->OpenIndexFile (FMODE_ReadOnly) != OK) {
        renameFile (tempName, filename, INDEX_SUFFIX);
        errorResult (ti, "Error renaming index file");
    }

    if (idxNew->CreateIndexFile (FMODE_WriteOnly) != OK) {
        renameFile (tempName, filename, INDEX_SUFFIX);
        errorResult (ti , "Error creating index file");
    }
    idxNew->SetType (idxOld->GetType());

    printf ("Writing new name and index files...\n");

    // For each entry in old index: read it, alter its name ID
    // values to match the new namebase, and write the entry to
    // the new index.

    gameNumberT newNumGames = 0;
    for (uint i=0; i < idxOld->GetNumGames(); i++) {
        updateProgressBar (ti, i, idxOld->GetNumGames());
        IndexEntry ieOld, ieNew;
        idxOld->ReadEntries (&ieOld, i, 1);
        if (ieOld.Verify (nb) != OK) {
              fprintf (stderr, "Warning: game %u: ", i+1);
              fprintf (stderr, "names were corrupt, they may be incorrect.\n");
        }
        err = idxNew->AddGame (&newNumGames, &ieNew);
        if (err != OK)  { break; }
        ieNew = ieOld;
#define MAP_ID(id, nt) idMapping[nt] [safeNameId(id, NAME_MAX_ID[nt])]
        ieNew.SetWhite (MAP_ID(ieOld.GetWhite(), NAME_PLAYER));
        ieNew.SetBlack (MAP_ID(ieOld.GetBlack(), NAME_PLAYER));
        ieNew.SetEvent (MAP_ID(ieOld.GetEvent(), NAME_EVENT));
        ieNew.SetSite  (MAP_ID(ieOld.GetSite(), NAME_SITE));
        ieNew.SetRound (MAP_ID(ieOld.GetRound(), NAME_ROUND));
#undef MAP_ID
        nbNew->IncFrequency (NAME_PLAYER, ieNew.GetWhite(), 1);
        nbNew->IncFrequency (NAME_PLAYER, ieNew.GetBlack(), 1);
        nbNew->IncFrequency (NAME_EVENT, ieNew.GetEvent(), 1);
        nbNew->IncFrequency (NAME_SITE, ieNew.GetSite(), 1);
        nbNew->IncFrequency (NAME_ROUND, ieNew.GetRound(), 1);
        err = idxNew->WriteEntries (&ieNew, newNumGames, 1);
        if (err != OK)  { break; }
    }

    for (nameT nt = NAME_FIRST; nt <= NAME_LAST; nt++)
        delete [] idMapping[nt];

    idxOld->CloseIndexFile();
    idxNew->CloseIndexFile();
    nbNew->SetFileName (filename);

    if (err == OK) {
        err = nbNew->WriteNameFile();
    }

    if (err != OK) {
        fprintf (stderr, "ERROR: I/O error!\n");
        fprintf (stderr, "Restoring original file and aborting.\n");
        removeFile (filename, INDEX_SUFFIX);
        renameFile (tempName, filename, INDEX_SUFFIX);
    }

    printf ("New index file sucessfully created; old index removed.\n");
    removeFile (tempName, INDEX_SUFFIX);
    removeFile (tempName, NAMEBASE_SUFFIX);

    delete nb;
    delete nbNew;
    delete idx;
    delete idxTemp;
    delete idxOld;
    delete idxNew;
    return TCL_OK;
}
//////////////////////////////////////////////////////////////////////
/// EPD functions

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_epd:
//    Scid EPD (position) file functions.
int
sc_epd (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "altered",  "available",  "close",  "create",
        "deepest",  "get",  "load",  "moves",
        "name",     "next",       "open",   "prev",
        "readonly", "set",        "size",   "strip",
        "write",    "exists",     "index",
        NULL
    };
    enum {
        EPD_ALTERED,  EPD_AVAILABLE,  EPD_CLOSE,  EPD_CREATE,
        EPD_DEEPEST,  EPD_GET,        EPD_LOAD,   EPD_MOVES,
        EPD_NAME,     EPD_NEXT,       EPD_OPEN,   EPD_PREV,
        EPD_READONLY, EPD_SET,        EPD_SIZE,   EPD_STRIP,
        EPD_WRITE,    EPD_EXISTS,     EPD_INDEX
    };
    int index = -1;

    if (argc > 1) { index = strUniqueMatch (argv[1], options); }

    if (index < 0) {
        return InvalidCommand (ti, "sc_epd", options);
    }

    // Check for epd subcommands that do not require an epdID parameter:
    // these are "available", "open" and "create".
    if (index == EPD_AVAILABLE) {
        uint avail = 0;
        for (int i=0; i < MAX_EPD; i++) {
            if (pbooks[i] == NULL) { avail++; }
        }
        return setUintResult (ti, avail);
    }
    if (index == EPD_OPEN) {
        return sc_epd_open (ti, argc, argv, false);
    }
    if (index == EPD_CREATE) {
        return sc_epd_open (ti, argc, argv, true);
    }

    // Parse the epdID parameter:
    int epdID = -1;
    if (argc >= 3) { epdID = strGetInteger (argv[2]) - 1; }
    if (epdID < 0  ||  epdID >= MAX_EPD  ||  pbooks[epdID] == NULL) {
        Tcl_AppendResult (ti, "Error: sc_epd ", options[index],
                          ": invalid EPD ID number", NULL);
        return TCL_ERROR;
    }

    switch (index) {
    case EPD_ALTERED:
        return setBoolResult (ti, pbooks[epdID]->IsAltered());

    case EPD_CLOSE:  // Closes EPD file without saving:
        if (pbooks[epdID] != NULL) {
          delete pbooks[epdID];
          pbooks[epdID] = NULL;
        }
        break;

    case EPD_DEEPEST:
        return sc_epd_deepest (ti, epdID);

    case EPD_GET:   // Retrieves the text for the current position:
        {
            const char * text;
            if (pbooks[epdID]->Find (db->game->GetCurrentPos(), &text) == OK) {
                Tcl_AppendResult (ti, (char *) text, NULL);
            }
        }
        break;

    case EPD_LOAD:
        {
          if (argc != 4) {
            return errorResult (ti, "Usage: sc_epd load <epdID> <index>");
          }
          int idx = strGetInteger (argv[3]);
          PBook * pb = pbooks[epdID];
          scratchPos->CopyFrom (db->game->GetCurrentPos());
          if (pb->FindByIndex (scratchPos, idx) == OK) {
            db->game->Clear();
            db->gameNumber = -1;
            db->game->SetStartPos (scratchPos);
            db->gameAltered = true;
          }
        }
        break;

    case EPD_MOVES:
        return sc_epd_moves (ti, epdID);

    case EPD_NAME:
        Tcl_AppendResult (ti, pbooks[epdID]->GetFileName(), NULL);
        break;

    case EPD_NEXT:
        return sc_epd_next (ti, epdID, true);

    case EPD_PREV:
        return sc_epd_next (ti, epdID, false);

    case EPD_READONLY:
        return setBoolResult (ti, pbooks[epdID]->IsReadOnly());

    case EPD_SET:   // Sets the text for the current position:
        if (argc != 4) {
            return errorResult (ti, "Usage: sc_epd set <epdID> <text>");
        }
        return sc_epd_set (ti, epdID, argv[3]);

    case EPD_SIZE:
        return setUintResult (ti, pbooks[epdID]->Size());

    case EPD_STRIP:
        if (argc != 4) {
            return errorResult (ti, "Usage: sc_epd strip <epdID> <epdcode>");
        }
        return setUintResult (ti, pbooks[epdID]->StripOpcode (argv[3]));

    case EPD_WRITE:
        return sc_epd_write (ti, epdID);

    case EPD_EXISTS:
        if (pbooks[epdID]->Find(db->game->GetCurrentPos(), NULL)) {
          return setBoolResult (ti, false);
        } else {
          return setBoolResult (ti, true);
        }

    case EPD_INDEX:
        // Get the node index for the current position, or if index is given,
        // set the node index to that value.
        {
          PBook * pb = pbooks[epdID];
          if (argc == 3) {
            return setIntResult (ti, pb->GetIndex(db->game->GetCurrentPos()));
          } else if (argc == 4) {
            return setIntResult (ti, pb->SetIndex(strGetUnsigned(argv[3])));
          } else {
            return errorResult (ti, "Usage: sc_epd index <epdID> [<index>]");
          }
        }

    default:
        ASSERT(0);  // Unreachable!
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_epd_deepest:
//    Returns the deepest ply in the current game (main moves only,
//    not variations) that matches a position in this EPD file.
uint
sc_epd_deepest (Tcl_Interp * ti, int epdID)
{
    PBook * pb = pbooks[epdID];
    uint ply = 0;

    db->game->SaveState();
    db->game->MoveToPly (0);
    do {
        if (pb->Find (db->game->GetCurrentPos(), NULL) == OK) {
            ply = db->game->GetCurrentPly();
        }
    } while (db->game->MoveForward() == OK);
    db->game->RestoreState ();
    return setUintResult (ti, ply);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_epd_moves:
//    Returns the list of all legal moves (in standard algebraic
//    notation) from the current position that have text in this EPD file.
int
sc_epd_moves (Tcl_Interp * ti, int epdID)
{
    PBook * pb = pbooks[epdID];
    const char * text;
    Position * gamePos = db->game->GetCurrentPos();

    scratchPos->CopyFrom (gamePos);
    MoveList moveList;
    gamePos->GenerateMoves (&moveList);
    sanListT sanList;
    gamePos->CalcSANStrings(&sanList, SAN_CHECKTEST);

    for (uint i=0; i < moveList.Size(); i++) {
        simpleMoveT * smPtr = moveList.Get(i);
        scratchPos->DoSimpleMove (smPtr);
        if (pb->Find (scratchPos, &text) == OK) {
            Tcl_AppendElement (ti, sanList.list[i]);
        }
        scratchPos->UndoSimpleMove (smPtr);
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_epd_next:
//    Clears the current game and sets its start position to
//    be that of the next position found in the EPD list (in the order
//    they are stored in the file).
int
sc_epd_next (Tcl_Interp * ti, int epdID, bool forwards)
{
    PBook * pb = pbooks[epdID];
    scratchPos->CopyFrom (db->game->GetCurrentPos());
    if (pb->FindNext (scratchPos, forwards) == OK) {
        db->game->Clear();
        db->gameNumber = -1;
        db->game->SetStartPos (scratchPos);
        db->gameAltered = true;
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_epd_open:
//    Open an EPD file for editing.
//    If the last parameter (create) is true, the file is created.
int
sc_epd_open (Tcl_Interp * ti, int argc, const char ** argv, bool create)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_epd create|open <filename>");
    }

    const char * filename = argv[2];

    // Check that this EPD file is not already open:
    int epdID;
    for (epdID = 0; epdID < MAX_EPD; epdID++) {
        if (pbooks[epdID] != NULL  &&
                strEqual (filename, pbooks[epdID]->GetFileName())) {
            return errorResult (ti, "The EPD file you selected is already open.");
        }
    }

    // Find a free EPD file slot:
    int freeID = -1;
    for (epdID = 0; epdID < MAX_EPD; epdID++) {
        if (pbooks[epdID] == NULL) { freeID = epdID; break; }
    }
    if (freeID == -1) {
        return errorResult (ti, "Too many EPD files are open; close one first.");
    }

    PBook * pb = new PBook;
    pbooks[freeID] = pb;
    pb->SetFileName (filename);

    errorT err = create ? pb->WriteFile() : pb->ReadFile();
    if (err != OK) {
        delete pb;
        pbooks[freeID] = NULL;
        Tcl_AppendResult (ti, "Unable to ", (create ? "create" : "open"),
                          " EPD file: ", filename, PBOOK_SUFFIX, NULL);
        return TCL_ERROR;
    }
    return setIntResult (ti, freeID + 1);
}

int
sc_epd_set (Tcl_Interp * ti, int epdID, const char * text)
{
    PBook * pb = pbooks[epdID];
    Position * pos = db->game->GetCurrentPos();

    // Insert() creates a new entry if the position doesn't exist and replaces
    // the existing comment if it already exists.
    pb->Insert (pos, text);
    return TCL_OK;
}

int
sc_epd_write (Tcl_Interp * ti, int epdID)
{
    PBook * pb = pbooks[epdID];
    if (pb->WriteFile() != OK) {
        return errorResult (ti, "Error writing EPD file.");
    }
    return TCL_OK;
}


//////////////////////////////////////////////////////////////////////
/// CLIPBASE functions

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_clipbase:
//    Game clipbase functions.
//    Copies a game to, or pastes from, the clipbase database.
int
sc_clipbase (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "copy", "clear", "paste", NULL
    };
    enum {
        CLIP_COPY, CLIP_CLEAR, CLIP_PASTE
    };
    int index = -1;

    if (argc > 1) { index = strUniqueMatch (argv[1], options); }

    switch (index) {
    case CLIP_COPY:
        return sc_clipbase_copy (cd, ti, argc, argv);

    case CLIP_CLEAR:
        return sc_clipbase_clear (ti);

    case CLIP_PASTE:
        return sc_clipbase_paste (cd, ti, argc, argv);

    default:
        return InvalidCommand (ti, "sc_clipbase", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_clipbase_clear:
//    Clears the clipbase by closing and recreating it.
int
sc_clipbase_clear (Tcl_Interp * ti)
{
    if (! clipbase->inUse) {
        return errorResult (ti, "The clipbase is not open.");
    }
    clipbase->game->Clear();
    clipbase->nb->Clear();
    clipbase->gfile->Close();
    clipbase->gfile->CreateMemoryOnly();
    clipbase->idx->CloseIndexFile();
    clipbase->idx->CreateMemoryOnly();
    clipbase->idx->SetType (2);

    // Hmmm - clearing clipbase in read_only resets read-only
    clipbase->fileMode = FMODE_Both;

    clipbase->numGames = 0;
    clearFilter (clipbase, clipbase->numGames);

    clipbase->inUse = true;
    clipbase->gameNumber = -1;
    clipbase->treeCache->Clear();
    clipbase->backupCache->Clear();
    recalcFlagCounts (clipbase);

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_clipbase_copy:
//    Copy the current game to the clipbase database.
int
sc_clipbase_copy (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    // Hmmm - errors currently caught
    if (! clipbase->inUse) {
        return errorResult (ti, "The clipbase is not open.");
    }
    if (db == clipbase) {
        return errorResult (ti, "You are already in the clipbase database.");
    }
    if (clipbase->fileMode == FMODE_ReadOnly) {
         return errorResult (ti, errMsgReadOnly(ti));
    }
    if (clipbase->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }

    // Sorry, clipbase changes get discarded
    sc_game_undo_reset (clipbase);
    clipbase->gameAltered = false;

    db->bbuf->Empty();
    db->game->SaveState();
    if (db->game->Encode (db->bbuf, NULL) != OK) {
        return errorResult (ti, "Error encoding game.");
    }
    db->game->RestoreState();
    db->bbuf->BackToStart();
    clipbase->game->Clear();

    if (clipbase->game->Decode (db->bbuf, GAME_DECODE_ALL) != OK) {
        return errorResult (ti, "Error decoding game.");
    }

    // Copy the standard tag values to the clipbase game:
    clipbase->game->CopyStandardTags (db->game);

    // Move to the current position in the clipbase game:
    clipbase->tbuf->Empty();
    db->game->WriteToPGN (clipbase->tbuf);
    uint location = db->game->GetPgnOffset (0);
    clipbase->tbuf->Empty();
    clipbase->game->MoveToLocationInPGN (clipbase->tbuf, location);

    // Now, add the game as the last game in the clipbase:
    db->game->SaveState();
    if (sc_savegame (ti, db->game, 0, clipbase) != TCL_OK) {
        return TCL_ERROR;
    }
    db->game->RestoreState();

    // Update the clipbase game number:
    clipbase->gameNumber = clipbase->numGames - 1;

    // We must ensure that the clipbase Index is still all in memory:
    clipbase->idx->ReadEntireFile();
    recalcFlagCounts (clipbase);

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_clipbase_paste:
//    Paste the active clipbase game, replacing the current game state.
int
sc_clipbase_paste (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    // Cannot paste the clipbase game when already in the clipbase:
    if (db == clipbase) { return TCL_OK; }

    db->tbuf->Empty();
    clipbase->game->WriteToPGN (db->tbuf);
    uint location = clipbase->game->GetPgnOffset (0);
    db->bbuf->Empty();
    if (clipbase->game->Encode (db->bbuf, NULL) != OK) {
        return errorResult (ti, "Error encoding game.");
    }

    // db changes get discarded
    sc_game_undo_reset (db);

    db->bbuf->BackToStart();
    db->game->Clear();
    db->gameNumber = -1;
    db->gameAltered = true;

    if (db->game->Decode (db->bbuf, GAME_DECODE_ALL) != OK) {
        return errorResult (ti, "Error decoding game.");
    }

    // Copy the standard tag values from the clipbase game:
    db->game->CopyStandardTags (clipbase->game);

    // Move to the current position in the clipbase game:
    db->tbuf->Empty();
    db->game->MoveToLocationInPGN (db->tbuf, location);

    return TCL_OK;
}

//////////////////////////////////////////////////////////////////////
// DATABASE COMPACTION

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_compact:
//   Database compaction functions.
int
sc_compact (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "games", "names", "stats", NULL
    };
    enum {
        COMPACT_GAMES, COMPACT_NAMES, COMPACT_STATS
    };
    int index = -1;

    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (db->memoryOnly) {
        return errorResult (ti, "This is a memory-only database, it cannot be compacted.");
    }

    if (argc > 1) { index = strUniqueMatch (argv[1], options); }

    switch (index) {
    case COMPACT_GAMES:
        return sc_compact_games (cd, ti, argc, argv);

    case COMPACT_NAMES:
        return sc_compact_names (cd, ti, argc, argv);

    case COMPACT_STATS:
        return sc_compact_stats (cd, ti, argc, argv);

    default:
        return InvalidCommand (ti, "sc_compact", options);
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_compact_games:
//    Compact the game file and index file of a database so all
//    deleted games are removed, the order of game file records
//    matches the index file order, and the game file is the
//    smallest possible size.
int
sc_compact_games (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = startProgressBar();

    if (db->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }

    // First, create new temporary index and game file:

    fileNameT newName;
    strCopy (newName, db->fileName);
    strAppend (newName, "TEMP");

    Index * newIdx = new Index;
    GFile * newGfile = new GFile;
    // Filter * newFilter = new Filter (0);
    newIdx->SetFileName (newName);

#define CLEANUP \
    delete newIdx; delete newGfile; \
    removeFile (newName, INDEX_SUFFIX); \
    removeFile (newName, GFILE_SUFFIX)
    // ; delete newFilter;

    if (newIdx->CreateIndexFile (FMODE_WriteOnly) != OK) {
        CLEANUP;
        return errorResult (ti, "Error creating temporary file; compaction cancelled");
    }
    if (newGfile->Create (newName, FMODE_WriteOnly) != OK) {
        CLEANUP;
        return errorResult (ti, "Error creating temporary file; compaction cancelled.");
    }

    gameNumberT newNumGames = 0;
    bool interrupted = 0;
    const char * errMsg = "";
    const char * errWrite = "Error writing temporary file; compaction cancelled.";

    uint updateStart, update;
    updateStart = update = 500;  // Update progress bar every 500 games
    for (uint i=0; i < db->numGames; i++) {
        if (showProgress) {
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, i, db->numGames);
                if (interruptedProgress()) {
                    errMsg = "User interruption; compaction cancelled.";
                    interrupted = true;
                    break;
                }
            }
        }
        IndexEntry * ieOld = db->idx->FetchEntry (i);
        if (ieOld->GetDeleteFlag()) {
            continue;
        }
        IndexEntry ieNew;
        errorT err;
        db->bbuf->Empty();
        err = db->gfile->ReadGame (db->bbuf, ieOld->GetOffset(),
                                   ieOld->GetLength());
        if (err != OK) {
            // Just skip corrupt games:
            continue;
        }
        db->bbuf->BackToStart();
        err = scratchGame->Decode (db->bbuf, GAME_DECODE_NONE);
        if (err != OK) {
            // Just skip corrupt games:
            continue;
        }
        err = newIdx->AddGame (&newNumGames, &ieNew);
        if (err != OK) {
            errMsg = "Error in compaction operation; compaction cencelled.";
            interrupted = true;
            break;
        }
        ieNew = *ieOld;
        db->bbuf->BackToStart();
        uint offset = 0;
        err = newGfile->AddGame (db->bbuf, &offset);
        if (err != OK) {
            errMsg = errWrite;
            interrupted = true;
            break;
        }
        ieNew.SetOffset (offset);

        // In 3.2, the maximum value for the game length in half-moves
        // stored in the Index was raised from 255 (8 bits) to over
        // 1000 (10 bits). So if the game and index values do not match,
        // update the index value now:
        if (scratchGame->GetNumHalfMoves() != ieNew.GetNumHalfMoves()) {
            ieNew.SetNumHalfMoves (scratchGame->GetNumHalfMoves());
        }

        err = newIdx->WriteEntries (&ieNew, newNumGames, 1);
        if (err != OK) {
            errMsg = errWrite;
            interrupted = true;
            break;
        }
        // newFilter->Append (db->filter->Get (i));
    }

    if (interrupted) {
        CLEANUP;
        return errorResult (ti, errMsg);
    }

    if (showProgress) { updateProgressBar (ti, 1, 1); }

    newIdx->SetType (db->idx->GetType());
    newIdx->SetDescription (db->idx->GetDescription());

    // Copy custom flags description
    char newDesc[ CUSTOM_FLAG_DESC_LENGTH + 1 ];
    for ( byte b = 1 ; b <= CUSTOM_FLAG_MAX ; b++ ) {
      db->idx->GetCustomFlagDesc( newDesc , b );
      newIdx->SetCustomFlagDesc( newDesc , b );
    }

    newIdx->SetAutoLoad (db->idx->GetAutoLoad());
    if (newIdx->CloseIndexFile() != OK  ||  newGfile->Close() != OK) {
        CLEANUP;
        errMsg = errWrite;
        return errorResult (ti, errMsg);
    }

    // Success: remove old index and game files, and the old filter:

    db->idx->CloseIndexFile();
    db->gfile->Close();
    // These four will fail on windows if a chess engine has been opened after the database
    // due to file descriptor inheritance.  It's a bit hard to handle accurately :(
    removeFile (db->fileName, INDEX_SUFFIX);
    removeFile (db->fileName, GFILE_SUFFIX);
    renameFile (newName, db->fileName, INDEX_SUFFIX);
    renameFile (newName, db->fileName, GFILE_SUFFIX);
    delete newIdx;
    delete newGfile;
    db->idx->SetFileName (db->fileName);
    db->idx->OpenIndexFile (db->fileMode);
    db->idx->ReadEntireFile ();
    db->gfile->Open (db->fileName, db->fileMode);

/*
    Probably too resource hungry to keep the old filter when compacting big bases

    if(db->dbFilter && db->dbFilter != db->filter)
        delete db->dbFilter;
    if(db->filter)
        delete db->filter;
    if(db->treeFilter)
        delete db->treeFilter;
    db->filter = newFilter;
    db->dbFilter = db->filter;
    db->treeFilter = NULL;
*/

    clearFilter (db, db->idx->GetNumGames());

    db->gameNumber = -1;
    db->numGames = db->idx->GetNumGames();
    recalcNameFrequencies (db->nb, db->idx);
    recalcFlagCounts (db);
    // Remove the out-of-date treecache file:
    db->treeCache->Clear();
    db->backupCache->Clear();
    removeFile (db->fileName, TREEFILE_SUFFIX);

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_compact_names:
//    Compact the name file of a database so it has no unused names.
//    This also requires rewriting the index file, since name ID
//    numbers will change.
//    if nb parameter is NULL, it is provided to fix a corrupted base
int
sc_compact_names (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    errorT err = OK;
    NameBase * nb = db->nb;
    idNumberT * idMapping [NUM_NAME_TYPES] = {NULL};
    bool showProgress = startProgressBar();

    if (db->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }

    NameBase * nbNew = new NameBase;
    nameT nt;

    // First, check if any names are unused:
    bool unusedNames = false;
    for (nt = NAME_FIRST; nt <= NAME_LAST; nt++) {
        uint numNames = db->nb->GetNumNames (nt);
        if (unusedNames) { break; }
        for (idNumberT id = 0; id < numNames; id++) {
            uint frequency = db->nb->GetFrequency (nt, id);
            if (frequency == 0) { unusedNames = true; break; }
        }
    }
    if (! unusedNames) {
        if (showProgress) { updateProgressBar (ti, 1, 1); }
        return TCL_OK;
    }

    for (nt = NAME_FIRST; nt <= NAME_LAST; nt++) {
        idNumberT numNames = nb->GetNumNames (nt);
        idMapping[nt] = new idNumberT [numNames];
        for (idNumberT oldID = 0; oldID < numNames; oldID++) {
            uint frequency = nb->GetFrequency (nt, oldID);
            if (frequency > 0) {
                uint newID = 0;
                const char * name = nb->GetName (nt, oldID);
                err = nbNew->AddName (nt, name, &newID);
                if (err != OK) {
                    errorResult (ti, "Error compacting namebase.");
                    break;
                }
                nbNew->IncFrequency (nt, newID, frequency);
                idMapping[nt][oldID] = newID;
            } else {
                idMapping[nt][oldID] = 0;
            }
            if (err != OK) { break; }
        }
        if (err != OK) { break; }
    }

    // Now correct the name ID numbers in all index entries:
    if (err == OK) {
        uint updateStart, update;
        updateStart = update = 5000;  // Update progress bar every 5000 games
        for (uint i=0; i < db->numGames; i++) {
            if (showProgress) {
                update--;
                if (update == 0) {
                    update = updateStart;
                    updateProgressBar (ti, i, db->numGames);
                    if (interruptedProgress()) break;
                }
            }
            IndexEntry * ie = db->idx->FetchEntry (i);
            IndexEntry ieNew = *ie;
            ieNew.SetWhite (idMapping [NAME_PLAYER][ie->GetWhite()]);
            ieNew.SetBlack (idMapping [NAME_PLAYER][ie->GetBlack()]);
            ieNew.SetEvent (idMapping [NAME_EVENT] [ie->GetEvent()]);
            ieNew.SetSite  (idMapping [NAME_SITE]  [ie->GetSite()]);
            ieNew.SetRound (idMapping [NAME_ROUND] [ie->GetRound()]);
            err = db->idx->WriteEntries (&ieNew, i, 1);
            if (err != OK) {
                errorResult (ti, "Error writing index file.");
                break;
            }
        }
    }

    if (showProgress) { updateProgressBar (ti, 1, 1); }

    // Now replace the old namebase and write the namebase file:
    if (err == OK) {
        nbNew->SetFileName (db->fileName);
        err = nbNew->WriteNameFile();
        if (err != OK) {
            errorResult (ti, "Error writing name file.");
        }
        delete db->nb;
        db->nb = nbNew;
    }

    for (nt = NAME_FIRST; nt <= NAME_LAST; nt++) {
        delete[] idMapping[nt];
    }

    if (err != OK) { return TCL_ERROR; }

    // Recompute player frequencies, ratings, etc:
    recalcNameFrequencies (db->nb, db->idx);
    recalcFlagCounts (db);
    recalcEstimatedRatings (db->nb);

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_compact_stats:
//    Returns information on compaction of the name or game file.
//    "sc_compact stats games" returns a 4-element list with the
//    current number of games, game file size (in kb), compacted
//    number of games, and compacted game file size (in kb).
//    "sc_compact stats names" returns an 8-element list: the number
//    of names (and then number of unused names) for each of the
//    player, event, site and round name types.
int
sc_compact_stats (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_compact stats games|names");
    }

    if (argv[2][0] == 'n') {
        for (nameT nt = NAME_PLAYER; nt < NUM_NAME_TYPES; nt++) {
            uint numNames = db->nb->GetNumNames (nt);
            uint numUnused = 0;
            for (idNumberT id = 0; id < numNames; id++) {
                uint frequency = db->nb->GetFrequency (nt, id);
                if (frequency == 0) { numUnused++; }
            }
            appendUintElement (ti, numNames);
            appendUintElement (ti, numUnused);
        }
    } else {
        if (argv[2][0] != 'g') {
          Tcl_AppendResult (ti, "Usage: sc_compact stats [games|names]", NULL);
          return TCL_ERROR;
        }

        bool setFilter = 0;
        if (strcmp( argv[2], "games_setfilter") == 0)
          setFilter = 1;

        // Game file compaction:

        uint nFullBlocks = 0;
        uint lastBlockBytes = 0;
        uint gameCount = 0;

        for (uint i=0; i < db->numGames; i++) {
            IndexEntry * ie = db->idx->FetchEntry (i);
            if (! ie->GetDeleteFlag()) {
                if (setFilter)
                  db->dbFilter->Set (i, 0);
                gameCount++;
                // Can this game fit in the current block?
                uint length = ie->GetLength();
                if (lastBlockBytes + length <= GF_BLOCKSIZE) {
                    lastBlockBytes += length;
                } else {
                    nFullBlocks++;
                    lastBlockBytes = length;
                }
            } else {
                if (setFilter)
                  db->dbFilter->Set (i, 1);
            }
        }
        if (setFilter)
            setMainFilter(db);

        uint oldBytes = db->gfile->GetFileSize();
        uint newBytes = nFullBlocks * GF_BLOCKSIZE + lastBlockBytes;

        appendUintElement (ti, db->numGames);
        appendUintElement (ti, oldBytes);
        appendUintElement (ti, gameCount);
        appendUintElement (ti, newBytes);
    }
    return TCL_OK;
}


//////////////////////////////////////////////////////////////////////
/// ECO Classification functions

// ecoTranslateT:
//    Structure for a linked list of ECO opening name translations.
//
struct ecoTranslateT {
    char   language;
    char * from;
    char * to;
    ecoTranslateT * next;
};

static ecoTranslateT * ecoTranslations = NULL;
void translateECO (Tcl_Interp * ti, const char * strFrom, DString * dstrTo);

int
sc_eco (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    int index = -1;
    static const char * options [] = {
        "base", "game", "read", "reset", "size", "summary",
        "translate", "find", NULL
    };
    enum {
        ECO_BASE, ECO_GAME, ECO_READ, ECO_RESET, ECO_SIZE, ECO_SUMMARY,
        ECO_TRANSLATE, ECO_FIND
    };

    if (argc > 1) { index = strUniqueMatch (argv[1], options); }

    switch (index) {
    case ECO_BASE:
        return sc_eco_base (cd, ti, argc, argv);

    case ECO_GAME:
        return sc_eco_game (cd, ti, argc, argv);

    case ECO_READ:
        return sc_eco_read (cd, ti, argc, argv);

    case ECO_RESET:
        if (ecoBook) { delete ecoBook; ecoBook = NULL; }
        break;

    case ECO_SIZE:
        return setUintResult (ti, ecoBook == NULL ? 0 : ecoBook->Size());

    case ECO_SUMMARY:
        return sc_eco_summary (cd, ti, argc, argv);

    case ECO_FIND:
        return sc_eco_find (cd, ti, argc, argv);

    case ECO_TRANSLATE:
        return sc_eco_translate (cd, ti, argc, argv);

    default:
        return InvalidCommand (ti, "sc_eco", options);
    }

    return TCL_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_eco_base:
//    Reclassifies all games in the current base by ECO code.
//
//    The first parameter inidicates if all games (not only those
//    with no existing ECO code) should be classified.
//       "0" or "nocode": only games with no ECO code.
//       "1" or "all": classify all games.
//       "date:yyyy.mm.dd": only games since date.
//    The second boolean parameter indicates if Scid-specific ECO
//    extensions (e.g. "B12j" instead of just "B12") should be used.
//
//    If the database is read-only, games can still be classified but
//    the results will not be stored to the database file.
int
sc_eco_base (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 4) {
        return errorResult (ti, "Usage: sc_eco <bool:all_games> <bool:extensions>");
    }
    if (!ecoBook) { return errorResult (ti, "No ECO Book is loaded."); }
    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    int option = -1;
    enum {ECO_NOCODE, ECO_ALL, ECO_DATE, ECO_FILTER};

    switch (argv[2][0]) {
    case '0':
    case 'n':
        option = ECO_NOCODE; break;
    case 'd':
        option = ECO_DATE; break;
    case 'f':
        option = ECO_FILTER; break;
    default:
        option = ECO_ALL; break;
    }

    bool extendedCodes = strGetBoolean(argv[3]);
    bool showProgress = startProgressBar();
    Game * g = scratchGame;
    IndexEntry * ie;
    uint updateStart, update;
    updateStart = update = 1000;  // Update progress bar every 1000 games
    errorT err = OK;
    uint countClassified = 0;  // Count of games classified.
    dateT startDate = ZERO_DATE;
    if (option == ECO_DATE) {
        startDate = date_EncodeFromString (&(argv[2][5]));
    }

    Timer timer;  // Time the classification operation.

    // Read each game:
    for (uint i=0; i < db->numGames; i++) {
        if (showProgress) {  // Update the percentage done bar:
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, i, db->numGames);
                if (interruptedProgress()) break;
            }
        }
        ie = db->idx->FetchEntry (i);
        if (ie->GetLength() == 0) { continue; }

        ecoT oldEcoCode = ie->GetEcoCode();

        // Ignore games not in current filter if directed:
        if (option == ECO_FILTER  &&  db->filter->Get(i) == 0) { continue; }

        // Ignore games with existing ECO code if directed:
        if (option == ECO_NOCODE  &&  oldEcoCode != 0) { continue; }

        // Ignore games before starting date if directed:
        if (option == ECO_DATE  &&  ie->GetDate() < startDate) { continue; }

        if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(),
                                 ie->GetLength()) != OK) {
            continue;
        }
        db->bbuf->BackToStart();
        g->Clear();
        if (g->DecodeStart (db->bbuf) != OK) { continue; }

        // First, read in the game -- with a limit of 30 moves per
        // side, since an ECO match after move 31 is very unlikely and
        // we can save time by setting a limit. Also, stop when the
        // material left in on the board is less than that of the
        // book position with the least material, since no further
        // positions in the game could possibly match.

        uint maxPly = 60;
        uint leastMaterial = ecoBook->FewestPieces();
        uint material;

        do {
            err = g->DecodeNextMove (db->bbuf, NULL);
            maxPly--;
            material = g->GetCurrentPos()->TotalMaterial();
        } while (err == OK  &&  maxPly > 0  &&  material >= leastMaterial);

        // Now, move back through the game to the start searching for a
        // match in the ECO book. Stop at the first match found since it
        // is the deepest.

        DString commentStr;
        bool found = false;

        do {
            if (ecoBook->FindOpcode (g->GetCurrentPos(), "eco",
                                     &commentStr) == OK) {
                found = true;
                break;
            }
            err = g->MoveBackup();
        } while (err == OK);

        ecoT ecoCode = ECO_None;
        if (found) {
            ecoCode = eco_FromString (commentStr.Data());
            if (! extendedCodes) {
                ecoCode = eco_BasicCode (ecoCode);
            }
        }
        ie->SetEcoCode (ecoCode);
        countClassified++;

        // If the database is read-only or the ECO code has not changed,
        // nothing needs to be written to the index file.
        // Write the updated entry if necessary:

        if (db->fileMode != FMODE_ReadOnly
                &&  ie->GetEcoCode() != oldEcoCode) {
            if (db->idx->WriteEntries (ie, i, 1) != OK) {
                return errorResult (ti, "Error writing index file.");
            }
        }
    }

    // Update the index file header:
    if (db->fileMode != FMODE_ReadOnly) {
        if (db->idx->WriteHeader() != OK) {
            return errorResult (ti, "Error writing index file.");
        }
    }

    recalcFlagCounts (db);
    if (showProgress) { updateProgressBar (ti, 1, 1); }

    int centisecs = timer.CentiSecs();
    char tempStr [100];
    sprintf (tempStr, "Classified %d game%s in %d%c%02d seconds",
             countClassified, strPlural (countClassified),
             centisecs / 100, decimalPointChar, centisecs % 100);
    Tcl_AppendResult (ti, tempStr, NULL);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_eco_game:
//    Returns ECO code for the curent game. If the optional
//    parameter <ply> is passed, it returns the ply depth of the
//    deepest match instead of the ECO code.
int
sc_eco_game (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    int found = 0;
    uint ply = 0;
    uint returnPly = 0;
    if (argc > 2) {
        if (argc == 3  &&  strIsPrefix (argv[2], "ply")) {
            returnPly = 1;
        } else {
            return errorResult (ti, "Usage: sc_game eco [ply]");
        }
    }
    if (!ecoBook) { return TCL_OK; }

    db->game->SaveState();
    db->game->MoveToPly (0);
    DString ecoStr;

    do {} while (db->game->MoveForward() == OK);
    do {
        if (ecoBook->FindOpcode (db->game->GetCurrentPos(), "eco",
                                 &ecoStr) == OK) {
            found = 1;
            ply = db->game->GetCurrentPly();
            break;
        }
    } while (db->game->MoveBackup() == OK);

    if (found) {
        if (returnPly) {
            setUintResult (ti, ply);
        } else {
            ecoT ecoCode = eco_FromString (ecoStr.Data());
            ecoStringT ecoStr;
            eco_ToExtendedString (ecoCode, ecoStr);
            Tcl_AppendResult (ti, ecoStr, NULL);
        }
    }
    db->game->RestoreState ();
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_eco_read:
//    Reads a book file for ECO classification.
//    Returns the book size (number of positions).
int
sc_eco_read (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc < 3) { return TCL_OK; }
    if (ecoBook) { delete ecoBook; }
    ecoBook = new PBook;
    ecoBook->SetFileName (argv[2]);
    errorT err = ecoBook->ReadEcoFile();
    if (err != OK) {
        if (err == ERROR_FileOpen) {
            Tcl_AppendResult (ti, "Unable to open the ECO file:\n",
                              argv[2], NULL);
        } else {
            Tcl_AppendResult (ti, "Unable to load the ECO file:\n",
                              argv[2], NULL);
            Tcl_AppendResult (ti, "\n\nError at line ", NULL);
            appendUintResult (ti, ecoBook->GetLineNumber());
        }
        delete ecoBook;
        ecoBook = NULL;
        return TCL_ERROR;
    }
    return setUintResult (ti, ecoBook->Size());
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_eco_summary:
//    Returns a listing of positions for the specified ECO prefix,
//    in plain text or color (Scid hypertext) format.
int
sc_eco_summary (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool color = 0;
    if (argc != 3  &&  argc != 4) {
        return errorResult (ti, "Usage: sc_eco summary <ECO-prefix> [<bool:color>]");
    }
    if (argc == 4) { color = strGetBoolean (argv[3]); }
    if (!ecoBook) { return TCL_OK; }
    DString * dstr = new DString;
    DString * temp = new DString;
    DString * thisEco = new DString;
    bool inMoveList = false;
    bool inThisEco = true;
    ecoBook->EcoSummary (argv[2], temp);
    translateECO (ti, temp->Data(), dstr);
    temp->Clear();
    thisEco->Clear();
    if (color) {
        DString * oldstr = dstr;
        dstr = new DString;
        const char * s = oldstr->Data();
        while (*s) {
            char ch = *s;
            switch (ch) {
            case '[':
                inThisEco = false;
                dstr->Append ("<tab>");
                dstr->AddChar (ch);
                break;
            case ']':
                // Fulvio's bugfix for
                // https://sourceforge.net/p/scid/bugs/63/
                dstr->AddChar (ch);
                dstr->Append ("<blue><run ::windows::eco::importMoveList {");
                inMoveList = true;
                temp->Clear();
                break;
            case '\n':
                if (inMoveList) {
                    dstr->Append ("} ", thisEco->Data(), ">", temp->Data());
                    thisEco->Clear();
                    inThisEco = true;
                    inMoveList = false;
                }
                dstr->Append ("</run></blue></tab><br>");
                break;
            default:
                dstr->AddChar (ch);
                if (inThisEco) { thisEco->AddChar (ch); }
                if (inMoveList) { temp->AddChar (transPiecesChar(ch)); }
            }
            s++;
        }
        delete oldstr;
    }
    Tcl_AppendResult (ti, dstr->Data(), NULL);
    delete temp;
    delete dstr;
    return TCL_OK;
}

//    Find ECO entries matching a case-sensitive string.

int
sc_eco_find (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3 && argc != 4) {
        return errorResult (ti, "Usage: sc_eco find <string> [color]");
    }
    if (!ecoBook) { return TCL_OK; }

    bool color = 0;
    // Currently only color output is used (tcl/windows/eco.tcl, sc_eco find, argc == 4) S.A.
    if (argc == 4) { color = strGetBoolean (argv[3]); }

    DString * dstr = new DString;
    DString * temp = new DString;
    DString * thisEco = new DString;
    bool inMoveList = false;
    bool inThisEco = true;
    ecoBook->EcoFind (argv[2], temp);
    translateECO (ti, temp->Data(), dstr);
    temp->Clear();
    thisEco->Clear();
    if (color) {
        DString * oldstr = dstr;
        dstr = new DString;
        const char * s = oldstr->Data();
        while (*s) {
            char ch = *s;
            switch (ch) {
            case '[':
                inThisEco = false;
                dstr->Append ("<tab>");
                dstr->AddChar (ch);
                break;
            case ']':
                dstr->AddChar (ch);
                dstr->Append ("<blue><run ::windows::eco::importMoveList {");
                inMoveList = true;
                temp->Clear();
                break;
            case '\n':
                if (inMoveList) {
                    dstr->Append ("} ", thisEco->Data(), ">", temp->Data());
                    thisEco->Clear();
                    inThisEco = true;
                    inMoveList = false;
                }
                dstr->Append ("</run></blue></tab><br>");
                break;
            default:
                dstr->AddChar (ch);
                if (inThisEco) { thisEco->AddChar (ch); }
                if (inMoveList) { temp->AddChar (transPiecesChar(ch)); }
            }
            s++;
        }
        delete oldstr;
    }
    Tcl_AppendResult (ti, dstr->Data(), NULL);
    delete temp;
    delete dstr;
    delete thisEco;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_eco_translate:
//    Adds a new ECO openings phrase translation.
int
sc_eco_translate (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 5) {
        return errorResult (ti, "Usage: sc_eco translate <lang> <from> <to>");
    }

    ecoTranslateT * trans = new ecoTranslateT;
    trans->next = ecoTranslations;
    trans->language = argv[2][0];
    trans->from = strDuplicate (argv[3]);
    trans->to = strDuplicate (argv[4]);
    ecoTranslations = trans;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// translateECO:
//    Translates an ECO opening name into the current language.
//
void
translateECO (Tcl_Interp * ti, const char * strFrom, DString * dstrTo)
{
    ecoTranslateT * trans = ecoTranslations;
    dstrTo->Clear();
    dstrTo->Append (strFrom);
    const char * language = Tcl_GetVar (ti, "language", TCL_GLOBAL_ONLY);
    if (language == NULL) { return; }
    char lang = language[0];
    while (trans != NULL) {
        if (trans->language == lang
            &&  strContains (dstrTo->Data(), trans->from)) {
            // Translate this phrase in the string:
            char * temp = strDuplicate (dstrTo->Data());
            dstrTo->Clear();
            char * in = temp;
            while (*in != 0) {
                if (strIsPrefix (trans->from, in)) {
                    dstrTo->Append (trans->to);
                    in += strLength (trans->from);
                } else {
                    dstrTo->AddChar (*in);
                    in++;
                }
            }
            delete[] temp;
        }
        trans = trans->next;
    }
}


//////////////////////////////////////////////////////////////////////
///  FILTER functions

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// filter_reset:
//    Resets the filter of the specified base to include all or no games.
void
filter_reset (scidBaseT * base, byte value)
{
    if (base->inUse) {
        base->dbFilter->Fill (value);
	if (base->dbFilter != base->filter)
	  base->filter->Fill (value);
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter: filter commands.  (Some) valid minor commands:
//    count:     returns the number of games in the filter.
//    reset:     resets the filter so all games are included.
//    remove:    removes game number <x> from the filter.
//    stats:     prints filter statistics.
//    value:     return value for this base and game
//               0 value means game is not in filter,
//               N value represents the ply at which to load game
int
sc_filter (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    int index = -1;
    static const char * options [] = {
        "copy", "count", "first", "frequency",
        "index", "last", "locate", "negate",
	"next", "previous", "remove", "reset", "end",
	"size", "stats", "textfind", "textfilter", "value", "ply", "clear", NULL
    };
    enum {
        FILTER_COPY, FILTER_COUNT, FILTER_FIRST, FILTER_FREQ,
        FILTER_INDEX, FILTER_LAST, FILTER_LOCATE, FILTER_NEGATE,
        FILTER_NEXT, FILTER_PREV, FILTER_REMOVE, FILTER_RESET, FILTER_END,
        FILTER_SIZE, FILTER_STATS, FILTER_TEXTFIND, FILTER_TEXTFILTER, FILTER_VALUE, FILTER_PLY,
        FILTER_CLEAR
    };

    if (argc > 1) { index = strUniqueMatch (argv[1], options); }

    switch (index) {
    case FILTER_COPY:
        return sc_filter_copy (cd, ti, argc, argv);

    case FILTER_COUNT:
        return sc_filter_count (cd, ti, argc, argv);

    case FILTER_FIRST:
        return sc_filter_first (cd, ti, argc, argv);

    case FILTER_FREQ:
        return sc_filter_freq (cd, ti, argc, argv);

    case FILTER_INDEX:
        return sc_filter_index (cd, ti, argc, argv);

    case FILTER_LAST:
        return sc_filter_last (cd, ti, argc, argv);

    case FILTER_LOCATE:
        return sc_filter_locate (cd, ti, argc, argv);

    case FILTER_NEGATE:
        return sc_filter_negate (cd, ti, argc, argv);

    case FILTER_NEXT:
        return sc_filter_next (cd, ti, argc, argv);

    case FILTER_PREV:
        return sc_filter_prev (cd, ti, argc, argv);

    case FILTER_RESET:
        return sc_filter_reset (cd, ti, argc, argv);

    case FILTER_END:
        return sc_filter_end (cd, ti, argc, argv);

    case FILTER_REMOVE:
        return sc_filter_remove (cd, ti, argc, argv);

    case FILTER_SIZE:   // Alias for "sc_filter count"
        return sc_filter_count (cd, ti, argc, argv);

    case FILTER_STATS:
        return sc_filter_stats (cd, ti, argc, argv);

    case FILTER_TEXTFIND:
        return sc_filter_textfind (cd, ti, argc, argv);

    case FILTER_TEXTFILTER:
        return sc_filter_textfilter (cd, ti, argc, argv);

    case FILTER_VALUE:
        return sc_filter_value (cd, ti, argc, argv);

    case FILTER_PLY:
        return sc_filter_ply (cd, ti, argc, argv);

    // --- clear filter
    case FILTER_CLEAR:
        return sc_filter_clear (cd, ti, argc, argv);

    default:
        return InvalidCommand (ti, "sc_filter", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_count:
//    Takes an optional database number and returns the current filter size.
//    Defaults to the active database.
int
sc_filter_count (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * basePtr = db;
    if (argc > 2) {
        int baseNum = strGetInteger (argv[2]);
        if (baseNum < 1 || baseNum > MAX_BASES) {
            return errorResult (ti, "Invalid database number.");
        }
        basePtr = &(dbList[baseNum - 1]);
    }
    return setUintResult (ti, basePtr->inUse ? basePtr->filter->Count() : 0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_copy:
//    Copies all the games in the source base filter to the target base.
int
sc_filter_copy (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = startProgressBar();
    if (argc != 4 && argc != 5) {
        return errorResult (ti, "Usage: sc_filter copy <fromBase> <toBase> [items]");
    }
    uint updateStart, update;
    updateStart = update = 1000;  // Update progress bar every 100 games

    int sourceBaseNum = strGetInteger (argv[2]);
    int targetBaseNum = strGetInteger (argv[3]);
    if (sourceBaseNum < 1  ||  sourceBaseNum > MAX_BASES) {
        return errorResult (ti, "Invalid source base number.");
    }
    if (targetBaseNum < 1  ||  targetBaseNum > MAX_BASES) {
        return errorResult (ti, "Invalid target base number.");
    }
    scidBaseT * sourceBase = &(dbList[sourceBaseNum - 1]);
    scidBaseT * targetBase = &(dbList[targetBaseNum - 1]);
    if (! sourceBase->inUse) {
        return errorResult (ti, "The source database is not open.");
    }
    if (! targetBase->inUse) {
        return errorResult (ti, "The target database is not open.");
    }
    if (targetBase->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, "The target database is read-only.");
    }

    // Currently only used by gamelist context menu gamelist::CopyFilter "Copy to filter", but could be expanded
    bool listMode = (argc == 5);
    uint targetCount;
    int largc = 0;
    const char ** largv;

    if (listMode) {
	if (Tcl_SplitList (ti, (char *)argv[4], &largc, (CONST84 char ***) &largv) != TCL_OK) {
	    return errorResult (ti, "Error parsing filter list.");
	}
        if (largc <= 0) {
	   Tcl_Free ((char *) largv);
           return errorResult (ti, "Zero args when parsing filter list.");
        }
        targetCount = (uint)largc;
     } else {
        targetCount = sourceBase->filter->Count();
     }

    // Now copy each game from source to target:
    uint count = 0;

    // Check target db has enough capacity
    uint newSize = targetBase->numGames + targetCount;
    targetBase->filter->SetCapacity(newSize);
    // (FilterSize, Count, is increased in sc_savegame below)
    if( targetBase->filter != targetBase->dbFilter )
        targetBase->dbFilter->SetCapacity(newSize);
    if( targetBase->treeFilter != NULL)
        targetBase->treeFilter->SetCapacity(newSize);

    if (listMode) {
	for (uint i=0; i < targetCount; i++) {
	    count++;
	    IndexEntry * ie = sourceBase->idx->FetchEntry (atoi(largv[i])-1);
	    sourceBase->bbuf->Empty();
	    if (sourceBase->gfile->ReadGame (sourceBase->bbuf, ie->GetOffset(), ie->GetLength()) != OK) {
		return errorResult (ti, "Error reading game file.");
	    }
	    if (sc_savegame (ti, sourceBase, sourceBase->bbuf, ie, targetBase) != TCL_OK) {
		    return TCL_ERROR;
	    }
	}
	Tcl_Free ((char *) largv);
    } else {

    for (uint i=0; i < sourceBase->numGames; i++) {
        if (sourceBase->filter->Get(i) == 0) { continue; }
        count++;
        if (showProgress) {  // Update the percentage done bar:
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, count, targetCount);
                if (interruptedProgress()) break;
            }
        }

        IndexEntry * ie = sourceBase->idx->FetchEntry (i);
        sourceBase->bbuf->Empty();
        if (sourceBase->gfile->ReadGame (sourceBase->bbuf, ie->GetOffset(),
                                         ie->GetLength()) != OK) {
            return errorResult (ti, "Error reading game file.");
        }

	if (sc_savegame (ti, sourceBase, sourceBase->bbuf, ie, targetBase) != TCL_OK) {
		return TCL_ERROR;
        }
    }

    }

    targetBase->gfile->FlushAll();

    // Now write the Index file header and the name file:
    if (targetBase->idx->WriteHeader() != OK) {
        return errorResult (ti, "Error writing index file.");
    }
    if (! targetBase->memoryOnly  &&  targetBase->nb->WriteNameFile() != OK) {
        return errorResult (ti, "Error writing name file.");
    }

    // Ensure that the Index is still all in memory:
    targetBase->idx->ReadEntireFile();

    recalcFlagCounts (targetBase);
    // The target base treecache is out of date:
    targetBase->treeCache->Clear();
    targetBase->backupCache->Clear();
    if (! targetBase->memoryOnly) {
        removeFile (targetBase->fileName, TREEFILE_SUFFIX);
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_first:
//    Returns the game number of the first game in the filter,
//    or 0 if the filter is empty.
int
sc_filter_first (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    uint first = 0;
    if (db->filter->Count() != 0) {
       first = db->filter->FilteredCountToIndex (1) + 1;
    }
    return setUintResult (ti, first);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_freq:
//    Returns a two-integer list showing how many filter games,
//    and how many total database games, meet the specified criteria.
//    Usage:
//        sc_filter freq date <startdate> [<endDate>]
//        sc_filter freq elo <lowerMeanElo> [<upperMeanElo>]
//     or sc_filter freq move <lowerhalfMove> <higherhalfMove>
//         add mode to count games with specified movenumber
//    where the final parameter defaults to the maximum allowed
//    date , Elo rating or moves.
//    Note for rating queries: only the rating values in the game
//    are used; estimates from other games or the spelling file
//    will be ignored. Also, if one player has an Elo rating but
//    the other does not, the other rating will be assumed to be
//    same as the nonzero rating, up to a maximum of 2200.
int
sc_filter_freq (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage =
        "Usage: sc_filter freq date|elo|move <startDate|minElo|lowerhalfMove> [<endDate|maxElo|higherhalfMove>][GuessElo]";

    bool eloMode = false;
    bool moveMode = false;
    bool guessElo = true;
    const char * options[] = { "date", "elo", "move", NULL };
    enum { OPT_DATE, OPT_ELO, OPT_MOVE };
    int option = -1;
    if (argc >= 4  &&  argc <= 6) {
        option = strUniqueMatch (argv[2], options);
    }
    switch (option) {
        case OPT_DATE: eloMode = false; break;
        case OPT_ELO: eloMode = true; break;
        case OPT_MOVE: moveMode = true; break;
        default: return errorResult (ti, usage);
    }

    dateT startDate = date_EncodeFromString (argv[3]);
    dateT endDate = DATE_MAKE (YEAR_MAX, 12, 31);
    uint minElo = strGetUnsigned (argv[3]);
    uint maxElo = MAX_ELO;
    uint maxMove, minMove;

    minMove = minElo;
    maxMove = minMove + 1;
    if (argc >= 5) {
        endDate = date_EncodeFromString (argv[4]);
        maxMove = maxElo = strGetUnsigned (argv[4]);
    }
    if (argc == 6) {
        guessElo = strGetUnsigned (argv[5]);
    }
    //Klimmek: define _NoEloGuess_: Do not guess Elo, else old behavior
    if ( guessElo ) {
    // Double min/max Elos to avoid halving every mean Elo:
        minElo = minElo + minElo;
        maxElo = maxElo + maxElo + 1;
    }
    // Calculate frequencies in the specified date or rating range:
    if (!db->inUse) {
        appendUintElement (ti, 0);
        appendUintElement (ti, 0);
        return TCL_OK;
    }

    uint filteredCount = 0;
    uint allCount = 0;

    if (eloMode) {
        for (uint gnum=0; gnum < db->numGames; gnum++) {
            IndexEntry * ie = db->idx->FetchEntry (gnum);
            if ( guessElo ) {
                uint wElo = ie->GetWhiteElo();
                uint bElo = ie->GetBlackElo();
                uint bothElo = wElo + bElo;
                if (wElo == 0  &&  bElo != 0) {
                    bothElo += (bElo > 2200 ? 2200 : bElo);
                } else if (bElo == 0  &&  wElo != 0) {
                    bothElo += (wElo > 2200 ? 2200 : wElo);
                }
                if (bothElo >= minElo  &&  bothElo <= maxElo) {
                    allCount++;
                    if (db->filter->Get(gnum) != 0) {
                        filteredCount++;
                    }
                }
            } else {
                //Klimmek: if lowest Elo in the Range: count them
                uint mini = ie->GetWhiteElo();
                if ( mini > ie->GetBlackElo() ) mini = ie->GetBlackElo();
                if (mini < minElo  ||  mini >= maxElo)
                    continue;
                allCount++;
                if (db->filter->Get(gnum) != 0) {
                    filteredCount++;
                }
            }
        }
    } else if ( moveMode ) {
        //Klimmek: count games with x Moves minMove=NumberHalfmove and maxMove Numberhalfmove+1
        for (uint gnum=0; gnum < db->numGames; gnum++) {
            IndexEntry * ie = db->idx->FetchEntry (gnum);
            uint move = ie->GetNumHalfMoves();
            if (move >= minMove  &&  move <= maxMove) {
                allCount++;
                if (db->filter->Get(gnum) != 0) {
                    filteredCount++;
                }
            }
        }
    }
    else { // datemode
        for (uint gnum=0; gnum < db->numGames; gnum++) {
            IndexEntry * ie = db->idx->FetchEntry (gnum);
            dateT date = ie->GetDate();
            if (date >= startDate  &&  date <= endDate) {
                allCount++;
                if (db->filter->Get(gnum) != 0) {
                    filteredCount++;
                }
            }
        }
    }
    appendUintElement (ti, filteredCount);
    appendUintElement (ti, allCount);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_index:
//    Returns the game number of the "count"th game in the filter,
//    or 0 if the filter is empty or "count" is greater than the
//    current filter size.
int
sc_filter_index (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    uint index = 0;
    uint count = 0;
    if (argc == 3) { count = strGetUnsigned (argv[2]); }
    if (count >= 1  &&  count <= db->filter->Count()) {
        index = db->filter->FilteredCountToIndex (count) + 1;
    }
    return setUintResult (ti, index);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_last:
//    Returns the game number of the last game in the filter,
//    or 0 if the filter is empty.
int
sc_filter_last (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    uint last = 0;
    uint count = db->filter->Count();
    if (count != 0) {
        last = db->filter->FilteredCountToIndex (count) + 1;
    }
    return setUintResult (ti, last);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_locate:
//    Given a game number, returns the count of filtered games up to
//    and including that game number. Used for jumping to a specified
//    game number in the game list.
int
sc_filter_locate (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_filter locate <gameNumber>");
    }
    uint filteredCount = 0;
    if (db->inUse) {
        uint gnumber = strGetUnsigned (argv[2]);
        filteredCount = db->filter->IndexToFilteredCount(gnumber);
    }
    return setUintResult (ti, filteredCount);
}

//~~~~~~~~~~~~~~~~~~
// sc_filter_negate:
//    Negates the filter to only include excluded games.
int
sc_filter_negate (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (db->inUse) {
        Filter * filter = db->dbFilter;
        uint i;
        for (i=0; i < db->numGames; i++) {
          filter->Set (i, filter->Get(i) == 0 ? 1 : 0);
        }
        setMainFilter (db);
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_next:
//    Returns number of next game in the filter.
//    (or the game after argv[2])
//    Note there is an offset difference of 1 between the tcl gamenumber and the C gamenumber
//    The conversion is done by both sides BEFORE passing to the other, which is a little strange
int
sc_filter_next (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{

    if (db->inUse) {

        uint nextNumber = db->gameNumber + 1;

	if (argc > 2)
	    nextNumber = strGetUnsigned (argv[2]) + 1;

        while (nextNumber < db->numGames) {
            if (db->filter->Get(nextNumber) > 0) {
                return setUintResult (ti, nextNumber + 1);
            }
            nextNumber++;
        }
    }
    return setUintResult (ti, 0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_prev:
//    Returns number of previous game in the filter.
//    Note: prevNumber seems to work fine as 'int' rather tha 'uint'
int
sc_filter_prev (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (db->inUse) {

        int prevNumber = db->gameNumber - 1;

	if (argc > 2)
	    prevNumber = strGetUnsigned (argv[2]) - 1;

        while (prevNumber >= 0) {
            if (db->filter->Get(prevNumber) > 0) {
                return setIntResult (ti, prevNumber + 1);
            }
            prevNumber--;
        }
    }
    return setUintResult (ti, 0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_remove:
//    Removes the numbered game from the filter.
int
sc_filter_remove (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3  &&  argc != 4) {
        return errorResult (ti, "Usage: sc_filter remove <from> [<to>]");
    }
    if (! db->inUse) { return TCL_OK; }
    uint from = strGetUnsigned (argv[2]);
    uint to = from;
    if (argc == 4) { to = strGetUnsigned (argv[3]); }
    if (from < 1) { from = 1; }
    if (to > db->numGames) { to = db->numGames; }

    for (uint gnumber = from; gnumber <= to; gnumber++) {
        db->dbFilter->Set (gnumber - 1, 0);
    }
    // Zero main filter games accordingly
    if( db->dbFilter != db->filter) {
        for (uint i=0; i < db->numGames; i++) {
            if( db->dbFilter->Get(i) == 0 )
                db->filter->Set(i,0);
        }
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_reset:
//    Takes an optional base number (defaults to current base) and resets
//    its filter to include all games.
int
sc_filter_reset (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * basePtr;

    if (argc > 2) {
        int baseNum = strGetInteger (argv[2]);
        if (baseNum < 1 || baseNum > MAX_BASES) {
            return errorResult (ti, "Invalid database number.");
        }
        basePtr = &(dbList[baseNum - 1]);
    } else {
        basePtr = db;
    }

    filter_reset (basePtr, 1);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_end:
//    Set all filter games to move 255
//    its filter to include all games.
int
sc_filter_end (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{

    scidBaseT * basePtr = db;
    if (argc > 2) {
        int baseNum = strGetInteger (argv[2]);
        if (baseNum < 1 || baseNum > MAX_BASES) {
            return errorResult (ti, "Invalid database number.");
        }
        basePtr = &(dbList[baseNum - 1]);
    }

    for (uint i=0; i < basePtr->numGames; i++)
    {
	if( basePtr->filter->Get(i) > 0)
	    basePtr->filter->Set(i,255);
    }

    return TCL_OK;
}

//    Takes an optional base number (defaults to current base) and
//    clears all filters ie. db filter as well as tree filter
//    To clear only the tree filter use sc_tree clean

int
sc_filter_clear (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * basePtr = db;
    if (argc > 2) {
        int baseNum = strGetInteger (argv[2]);
        if (baseNum < 1 || baseNum > MAX_BASES) {
            return errorResult (ti, "Invalid database number.");
        }
        basePtr = &(dbList[baseNum - 1]);
    }
    clearFilter(basePtr, basePtr->numGames);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_stats:
//    Returns statistics about the filter.
int
sc_filter_stats (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    enum {STATS_ALL, STATS_ELO, STATS_YEAR};

    if (argc < 2 || argc > 5) {
        return errorResult (ti, "Usage: sc_filter stats [all | elo <xx> | year <xx>]");
    }
    int statType = STATS_ALL;
    uint min = 0;
    uint max = 0;
    uint inv_max = 0;
    if (argc > 2) {
        if (argv[2][0] == 'e') { statType = STATS_ELO; }
        if (argv[2][0] == 'y') { statType = STATS_YEAR; }
    }
    if (statType == STATS_ELO  ||  statType == STATS_YEAR) {
        if (argc < 4) {
            return errorResult (ti, "Incorrect number of parameters.");
        }
        min = strGetUnsigned (argv[3]);
        max = strGetUnsigned (argv[4]);
        //Klimmek: +10000 workaround to trigger max elo in filter function
        if ( max > 10000 ) {
            max -= 10000;
            inv_max = 1;
        }
    }
    uint results[4] = {0, 0, 0, 0};
    uint total = 0;
    for (uint i=0; i < db->numGames; i++) {
        IndexEntry * ie = db->idx->FetchEntry (i);
        if (db->filter->Get(i)) {
            if ( max == 0 ) { //Old Statistic :
                if (statType == STATS_ELO  &&
                    (ie->GetWhiteElo() < min  ||  ie->GetBlackElo() < min)) {
                    continue;
                }
                if (statType == STATS_YEAR
                    &&  date_GetYear(ie->GetDate()) < min) {
                    continue;
                }
            } else { //Klimmek:  new statistic: evaluation in intervalls
                //count all games where player whith highest Elo is in the specific range
                if (statType == STATS_ELO ) {
                    if (inv_max) {
                        uint maxi = ie->GetWhiteElo();
                        if ( maxi < ie->GetBlackElo() ) maxi = ie->GetBlackElo();
                        if (maxi < min  ||  maxi >= max)
                            continue;
                    }
                    else {
                //count all games where player whith lowest Elo is in the specific range
                        uint mini = ie->GetWhiteElo();
                        if ( mini > ie->GetBlackElo() ) mini = ie->GetBlackElo();
                        if (mini < min  ||  mini >= max)
                            continue;
                    }
                }
                if (statType == STATS_YEAR
                    &&  ( date_GetYear(ie->GetDate()) < min || date_GetYear(ie->GetDate()) >= max) ) {
                    continue;
                }
            }
            results[ie->GetResult()]++;
            total++;
        }
    }
    char temp[80];
    uint percentScore = results[RESULT_White] * 2 + results[RESULT_Draw] +
        results[RESULT_None];
    percentScore = total ? percentScore * 500 / total : 0;
    sprintf (temp, "%7u %7u %7u %7u   %3u%c%u%%",
             total,
             results[RESULT_White],
             results[RESULT_Draw],
             results[RESULT_Black],
             percentScore / 10, decimalPointChar, percentScore % 10);
    Tcl_AppendResult (ti, temp, NULL);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_textfind:
//    Finds the next game that contains the specified text
//    and ignoring spaces, in its White, Black, Event or Site fields.
//
//    Stevenaaus 29/05/2010
//    This proc has been hacked to handle case sensitive searches,
int
sc_filter_textfind (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool (*stringcompare)(const char*, const char*);

    if (argc != 5) {
        return errorResult (ti, "Usage: sc_filter textfind <case_bool> <startGame> <searchText>");
    }

    if (strGetUnsigned(argv[2])==0)
	stringcompare = strContains;
    else
	stringcompare = strCaseContains;

    const char * text = argv[4];
    char datestring[12];
    datestring[0]=0;
    if (db->inUse) {
        NameBase * nb = db->nb;
        uint filteredCount = strGetUnsigned (argv[3]);
        uint start = db->filter->FilteredCountToIndex (filteredCount) + 1;

        while (start < db->numGames) {
            if (db->filter->Get(start) > 0) {
                filteredCount++;
                IndexEntry * ie = db->idx->FetchEntry (start);
		date_DecodeToString(ie->GetDate(), datestring);
                if ((stringcompare (ie->GetWhiteName (nb), text))  ||
                    (stringcompare (ie->GetBlackName (nb), text))  ||
                    (stringcompare (ie->GetEventName (nb), text))  ||
                    (stringcompare (datestring, text))  ||
                    (stringcompare (ie->GetSiteName (nb), text)))
                {
			return setUintResult (ti, filteredCount);
                }
            }
            start++;
        }
    }
    return setUintResult (ti, 0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_textfilter:
//    Stevenaaus 29/05/2010
//    Removes games that match string
//    in its White, Black, Event or Site fields.
//
//    Returns number of games removed
int
sc_filter_textfilter (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool (*stringcompare)(const char*, const char*);
    uint number_removed = 0;

    if (argc != 4) {
        return errorResult (ti, "Usage: sc_filter textfilter <case_bool> <searchText>");
    }

    if (strGetUnsigned(argv[2])==0)
	stringcompare = strContains;
    else
	stringcompare = strCaseContains;

    const char * text = argv[3];
    char datestring[12];
    datestring[0]=0;

    if (db->inUse) {
        NameBase * nb = db->nb;
        uint filteredCount = 1;
        uint start = 0;

        while (start < db->numGames) {

            if (db->dbFilter->Get(start) > 0) {
                filteredCount++;
                IndexEntry * ie = db->idx->FetchEntry (start);
		date_DecodeToString(ie->GetDate(), datestring);

                if ((stringcompare (ie->GetWhiteName (nb), text))  ||
                    (stringcompare (ie->GetBlackName (nb), text))  ||
                    (stringcompare (ie->GetEventName (nb), text))  ||
                    (stringcompare (datestring, text))  ||
                    (stringcompare (ie->GetSiteName (nb), text)))
                {
			// return setUintResult (ti, filteredCount);
                } else {
			// remove this game
			db->dbFilter->Set (start, 0);
        	        number_removed++;
		}
            }
            start++;
        }
    }
    setMainFilter(db);
    return setUintResult (ti, number_removed);
}

int
sc_filter_value (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * base = db;
    uint gnum = db->gameNumber + 1;

    if (argc > 4)
        return errorResult (ti, "Usage : sc_filter_value [game [base]]");

    if (argc >= 3) {
        if (argc == 4) {
            int baseNum = strGetInteger (argv[3]);
            if (baseNum < 1  ||  baseNum > MAX_BASES || !((base = &(dbList[baseNum - 1]))->inUse) )
              return errorResult (ti, "sc_filter_value: Illegal base");
        }
        gnum = strGetUnsigned (argv[2]);
    }

    if (gnum < 1  ||  gnum > base->numGames)
      return errorResult (ti, "sc_filter_value: game out of range");

    gnum--;

    return setUintResult (ti, base->filter->Get (gnum));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_filter_ply [game [base]]
//    Returns the filter ply of the specified game [and base]
//    Currently used only by game/browser.tcl - so if tree is open, use this value

int
sc_filter_ply (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * base = db;
    uint gnum = db->gameNumber + 1;
    uint ply;

    if (argc > 4)
	return errorResult (ti, "Usage : sc_filter_value [game [base]]");

    if (argc >= 3) {
	if (argc == 4) {
	    int baseNum = strGetInteger (argv[3]);
	    if (baseNum < 1  ||  baseNum > MAX_BASES || !((base = &(dbList[baseNum - 1]))->inUse) )
	      return errorResult (ti, "sc_filter_value: Illegal base");
	}
        gnum = strGetUnsigned (argv[2]);
    } 

    if (gnum < 1  ||  gnum > base->numGames)
      return errorResult (ti, "sc_filter_value: game out of range");

    gnum--;

    if (base->treeFilter && (ply = base->treeFilter->Get (gnum)) > 0)
        return setUintResult (ti, ply);
    else
	return setUintResult (ti, base->filter->Get (gnum));
}

// Gerd contributed treeFilter code, but then rewrote it (differently) for Scid
// Our implementation seems robust and simpler, but in't maintained by Gerd :<
//
// Allow for a second filter when the tree window is open
// When tree is closed, things look like this:
//     db->filter = new Filter(0);
//     db->dbFilter = db->filter;
//     db->treeFilter = NULL;
// When the tree is open && "Adjust Filter" is selected, it performs a filter
// AND with the tree position... Quite handy ;>

void
updateMainFilter( scidBaseT * dbase)
{
    if( dbase->dbFilter != dbase->filter)
    {
        for (uint i=0; i < dbase->numGames; i++)
        {
            if( dbase->dbFilter->Get(i) > 0 && dbase->treeFilter->Get(i) > 0)
                dbase->filter->Set(i,dbase->treeFilter->Get(i));
            else
                dbase->filter->Set(i,0);
        }
    }
}

// Same as above, but only update non-zero values for their ply
void
updateMainFilter2( scidBaseT * dbase)
{
    if( dbase->dbFilter != dbase->filter)
    {
        for (uint i=0; i < dbase->numGames; i++)
        {
            if( dbase->dbFilter->Get(i) > 0 && dbase->treeFilter->Get(i) > 0)
                dbase->filter->Set(i,dbase->treeFilter->Get(i));
        }
    }
}

void
setMainFilter( scidBaseT * dbase)
{
    if (dbase->filter != dbase->dbFilter)
    {
      for (uint i=0; i < dbase->numGames; i++) {
	dbase->filter->Set(i,dbase->dbFilter->Get(i));
      }
    }
}

void
clearFilter( scidBaseT * dbase, uint size)
{
    if(dbase->dbFilter && dbase->dbFilter != dbase->filter)
        delete dbase->dbFilter;
    if(dbase->filter)
        delete dbase->filter;
    if(dbase->treeFilter)
        delete dbase->treeFilter;
    dbase->filter = new Filter (size);
    dbase->dbFilter = dbase->filter;
    dbase->treeFilter = NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_flags
// Usage: sc_flags <gameNum>
// returns a string with all set flags for this game
//
// Could be incorporated into sc_game
//

int
sc_flags (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    char userFlags[32]="";
    gameNumberT gnum;

    if (argc < 2 || argc > 3) {
      return errorResult (ti, "Usage: sc_flags <gamenumber> [verbose]");
    }

    gnum = strGetUnsigned (argv[1]);

    if (gnum == 0) {
      Tcl_AppendResult (ti, "", NULL);
      return TCL_OK;
    }
    gnum--;

    IndexEntry * ie = db->idx->FetchEntry(gnum);
    if (ie == NULL) {
      return setUintResult (ti, 0);
    }

    ie->GetFlagStr (userFlags, NULL);

    if (argc == 2) {
      Tcl_AppendResult (ti, userFlags, NULL);
    } else {
      // verbose mode
      // Is this game deleted or have other user-settable flags

      // max is 19 flags (previously ???)
      int flagCount = 0;
      int deleted = 0;

      if (ie->GetFlagStr (userFlags, NULL) != 0) {
	  // Print other flags set for this game (except "D" for Deleted, which is shown above)
	  const char * flagStr = userFlags;
	  if (*flagStr == 'D') {
              deleted = 1;
	      Tcl_AppendResult (ti, "(", translate (ti, "deleted"), ")  ",  NULL);
              flagStr++;
          }

	  if (*flagStr != 0) {
	      char flagName[100];
	      uint custom;
	      Tcl_AppendResult (ti, translate (ti, "flags", "flags"), ": ", NULL); // , flagStr
	      while (*flagStr != 0) {
		  flagName[0] = 0;
		  custom = 0;
		  switch (*flagStr) {
		      case 'W': strcpy (flagName, "WhiteOpFlag"); break;
		      case 'B': strcpy (flagName, "BlackOpFlag"); break;
		      case 'M': strcpy (flagName, "MiddlegameFlag"); break;
		      case 'E': strcpy (flagName, "EndgameFlag"); break;
		      case 'N': strcpy (flagName, "NoveltyFlag"); break;
		      case 'P': strcpy (flagName, "PawnFlag"); break;
		      case 'T': strcpy (flagName, "TacticsFlag"); break;
		      case 'Q': strcpy (flagName, "QsideFlag"); break;
		      case 'K': strcpy (flagName, "KsideFlag"); break;
		      case '!': strcpy (flagName, "BrilliancyFlag"); break;
		      case '?': strcpy (flagName, "BlunderFlag"); break;
		      case 'U': strcpy (flagName, "UserFlag"); break;
		      default : custom = (uint)(*flagStr - '1') + 1;
				if (custom >=1 && custom <= CUSTOM_FLAG_MAX) {
				    db->idx->GetCustomFlagDesc(flagName, custom);
				    if (!flagName[0])
					sprintf (flagName, "(%i)", custom);
				}
		  }
		  if (flagName[0]) {
		      if (custom)
			  Tcl_AppendResult (ti, (flagCount > 0 ? ", " : ""), flagName, NULL);
		      else
			  Tcl_AppendResult (ti, (flagCount > 0 ? ", " : ""), translate (ti, flagName), NULL);
		  }
		  flagCount++;
		  flagStr++;
	      }
	  }
      }

      /*** Twins (duplicates) ***/
      if (db->duplicates != NULL  &&  db->duplicates[gnum] != 0) {
          if (deleted > 0 || flagCount > 0)
	    Tcl_AppendResult (ti, "  (", translate (ti, "twin"), ")", NULL);
          else
	    Tcl_AppendResult (ti, "(", translate (ti, "twin"), ")", NULL);
      }

    }
    return TCL_OK;
}

//////////////////////////////////////////////////////////////////////
///  GAME functions

int
sc_game (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "altered",    "setaltered", "crosstable", "eco",        "find",
        "firstMoves", "flag",       "import",     "info",
        "load",       "list",       "merge",      "moves",
        "new",        "novelty",    "number",     "pgn",
        "pop",        "push",       "reorder",    "save",       "scores",   "values",
        "startBoard", "startPos",   "strip",      "summary",    "tags",
        "truncate", "truncatefree", "undo",      "undoPoint",   "redo",  NULL
    };
    enum {
        GAME_ALTERED,    GAME_SET_ALTERED, GAME_CROSSTABLE, GAME_ECO,        GAME_FIND,
        GAME_FIRSTMOVES, GAME_FLAG,       GAME_IMPORT,     GAME_INFO,
        GAME_LOAD,       GAME_LIST,       GAME_MERGE,      GAME_MOVES,
        GAME_NEW,        GAME_NOVELTY,    GAME_NUMBER,     GAME_PGN,
        GAME_POP,        GAME_PUSH,       GAME_REORDER,    GAME_SAVE,       GAME_SCORES,     GAME_VALUES,
        GAME_STARTBOARD, GAME_STARTPOS,   GAME_STRIP,      GAME_SUMMARY,    GAME_TAGS,
        GAME_TRUNCATE, GAME_TRUNCATEANDFREE, GAME_UNDO,    GAME_MAKE_UNDO_POINT,  GAME_REDO
    };
    int index = -1;
    char old_language = 0;
    uint location;
    Game * g;
    int  i;

    if (argc > 1) { index = strUniqueMatch (argv[1], options);}

    const char *trial_mode_val;
    bool trialMode;
    if ((trial_mode_val = Tcl_GetVar (ti, "trialMode", TCL_GLOBAL_ONLY)) != NULL)
        trialMode = trial_mode_val[0] == '1';
    else
        trialMode = false;

    switch (index) {
    case GAME_ALTERED:
        return setBoolResult (ti, db->gameAltered);

    case GAME_SET_ALTERED:
        if (argc != 3 ) {
          return errorResult (ti, "Usage: sc_game setaltered [0|1]");
        }
        db->gameAltered = strGetUnsigned (argv[2]);
        break;
    case GAME_CROSSTABLE:
        return sc_game_crosstable (cd, ti, argc, argv);

    case GAME_ECO:  // "sc_game eco" is equivalent to "sc_eco game"
        return sc_eco_game (cd, ti, argc, argv);

    case GAME_FIND:
        return sc_game_find (cd, ti, argc, argv);

    case GAME_FIRSTMOVES:
        return sc_game_firstMoves (cd, ti, argc, argv);

    case GAME_FLAG:
        return sc_game_flag (cd, ti, argc, argv);

    case GAME_IMPORT:
        return sc_game_import (cd, ti, argc, argv);

    case GAME_INFO:
        return sc_game_info (cd, ti, argc, argv);

    case GAME_LOAD:
        return sc_game_load (cd, ti, argc, argv);

    case GAME_LIST:
        return sc_game_list (cd, ti, argc, argv);

    case GAME_MERGE:
        return sc_game_merge (cd, ti, argc, argv);

    case GAME_MOVES:
        return sc_game_moves (cd, ti, argc, argv);

    case GAME_NEW:
        sc_game_undo_reset(db);
        return sc_game_new (cd, ti, argc, argv);

    case GAME_NOVELTY:
        return sc_game_novelty (cd, ti, argc, argv);

    case GAME_NUMBER:
        return setUintResult (ti, db->gameNumber + 1);

    case GAME_PGN:
        return sc_game_pgn (cd, ti, argc, argv);

    case GAME_POP:
        return sc_game_pop (cd, ti, argc, argv);

    case GAME_PUSH:
        return sc_game_push (cd, ti, argc, argv);

    case GAME_REORDER:
        return sc_game_reorder (cd, ti, argc, argv);

    case GAME_SAVE:
        return sc_game_save (cd, ti, argc, argv);

    case GAME_SCORES:
        return sc_game_scores (cd, ti, argc, argv);

    case GAME_VALUES:
        return sc_game_values (cd, ti, argc, argv);

    case GAME_STARTBOARD:
        return sc_game_startBoard (cd, ti, argc, argv);

    case GAME_STARTPOS:
        return sc_game_startPos (cd, ti, argc, argv);

    case GAME_STRIP:
        return sc_game_strip (cd, ti, argc, argv);

    case GAME_SUMMARY:
        return sc_game_summary (cd, ti, argc, argv);

    case GAME_TAGS:
        return sc_game_tags (cd, ti, argc, argv);

    case GAME_TRUNCATE:
        old_language = language;
        language = 0;
        if (argc > 2 && strIsPrefix (argv[2], "-start")) {
            // "sc_game truncate -start" truncates the moves up to the
            // current position:
            db->game->TruncateStart();
        } else {
            // Truncate from the current position to the end of the game
            db->game->Truncate();
        }
        db->gameAltered = true;
        language = old_language;
        break;

    case GAME_TRUNCATEANDFREE:
	// Unused todo : test TruncateAndFree (which fixes minor memory leaks)

	old_language = language;
	language = 0;
	db->game->TruncateAndFree();
	db->gameAltered = true;
	language = old_language;
	break;

    case GAME_UNDO:
	if (trialMode) 
	    return errorResult (ti, "'undo' disabled in trial mode");

	if (db->undoIndex <= -1)
	    return setResult (ti, "0");

	g = db->game;
	// Save this game for later redo
	db->game = db->undoGame[db->undoIndex];
	db->undoGame[db->undoIndex] = g;

	db->undoIndex--;
	db->gameAltered = (db->undoIndex != db->undoCurrent) || db->undoCurrentNotAvail ;
	break;

    case GAME_MAKE_UNDO_POINT:
        if (trialMode)
            break;

	// Delete any undo checkpoints getting discarded
	i = db->undoIndex + 1;
	while (i <= db->undoMax) {
	      delete db->undoGame[i];
	      db->undoGame[i] = NULL;
	     i++;
	}

	db->undoIndex++;

	if ( db->undoIndex >= UNDO_MAX ) {
	    // BUFFER FULL
	    delete db->undoGame[0];
	    for (i = 0 ; i < UNDO_MAX-1 ; i++) {
		db->undoGame[i] = db->undoGame[i+1];
	    }
	    db->undoIndex = UNDO_MAX-1;
	    db->undoMax = UNDO_MAX-1;

	    // decrement undoCurrent and check if full
	    if (db->undoCurrent-- < -1) {
	      db->undoCurrentNotAvail = true;
	      db->undoCurrent = -1;
	    }
	}

	db->undoMax = db->undoIndex;

	g = new Game;
	db->undoGame[db->undoIndex] = g;

	db->game->SaveState();
	db->game->Encode (db->bbuf, NULL);
	db->game->RestoreState();
	db->bbuf->BackToStart();
	g->Decode (db->bbuf, GAME_DECODE_ALL);
	g->CopyStandardTags (db->game);
	db->game->ResetPgnStyle (PGN_STYLE_VARS);
	db->game->SetPgnFormat (PGN_FORMAT_Plain);
	db->tbuf->Empty();
	db->game->WriteToPGN (db->tbuf);
	location = db->game->GetPgnOffset (0);
	db->tbuf->Empty();
	g->MoveToLocationInPGN (db->tbuf, location);

        break;

    case GAME_REDO:
	if (trialMode)
	    return errorResult (ti, "'redo' disabled in trial mode");

	if (db->undoIndex >= db->undoMax)
	    return setResult (ti, "0");

	db->undoIndex++;
	// swap current game and undoGame[db->undoIndex]
	g = db->undoGame[db->undoIndex];
	db->undoGame[db->undoIndex] = db->game;

	// db->gameAltered = true; //g->GetAltered();
	db->gameAltered = (db->undoIndex != db->undoCurrent) || db->undoCurrentNotAvail ;
	db->game = g;
	break;

    default:
        return InvalidCommand (ti, "sc_game", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// isCrosstableGame:
//    Returns true if the game with the specified index entry
//    is considered a crosstable game. It must have the specified
//    Event and Site, and a Date within the specified range or
//    have the specified non-zero EventDate.
static inline bool
isCrosstableGame (IndexEntry * ie, idNumberT siteID, idNumberT eventID,
                  dateT startDate, dateT endDate, dateT eventDate)
{
    if (ie->GetSite() != siteID  ||  ie->GetEvent() != eventID) {
      return false;
    }
    if (eventDate != ZERO_DATE  &&  ie->GetEventDate() == eventDate) {
      return true;
    }

    dateT date = ie->GetDate();

    if (date_GetMonthDay (date) == 0) {
      return (date_GetYear(date) == date_GetYear((startDate+endDate)/2));
    }

    return (date >= startDate && date <= endDate);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_crosstable:
//    Returns the crosstable for the current game.
int
sc_game_crosstable (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "plain", "html", "hypertext", "latex", "filter", "count", NULL
    };
    enum {
        OPT_PLAIN, OPT_HTML, OPT_HYPERTEXT, OPT_LATEX, OPT_FILTER, OPT_COUNT
    };
    int option = -1;
    char stemp[1000];

    const char * usageMsg =
        "Usage: sc_game crosstable plain|html|hypertext|filter|count [name|rating|score|country] [allplay|swiss] [(+|-)(colors|countries|tallies|ratings|titles|groups|breaks|numcolumns)] [-gameNumber GAME|-round ROUND]";

    static const char * extraOptions [] = {
        "allplay", "knockout", "swiss", "auto",
        "name", "rating", "score", "country",
        "-ages", "+ages",               // Show player ages
        "-breaks", "+breaks",           // Show tiebreak scores
        "-colors", "+colors",           // Show game colors in Swiss table
        "-countries", "+countries",     // Show current countries
        "-tallies", "+tallies",
        "-ratings", "+ratings",         // Show Elo ratings
        "-titles", "+titles",           // Show FIDE titles
        "-groups", "+groups",           // Separate players into score groups
        "-deleted", "+deleted",         // Include deleted games in table
        "-numcolumns", "+numcolumns",   // All-play-all numbered columns
        "-gameNumber",
        "-threewin", "+threewin",       // Give 3 points for win, 1 for draw
        "-tiewin", "+tiewin",           // Number of wins is 2nd tie-break
        "-tiehead", "+tiehead",         // Head to head is 1st tie-break
        "-round",
        NULL
    };
    enum {
        EOPT_ALLPLAY, EOPT_KNOCKOUT, EOPT_SWISS, EOPT_AUTO,
        EOPT_SORT_NAME, EOPT_SORT_RATING, EOPT_SORT_SCORE, EOPT_SORT_COUNTRY,
        EOPT_AGES_OFF, EOPT_AGES_ON,
        EOPT_BREAKS_OFF, EOPT_BREAKS_ON,
        EOPT_COLORS_OFF, EOPT_COLORS_ON,
        EOPT_COUNTRIES_OFF, EOPT_COUNTRIES_ON,
        EOPT_TALLIES_OFF, EOPT_TALLIES_ON,
        EOPT_RATINGS_OFF, EOPT_RATINGS_ON,
        EOPT_TITLES_OFF, EOPT_TITLES_ON,
        EOPT_GROUPS_OFF, EOPT_GROUPS_ON,
        EOPT_DELETED_OFF, EOPT_DELETED_ON,
        EOPT_NUMCOLUMNS_OFF, EOPT_NUMCOLUMNS_ON,
        EOPT_GNUMBER,
        EOPT_THREEWIN_OFF, EOPT_THREEWIN_ON,
        EOPT_TIEWIN_OFF, EOPT_TIEWIN_ON, EOPT_TIEHEAD_OFF, EOPT_TIEHEAD_ON,
        EOPT_ROUND
    };

    int sort = EOPT_SORT_SCORE;
    crosstableModeT mode = CROSSTABLE_AllPlayAll;
    bool showAges = true;
    bool showColors = true;
    bool showCountries = true;
    bool showTallies = true;
    bool showRatings = true;
    bool showTitles = true;
    bool showBreaks = false;
    bool scoreGroups = false;
    bool useDeletedGames = false;
    bool numColumns = false;  // Numbers for columns in all-play-all table
    uint numTableGames = 0;
    uint gameNumber = 0;
    bool threewin = false;
    bool tiewin = false;
    bool tiehead = false;
    uint roundNumber = 0;

    if (argc >= 3) { option = strUniqueMatch (argv[2], options); }
    if (option < 0) { return errorResult (ti, usageMsg); }

    for (int arg=3; arg < argc; arg++) {
        int extraOption = strUniqueMatch (argv[arg], extraOptions);
        switch (extraOption) {
            case EOPT_ALLPLAY:        mode = CROSSTABLE_AllPlayAll; break;
            case EOPT_KNOCKOUT:       mode = CROSSTABLE_Knockout;   break;
            case EOPT_SWISS:          mode = CROSSTABLE_Swiss;      break;
            case EOPT_AUTO:           mode = CROSSTABLE_Auto;       break;
            case EOPT_SORT_NAME:      sort = EOPT_SORT_NAME;   break;
            case EOPT_SORT_RATING:    sort = EOPT_SORT_RATING; break;
            case EOPT_SORT_SCORE:     sort = EOPT_SORT_SCORE;  break;
            case EOPT_SORT_COUNTRY:   sort = EOPT_SORT_COUNTRY;  break;
            case EOPT_AGES_OFF:       showAges = false;        break;
            case EOPT_AGES_ON:        showAges = true;         break;
            case EOPT_BREAKS_OFF:     showBreaks = false;      break;
            case EOPT_BREAKS_ON:      showBreaks = true;       break;
            case EOPT_COLORS_OFF:     showColors = false;      break;
            case EOPT_COLORS_ON:      showColors = true;       break;
            case EOPT_COUNTRIES_OFF:  showCountries = false;   break;
            case EOPT_COUNTRIES_ON:   showCountries = true;    break;
            case EOPT_TALLIES_OFF:    showTallies = false;     break;
            case EOPT_TALLIES_ON:     showTallies = true;      break;
            case EOPT_RATINGS_OFF:    showRatings = false;     break;
            case EOPT_RATINGS_ON:     showRatings = true;      break;
            case EOPT_TITLES_OFF:     showTitles = false;      break;
            case EOPT_TITLES_ON:      showTitles = true;       break;
            case EOPT_GROUPS_OFF:     scoreGroups = false;     break;
            case EOPT_GROUPS_ON:      scoreGroups = true;      break;
            case EOPT_DELETED_OFF:    useDeletedGames = false; break;
            case EOPT_DELETED_ON:     useDeletedGames = true;  break;
            case EOPT_NUMCOLUMNS_OFF: numColumns = false;      break;
            case EOPT_NUMCOLUMNS_ON:  numColumns = true;       break;
            case EOPT_GNUMBER:
                // Game number to print the crosstable for is
                // given in the next argument:
                if (arg+1 >= argc) { return errorResult (ti, usageMsg); }
                gameNumber = strGetUnsigned (argv[arg+1]);
                arg++;
                break;
            case EOPT_THREEWIN_OFF:  threewin = false ; break;
            case EOPT_THREEWIN_ON:   threewin = true  ; break;
            case EOPT_TIEWIN_OFF:  tiewin = false ; break;
            case EOPT_TIEWIN_ON:   tiewin = true  ; break;
            case EOPT_TIEHEAD_OFF:  tiehead = false ; break;
            case EOPT_TIEHEAD_ON:   tiehead = true  ; break;
            case EOPT_ROUND:
                if (arg+1 >= argc) { return errorResult (ti, usageMsg); }
                roundNumber = strGetUnsigned (argv[arg+1]);
                // Clear filter here
                db->dbFilter->Fill (0);
                updateMainFilter (db);
                arg++;
                break;
            default: return errorResult (ti, usageMsg);
        }
    }
    if (!db->inUse) { return TCL_OK; }

    const char * newlineStr = "";
    switch (option) {
        case OPT_PLAIN:     newlineStr = "\n";     break;
        case OPT_HTML:      newlineStr = "<br>\n"; break;
        case OPT_HYPERTEXT: newlineStr = "<br>";   break;
        case OPT_LATEX:     newlineStr = "\\\\\n"; break;
    }

    // Load crosstable game if necessary:
    Game * g = db->game;
    if (gameNumber > 0) {
        g = scratchGame;
        g->Clear();
        if (gameNumber > db->numGames) {
            return setResult (ti, "Invalid game number");
        }
        IndexEntry * ie = db->idx->FetchEntry (gameNumber - 1);
        if (ie->GetLength() == 0) {
            return errorResult (ti, "Error: empty game file record.");
        }
        if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(),
                                   ie->GetLength()) != OK) {
            return errorResult (ti, "Error reading game file.");
        }
        if (g->Decode (db->bbuf, GAME_DECODE_ALL) != OK) {
            return errorResult (ti, "Error decoding game.");
            }
        g->LoadStandardTags (ie, db->nb);
    }

    idNumberT eventId = 0, siteId = 0;
    if (db->nb->FindExactName (NAME_EVENT, g->GetEventStr(), &eventId) != OK) {
        return TCL_OK;
    }
    if (db->nb->FindExactName (NAME_SITE, g->GetSiteStr(), &siteId) != OK) {
        return TCL_OK;
    }

    dateT firstDate;
    dateT lastDate;
    dateT eventDate = g->GetEventDate();

    // If game date is a year only, then allow any game in this year to match

    if (date_GetMonthDay (g->GetDate()) == 0) {
      firstDate = g->GetDate();
      lastDate = date_AddMonths (g->GetDate(), 12) - 1;
    } else {
      // Restrict games in tournament to be current game date +/- 3 months:
      firstDate = date_AddMonths (g->GetDate(), -3);
      lastDate = date_AddMonths (g->GetDate(), 3);
    }

    dateT firstSeenDate = g->GetDate();
    dateT lastSeenDate = g->GetDate();

    Crosstable * ctable = new Crosstable;
    if (spellChecker[NAME_PLAYER] != NULL) {
        ctable->UseSpellChecker (spellChecker[NAME_PLAYER]);
    }
    if (sort == EOPT_SORT_NAME) { ctable->SortByName(); }
    if (sort == EOPT_SORT_RATING) { ctable->SortByElo(); }
    if (sort == EOPT_SORT_COUNTRY) { ctable->SortByCountry(); }

    ctable->SetThreeWin(threewin);
    ctable->SetTieWin(tiewin);
    ctable->SetTieHead(tiehead);
    ctable->SetSwissColors (showColors);
    ctable->SetAges (showAges);
    ctable->SetCountries (showCountries);
    ctable->SetTallies (showTallies);
    ctable->SetElos (showRatings);
    ctable->SetTitles (showTitles);
    ctable->SetTiebreaks (showBreaks);
    ctable->SetSeparateScoreGroups (scoreGroups);
    ctable->SetDecimalPointChar (decimalPointChar);
    ctable->SetNumberedColumns (numColumns);

    switch (option) {
        case OPT_PLAIN:     ctable->SetPlainOutput();     break;
        case OPT_HTML:      ctable->SetHtmlOutput();      break;
        case OPT_HYPERTEXT: ctable->SetHypertextOutput(); break;
        case OPT_LATEX:     ctable->SetLaTeXOutput();     break;
    }

    // Find all games that should be listed in the crosstable:
    bool tableFullMessage = false;
    for (uint i=0; i < db->numGames; i++) {
        IndexEntry * ie = db->idx->FetchEntry (i);
        if (ie->GetDeleteFlag()  &&  !useDeletedGames) { continue; }
        if (! isCrosstableGame (ie, siteId, eventId, firstDate, lastDate,
                                eventDate)) {
            continue;
        }
        idNumberT whiteId = ie->GetWhite();
        const char * whiteName = db->nb->GetName (NAME_PLAYER, whiteId);
        idNumberT blackId = ie->GetBlack();
        const char * blackName = db->nb->GetName (NAME_PLAYER, blackId);

        // Ensure we have two different players:
        if (whiteId == blackId) { continue; }

        // If option is OPT_FILTER, adjust the filter and continue &&&

        if (option == OPT_FILTER) {
            if (roundNumber == 0) {
              db->dbFilter->Set (i, 1);
            } else {
              uint thisRound = strGetUnsigned (db->nb->GetName (NAME_ROUND, ie->GetRound()));
              if (thisRound == roundNumber)
                db->dbFilter->Set (i, 1);
            }
            continue;
        }

        // If option is OPT_COUNT, increment game count and continue:
        if (option == OPT_COUNT) {
            numTableGames++;
            continue;
        }

        // Add the two players to the crosstable:
        if (ctable->AddPlayer (whiteId, whiteName, ie->GetWhiteElo()) != OK  ||
            ctable->AddPlayer (blackId, blackName, ie->GetBlackElo()) != OK)
        {
            if (! tableFullMessage) {
                tableFullMessage = true;
		sprintf (stemp, "<red>Player limit of %u reached. Table is incomplete</red>\n\n", CROSSTABLE_MaxPlayers);
                Tcl_AppendResult (ti, stemp, NULL);
            }
            continue;
        }

        uint round = strGetUnsigned (db->nb->GetName (NAME_ROUND, ie->GetRound()));
        dateT date = ie->GetDate();
        resultT result = ie->GetResult();
        ctable->AddResult (i+1, whiteId, blackId, result, round, date);
        if (date < firstSeenDate) { firstSeenDate = date; }
        if (date > lastSeenDate) { lastSeenDate = date; }
    }

    if (option == OPT_COUNT) {
        // Just return a count of the number of tournament games:
        delete ctable;
        return setUintResult (ti, numTableGames);
    }
    if (option == OPT_FILTER) {
        delete ctable;
        setMainFilter(db);
        return TCL_OK;
    }
    if (ctable->NumPlayers() < 2) {
        delete ctable;
        return setResult (ti, "No crosstable for this game.");
    }

    if (option == OPT_LATEX) {
        Tcl_AppendResult (ti, "\\documentclass[10pt,a4paper]{article}\n\n",
                          "\\usepackage{a4wide}\n\n",
                          "\\begin{document}\n\n",
                          "\\setlength{\\parindent}{0cm}\n",
                          "\\setlength{\\parskip}{0.5ex}\n",
                          "\\small\n", NULL);
    }

    if (mode == CROSSTABLE_Auto) { mode = ctable->BestMode(); }

    // Limit all-play-all tables to 300 players:
    uint apaLimit = 300;
    if (mode == CROSSTABLE_AllPlayAll  &&
            ctable->NumPlayers() > apaLimit  &&
            !tableFullMessage) {
        Tcl_AppendResult (ti, "Warning: Too many players for all-play-all; try displaying as a swiss tournament.\n\n", NULL);
    }

    sprintf (stemp, "%s%s%s, ", g->GetEventStr(), newlineStr, g->GetSiteStr());
    Tcl_AppendResult (ti, stemp, NULL);
    date_DecodeToString (firstSeenDate, stemp);
    strTrimDate (stemp);
    Tcl_AppendResult (ti, stemp, NULL);
    if (lastSeenDate != firstSeenDate) {
        date_DecodeToString (lastSeenDate, stemp);
        strTrimDate (stemp);
        Tcl_AppendResult (ti, " - ", stemp, NULL);
    }
    Tcl_AppendResult (ti, newlineStr, NULL);

    eloT avgElo = ctable->AvgRating();
    if (avgElo > 0  &&  showRatings) {
        Tcl_AppendResult (ti, translate (ti, "AverageRating", "Average Rating"),
                          ": ", NULL);
        appendUintResult (ti, avgElo);
        uint category = ctable->FideCategory (avgElo);
        if (category > 0  &&  mode == CROSSTABLE_AllPlayAll) {
            sprintf (stemp, "  (%s %u)",
                     translate (ti, "Category", "Category"), category);
            Tcl_AppendResult (ti, stemp, NULL);
        }
        Tcl_AppendResult (ti, newlineStr, NULL);
    }

    DString * dstr = new DString;
    if (mode != CROSSTABLE_AllPlayAll) { apaLimit = 0; }
    ctable->PrintTable (dstr, mode, apaLimit, db->gameNumber+1);

    Tcl_AppendResult (ti, dstr->Data(), NULL);
    if (option == OPT_LATEX) {
        Tcl_AppendResult (ti, "\n\\end{document}\n", NULL);
    }
    delete ctable;
    delete dstr;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_find:
//    Returns the game number of the game in that current database
//    that best matches the specified number, player names, site,
//    round, year and result.
//    This command is used primarily to locate a bookmarked game in
//    a database where the number may be inaccurate due to database
//    sorting or compaction.
int
sc_game_find (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 9) {
        return errorResult (ti, "sc_game_find: Incorrect parameters");
    }

    uint gnum = strGetUnsigned (argv[2]);
    if (gnum == 0) { return setUintResult (ti, 0); }
    gnum--;
    const char * whiteStr = argv[3];
    const char * blackStr = argv[4];
    const char * siteStr = argv[5];
    const char * roundStr = argv[6];
    uint year = strGetUnsigned(argv[7]);
    resultT result = strGetResult (argv[8]);

    idNumberT white, black, site, round;
    white = black = site = round = 0;
    db->nb->FindExactName (NAME_PLAYER, whiteStr, &white);
    db->nb->FindExactName (NAME_PLAYER, blackStr, &black);
    db->nb->FindExactName (NAME_SITE, siteStr, &site);
    db->nb->FindExactName (NAME_ROUND, roundStr, &round);

    // We give each game a "score" which is 1 for each matching field.
    // So the best possible score is 6.

    // First, check if the specified game number matches all fields:
    if (db->numGames > gnum) {
        uint score = 0;
        IndexEntry * ie = db->idx->FetchEntry (gnum);
        if (ie->GetWhite() == white) { score++; }
        if (ie->GetBlack() == black) { score++; }
        if (ie->GetSite() == site) { score++; }
        if (ie->GetRound() == round) { score++; }
        if (ie->GetYear() == year) { score++; }
        if (ie->GetResult() == result) { score++; }
        if (score == 6) { return setUintResult (ti, gnum+1); }
    }

    // Now look for the best matching game:
    uint bestNum = 0;
    uint bestScore = 0;

    for (uint i=0; i < db->numGames; i++) {
        uint score = 0;
        IndexEntry * ie = db->idx->FetchEntry (i);
        if (ie->GetWhite() == white) { score++; }
        if (ie->GetBlack() == black) { score++; }
        if (ie->GetSite() == site) { score++; }
        if (ie->GetRound() == round) { score++; }
        if (ie->GetYear() == year) { score++; }
        if (ie->GetResult() == result) { score++; }
        // Update if the best score, favouring the specified game number
        // in the case of a tie:
        if (score > bestScore  ||  (score == bestScore  &&  gnum == i)) {
            bestScore = score;
            bestNum = i;
        }
        // Stop now if the best possible match is found:
        if (score == 6) { break; }
    }
    return setUintResult (ti, bestNum + 1);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_firstMoves:
//    get the first few moves of the specified game as  a text line.
//    A game number 0 indicates to use the current active game.
//    E.g., "sc_game firstMoves 0 4" might return "1.e4 e5 2.Nf3 Nf6"
int
sc_game_firstMoves (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 4) {
        return errorResult (ti, "Usage: sc_game firstMoves <gameNum> <numMoves>");
    }
    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    uint gNum = strGetUnsigned (argv[2]);
    if (gNum > db->numGames) {
        return errorResult (ti, "Invalid game number.");
    }

    int plyCount = strGetInteger (argv[3]);
    Game * g = scratchGame;

    if (gNum == 0) {
        g = db->game;
    } else {
        db->bbuf->Empty();
        IndexEntry * ie = db->idx->FetchEntry (gNum - 1);
        if (ie->GetLength() == 0) {
            return setResult (ti, "(This game has no move data)");
        }

        if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(),
                                 ie->GetLength()) != OK) {
            return errorResult (ti, "Error reading game.");
        }

        g->Clear();
        if (g->Decode (db->bbuf, GAME_DECODE_NONE) != OK) {
            return errorResult (ti, "Error decoding game.");
        }
    }

    // Check plyCount is a reasonable value, or set it to current plycount.
    if (plyCount < 0  ||  plyCount > 80) { plyCount = g->GetCurrentPly(); }
    DString * dstr = new DString;
    g->GetPartialMoveList (dstr, 0, plyCount);
    Tcl_AppendResult (ti, dstr->Data(), NULL);
    delete dstr;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_flag:
//    If there is two args, this returns the specified flag status for the
//    specified game. If there are three args, the 2nd arg (0 or 1 or invert)
//    sets the specified flag for the game.
//    Flags that can be specified: delete, user, ...
//    Extra calling methods:
//      sc_game flag <flag> filter <0|1|invert> operates on all filtered games.
//      sc_game flag <flag> all <0|1|invert> operates on all games.
//      sc_game flag <1-6> description
//      sc_game flag <1-6> setdescription [set ::customEntry$i]

int
sc_game_flag (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_game flag <flag> <gameNum> [0|1]";
    if (argc < 3  ||  argc > 5) {
        return errorResult (ti, usage);
    }
    if (!db->inUse) {
        return setResult (ti, errMsgNotOpen(ti));
    }

    uint startGnum = 0;
    uint endGnum = db->numGames;
    bool filteredOnly = false;
    uint flagType = 0;

    flagType = 1 << IndexEntry::CharToFlag (argv[2][0]);

    if (strEqual (argv[3], "description")) {
        // Returns the description associated with a Custom flag
      if (argc != 4) {
        return errorResult (ti, usage);
      } else {
        char desc[CUSTOM_FLAG_DESC_LENGTH+1];
        uint num = IndexEntry::CharToFlag (argv[2][0]) - 15;
        if (num < 1 || num > CUSTOM_FLAG_MAX )
          return errorResult (ti, "Custom flag number out of range");
        db->idx->GetCustomFlagDesc(desc, num);
        Tcl_AppendResult (ti, desc, NULL);
        return TCL_OK;
      }
    } else if (strEqual (argv[3], "setdescription")) {
      if (argc != 5) {
        return errorResult (ti, usage);
      } else {
        uint num = IndexEntry::CharToFlag (argv[2][0]) - 15;
        if (num < 1 || num > CUSTOM_FLAG_MAX )
          return errorResult (ti, "Custom flag number out of range");
        db->idx->SetCustomFlagDesc( argv[4], num);
        return TCL_OK;
      }
    } else if (strEqual (argv[3], "all")) {
        // Delete or undelete all games: the flag value must be specified.
        if (argc != 5) {
            return errorResult (ti, usage);
        }

    } else if (strEqual (argv[3], "filter")) {
        // Delete or undelete all filtered games:
        if (argc != 5) {
            return errorResult (ti, usage);
        }
        filteredOnly = true;

    } else {
        uint gNum = strGetUnsigned (argv[3]);
        // We ignore a request to (un)delete game number zero, but if the
        // specified number exceeds the number of games, return an error:
        if (gNum == 0) { return TCL_OK; }
        if (gNum > db->numGames) {
            return errorResult (ti, "Invalid game number.");
        }
        gNum--;   // Set numbering to be from 0, rather than 1.
        if (argc == 4) {  // Get current flag value:
            IndexEntry * ie = db->idx->FetchEntry (gNum);
            return setBoolResult (ti, ie->GetFlag (flagType));
        }
        startGnum = gNum;
        endGnum = gNum + 1;
    }

    if (db->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }

    for (uint gNum = startGnum; gNum < endGnum; gNum++) {
        IndexEntry * ie = db->idx->FetchEntry (gNum);

        // Only update this games index if it is in the filter, if
        // the filteredOnly flag is true:
        if (!filteredOnly  ||  db->filter->Get(gNum) > 0) {
            bool newValue = strGetBoolean (argv[4]);
            bool oldValue = ie->GetFlag (flagType);
            if (strIsPrefix (argv[4], "invert")) {
                // User wants to toggle the state of this game:
                newValue = !(ie->GetFlag (flagType));
            }
            if (oldValue != newValue) {
                IndexEntry iE = *ie;
                iE.SetFlag (flagType, newValue);
                db->idx->WriteEntries (&iE, gNum, 1);
            }
        }
    }
    recalcFlagCounts (db);

    return TCL_OK;
}

int
sc_game_import (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_game import <pgn-text>");
    }
    PgnParser parser (argv[2]);
    errorT err = parser.ParseGame (db->game);
    if (err == ERROR_NotFound) {
        // No PGN header tags were found, so try just parsing moves:
        db->game->Clear();
        parser.Reset (argv[2]);
        parser.SetEndOfInputWarnings (false);
        parser.SetResultWarnings (false);
        err = parser.ParseMoves (db->game);
    }
    db->gameAltered = true;
    if (err == OK  &&  parser.ErrorCount() == 0) {
        return setResult (ti, "PGN text imported with no errors or warnings.");
    }
    Tcl_AppendResult (ti, "Errors/warnings importing PGN text:\n",
                          parser.ErrorMessages(), NULL);
    if (err == ERROR_NotFound) {
        Tcl_AppendResult (ti, "ERROR: No PGN header tag (e.g. ",
                          "[Result \"1-0\"]) found.", NULL);
    }
    return (err == OK ? TCL_OK : TCL_ERROR);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// probe_tablebase:
//    Probes the tablebases for the current position, and returns
//    the score, a descriptive score with optimal moves, or just a
//    (random) optimal move.
bool
probe_tablebase (Tcl_Interp * ti, int mode, DString * dstr)
{
    int score = 0;
    bool showResult = false;
    bool showSummary = false;
    bool fullReport = false;
    bool optimalMoves = false;
    colorT toMove = db->game->GetCurrentPos()->GetToMove();

/*
#define PROBE_NONE    0
#define PROBE_RESULT  1
#define PROBE_SUMMARY 2   these three from the gameinfo widget
#define PROBE_REPORT  3   tablebase window
#define PROBE_OPTIMAL 4   do training in tree.tcl
*/
    switch (mode) {
    case PROBE_RESULT:
        showResult = true;
        break;
    case PROBE_SUMMARY:
        showResult = true;
        showSummary = true;
        break;
    case PROBE_REPORT:
        fullReport = true;
        break;
    case PROBE_OPTIMAL:
        optimalMoves = true;
        break;
    default:
        return false;
    }

    if (scid_TB_Probe (db->game->GetCurrentPos(), &score) != OK) {
        // Is it desirable to show captures in the gameinfo tablebase results ???
        // scid_TB_Probe is returning !OK (for eg, if we can take a piece).
        // and the winning move is not being shown in the gameinfo tb summary
        // eg,    3k2B1/8/8/1N6/K7/8/b7/8 w - - 0 2
        // for the fullReport, it is being calculated as bestMove
        if (! fullReport) { return false; }
    }

    Position * gamePos = NULL;
    bool moveFound [MAX_LEGAL_MOVES];
    int moveScore [MAX_LEGAL_MOVES];
    bool movePrinted [MAX_LEGAL_MOVES];
    uint winCount = 0;
    uint drawCount = 0;
    uint lossCount = 0;
    uint unknownCount = 0;

    MoveList moveList;
    sanListT sanList;
    gamePos = db->game->GetCurrentPos();
    gamePos->GenerateMoves (&moveList);
    gamePos->CalcSANStrings (&sanList, SAN_CHECKTEST);

    if (showSummary  ||  fullReport  ||  optimalMoves) {
        scratchPos->CopyFrom (gamePos);

        for (uint i=0; i < moveList.Size(); i++) {
            simpleMoveT * smPtr = moveList.Get(i);
            scratchPos->DoSimpleMove (smPtr);
            moveFound[i] = false;
            movePrinted[i] = false;
            int newScore = 0;
            if (scid_TB_Probe (scratchPos, &newScore) == OK) {
                moveFound[i] = true;
                moveScore[i] = newScore;
                if (newScore < 0) {
                    winCount++;
                } else if (newScore == 0) {
                    drawCount++;
                } else {
                    lossCount++;
                }
            } else {
                unknownCount++;
            }
            scratchPos->UndoSimpleMove (smPtr);
        }
    }

    // Optimal moves mode: return only the optimal moves, nothing else.
    if (optimalMoves) {
        uint count = 0;
        for (uint i=0; i < moveList.Size(); i++) {
            if ((score >= 0  &&  moveScore[i] == -score)  ||
                (score < 0  &&  moveScore[i] == -score - 1)) {
                if (count > 0) { dstr->Append (" "); }
                dstr->Append (sanList.list[i]);
                count++;
            }
        }
        return true;
    }

    if (fullReport) {
        char tempStr [80];
        sprintf (tempStr, "Won %u  Drawn %u  Loss %u  ? %u",
                 winCount, drawCount, lossCount, unknownCount);
        dstr->Append (tempStr);
        int prevScore = -9999999;   // Lower than any possible TB score
        bool first = true;

        while (1) {
            bool found = false;
            uint index = 0;
            int bestScore = 0;
            const char * bestMove = "";
            for (uint i=0; i < moveList.Size(); i++) {
                if (movePrinted[i]) { continue; }
                if (! moveFound[i]) { continue; }
                int newScore = - moveScore[i];
                if (!found  ||
                    (newScore > 0 && bestScore <= 0)  ||
                    (newScore > 0 && newScore < bestScore)  ||
                    (newScore == 0 && bestScore < 0)  ||
                    (newScore < 0 && bestScore < 0 && newScore < bestScore)  ||
                    (newScore == bestScore &&
                     strCompare (bestMove, sanList.list[i]) > 0) ) {
                    found = true;
                    index = i;
                    bestScore = newScore;
                    bestMove = sanList.list[i];
                }
            }
            if (!found) { break; }
            movePrinted[index] = true;
            if (first ||
                (bestScore > 0  && prevScore < 0)  ||
                (bestScore == 0 && prevScore != 0)  ||
                (bestScore < 0  && prevScore >= 0)) {
                dstr->Append ("\n");
                first = false;
                const char * tag = NULL;
                const char * msg = NULL;
                if (bestScore > 0) {
                    tag = "WinningMoves"; msg = "Winning moves";
                } else if (bestScore < 0) {
                    tag = "LosingMoves"; msg = "Losing moves";
                } else {
                    tag = "DrawingMoves"; msg = "Drawing moves";
                }
                dstr->Append ("\n", translate(ti, tag, msg));
            }
            if (bestScore != prevScore) {
                if (bestScore > 0) {
                    sprintf (tempStr, " +%3d   ", bestScore);
                } else if (bestScore == 0) {
                    strCopy (tempStr, " =      ");
                } else {
                    sprintf (tempStr, " -%3d   ", -bestScore);
                }
                dstr->Append ("\n", tempStr);
            } else {
                dstr->Append (", ");
            }
            prevScore = bestScore;
            dstr->Append (bestMove);
        }
        if (unknownCount > 0) {
            dstr->Append ("\n\n");
            dstr->Append (translate (ti, "UnknownMoves", "Unknown-result moves"));
            dstr->Append ("\n ?      ");
            bool firstUnknown = true;
            while (1) {
                bool found = false;
                const char * bestMove = "";
                uint index = 0;
                for (uint i=0; i < moveList.Size(); i++) {
                    if (!moveFound[i]  && !movePrinted[i]) {
                        if (!found  ||
                            strCompare (bestMove, sanList.list[i]) > 0) {
                            found = true;
                            bestMove = sanList.list[i];
                            index = i;
                        }
                    }
                }
                if (!found) { break; }
                movePrinted[index] = true;
                if (!firstUnknown) {
                    dstr->Append (", ");
                }
                firstUnknown = false;
                dstr->Append (bestMove);
            }
        }
        dstr->Append ("\n");
        return true;
    }

    if (score == 0) {
        // Print drawn tablebase position info:
        if (showResult) {
            dstr->Append ("= [", translate (ti, "Draw"));
        }
        if (showSummary) {
            uint drawcount = 0;
            uint losscount = 0;
            const char * drawlist [MAX_LEGAL_MOVES];
            const char * losslist [MAX_LEGAL_MOVES];

            for (uint i=0; i < moveList.Size(); i++) {
                if (moveFound[i]) {
                    if (moveScore[i] == 0) {
                        drawlist[drawcount] = sanList.list[i];
                        drawcount++;
                    } else {
                        losslist[losscount] = sanList.list[i];
                        losscount++;
                    }
                }
            }
            if (moveList.Size() == 0) {
                dstr->Append (" (", translate (ti, "stalemate"), ")");
            } else if (drawcount == moveList.Size()) {
                dstr->Append (" ", translate (ti, "withAllMoves"));
            } else if (drawcount == 1) {
                dstr->Append (" ", translate (ti, "with"));
                dstr->Append (" <blue><run addSanMove ",drawlist[0]," -animate>",drawlist[0],"</run></blue>");
            } else if (drawcount+1 == moveList.Size() && losscount==1) {
                dstr->Append (" ", translate (ti, "withAllButOneMove"));
            } else if (drawcount > 0) {
                dstr->Append (" ", translate (ti, "with"), " ");
                dstr->Append (drawcount);
                dstr->Append (" ");
                if (drawcount == 1) {
                    dstr->Append (translate (ti, "move"));
                } else {
                    dstr->Append (translate (ti, "moves"));
                }
                dstr->Append (": ");
                for (uint m=0; m < drawcount; m++) {
                    if (m < 3) {
                        if (m > 0) { dstr->Append (", "); }
                        dstr->Append ("<blue><run addSanMove ",drawlist[m]," -animate>",drawlist[m],"</run></blue>");
                    }
                }
                if (drawcount > 3) { dstr->Append (", ..."); }
            }
            if (losscount > 0) {
                dstr->Append (" (");
                if (losscount == 1) {
                    if (losscount+drawcount == moveList.Size()) {
                        dstr->Append (translate (ti, "only"), " ");
                    }
                    dstr->Append (losslist[0], " ", translate (ti, "loses"));
                } else if (drawcount < 4  &&
                           drawcount+losscount == moveList.Size()) {
                    dstr->Append (translate (ti, "allOthersLose"));
                } else {
                    dstr->Append (losscount);
                    dstr->Append (" ", translate (ti, "lose"), ": ");
                    for (uint m=0; m < losscount; m++) {
                        if (m < 3) {
                            if (m > 0) { dstr->Append (", "); }
                            dstr->Append (losslist[m]);
                        }
                    }
                    if (losscount > 3) { dstr->Append (", ..."); }
                }
                dstr->Append (")");
            }
        }
        if (showResult) { dstr->Append ("]"); }

    } else if (score > 0) {
        // Print side-to-move-mates tablebase info:
        if (showResult) {
            char temp[200];
            sprintf (temp, "%s:%d [%s %s %d",
                     toMove == WHITE ? "+-" : "-+", score,
                     translate (ti, toMove == WHITE ? "White" : "Black"),
                     translate (ti, "matesIn"), score);
            dstr->Append (temp);
        }

        // Now show all moves that mate optimally.
        // This requires generating all legal moves, and trying each
        // to find its tablebase score; optimal moves will have
        // the condition (new_score == -old_score).

        if (showSummary) {
            uint count = 0;

            for (uint i=0; i < moveList.Size(); i++) {
                if (moveFound[i]  &&  moveScore[i] == -score) {
                    count++;
                    if (count == 1) {
                        dstr->Append (" ", translate (ti, "with"), ": ");
                    } else {
                        dstr->Append (", ");
                    }
                    dstr->Append ("<blue><run addSanMove ",sanList.list[i]," -animate>",sanList.list[i],"</run></blue>");
                }
            }
        }
        if (showResult) { dstr->Append ("]"); }

    } else {
        // Score is negative so side to move is LOST:
        if (showResult) {
            char tempStr [80];
            if (score == -1) {
                sprintf (tempStr, "# [%s %s %s",
                         translate (ti, toMove == WHITE ? "Black" : "White"),
                         translate (ti, "checkmate"),
                         translate (ti, toMove == WHITE ? "White" : "Black"));
            } else {
                sprintf (tempStr, "%s:%d [%s %s %d",
                         toMove == WHITE ? "-+" : "+-", -1 - score,
                         translate (ti, toMove == WHITE ? "Black" : "White"),
                         translate (ti, "matesIn"),
                         -1 - score);
            }
            dstr->Append (tempStr);
        }

        // Now show all moves that last optimally.
        // This requires generating all legal moves, and trying
        // each to find its tablebase score; optimal moves will
        // have the condition (new_score == (-old_score - 1)).

        if (showSummary) {
            uint count = 0;
            for (uint i=0; i < moveList.Size(); i++) {
                if (moveFound[i]  &&  moveScore[i] == (-score - 1)) {
                    count++;
                    dstr->Append (", ");
                    if (count == 1) {
                        dstr->Append (translate (ti, "longest"), ": ");
                    }
                    dstr->Append ("<blue><run addSanMove ",sanList.list[i]," -animate>",sanList.list[i],"</run></blue>");
                }
            }
        }
        if (showResult) { dstr->Append ("]"); }
    }

    return true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_info:
//    Return the Game Info string for the active game.
//    The returned text includes color codes.
int
sc_game_info (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool hideNextMove = false;
    uint showMaterialValue = 1; // 0 shows material in gameinfo
    bool showFEN = false;
    uint commentWidth = 70;
    // Making it too large means the x xscrollbar is shown, sometimes obscuring the comment line altogether
    uint commentHeight = 1;
    bool fullComment = false;
    uint showTB = 2;  // 0 = no TB output, 1 = score only, 2 = best moves.
    char temp[1024];
    char tempTrans[10];

    int arg = 2;
    while (arg < argc) {
        if  (strIsPrefix (argv[arg], "-hideNextMove")) {
            if (arg+1 < argc) {
                arg++;
                hideNextMove = strGetBoolean(argv[arg]);
            }
        } else if  (strIsPrefix (argv[arg], "-materialValue")) {
            if (arg+1 < argc) {
                arg++;
                showMaterialValue = strGetInteger(argv[arg]);
            }
        } else if  (strIsPrefix (argv[arg], "-tb")) {
            if (arg+1 < argc) {
                arg++;
                showTB = strGetUnsigned(argv[arg]);
            }
        } else if  (strIsPrefix (argv[arg], "-fen")) {
            if (arg+1 < argc) {
                arg++;
                showFEN = strGetBoolean(argv[arg]);
            }
        } else if  (strIsPrefix (argv[arg], "-cfull")) {
            // -cwidth , -cheight seem unused. S.A
            // Show full comment:
            if (arg+1 < argc) {
                arg++;
                fullComment = strGetBoolean(argv[arg]);
                if (fullComment) {
                    // unused
                    commentWidth = 99999;
                    commentHeight = 99999;
                }
            }
        } else if  (strIsPrefix (argv[arg], "-cwidth")) {
            if (arg+1 < argc) {
                arg++;
                commentWidth = strGetBoolean(argv[arg]);
            }
        } else if  (strIsPrefix (argv[arg], "-cheight")) {
            if (arg+1 < argc) {
                arg++;
                commentHeight = strGetBoolean(argv[arg]);
            }
        } else if (strIsPrefix (argv[arg], "white")) {
            Tcl_AppendResult (ti, db->game->GetWhiteStr(), NULL);
            return TCL_OK;
        } else if (strIsPrefix (argv[arg], "black")) {
            Tcl_AppendResult (ti, db->game->GetBlackStr(), NULL);
            return TCL_OK;
        } else if (strIsPrefix (argv[arg], "event")) {
            Tcl_AppendResult (ti, db->game->GetEventStr(), NULL);
            return TCL_OK;
        } else if (strIsPrefix (argv[arg], "site")) {
            Tcl_AppendResult (ti, db->game->GetSiteStr(), NULL);
            return TCL_OK;
        } else if (strIsPrefix (argv[arg], "round")) {
            Tcl_AppendResult (ti, db->game->GetRoundStr(), NULL);
            return TCL_OK;
        } else if (strIsPrefix (argv[arg], "date")) {
            char dateStr [12];
            date_DecodeToString (db->game->GetDate(), dateStr);
            Tcl_AppendResult (ti, dateStr, NULL);
            return TCL_OK;
        } else if (strIsPrefix (argv[arg], "year")) {
            return setUintResult (ti, date_GetYear (db->game->GetDate()));
        } else if (strIsPrefix (argv[arg], "result")) {
            return setResult (ti, RESULT_STR[db->game->GetResult()]);
        } else if (strIsPrefix (argv[arg], "nextMove")) {
            db->game->GetSAN (temp);
            strcpy(tempTrans, temp);
            transPieces(tempTrans);
            Tcl_AppendResult (ti, tempTrans, NULL);
            return TCL_OK;
// nextMoveNT is the same as nextMove, except that the move is not translated
        } else if (strIsPrefix (argv[arg], "nextMoveNT")) {
            db->game->GetSAN (temp);
            Tcl_AppendResult (ti, temp, NULL);
            return TCL_OK;
// returns next move played in UCI format
        } else if (strIsPrefix (argv[arg], "nextMoveUCI")) {
            db->game->GetNextMoveUCI (temp);
            Tcl_AppendResult (ti, temp, NULL);
            return TCL_OK;
        } else if (strIsPrefix (argv[arg], "previousMove")) {
            db->game->GetPrevSAN (temp);
            strcpy(tempTrans, temp);
            transPieces(tempTrans);
            Tcl_AppendResult (ti, tempTrans, NULL);
            return TCL_OK;
// previousMoveNT is the same as previousMove, except that the move is not translated
        } else if (strIsPrefix (argv[arg], "previousMoveNT")) {
            db->game->GetPrevSAN (temp);
            Tcl_AppendResult (ti, temp, NULL);
            return TCL_OK;
// returns previous move played in UCI format
        } else if (strIsPrefix (argv[arg], "previousMoveUCI")) {
            db->game->GetPrevMoveUCI (temp);
            Tcl_AppendResult (ti, temp, NULL);
            return TCL_OK;
        } else if (strIsPrefix (argv[arg], "duplicate")) {
            uint dupGameNum = 0;
            if (db->duplicates != NULL  &&  db->gameNumber >= 0) {
                dupGameNum = db->duplicates [db->gameNumber];
            }
            return setUintResult (ti, dupGameNum);
        } else if (strIsPrefix (argv[arg], "halfmoves")) {
            return setUintResult (ti, db->game->GetNumHalfMoves());
        }
        arg++;
    }

    // <br> is used for newlines S.A. &&&
    // Other markups (eg <red>, <gbold>) are text tags defined in htext.tcl
    // <bold> markup seems broken, so using <gbold>
    // A few fields are alligned with TABS

    /*** Player names, result line ****/

    if ( db->game->GetWhiteStr()[0] == '?' && db->game->GetWhiteStr()[1] == 0 )
      sprintf (temp, "<center><gbold><run gameSave -1 white>?</run></gbold>");
    else
      sprintf (temp, "<center><gbold><pi %s>%s</pi></gbold>", db->game->GetWhiteStr(), db->game->GetWhiteStr());

    Tcl_AppendResult (ti, temp, NULL);

    eloT elo = db->game->GetWhiteElo();
    bool eloEstimated = false;
    if (elo == 0) {
        elo = db->game->GetWhiteEstimateElo();
        eloEstimated = true;
    }
    if (elo != 0) {
        sprintf (temp, " <gray>(%u%s)</gray>", elo, eloEstimated ? "*" : "");
        Tcl_AppendResult (ti, temp, NULL);
    }

    if ( db->game->GetBlackStr()[0] == '?' && db->game->GetBlackStr()[1] == 0 )
      sprintf (temp, "    --    <gbold><run gameSave -1 black>?</run></gbold>");
    else
      sprintf (temp, "    --    <gbold><pi %s>%s</pi></gbold>", db->game->GetBlackStr(), db->game->GetBlackStr());
    Tcl_AppendResult (ti, temp, NULL);

    elo = db->game->GetBlackElo();
    eloEstimated = false;
    if (elo == 0) {
        elo = db->game->GetBlackEstimateElo();
        eloEstimated = true;
    }
    if (elo != 0) {
        sprintf (temp, " <gray>(%u%s)</gray>", elo, eloEstimated ? "*" : "");
        Tcl_AppendResult (ti, temp, NULL);
    }

    resultT result = db->game->GetResult();
    if (! hideNextMove && result )  {
      Tcl_AppendResult (ti, "    (", RESULT_LONGSTR[result] ,")", NULL);
    }

    Tcl_AppendResult (ti, "</center>", NULL);

    /*** Event line ****/

    char dateStr[20];
    date_DecodeToString (db->game->GetDate(), dateStr);
    strTrimDate (dateStr);

    // Hide some of this info if unknown

    sprintf (temp, "<br>%s : <blue>", translate (ti, "Event"));
    Tcl_AppendResult (ti, temp, NULL);

    if ( *db->game->GetEventStr() == '?' && *db->game->GetSiteStr() == '?' )
      sprintf (temp, "<run gameSave -1 event>?</run>");
    else {
      if (*db->game->GetSiteStr() == '?' )
        sprintf (temp, "<run ::crosstab::Open>%s</run>", db->game->GetEventStr());
      else
        sprintf (temp, "<run ::crosstab::Open>%s, %s</run>", db->game->GetEventStr(), db->game->GetSiteStr());
    }
    Tcl_AppendResult (ti, temp, "</blue>", NULL);

    if ( *dateStr == '?' )
      sprintf (temp, "  (<blue><run gameSave -1 date>?</run></blue>");
    else
      sprintf (temp, "  (%s", dateStr);

    Tcl_AppendResult (ti, temp, NULL);

    if ( *db->game->GetRoundStr() == '?' )
      sprintf (temp, ")");
    else
      sprintf (temp, ", %s %s)", translate (ti, "Round"), db->game->GetRoundStr());

    Tcl_AppendResult (ti, temp, NULL);

    if (db->game->GetEco() != 0) {
        ecoStringT fullEcoStr;
        eco_ToExtendedString (db->game->GetEco(), fullEcoStr);
        ecoStringT basicEcoStr;
        strCopy (basicEcoStr, fullEcoStr);
        if (strLength(basicEcoStr) >= 4) { basicEcoStr[3] = 0; }
        Tcl_AppendResult (ti, "   <blue><run ::windows::eco::Refresh ", basicEcoStr, ">", fullEcoStr, "</run></blue>", NULL);
    }

    /*** Check ECO book for the current position ***/

    if (ecoBook) {
        DString ecoComment;
        if (ecoBook->FindOpcode (db->game->GetCurrentPos(), "eco",
                                 &ecoComment) == OK) {
            ecoT eco = eco_FromString (ecoComment.Data());
            ecoStringT estr;
            eco_ToExtendedString (eco, estr);
            // make init position show all ecocodes
            if (!strCompare(estr,"A00a"))
	      strCopy (estr, "{}");

            uint len = strLength (estr);
            if (len >= 4) { estr[3] = 0; }
            DString * tempDStr = new DString;
            translateECO (ti, ecoComment.Data(), tempDStr);
	    if (db->game->GetEco() == 0)
	      Tcl_AppendResult (ti, "  <blue><run ::windows::eco::Refresh ", estr, ">", tempDStr->Data(), "</run></blue>", NULL);
            else
	      Tcl_AppendResult (ti, "  (<blue><run ::windows::eco::Refresh ", estr, ">", tempDStr->Data(), "</run></blue>)", NULL);
            delete tempDStr;
        }
    }

    /*** Move  , Material, Length line ****/

    char san [20];
    byte *nags;
    colorT toMove = db->game->GetCurrentPos()->GetToMove();
    uint moveCount = db->game->GetCurrentPos()->GetFullMoveCount();
    uint prevMoveCount = moveCount;
    if (toMove == WHITE) { prevMoveCount--; }

    db->game->GetPrevSAN (san);
    strcpy(tempTrans, san);
    transPieces(tempTrans);
    bool printNags = true;
    if (san[0] == 0) {
        strCopy (temp, "");
        strAppend (temp, db->game->GetVarLevel() == 0 ?
                   translate (ti, "GameStart", "Start of game") :
                   translate (ti, "LineStart", "Start of line"));
        strAppend (temp, "");
        printNags = false;
    } else {
        sprintf (temp, "%u.   <blue>%s%s",
                 prevMoveCount, toMove==WHITE ? "...  " : "", tempTrans);
        printNags = true;
    }

    Tcl_AppendResult (ti, "<br>", temp, "</blue>", NULL);

    nags = db->game->GetNags();
    if (printNags  &&  *nags != 0 ) {
	Tcl_AppendResult (ti, "<gray>",  NULL);
        for (uint nagCount = 0 ; nags[nagCount] != 0; nagCount++) {
	  char nagstr[20];
	  game_printNag (nags[nagCount], nagstr, true, PGN_FORMAT_Plain);
	  // if (nagCount > 0  ||  (nagstr[0] != '!' && nagstr[0] != '?')) {
	  //     Tcl_AppendResult (ti, " ", NULL);
	  // }
	  Tcl_AppendResult (ti, nagstr, NULL);
        }
	Tcl_AppendResult (ti, "</gray>",  NULL);
    }

    /*** (Var) ***/
    Tcl_AppendResult (ti, "\t", NULL);
    if (db->game->GetVarLevel() > 0) {
        Tcl_AppendResult (ti, "<gray><run sc_var exit; updateBoard -animate>  (Var)</run></gray>", NULL);
    }

    /*** Next Move ***/

    db->game->GetSAN (san);
    strcpy(tempTrans, san);
    transPieces(tempTrans);
    if (san[0] == 0) {
        strCopy (temp, "");
        strAppend (temp, db->game->GetVarLevel() == 0 ?
		   translate (ti, "GameEnd", "End of game") :
		   translate (ti, "LineEnd", "End of line"));
        strAppend (temp, "");
        printNags = false;
    } else {
      if (hideNextMove) {
	  sprintf (temp, "%u.   %s(", moveCount, toMove==WHITE ? "" : "...  ");
	  strAppend (temp, translate (ti, "hidden"));
	  strAppend (temp, ")");
	  printNags = false;
      } else {
	  sprintf (temp, "<run ::move::Forward>%u.<blue>%s%s",
		moveCount, toMove==WHITE ? "" : "..", tempTrans);//san);
	  printNags = true;
      }
    }

    if (!hideNextMove) {
      Tcl_AppendResult (ti, "\t", translate (ti, "NextMove", "Next"), NULL);
      Tcl_AppendResult (ti, ":  ", temp, "</blue>", NULL);

      nags = db->game->GetNextNags();
      if (printNags  &&  *nags != 0) {
	  Tcl_AppendResult (ti, "<gray>", NULL);
	  for (uint nagCount = 0 ; nags[nagCount] != 0; nagCount++) {
	      char nagstr[20];
	      game_printNag (nags[nagCount], nagstr, true, PGN_FORMAT_Plain);
	      Tcl_AppendResult (ti, nagstr, NULL);
	  }
 	  Tcl_AppendResult (ti, "</gray>", NULL);
      }
      Tcl_AppendResult (ti, "</run>", NULL);
    }
    Tcl_AppendResult (ti, "\t\t", NULL);

    /*** Length ***/
    if (hideNextMove) {
        // sprintf (temp, "(%s: %s)", translate (ti, "Result"), translate (ti, "hidden"));
    } else {
      sprintf (temp, "%s: %-5u   ", translate (ti, "Length"), (db->game->GetNumHalfMoves() + 1) / 2);
      Tcl_AppendResult (ti, temp, NULL);
    }

    /*** Material ***/
    if (showMaterialValue == 0) {
        uint mWhite = db->game->GetCurrentPos()->MaterialValue (WHITE);
        uint mBlack = db->game->GetCurrentPos()->MaterialValue (BLACK);
        sprintf (temp, "%s: ", translate (ti, "Material"));
        Tcl_AppendResult (ti, temp, NULL);
        if (mWhite > mBlack) {
            sprintf (temp, "+%-2u", mWhite - mBlack);
        } else if (mBlack > mWhite) {
            sprintf (temp, "-%-2u", mBlack - mWhite);
        } else {
            sprintf (temp, "%-3u", 0);
        }
        Tcl_AppendResult (ti, temp, NULL);
    }

    /*** Comment ***/

    if (db->game->GetMoveComment() != NULL) {
        Tcl_AppendResult (ti, "<br><green><run ::commenteditor::Open>", NULL);
        char * str = strDuplicate(db->game->GetMoveComment());
        strTrimMarkCodes (str);
        const char * s = str;
        uint len;
        uint lines = 0;
	char ch;

        // Add the first commentWidth characters of the comment. Newlines are converted to space.
        for (len = 0; len < commentWidth; len++, s++) {
            ch = *s;
            if (ch == 0) { break; }
            if (ch == '\n') {
                lines++;
                if (commentHeight > 1)
		  Tcl_AppendResult (ti, "<br>", NULL);
                else
		  appendCharResult (ti, ' ');
            } else if (ch == '<') {
                Tcl_AppendResult (ti, "<lt>", NULL);
            } else if (ch == '>') {
                Tcl_AppendResult (ti, "<gt>", NULL);
            } else {
                appendCharResult (ti, ch);
            }
        }
        // Complete the current comment word and add "..." if necessary:
        if (len == commentWidth) {
            char ch = *s;
            while (ch != ' '  &&  ch != '\n'  &&  ch != 0) {
                appendCharResult (ti, ch);
                s++;
                ch = *s;
            }
            if (ch != 0) {
                Tcl_AppendResult (ti, "...", NULL);
            }
        }
        Tcl_AppendResult (ti, "</run></green>", NULL);
        delete[] str;
    }


    /*** Variations ***/
    // (Often hid off the bottom of the gameinfo widget)

    uint varCount = db->game->GetNumVariations();
    if (!hideNextMove  &&  varCount > 0) {
        Tcl_AppendResult (ti, "<br>", translate (ti, "Variations"), ":", NULL);
        // Only show the first 5 variations
        for (uint vnum = 0; vnum < varCount && vnum < 5; vnum++) {
            char s[20];
            db->game->MoveIntoVariation (vnum);
            db->game->GetSAN (s);
            strcpy(tempTrans, s);
            transPieces(tempTrans);
            sprintf (temp, "   <run sc_var enter %u; updateBoard -animate>%u",
                     vnum, vnum+1);
            Tcl_AppendResult (ti, "<green>", temp, "</green>: ", NULL);
            if (s[0] == 0) {
                sprintf (temp, "<blue>(empty)</blue>");
            } else {
                sprintf (temp, "<blue>%u.%s%s</blue>",
                         moveCount, toMove == WHITE ? "" : "...  ", tempTrans);//s);
            }
            Tcl_AppendResult (ti, temp, NULL);
            byte * firstNag = db->game->GetNextNags();
            if (*firstNag >= NAG_GoodMove  &&  *firstNag <= NAG_DubiousMove) {
                game_printNag (*firstNag, s, true, PGN_FORMAT_Plain);
                Tcl_AppendResult (ti, "<gray>", s, "</gray>", NULL);
            }
            Tcl_AppendResult (ti, "</run>", NULL);
            db->game->MoveExitVariation ();
        }
    }

    /*** Tablebases and FEN ***/

    if (!hideNextMove) {
        DString * tbStr = new DString;
        if (probe_tablebase (ti, showTB, tbStr)) {
            Tcl_AppendResult (ti, "<br>TB: ",tbStr->Data(), NULL);
        }
        delete tbStr;
    }

    if (showFEN) {
        char boardStr [200];
        db->game->GetCurrentPos()->PrintFEN (boardStr, FEN_ALL_FIELDS);
        Tcl_AppendResult (ti, "<br><gray>", boardStr, "</gray>", NULL);
    }
    return TCL_OK;
}

//    Returns a portion of the game list according to the current filter.
//    Takes start and count, where start is in the range (1..FilterCount).
//    The next argument is the format string -- see index.cpp for details
//    of format strings.
//
//    If the format string is "-current" (seems unused), then a single integer value
//    is returned indicating the line number where the current game
//    occured in the output (where 1 is the first line), or 0 if it
//    did not occur in the output at all.
//
//    '!' prefixing the formatStr indicates next 'Moves' are to be calculated.

int
sc_game_list (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = startProgressBar();
    if (argc != 5  &&  argc != 6) {
        return errorResult (ti, "Usage: sc_game list <start> <count> [!]<format> [<file>]");
    }
    if (! db->inUse) { return TCL_OK; }

    uint start, count;
    start = strGetUnsigned (argv[2]);
    count = strGetUnsigned (argv[3]);
    if (start < 1  ||  start > db->numGames) { return TCL_OK; }
    uint fcount = db->filter->Count();
    if (fcount > count) { fcount = count; }

    FILE * fp = NULL;

    if (argc == 6) {
	fp = fopen (argv[5], "w");
	if (fp == NULL) {
	    Tcl_AppendResult (ti, "Error opening file: ", argv[5], NULL);
	    return TCL_ERROR;
	}
    }

    uint index = db->filter->FilteredCountToIndex(start);
    IndexEntry * ie;
    const char * formatStr = argv[4];
    bool nextMoves = 0;

    // '!' prefix in formatStr indicates nextMoves should be processed.
    if (formatStr[0] == '!') {
	nextMoves = 1;
        formatStr++;
    }


    char temp[2048];
    int update, updateStart;
    update = updateStart = 5000;
    uint linenum = 0;

    DString * moveStr = new DString;
    Game * g = scratchGame;
    uint gamePly = db->game->GetCurrentPly();
    bool gameNonStd = db->game->HasNonStandardStart();

    // Seems unused ? S.A.
    bool returnLineNum = strEqual (formatStr, "-current");

    while (index < db->numGames  &&  count > 0) {
        if (db->filter->Get(index)) {
            if (showProgress) {  // Update the percentage done bar:
                if (update <= 0) {
                    update = updateStart;
                    if (fp != NULL) { fflush (fp); }
                    updateProgressBar (ti, start, fcount);
                    if (interruptedProgress()) { break; }
                }
                update--;
            }
            linenum++;
            if (returnLineNum  &&  (int)index == db->gameNumber) {
                if (fp != NULL) { fclose (fp); }
                return setUintResult (ti, linenum);
            }
            ie = db->idx->FetchEntry (index);

            if (nextMoves) {
              // Hack the gamelist 'Opening moves' to 'Next moves', and do a quick load of each game
              // based on "sc_game firstMoves <gameNum> <numMoves>"

              if (gamePly > 0 || ! ie->GetStoredLineCode() || gameNonStd || ie->GetStartFlag() ) {
                db->bbuf->Empty();
                if (ie->GetLength() == 0) {
                    // todo
                } else {
                    if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(), ie->GetLength()) != OK) {
                        printf ("mybad handling game %u\n", index);
                    } else {
                        g->Clear();
                        if (g->Decode (db->bbuf, GAME_DECODE_NONE) != OK) {
                          printf ("mybad decoding game %u\n", index);
                        } else {
                          moveStr->Clear();
                          // Todo! - Make this work with position searches,
                          // where each game has it's own ply according to search result.

                          // Cases to consider
                          // * NonStandardStart
                          // * Move repetition means db->treeFilter->Get(index) may not be accurate as it gives the first pos occurence
                          // * Transpostion at different depth means we should generally use treeFilter

                          uint ply;

                          if ((int)index == db->gameNumber) {
                            ply = gamePly;
                          } else {
                            ply = db->treeFilter->Get(index) - 1;
                            if (!(gameNonStd || ie->GetStartFlag() || ply == gamePly)) {
                                // Check for repetition - move to gamePly and check if position is the same
                                Position tempPos;
                                tempPos.CopyFrom(db->game->GetCurrentPos());
                                g->MoveToPly(gamePly);
                                if (gamePly == g->GetCurrentPly() &&  \
                                    g->GetCurrentPos()->Compare(&tempPos) == EQUAL_TO) {
                                  ply = gamePly;
                                }
                             }
                          }

                          // Show arbitary 3 moves (6 ply)
                          g->GetPartialMoveList (moveStr, ply, 6);
                        }
                    }
                }
              } else {
                // prepend " " to StoredLine to allign with GetPartialMoveList
                moveStr->Clear();
                moveStr->Append(" ");
                moveStr->Append(StoredLine::GetText(ie->GetStoredLineCode()));
              }
              ie->PrintGameInfo (temp, start, index+1, db->nb, formatStr, moveStr->Data());
            } else {
              ie->PrintGameInfo (temp, start, index+1, db->nb, formatStr, "");
            }

            if (fp == NULL) {
                // separate lines by newline &&& Issues ?
                Tcl_AppendResult (ti, temp, "\n", NULL);
            } else {
                //fputs (temp, fp);
                fputs (temp, fp);
            }
            count--;
            start++;
        }
        index++;
    }
    delete moveStr;
    if (showProgress) { updateProgressBar (ti, 1, 1); }
    if (fp != NULL) { fclose (fp); }
    if (returnLineNum) { return setUintResult (ti, 0); }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_load:
//    Takes a game number and loads the game
int
sc_game_load (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_game load <gameNumber>");
    }

    sc_game_undo_reset(db);

    db->bbuf->Empty();
    uint gnum = strGetUnsigned (argv[2]);
    char errorStr[100];

    if (argv[2][0] == 'a') {
        // Load the autoload game for this base:
        gnum = db->idx->GetAutoLoad();
        if (gnum < 1) {
            db->game->Clear();
            return TCL_OK;
        }
        if (gnum > db->numGames) { gnum = db->numGames; }
    }

    // Check the game number is valid::
    if (gnum < 1  ||  gnum > db->numGames) {
        return errorResult (ti, "Invalid game number.");
    }

    // We number games from 0 internally, so subtract one:
    gnum--;

    IndexEntry * ie = db->idx->FetchEntry (gnum);

    if (db->gfile->ReadGame (db->bbuf,ie->GetOffset(),ie->GetLength()) != OK) {
        sprintf (errorStr, "Game %u appears to be corrupt.", gnum+1);
        return errorResult (ti, errorStr);
    }
    if (db->game->Decode (db->bbuf, GAME_DECODE_ALL) != OK) {
        sprintf (errorStr, "Error decoding game %u.", gnum+1);
        return errorResult (ti, errorStr);
    }

    // Get ply from tree filter if no interesting ply from filter
    // We have to consider things like "move search" performed, while tree is open.

    uint ply = db->filter->Get(gnum);

    if (ply <= 1 && db->treeFilter)
        ply = db->treeFilter->Get(gnum);

    if (ply > 0) {
	db->game->MoveToPly(ply - 1);
    } else {
	db->game->MoveToPly(0);
    }


    db->game->LoadStandardTags (ie, db->nb);
    db->gameNumber = gnum;
    db->gameAltered = false;
    return OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_merge:
//    Merge the specified game into a variation from the current
//    game position.
int
sc_game_merge (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_game merge <baseNum> <gameNum> [<endPly>]";
    scidBaseT * base = db;
    if (argc < 4  ||  argc > 5) { return errorResult (ti, usage); }

    int baseNum = strGetInteger (argv[2]);
    if (baseNum >= 1  &&  baseNum <= MAX_BASES) {
        base = &(dbList[baseNum - 1]);
    }
    uint gnum = strGetUnsigned (argv[3]);
    uint endPly = 9999;     // Defaults to huge number for all moves.
    if (argc == 5) { endPly = strGetUnsigned (argv[4]); }

    // Check we have a valid database and game number:
    if (! base->inUse) {
        return errorResult (ti, "The selected database is not open.");
    }
    if (gnum < 1  ||  gnum > base->numGames) {
        return errorResult (ti, "Invalid game number.");
    }
    // Number games from 0 internally:
    gnum--;

    // Check that the specified game can be merged:
    if (base == db  &&  (int)gnum == db->gameNumber) {
        return errorResult (ti, "Game cannot be merged into itself.");
    }
    if (db->game->AtStart()  &&  db->game->AtEnd()) {
        return errorResult (ti, "The current game has no moves.");
    }
    if (db->game->HasNonStandardStart()) {
        return errorResult (ti, "The current game has a non-standard start position.");
    }

    // Load the merge game:

    IndexEntry * ie = base->idx->FetchEntry (gnum);
    base->bbuf->Empty();
    if (base->gfile->ReadGame (base->bbuf, ie->GetOffset(),
                               ie->GetLength()) != OK) {
        return errorResult (ti, "Error loading game.");
    }
    Game * merge = scratchGame;
    merge->Clear();
    if (merge->Decode (base->bbuf, GAME_DECODE_NONE) != OK) {
        return errorResult (ti, "Error decoding game.");
    }
    merge->LoadStandardTags (ie, base->nb);

    // Set up an array of all the game positions in the merge game:
    uint nMergePos = merge->GetNumHalfMoves() + 1;
    typedef char compactBoardStr [36];
    compactBoardStr * mergeBoards = new compactBoardStr [nMergePos];
    merge->MoveToPly (0);
    for (uint i=0; i < nMergePos; i++) {
        merge->GetCurrentPos()->PrintCompactStr (mergeBoards[i]);
        merge->MoveForward();
    }

    // Now find the deepest position in the current game that occurs
    // in the merge game:
    db->game->MoveToPly (0);
    uint matchPly = 0;
    uint mergePly = 0;
    uint ply = 0;
    bool done = false;
    while (!done) {
        if (db->game->MoveForward() != OK) { done = true; }
        ply++;
        compactBoardStr currentBoard;
        db->game->GetCurrentPos()->PrintCompactStr (currentBoard);
        for (uint n=0; n < nMergePos; n++) {
            if (strEqual (currentBoard, mergeBoards[n])) {
                matchPly = ply;
                mergePly = n;
            }
        }
    }
    delete[] mergeBoards;

    if (matchPly == 0) {
        return errorResult (ti, "No matching position found.");
    }

    // Now the games match at the locations matchPly in the current
    // game and mergePly in the merge game.
    // Create a new variation and add merge-game moves to it:
    db->game->MoveToPly (matchPly);
    bool atLastMove = db->game->AtEnd();
    simpleMoveT * sm = NULL;
    if (atLastMove) {
        // At end of game, so remember final game move for replicating
        // at the start of the variation:
        db->game->MoveBackup();
        sm = db->game->GetCurrentMove();
        db->game->MoveForward();
    }
    db->game->MoveForward();
    db->game->AddVariation();
    db->gameAltered = true;
    if (atLastMove) {
        // We need to replicate the last move of the current game.
        db->game->AddMove (sm, NULL);
    }

    // Add a comment describing the merge-game details
    DString * dstr = new DString;
    dstr->Append (ie->GetWhiteName (base->nb));
    dstr->Append (" -- ");
    dstr->Append (ie->GetBlackName (base->nb));
    dstr->Append (" ", RESULT_LONGSTR[ie->GetResult()]);
    if (endPly < merge->GetNumHalfMoves()) {
        dstr->Append (" (", (merge->GetNumHalfMoves()+1) / 2, " ");
        dstr->Append (translate (ti, "moves"), ")");
    }
    dstr->Append ("\n", ie->GetEventName (base->nb));
    dstr->Append (", ", ie->GetYear());
    db->game->SetMoveComment (dstr->Data());
    delete dstr;

    merge->MoveToPly (mergePly);
    ply = mergePly;
    // Show at least one move
    if (ply >= endPly) endPly = ply + 1;
    while (ply < endPly) {
        simpleMoveT * mergeMove = merge->GetCurrentMove();
        if (merge->MoveForward() != OK) { break; }
        if (mergeMove == NULL) { break; }
        if (db->game->AddMove (mergeMove, NULL) != OK) { break; }
        ply++;
    }

    // And exit the new variation:
    db->game->MoveExitVariation();
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_moves:
//    Return a string of the moves reaching the current game position.
//    Default output is standard algebraic with move numbers.
//    Optional args -
//      "coord" for coordinate notation 
//      "nonums" for standard algebraic without move numbers.
int
sc_game_moves (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool sanFormat = true;
    bool printMoves = true;
    uint plyCount = 0;
    Game * g = db->game;
    uint MAXMOVES = g->GetCurrentPly() + 10; // overhead
    sanStringT * moveStrings = new sanStringT [MAXMOVES];

    const char * usage = "Usage: sc_game moves [coord | nonums]";

    if (argc > 3) {
        return errorResult (ti, usage);
    }

    if (argc == 3) {
        if (strcmp (argv[2],"coord") == 0) {
          sanFormat = false;
        } else if (strcmp (argv[2],"nonums") == 0)  {
          // currently unused - S.A.
          printMoves = false;
        } else {
          return errorResult (ti, usage);
        }
    }

    g->SaveState();
    while (! g->AtStart()) {
        if (g->AtVarStart()) {
            g->MoveExitVariation();
            continue;
        }
        g->MoveBackup();
        simpleMoveT * sm = g->GetCurrentMove();
        if (sm == NULL) { break; }
        char * s = moveStrings[plyCount];
        if (sanFormat) {
            g->GetSAN (s);
        } else {
            if (sm->from == sm->to) {
              /* null move "0000"
              *s++ = '0';
              *s++ = '0';
              *s++ = '0';
              *s++ = '0';
              */

              // instead of appending a nullmove, simpler to exit and return only '0000'
	      g->RestoreState();
	      delete[] moveStrings;
	      Tcl_AppendResult (ti, "0000", NULL);
	      return TCL_OK;
            } else {
	      *s++ = square_FyleChar(sm->from);
	      *s++ = square_RankChar(sm->from);
	      *s++ = square_FyleChar(sm->to);
	      *s++ = square_RankChar(sm->to);
	      if (sm->promote != EMPTY) {
		  *s++ = tolower(piece_Char (piece_Type (sm->promote)));
	      }
            }
            *s = 0;
        }
        plyCount++;
        if (plyCount > MAXMOVES) {
            // Oops  - shouldnt happen.
            // Too many moves, just give up:
            g->RestoreState();
            delete[] moveStrings;
            return TCL_OK;
        }
    }
    g->RestoreState();
    uint count = 0;
    for (uint i = plyCount; i > 0; i--, count++) {
        char move [20];
        if (sanFormat) {
            move[0] = 0;
            if (printMoves  &&  (count % 2 == 0)) {
                sprintf (move, "%u.", (count / 2) + 1);
            }
            strAppend (move, moveStrings[i - 1]);
        } else {
            strCopy (move, moveStrings [i - 1]);
        }
	Tcl_AppendResult (ti, (count == 0 ? "" : " "), move, NULL);
    }
    delete[] moveStrings;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_new:
//    Clears the current game.
int
sc_game_new (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    db->game->Clear();
    db->gameNumber = -1;
    db->gameAltered = false;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_novelty:
//    Finds the first move in the current game (after the deepest
//    position found in the ECO book) that reaches a position not
//    found in the selected database. It then moves to that point
//    in the game and returns a text string of the move.
int
sc_game_novelty (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * base = db;
    int current = db->gameNumber;
    const char * updateLabel = NULL;
    const char * interruptedStr =
        translate (ti, "NoveltyInterrupt", "Novelty search interrupted");
    const char * noNoveltyStr =
        translate (ti, "NoveltyNone", "No novelty was found for this game");

    const char * usage =
        "Usage: sc_game novelty [-older|-all] [-updatelabel <label>] [base]";

    bool olderGamesOnly = false;
    dateT currentDate = db->game->GetDate();

    int baseArg = 2;

    if (argc >= baseArg
        &&  argv[baseArg][0] == '-'  &&  argv[baseArg][1] == 'a'
        &&  strIsPrefix (argv[baseArg], "-all")) {
        olderGamesOnly = false;
        baseArg++;
    }
    if (argc >= baseArg
        &&  argv[baseArg][0] == '-'  &&  argv[baseArg][1] == 'o'
        &&  strIsPrefix (argv[baseArg], "-older")) {
        olderGamesOnly = true;
        baseArg++;
    }
    if (argc >= baseArg+2  &&
        argv[baseArg][0] == '-'  &&  argv[baseArg][1] == 'u'  &&
        strIsPrefix (argv[baseArg], "-updatelabel")) {
        updateLabel = argv [baseArg+1];
        baseArg += 2;
    }
    if (argc < baseArg  ||  argc > baseArg+1) {
        return errorResult (ti, usage);
    }
    if (argc == baseArg+1) {
        int baseNum = strGetInteger (argv[baseArg]);
        if (baseNum >= 1  &&  baseNum <= MAX_BASES) {
            base = &(dbList[baseNum - 1]);
        }
    }

    if (! base->inUse) {
        return errorResult (ti, "The selected database is not open.");
    }

    if (db->game->HasNonStandardStart()) {
        return errorResult (ti, "This game has a non-standard start position.");
    }

    if (updateLabel != NULL) {
        progBar.interrupt = false;
    }

    // First, move to the deepest ECO position in the game.
    // This code is adapted from sc_eco_game().
    db->game->MoveToPly (0);
    if (ecoBook) {
        DString ecoStr;
        do {} while (db->game->MoveForward() == OK);
        do {
            if (ecoBook->FindOpcode (db->game->GetCurrentPos(), "eco",
                                     &ecoStr) == OK) {
                break;
            }
            if (updateLabel != NULL) {
                char text [250];
                sprintf (text, "%s configure -text {Finding last opening position ...}",
                         updateLabel);
                Tcl_Eval (ti, text);
                Tcl_Eval (ti, "update");
                if (interruptedProgress()) {
                    return errorResult (ti, interruptedStr);
                }
            }
        } while (db->game->MoveBackup() == OK);
    }

    // Now keep doing an exact position search (ignoring the current
    // game) and skipping to the next game position whenever a match
    // is found, until a position not in any database game is reached:

    bool foundMatch = true;
    while (1) {
        // Loop searching on each game:
        Position * pos = db->game->GetCurrentPos();

        if (updateLabel != NULL) {
            char text [250];
            char san [16];
            db->game->GetSAN (san);
            Tcl_AppendResult (ti, san, NULL);
            sprintf (text, "%s configure -text {Trying: %u%s%s ...}",
                     updateLabel, pos->GetFullMoveCount(),
                     pos->GetToMove() == WHITE ? "." : "...", san);
            Tcl_Eval (ti, text);
            Tcl_Eval (ti, "update");
            if (interruptedProgress()) {
                return errorResult (ti, interruptedStr);
            }
        }

        matSigT msig = matsig_Make (pos->GetMaterial());
        uint hpSig = pos->GetHPSig();
        IndexEntry * ie;
        Game * g = scratchGame;
        foundMatch = false;

        for (uint gameNum=0; gameNum < base->numGames; gameNum++) {
            // Check for interruption every 64k games:
            if (updateLabel != NULL  &&  ((gameNum & 65535) == 65535)) {
                Tcl_Eval (ti, "update");
                if (interruptedProgress()) {
                    return errorResult (ti, interruptedStr);
                }
            }
            // Ignore the current game:
            if (db == base  &&  current >= 0) {
                if ((uint)current == gameNum) { continue; }
            }

            ie = base->idx->FetchEntry (gameNum);
            if (ie->GetLength() == 0) { continue; }

            // Ignore games with non-standard start:
            if (ie->GetStartFlag()) { continue; }

            // Ignore newer games if requested:
            if (olderGamesOnly  &&  ie->GetDate() >= currentDate) {
                continue;
            }

            // Check home pawn signature optimisation:
            if (hpSig != 0xFFFF) {
                const byte * hpData = ie->GetHomePawnData();
                if (! hpSig_PossibleMatch (hpSig, hpData)) {
                    continue;
                }
            }

            // Check material signature optimisation:
            if (!matsig_isReachable (msig, ie->GetFinalMatSig(),
                                     ie->GetPromotionsFlag(),
                                     ie->GetUnderPromoFlag())) {
                continue;
            }

            if (base->gfile->ReadGame (db->bbuf, ie->GetOffset(),
                                     ie->GetLength()) != OK) {
                return errorResult (ti, "Error reading game file.");
            }
            if (g->ExactMatch (pos, db->bbuf, NULL)) {
                foundMatch = true;
                break;
            }
        } // for-loop

        // Now, if foundMatch is false, we have our novelty position:
        if (! foundMatch) { break; }

        // Otherwise, go to the next game position and try again:
        if (db->game->MoveForward() != OK) { break; }
    }

    if (foundMatch) {
        return errorResult (ti, noNoveltyStr);
    }

    db->game->MoveBackup();
    Position * pos = db->game->GetCurrentPos();
    appendUintResult (ti, pos->GetFullMoveCount());
    Tcl_AppendResult (ti, pos->GetToMove() == WHITE ? "." : "...", NULL);
    char san [16];
    db->game->GetSAN (san);
    Tcl_AppendResult (ti, san, NULL);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_pgn:
//    Returns the PGN representation of the game.
//    Optional args:
//      -format (plain|html|latex): output format. Default=plain.
//      -shortHeader (0|1): short, 3-line (non-PGN) header. Default=0.
//      -space (0|1): printing a space after move numbers. Default=0.
//      -tags (0|1): printing (nonstandard) tags. Default=1.
//      -comments (0|1): printing nags/comments. Default=1.
//      -variations (0|1): printing variations. Default=1.
//      -indentVars (0|1): indenting variations. Default=0.
//      -indentComments (0|1): indenting comments. Default=0.
//      -width (number): line length for wordwrap. Default=huge (99999),
//        to let a Tk text widget do its own line-breaking.
//      -base (number): Print the game from the numbered base.
//      -gameNumber (number): Print the numbered game instead of the
//        active game.
//      -unicode (0|1): use unicocde characters (e.g. U+2654 for king). Default=0.
//      -stripbraces (0|1): remove {, } from inside PGN comments
int
sc_game_pgn (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "-column", "-comments", "-base", "-gameNumber", "-format",
        "-shortHeader", "-indentComments", "-indentVariations",
        "-symbols", "-tags", "-variations", "-width", "-space",
        "-markCodes", "-scidFlags", "-unicode", "-stripbraces",
        NULL
    };
    enum {
        OPT_COLUMN, OPT_COMMENTS, OPT_BASE, OPT_GAME_NUMBER, OPT_FORMAT,
        OPT_SHORT_HDR, OPT_INDENT_COMMENTS, OPT_INDENT_VARS,
        OPT_SYMBOLS, OPT_TAGS, OPT_VARS, OPT_WIDTH, OPT_SPACE,
        OPT_NOMARKS, OPT_SCIDFLAGS, OPT_UNICODE, OPT_STRIPBRACES,
    };

    scidBaseT * base = db;
    Game * g = db->game;
    uint lineWidth = 99999;
    g->ResetPgnStyle();
    g->SetPgnFormat (PGN_FORMAT_Plain);
    g->AddPgnStyle (PGN_STYLE_TAGS | PGN_STYLE_COMMENTS | PGN_STYLE_VARS);

    // Parse all the command options:
    // Note that every option takes a value so options/values always occur
    // in pairs, which simplifies the code.

    int thisArg = 2;
    while (thisArg < argc) {
        int index = strUniqueMatch (argv[thisArg], options);
        if (index == -1) {
            Tcl_AppendResult (ti, "Invalid option to sc_game pgn: ",
                              argv[thisArg], "; valid options are: ", NULL);
            for (const char ** s = options; *s != NULL; s++) {
                Tcl_AppendResult (ti, *s, " ", NULL);
            }
            return TCL_ERROR;
        }

        // Check that our option has a value:
        if (thisArg+1 == argc) {
            Tcl_AppendResult (ti, "Invalid option value: sc_game pgn ",
                              options[index], " requires a value.", NULL);
            return TCL_ERROR;
        }

        uint value = strGetUnsigned (argv[thisArg+1]);

        if (index == OPT_WIDTH) {
            lineWidth = value;

        } else if (index == OPT_BASE) {
            if (value >= 1  &&  value <= (uint)MAX_BASES) {
                base = &(dbList[value - 1]);
            }
            if (! base->inUse) {
                return setResult (ti, "The selected database is not in use.");
            }
            g = base->game;

        } else if (index == OPT_GAME_NUMBER) {
            // Print the numbered game instead of the active game:

            g = scratchGame;
            g->Clear();
            if (value < 1  ||  value > base->numGames) {
                return setResult (ti, "Invalid game number");
            }
            IndexEntry * ie = base->idx->FetchEntry (value - 1);
            if (ie->GetLength() == 0) {
                return errorResult (ti, "Error: empty game file record.");
            }
            if (base->gfile->ReadGame (base->bbuf, ie->GetOffset(),
                                     ie->GetLength()) != OK) {
                return errorResult (ti, "Error reading game file.");
            }
            if (g->Decode (base->bbuf, GAME_DECODE_ALL) != OK) {
                return errorResult (ti, "Error decoding game.");
            }
            g->LoadStandardTags (ie, base->nb);

        } else if (index == OPT_FORMAT) {
            // The option value should be "plain", "html" or "latex".
            if (! g->SetPgnFormatFromString (argv[thisArg+1])) {
                return errorResult (ti, "Invalid -format option.");
            }

        } else {
            // The option is a boolean affecting pgn style:
            uint bitmask = 0;
            switch (index) {
                case OPT_COLUMN:
                    bitmask = PGN_STYLE_COLUMN;          break;
                case OPT_COMMENTS:
                    bitmask = PGN_STYLE_COMMENTS;        break;
                case OPT_SYMBOLS:
                    bitmask = PGN_STYLE_SYMBOLS;         break;
                case OPT_TAGS:
                    bitmask = PGN_STYLE_TAGS;            break;
                case OPT_VARS:
                    bitmask = PGN_STYLE_VARS;            break;
                case OPT_SHORT_HDR:
                    bitmask = PGN_STYLE_SHORT_HEADER;    break;
                case OPT_SPACE:
                    bitmask = PGN_STYLE_MOVENUM_SPACE;   break;
                case OPT_INDENT_VARS:
                    bitmask = PGN_STYLE_INDENT_VARS;     break;
                case OPT_INDENT_COMMENTS:
                    bitmask = PGN_STYLE_INDENT_COMMENTS; break;
                case OPT_NOMARKS:
                    bitmask = PGN_STYLE_STRIP_MARKS;     break;
                case OPT_SCIDFLAGS:
                    bitmask = PGN_STYLE_SCIDFLAGS;       break;
                case OPT_UNICODE:
                    bitmask = PGN_STYLE_UNICODE;         break;
                case OPT_STRIPBRACES:
                    bitmask = PGN_STYLE_STRIP_BRACES;    break;
                default: // unreachable!
                    return errorResult (ti, "Invalid option.");
            };
            if (bitmask > 0) {
                if (value) {
                    g->AddPgnStyle (bitmask);
                } else {
                    g->RemovePgnStyle (bitmask);
                }
            }
        }
        thisArg += 2;
    }

    base->tbuf->Empty();
    base->tbuf->SetWrapColumn (lineWidth);
    g->WriteToPGN (base->tbuf);
    Tcl_AppendResult (ti, base->tbuf->GetBuffer(), NULL);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_pop:
//    Restores the last game saved with sc_game_push.
int
sc_game_pop (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (db->game->GetNextGame() != NULL) {
        Game * g = db->game->GetNextGame();
        delete db->game;
        db->gameAltered = g->GetAltered();
        db->game = g;
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_push:
//    Saves the current game and pushes a new empty game onto
//    the game state stack.
//    If the optional argument "copy" is present, the new game will be
//    a copy of the current game. If the argument is "copyfast" tags, comments are not decoded
int
sc_game_push (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool copy = false;
    bool copyfast = false;
    if ( argc > 2 && !strcmp( argv[2], "copy" ) ) {
        copy = true;
    }
    else if ( argc > 2 && !strcmp( argv[2], "copyfast" ) ) {
        copy = true;
        copyfast = true;
    }

    Game * g = new Game;
    g->SetNextGame (db->game);
    db->game->SetAltered (db->gameAltered);

    if (copy) {
        db->game->SaveState();
        db->game->Encode (db->bbuf, NULL);
        db->game->RestoreState();
        db->bbuf->BackToStart();
        if (copyfast)
          g->Decode (db->bbuf, GAME_DECODE_NONE);
        else
          g->Decode (db->bbuf, GAME_DECODE_ALL);
        g->CopyStandardTags (db->game);
        db->game->ResetPgnStyle (PGN_STYLE_VARS);
        db->game->SetPgnFormat (PGN_FORMAT_Plain);
        db->tbuf->Empty();
        db->game->WriteToPGN (db->tbuf);
        uint location = db->game->GetPgnOffset (0);
        db->tbuf->Empty();
        g->MoveToLocationInPGN (db->tbuf, location);
    }

    db->game = g;
    db->gameAltered = false;

    return TCL_OK;
}

// Do a minor shuffle of the si4 index file to reorder a single game. Author stevenaaus
// NB this is not atomic - ie, interuptions to scid or system in the process can be bad
// TODO - maybe enable reordering of a group of consecutive games

int
sc_game_reorder (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_game reorder GameNum GameDest.";

    if ( argc != 4 ) {
        Tcl_AppendResult (ti, usage, NULL);
        return TCL_ERROR;
    }

    // unneeded  / wrong ?
    if (!db->idx->AllInMemory()) {
      Tcl_AppendResult (ti, "NOT in memory", NULL);
      return TCL_ERROR;
    }

    uint gnum,newgame;
    int direction;

    if (sscanf (argv[2], "%u", &gnum ) != 1 || gnum == 0) {
	Tcl_AppendResult (ti, "Game Reorder: invalid game number.", NULL);
        return TCL_ERROR;
    }
    gnum--;

    if (sscanf (argv[3], "%u", &newgame ) != 1 || newgame == 0 || newgame > db->numGames ) {
	Tcl_AppendResult (ti, "Game Reorder: invalid destination.", NULL);
        return TCL_ERROR;
    }
    newgame--;

    if (newgame == gnum) {
	Tcl_AppendResult (ti, "Game Reorder: game and destination are the same.", NULL);
        return TCL_ERROR;
    }

    if (newgame < gnum)
      direction = -1;
    else
      direction = 1;

    IndexEntry ie_gnum, *ie, *ie_next;

    uint ie_gnum_Filter, ie_next_Filter;

    // save source game, filter
    ie = db->idx->FetchEntry (gnum);
    ie_gnum = *ie;
    ie_gnum_Filter = db->filter->Get(gnum);

    //// Shuffle games in the index file.
    // WriteEntries also updates the in-memory data structs
    // Is there a better way to do this ?

    for (uint i = gnum; i != newgame; i = i + direction) {

	ie_next = db->idx->FetchEntry(i + direction);
	ie_next_Filter = db->filter->Get(i + direction);

	if (ie_next == NULL) {
	  Tcl_AppendResult (ti, "Game Reorder: game out of bounds, bad.", NULL);
	  return TCL_ERROR;
	}

        if (db->idx->WriteEntries (ie_next, i, 1) != OK) {
          Tcl_AppendResult (ti, "Game Reorder: error writing index file.", NULL);
          return TCL_ERROR;
        }

	db->filter->Set(i,ie_next_Filter);
    }

    // Write original game to new position

    if (db->idx->WriteEntries (&ie_gnum, newgame, 1) != OK) {
      Tcl_AppendResult (ti, "Game Reorder: error writing source game into index file.", NULL);
      return TCL_ERROR;
    }

    db->filter->Set(newgame, ie_gnum_Filter);

    db->treeCache->Clear();
    db->backupCache->Clear();

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_savegame:
//    Called by sc_game_save and by clipbase functions to save
//    a new game or replacement game to a database.
//    Any errors are appended to the Tcl interpreter result.
//
//   (NB - See below for another sc_savegame, which is used in switcher game copy (and else???))
int
sc_savegame (Tcl_Interp * ti, Game * game, gameNumberT gnum, scidBaseT * base)
{
    char temp[200];
    if (! base->inUse) {
        Tcl_AppendResult (ti, errMsgNotOpen(ti), NULL);
        return TCL_ERROR;
    }
    if (base->fileMode == FMODE_ReadOnly  &&  !(base->memoryOnly)) {
        Tcl_AppendResult (ti, errMsgReadOnly(ti), NULL);
        return TCL_ERROR;
    }
    if (base == clipbase   &&  base->numGames >= clipbaseMaxGames) {
        sprintf (temp, "The clipbase has a limit of %u games.\n", clipbaseMaxGames);
        Tcl_AppendResult (ti, temp, NULL);
        return TCL_ERROR;
    }

    base->bbuf->Empty();
    bool replaceMode = false;
    gameNumberT gNumber = 0;
    if (gnum > 0) {
        gNumber = gnum - 1;  // Number games from zero.
        replaceMode = true;
    }

    // Grab a new idx entry, if needed:
    IndexEntry * oldIE = NULL;
    IndexEntry iE;
    iE.Init();

    errorT result;
    if ((result = game->Encode (base->bbuf, &iE)) != OK) {
      if (result == ERROR_GameFull) {
	sprintf (temp, "Error encoding game. Game size %u, Max game size %u. \n", base->bbuf->GetByteCount(), MAX_GAME_LENGTH);
        Tcl_AppendResult (ti, temp, NULL);
      } else {
        Tcl_AppendResult (ti, "Error encoding game.\n", NULL);
      }
      return TCL_ERROR;
    }

    // game->Encode computes flags, so we have to re-set flags if replace mode
    if (replaceMode) {
        oldIE = base->idx->FetchEntry (gNumber);
        // Remember previous user-settable flags:
        for (uint flag = 0; flag < IDX_NUM_FLAGS; flag++) {
            char flags [32];
            oldIE->GetFlagStr (flags, NULL);
            iE.SetFlagStr (flags);
        }
    } else {
      // add game without resetting the index, because it has been filled by game->encode above
      if (base->idx->AddGame (&gNumber, &iE, false) != OK) {
        sprintf (temp, "Scid maximum games (%u) reached.\n", MAX_GAMES);
        Tcl_AppendResult (ti, temp, NULL);
        return TCL_ERROR;
      }
      base->numGames = base->idx->GetNumGames();
    }

    base->bbuf->BackToStart();

    // Now try writing the game to the gfile:
    uint offset = 0;
    if (base->gfile->AddGame (base->bbuf, &offset) != OK) {
        Tcl_AppendResult (ti, "Error writing game file.", NULL);
        return TCL_ERROR;
    }
    iE.SetOffset (offset);
    iE.SetLength (base->bbuf->GetByteCount());

    // Now we add the names to the NameBase
    // If replacing, we decrement the frequency of the old names.
    const char * s;
    idNumberT id = 0;

    // WHITE:
    s = game->GetWhiteStr();  if (!s) { s = "?"; }
    if (base->nb->AddName (NAME_PLAYER, s, &id) == ERROR_NameBaseFull) {
        sprintf (temp, "Player Name limit of %u exceeded\n", NAME_MAX_ID [NAME_PLAYER]);
        Tcl_AppendResult (ti, temp, NULL);
        return TCL_ERROR;
    }
    base->nb->IncFrequency (NAME_PLAYER, id, 1);
    iE.SetWhite (id);

    // BLACK:
    s = game->GetBlackStr();  if (!s) { s = "?"; }
    if (base->nb->AddName (NAME_PLAYER, s, &id) == ERROR_NameBaseFull) {
        sprintf (temp, "Player Name limit of %u exceeded\n", NAME_MAX_ID [NAME_PLAYER]);
        Tcl_AppendResult (ti, temp, NULL);
        return TCL_ERROR;
    }
    base->nb->IncFrequency (NAME_PLAYER, id, 1);
    iE.SetBlack (id);

    // EVENT:
    s = game->GetEventStr();  if (!s) { s = "?"; }
    if (base->nb->AddName (NAME_EVENT, s, &id) == ERROR_NameBaseFull) {
        sprintf (temp, "Event Name limit of %u exceeded\n", NAME_MAX_ID [NAME_EVENT]);
        Tcl_AppendResult (ti, temp, NULL);
        return TCL_ERROR;
    }
    base->nb->IncFrequency (NAME_EVENT, id, 1);
    iE.SetEvent (id);

    // SITE:
    s = game->GetSiteStr();  if (!s) { s = "?"; }
    if (base->nb->AddName (NAME_SITE, s, &id) == ERROR_NameBaseFull) {
        sprintf (temp, "Site Name limit of %u exceeded\n", NAME_MAX_ID [NAME_SITE]);
        Tcl_AppendResult (ti, temp, NULL);
        return TCL_ERROR;
    }
    base->nb->IncFrequency (NAME_SITE, id, 1);
    iE.SetSite (id);

    // ROUND:
    s = game->GetRoundStr();  if (!s) { s = "?"; }
    if (base->nb->AddName (NAME_ROUND, s, &id) == ERROR_NameBaseFull) {
        sprintf (temp, "Round Name limit of %u exceeded\n", NAME_MAX_ID [NAME_ROUND]);
        Tcl_AppendResult (ti, temp, NULL);
        return TCL_ERROR;
    }
    base->nb->IncFrequency (NAME_ROUND, id, 1);
    iE.SetRound (id);

    // If replacing, decrement the frequency of the old names:
    if (replaceMode) {
        base->nb->IncFrequency (NAME_PLAYER, oldIE->GetWhite(), -1);
        base->nb->IncFrequency (NAME_PLAYER, oldIE->GetBlack(), -1);
        base->nb->IncFrequency (NAME_EVENT,  oldIE->GetEvent(), -1);
        base->nb->IncFrequency (NAME_SITE,   oldIE->GetSite(),  -1);
        base->nb->IncFrequency (NAME_ROUND,  oldIE->GetRound(), -1);
    }

    // Flush the gfile so it is up-to-date with other files:
    // This made copying games between databases VERY slow, so it
    // is now done elsewhere OUTSIDE a loop that copies many
    // games, such as in sc_filter_copy().
    //base->gfile->FlushAll();

    // Last of all, we write the new idxEntry, but NOT the index header
    // or the name file, since there might be more games saved yet and
    // writing them now would then be a waste of time.

    if (base->idx->WriteEntries (&iE, gNumber, 1) != OK) {
        Tcl_AppendResult (ti, "Error writing index file.", NULL);
        return TCL_ERROR;
    }

    // We need to increase the filter size if a game was added:
    if (! replaceMode) {
        base->filter->Append (1);  // Added game is in filter by default.
        if(base->filter != base->dbFilter)
           base->dbFilter->Append (1);
        if(base->treeFilter)
           base->treeFilter->Append (1);

        if (base->duplicates != NULL) {
            delete[] base->duplicates;
            base->duplicates = NULL;
        }
    }
    base->undoCurrent = base->undoIndex;
    base->undoCurrentNotAvail = false;

    return OK;
}


int
sc_savegame (Tcl_Interp * ti, scidBaseT * sourceBase, ByteBuffer * bbuf, IndexEntry * srcIe, scidBaseT * base)
{
    char temp[200];
    if (! base->inUse) {
        Tcl_AppendResult (ti, errMsgNotOpen(ti), NULL);
        return TCL_ERROR;
    }
    if (base->fileMode == FMODE_ReadOnly  &&  !(base->memoryOnly)) {
        Tcl_AppendResult (ti, errMsgReadOnly(ti), NULL);
        return TCL_ERROR;
    }
    if (base == clipbase   &&  base->numGames >= clipbaseMaxGames) {
        sprintf (temp, "The clipbase has a limit of %u games.\n",
                 clipbaseMaxGames);
        Tcl_AppendResult (ti, temp, NULL);
        return TCL_ERROR;
    }

    base->bbuf->Empty();
    bool replaceMode = false;

    // Grab a new idx entry, if needed:
    IndexEntry iE;
    iE.Init();
    gameNumberT gNumber = 0;

	memcpy( (void *) &iE, (void *) srcIe, sizeof(IndexEntry));

    // add game without resetting the index, because it has been filled by game->encode above
    if (base->idx->AddGame (&gNumber, &iE, false) != OK) {
        sprintf (temp, "Scid maximum games (%u) reached.\n", MAX_GAMES);
        Tcl_AppendResult (ti, temp, NULL);
        return TCL_ERROR;
    }
    base->numGames = base->idx->GetNumGames();

    base->bbuf->BackToStart();

    // Now try writing the game to the gfile:
    uint offset = 0;
    if (base->gfile->AddGame (bbuf, &offset) != OK) {
        Tcl_AppendResult (ti, "Error writing game file.", NULL);
        return TCL_ERROR;
    }
    iE.SetOffset (offset);
    iE.SetLength (bbuf->GetByteCount());

    // Now we add the names to the NameBase
    // If replacing, we decrement the frequency of the old names.
    const char * s;
    idNumberT id = 0;

    // WHITE:
	s = srcIe->GetWhiteName( sourceBase->nb);  if (!s) { s = "?"; }
    if (base->nb->AddName (NAME_PLAYER, s, &id) == ERROR_NameBaseFull) {
        Tcl_AppendResult (ti, "Too many player names.", NULL);
        return TCL_ERROR;
    }
    base->nb->IncFrequency (NAME_PLAYER, id, 1);
    iE.SetWhite (id);

    // BLACK:
    s = srcIe->GetBlackName( sourceBase->nb);  if (!s) { s = "?"; }
    if (base->nb->AddName (NAME_PLAYER, s, &id) == ERROR_NameBaseFull) {
        Tcl_AppendResult (ti, "Too many player names.", NULL);
        return TCL_ERROR;
    }
    base->nb->IncFrequency (NAME_PLAYER, id, 1);
    iE.SetBlack (id);

    // EVENT:
    s = srcIe->GetEventName( sourceBase->nb);  if (!s) { s = "?"; }
    if (base->nb->AddName (NAME_EVENT, s, &id) == ERROR_NameBaseFull) {
        Tcl_AppendResult (ti, "Too many event names.", NULL);
        return TCL_ERROR;
    }
    base->nb->IncFrequency (NAME_EVENT, id, 1);
    iE.SetEvent (id);

    // SITE:
    s = srcIe->GetSiteName( sourceBase->nb);  if (!s) { s = "?"; }
    if (base->nb->AddName (NAME_SITE, s, &id) == ERROR_NameBaseFull) {
        Tcl_AppendResult (ti, "Too many site names.", NULL);
        return TCL_ERROR;
    }
    base->nb->IncFrequency (NAME_SITE, id, 1);
    iE.SetSite (id);

    // ROUND:
    s = srcIe->GetRoundName( sourceBase->nb);  if (!s) { s = "?"; }
    if (base->nb->AddName (NAME_ROUND, s, &id) == ERROR_NameBaseFull) {
        Tcl_AppendResult (ti, "Too many round names.", NULL);
        return TCL_ERROR;
    }
    base->nb->IncFrequency (NAME_ROUND, id, 1);
    iE.SetRound (id);

    // Last of all, we write the new idxEntry, but NOT the index header
    // or the name file, since there might be more games saved yet and
    // writing them now would then be a waste of time.

    if (base->idx->WriteEntries (&iE, gNumber, 1) != OK) {
        Tcl_AppendResult (ti, "Error writing index file.", NULL);
        return TCL_ERROR;
    }

    // We need to increase the filter size if a game was added:
    if (! replaceMode) {
        base->filter->Append (1);  // Added game is in filter by default.
        if(base->filter != base->dbFilter)
           base->dbFilter->Append (1);
        if(base->treeFilter)
           base->treeFilter->Append (1);

        if (base->duplicates != NULL) {
            delete[] base->duplicates;
            base->duplicates = NULL;
        }
    }
    return OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_save:
//    Saves the current game. If the parameter is 0, a NEW
//    game is added; otherwise, that game number is REPLACED.
int
sc_game_save (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{

    if (argc != 3) {
        return errorResult (ti, "Usage: sc_game save <gameNumber>");
    }
    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (db->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }
    db->bbuf->Empty();

    uint gnum = strGetUnsigned (argv[2]);
    if (gnum > db->numGames) {
        char tempStr[20];
        sprintf (tempStr, "%u", db->numGames);
        Tcl_AppendResult (ti, "Invalid game number; there are only ",
                          tempStr, " games in this database.", NULL);
        return TCL_ERROR;
    }

    db->game->SaveState ();
    if (sc_savegame (ti, db->game, gnum, db) != OK) { return TCL_ERROR; }
    db->gfile->FlushAll();
    db->game->RestoreState ();
    if (db->idx->WriteHeader() != OK) {
        return errorResult (ti, "Error writing index file.");
    }
    if (! db->memoryOnly  &&  db->nb->WriteNameFile() != OK) {
        return errorResult (ti, "Error writing name file.");
    }

    if (gnum == 0) {
        // Saved new game, so set gameNumber to the saved game number:
        db->gameNumber = db->numGames - 1;
    }
    db->gameAltered = false;

    // We must ensure that the Index is still all in memory:
    db->idx->ReadEntireFile();

    recalcFlagCounts (db);
    // Finally, saving a game makes the treeCache out of date:
    db->treeCache->Clear();
    db->backupCache->Clear();
    if (! db->memoryOnly) { removeFile (db->fileName, TREEFILE_SUFFIX); }

    return TCL_OK;
}

//    Called by sc_game_scores to check a comment for a numeric
//    evaluation (a score), and add it to the list result for the
//    specified Tcl interpreter if a score is found.
//
static bool
addScoreToList (Tcl_Interp * ti, int moveCounter, const char * comment,
                bool negate, float min, float max)
{
    char buffer[1024];
    if (comment == NULL) { return false; }
    while (*comment != 0  &&  *comment != '+'  &&  *comment != '-') {
        comment++;
    }
    if (*comment == 0  || ! isdigit(*(comment+1))) { return false; }
    //Klimmek: ignore game results like 1-0 or 0-1 in a comment
    if (*comment == '-' && isdigit(*(comment-1))) { return false; }
    // OK, now we have found "+[digit]" or "-[digit]" in the comment,
    // so extract its evaluation and add it to our list:
    sprintf (buffer, "%.1f", (float)moveCounter * 0.5 + 0.5);
    Tcl_AppendElement (ti, buffer);
    float f;
    sscanf (comment, "%f", &f);
    if (negate) { f = -f; }
    if (f < min) { f = min; } // min is -10
    if (f > max) { f = max; }
    /* Hmm - can't do this as equal games show the y-axis -1 to 1, and .05 is too noticeable
     * // pad out zero scores so we can see something
     * if (f >= 0 && f < .05) f=.05;
     * if (f < 0 && f > -.05) f=-.05;
    */
    sprintf (buffer, "%.2f", f);
    Tcl_AppendElement (ti, buffer);
    return true;
}


//    Returns a Tcl list of the numeric scores of each move, as found
//    in the commment for each move.
//    A score is a number with the format
//        "+digits.digits" or
//        "-digits.digits"
//    found somewhere in the comment of the move
//
//    Remove compatability with Crafty annotations
//    because uncommented moves with a commented variations give wrong graphs - S.A.
//
//    Scid annotations which have the form
//        1.e4 {"+0.13: ...."} e5 ...
//    version 18.
//
//    The list returned should be read in pairs of values: the first is the
//    move (0.0 = start, 0.5 after White's first move, 1.0 after Black's
//    first move, etc) and the second is the value found.
//
//    Note - This now gives 1.0 after whites first move, 1.5 after blacks first move
//    so as to better work with the Score graph X axis. See svn revision 1346
int
sc_game_scores (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    int moveCounter = 0;

    float max = 10.0;
    float min = -max;
    bool inv_w = false;
    bool inv_b = false;

    if (argc != 4)
        return errorResult (ti, "Usage: sc_game scores invert_white invert_black");

    inv_w = atoi (argv[2]);
    inv_b = atoi (argv[3]);

    Game * g = db->game;
    const char * comment;
    g->SaveState ();
    g->MoveToPly (0);
    while (g->MoveForward() == OK) {
        moveCounter++;
        comment = g->GetMoveComment();
        // Klimmek: use invertflags
        addScoreToList (ti, moveCounter, comment, moveCounter % 2 ? inv_b : inv_w, min, max);
    }
    db->game->RestoreState ();
    return TCL_OK;
}

// Similar to sc_game_scores, but returns only [%<type> VALUE] lists

int
sc_game_values (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3)
        return errorResult (ti, "Usage: sc_game values <type>");

    int moveCounter = 0;

    Game * g = db->game;
    const char * comment;
    char type[50];
    sprintf (type, "[%%%s ", argv[2]);

    g->SaveState ();
    g->MoveToPly (0);
    while (g->MoveForward() == OK) {
        int offset;
        moveCounter++;
        comment = g->GetMoveComment();
        if (comment && (offset = strContainsIndex (comment, type)) > -1) {

	    char buffer[1024], buffer2[1024];
	    sprintf (buffer, "%.1f", (float)moveCounter * 0.5 + 0.5);

	    const char * p = comment + offset + strlen(type);

            int found = 0, i = 0 ;
            while (p[i] && i < 1023) {
		if (p[i] == ']') {
		    found = 1;
		    break;
		}
		buffer2[i] = p[i];
		i++;
            }
            if (found) {
		buffer2[i] = 0;
		Tcl_AppendElement (ti, buffer);
		Tcl_AppendElement (ti, buffer2);
            }
        }
    }
    db->game->RestoreState ();
    return TCL_OK;
}

//  Tests for non-standard start, or sets the start position from a FEN string.

int
sc_game_startBoard (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc == 2) {
        return setBoolResult (ti, db->game->HasNonStandardStart());
    } else if (argc != 3) {
        return errorResult (ti, "Usage: sc_game startBoard <fenString>");
    }
    const char * str = argv[2];
    if (strIsPrefix ("random:", str)) {
        // A "FEN" string that starts with "random:" is interpreted as a
        // material configuration, and a random position with this
        // set of material is generated. For example, "random:krpkr"
        // generates a random legal Rook+Pawn-vs-Rook position.
        if (scratchPos->Random (str+7) != OK) {
            return errorResult (ti, "Invalid material string.");
        }
    } else {
        if (scratchPos->ReadFromFEN (str) != OK) {
            if (scratchPos->ReadFromLongStr (str) != OK) {
                return errorResult (ti, "Invalid FEN string.");
            }
        }
        // ReadFromFEN checks that there is one king of each side, but it
        // does not check that the position is actually legal:
        if (! scratchPos->IsLegal()) {
            // Illegal position! Find out why to return a useful error:
           squareT wk = scratchPos->GetKingSquare (WHITE);
           squareT bk = scratchPos->GetKingSquare (BLACK);
           if (square_Adjacent (wk, bk)) {
               return errorResult (ti, "Illegal position: adjacent kings.");
           }
           // No adjacent kings, so enemy king must be in check.
           return errorResult (ti, "Illegal position: enemy king in check.");
        }
    }
    db->game->SetStartPos (scratchPos);
    db->gameAltered = true;
    return TCL_OK;
}

// returns the games initial position

int
sc_game_startPos (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    char boardStr [200] = "";
    if (db->game->HasNonStandardStart()) 
	db->game->GetStartPos()->PrintFEN (boardStr, FEN_ALL_FIELDS);
    Tcl_AppendResult (ti, boardStr, NULL);
    return TCL_OK;
}

//    Strip all comments or variations from game(s)
//    todo - this should probably be rewritten properly, instead of using/overloading PGN parsing 
int
sc_game_strip (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage =
        "Usage: sc_game strip [comments|variations] [all|filter]";

    const char * options[] = { "comments", "variations", NULL };
    enum { OPT_COMS, OPT_VARS };

    const char * filter[]  = { "all", "filter", NULL };
    enum { OPT_ALL, OPT_FILTER };

    if (argc < 3 || argc > 4) 
        return errorResult (ti, usage);

    // we need to switch off short header style or PGN parsing will not work
    uint  old_style = db->game->GetPgnStyle ();
    uint  old_ply = db->game->GetCurrentPly ();
    if (old_style & PGN_STYLE_SHORT_HEADER)
      db->game->SetPgnStyle (PGN_STYLE_SHORT_HEADER, false);

    db->game->AddPgnStyle (PGN_STYLE_TAGS);
    db->game->AddPgnStyle (PGN_STYLE_COMMENTS);
    db->game->AddPgnStyle (PGN_STYLE_VARS);
    db->game->SetPgnFormat (PGN_FORMAT_Plain);

    int type = -1;
    type = strUniqueMatch (argv[2], options);

    switch (type) {
        case OPT_COMS: db->game->RemovePgnStyle (PGN_STYLE_COMMENTS); break;
        case OPT_VARS: db->game->RemovePgnStyle (PGN_STYLE_VARS); break;
        default: return errorResult (ti, usage);
    }

    bool singleGame = {argc == 3};

    if (singleGame) {
	int old_lang = language;
	language = 0;
	db->tbuf->Empty();
	db->tbuf->SetWrapColumn (99999);
	db->game->WriteToPGN (db->tbuf);
	PgnParser parser;

	parser.Reset ((const char *) db->tbuf->GetBuffer());
	scratchGame->Clear();
	if (parser.ParseGame (scratchGame)) {
	    return errorResult (ti, "Error: unable to strip this game.");
	}
	parser.Reset ((const char *) db->tbuf->GetBuffer());
	db->game->Clear();
	parser.ParseGame (db->game);

	// Restore PGN style (Short header)
	if (old_style & PGN_STYLE_SHORT_HEADER)
	    db->game->SetPgnStyle (PGN_STYLE_SHORT_HEADER, true);

	db->game->MoveToPly (old_ply);
	db->gameAltered = true;
	language = old_lang;
	return TCL_OK;
    } else {
	int old_lang = language;
	language = 0;
	PgnParser parser;

	db->tbuf->Empty();
	db->tbuf->SetWrapColumn (99999);

	bool limitToFilter;
	int index = strUniqueMatch (argv[3], filter);

	switch (index) {
	  case OPT_ALL:    limitToFilter = 0 ; break;
	  case OPT_FILTER: limitToFilter = 1 ; break;
	  default: return errorResult (ti, usage);
        }

	bool showProgress = startProgressBar();
	uint updateStart, update;
	updateStart = update = 250;  // Update progress bar every .... games

	IndexEntry * ie;
	Game * g = scratchGame;

        g->SetPgnStyle (PGN_STYLE_COMMENTS,true);
        g->SetPgnStyle (PGN_STYLE_VARS,true);
	switch (type) {
	    case OPT_COMS: g->RemovePgnStyle (PGN_STYLE_COMMENTS); break;
	    case OPT_VARS: g->RemovePgnStyle (PGN_STYLE_VARS); break;
	    default: return errorResult (ti, usage);
	}

	for (uint gameNum=0; gameNum < db->numGames; gameNum++) {
	    if (showProgress) {
		update--;
		if (update == 0) {
		    update = updateStart;
		    updateProgressBar (ti, gameNum, db->numGames);
		    if (interruptedProgress()) { break; }
		}
	    }

            if (limitToFilter && db->dbFilter->Get(gameNum) == 0) {
                continue;
            }

	    ie = db->idx->FetchEntry (gameNum);

	    if ((type == OPT_COMS && ie->GetCommentCount() == 0) ||
                (type == OPT_VARS && ie->GetVariationCount() == 0))
		continue;

// Fixme ? Possibly some major inaccuracies - S.A.

	    // Load the game.
	    if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(), ie->GetLength()) != OK) {
		return errorResult (ti, "Error reading game file.");
	    }
	    g->Clear();
	    if (g->Decode (db->bbuf, GAME_DECODE_ALL) != OK)
		continue;
	    g->LoadStandardTags (ie, db->nb); // for metadata matches
	    // g->SetNumber (gameNum + 1); // for game range tests
	    // g->SetAltered(false);

	    db->tbuf->Empty();
	    db->tbuf->SetWrapColumn (99999);

	    g->WriteToPGN (db->tbuf);
	    parser.Reset ((const char *) db->tbuf->GetBuffer());
	    parser.ParseGame (g);

	    if (sc_savegame (ti, g, gameNum+1, db) != OK) { return TCL_ERROR; }

// necessary ??
	    db->gfile->FlushAll();
	    if (db->idx->WriteHeader() != OK) {
		return errorResult (ti, "Error writing index file.");
	    }

	}

// New namefile/flush code (from backend of sc_game_save) for batch stripping comments/vars
// Some names will get altered by our bastard method of writing to pgn and then rereading the game
// eg those with " in a name, and [White "Kramnik,V] is now [White "Kramnik, V"]
// 
// !!!!!!!!!!!!!!!!!!! S.A. 28/9/2020
    if (! db->memoryOnly  &&  db->nb->WriteNameFile() != OK) {
        return errorResult (ti, "Error writing name file.");
    }

    db->gameAltered = false;

    // We must ensure that the Index is still all in memory:
    db->idx->ReadEntireFile();

    recalcFlagCounts (db);
    // Finally, saving a game makes the treeCache out of date:
    db->treeCache->Clear();
    db->backupCache->Clear();
    if (! db->memoryOnly) { removeFile (db->fileName, TREEFILE_SUFFIX); }

// !!!!!!!!!!!!!!!!!!!

	if (showProgress) { updateProgressBar (ti, 1, 1); }

	// hmmm scratchGame is static. Maybe we should reset its style
        g->SetPgnStyle (PGN_STYLE_COMMENTS,true);
        g->SetPgnStyle (PGN_STYLE_VARS,true);

	// Restore PGN style (Short header)
	if (old_style & PGN_STYLE_SHORT_HEADER)
	    db->game->SetPgnStyle (PGN_STYLE_SHORT_HEADER, true);

// end of fix me

	language = old_lang;
	return TCL_OK;
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_summary:
//    Returns summary information of the specified game:
//    its players, site, etc; or its moves; or all its boards
//    positions.
int
sc_game_summary (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_game summary [-base <baseNum>] [-gameNumber <gameNum>] header|boards|moves";

    const char * options[] = {
        "-base", "-gameNumber", NULL
    };
    enum { OPT_BASE, OPT_GNUM };

    scidBaseT * base = db;
    uint gnum = 0;

    int arg = 2;
    while (arg+1 < argc) {
        const char * value = argv[arg+1];
        int index = strUniqueMatch (argv[arg], options);
        arg += 2;

        if (index == OPT_BASE) {
            int baseNum = strGetUnsigned (value);
            if (baseNum >= 1 && baseNum <= MAX_BASES) {
                base = &(dbList[baseNum - 1]);
            }
        } else if (index == OPT_GNUM) {
            gnum = strGetUnsigned (value);
        } else {
            return errorResult (ti, usage);
        }
    }
    if (arg+1 != argc) { return errorResult (ti, usage); }

    enum modeT { MODE_HEADER, MODE_BOARDS, MODE_MOVES };
    modeT mode = MODE_HEADER;
    switch (tolower(argv[arg][0])) {
        case 'h': mode = MODE_HEADER; break;
        case 'b': mode = MODE_BOARDS; break;
        case 'm': mode = MODE_MOVES; break;
        default: return errorResult (ti, usage);
    }

    Game *g;
    if (gnum == 0) {
        g = base->game;
    } else {
        g = scratchGame;
        // Load the specified game number:
        if (! base->inUse) {
            return errorResult (ti, "This database is not in use.");
        }
        if (gnum > base->numGames) {
            return errorResult (ti, "Invalid game number.");
        }
        gnum--;
        IndexEntry * ie = base->idx->FetchEntry (gnum);
        base->bbuf->Empty();
        if (base->gfile->ReadGame (base->bbuf, ie->GetOffset(), ie->GetLength()) != OK) {
            return errorResult (ti, "Error loading game.");
        }
        g->Clear();
        if (g->Decode (base->bbuf, GAME_DECODE_COMMENTS) != OK) {
            return errorResult (ti, "Error decoding game.");
        }
        g->LoadStandardTags (ie, base->nb);
    }

    // Return header summary if requested:
    if (mode == MODE_HEADER) {
        DString * dstr = new DString;
        dstr->Append (g->GetWhiteStr());
        eloT elo = g->GetWhiteElo();
        if (elo > 0) { dstr->Append (" (", elo, ")"); }
        dstr->Append ("  --  ", g->GetBlackStr());
        elo = g->GetBlackElo();
        if (elo > 0) { dstr->Append (" (", elo, ")"); }
        dstr->Append ("\n", g->GetEventStr());

        const char * round = g->GetRoundStr();
        if (! strIsUnknownName(round))
            dstr->Append (" (", translate (ti,"Round")," ", round, ")");

        const char * site = g->GetSiteStr();
        if (! strIsUnknownName(site))
            dstr->Append ("  ", site);

        char dateStr [20];
        date_DecodeToString (g->GetDate(), dateStr);
        // Remove ".??" or ".??.??" from end of date:
        if (dateStr[4] == '.'  &&  dateStr[5] == '?') { dateStr[4] = 0; }
        if (dateStr[7] == '.'  &&  dateStr[8] == '?') { dateStr[7] = 0; }
        dstr->Append ("\n",dateStr, "  ");

        uint m = (g->GetNumHalfMoves() + 1) / 2;
        dstr->Append ("  ", m , " ", translate (ti,"Moves"));

        dstr->Append ("   (" , RESULT_LONGSTR[g->GetResult()] , ")");
        Tcl_AppendResult (ti, dstr->Data(), NULL);
        delete dstr;
        return TCL_OK;
    }

    // List of the Boards or Moves
    // Moves now returns a double list n the form "premovecommment m1 c1 m2 c2 ... mN cN result"
    const char *tempStr;
          char *tempStrClean;
    g->SaveState();
    g->MoveToPly (0);

    if (mode == MODE_BOARDS) {
        while (1) {
            char boardStr[100];
            g->GetCurrentPos()->MakeLongStr (boardStr);
            Tcl_AppendElement (ti, boardStr);
            if (g->MoveForward() != OK) { break; }
         }
     } else { // mode == MODE_MOVES

          // initial comment
          tempStr = g->GetMoveComment();
          if (tempStr) {
              tempStrClean = strDuplicate(tempStr);
              strTrimMarkCodes (tempStrClean);
              if (tempStrClean && *tempStrClean && !strIsScore((const char *)tempStrClean)) {
                  Tcl_AppendElement (ti, tempStrClean);
              } else {
                  Tcl_AppendElement (ti, "");
              }
              delete[] tempStrClean;
          } else {
              Tcl_AppendElement (ti, "");
          }

          while (1) {

              colorT toMove = g->GetCurrentPos()->GetToMove();
              uint moveCount = g->GetCurrentPos()->GetFullMoveCount();
              char san [20];
              g->GetSAN (san);
              if (san[0] != 0) {
                  char temp[40];
                  if (toMove == WHITE) {
                      sprintf (temp, "%u.%s", moveCount, san);
                  } else {
                      strCopy (temp, san);
                  }
                  byte * nags = g->GetNextNags();
                  if (*nags != 0) {
                      for (uint nagCount = 0 ; nags[nagCount] != 0; nagCount++) {
                          char nagstr[20];
                          game_printNag (nags[nagCount], nagstr, true, PGN_FORMAT_Plain);
                          if (nagCount > 0  || (nagstr[0] != '!' && nagstr[0] != '?')) {
                              strAppend (temp, " ");
                          }
                          strAppend (temp, nagstr);
                      }
                  }

                  Tcl_AppendElement (ti, temp);

                  if (g->MoveForward() != OK) {
                    // unused now ? S.A
                    break;
                  }
                  // GetMoveComment is a funny thing. Get comment is out of sync with the move
                  tempStr = g->GetMoveComment();
                  if (tempStr) {
                      tempStrClean = strDuplicate(tempStr);
                      strTrimMarkCodes (tempStrClean);
                      if (tempStrClean && *tempStrClean && !strIsScore((const char *)tempStrClean)) {
                          Tcl_AppendElement (ti, tempStrClean);
                      } else {
                          Tcl_AppendElement (ti, "");
                      }
                      delete[] tempStrClean;
                  } else {
                      Tcl_AppendElement (ti, "");
                  }
              } else {
                  Tcl_AppendElement (ti, (char *)RESULT_LONGSTR[g->GetResult()]);
                  break;
              }
          }
      }

      g->RestoreState();
      return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_tags:
//   Get, set or reload the current game tags, or share them
//   with another game.
int
sc_game_tags (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * options[] = {
        "get", "set", "reload", "share", NULL
    };
    enum { OPT_GET, OPT_SET, OPT_RELOAD, OPT_SHARE };

    int index = -1;
    if (argc >= 3) { index = strUniqueMatch (argv[2], options); }

    switch (index) {
        case OPT_GET:    return sc_game_tags_get (cd, ti, argc, argv);
        case OPT_SET:    return sc_game_tags_set (cd, ti, argc, argv);
        case OPT_RELOAD: return sc_game_tags_reload (cd, ti, argc, argv);
        case OPT_SHARE:  return sc_game_tags_share (cd, ti, argc, argv);
        default:         return InvalidCommand (ti, "sc_game tags", options);
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_tags_get:
//    Gets a tag for the active game given its name.
//    Valid names are:  Event, Site, Date, Round, White, Black,
//       WhiteElo, BlackElo, ECO, Extra.
//    All except the last (Extra) return the tag value as a string.
//    For "Extra", the function returns all the extra tags as one long
//    string, in PGN format, one tag per line.
int
sc_game_tags_get (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{

    static const char * options [] = {
        "Event", "Site", "Date", "Year", "Month", "Day",
        "Round", "White", "Black", "Result", "WhiteElo",
        "BlackElo", "WhiteRType", "BlackRType", "ECO",
        "EDate", "EYear", "EMonth", "EDay", "Extra",
        NULL
    };
    enum {
        T_Event, T_Site, T_Date, T_Year, T_Month, T_Day,
        T_Round, T_White, T_Black, T_Result, T_WhiteElo,
        T_BlackElo, T_WhiteRType, T_BlackRType, T_ECO,
        T_EDate, T_EYear, T_EMonth, T_EDay, T_Extra
    };

    const char * usage = "Usage: sc_game tags get [-last] <tagName>";
    const char * tagName;
    Game * g = db->game;

    if (argc < 4  ||  argc > 5) {
        return errorResult (ti, usage);
    }
    tagName = argv[3];
    if (argc == 5) {
        if (!strEqual (argv[3], "-last")) { return errorResult (ti, usage); }
        tagName = argv[4];
        if (db->numGames > 0) {
            gameNumberT prevgame;
            // db->gameNumber is set to -1 for a new game, and it is (int) when perhaps it should be (uint)
            if (db->gameNumber == 0 || db->gameNumber == -1 || db->gameNumber > int(db->numGames)) {
              prevgame = db->numGames - 1;
            } else {
              prevgame = db->gameNumber - 1;
            }
            g = scratchGame;
            IndexEntry * ie = db->idx->FetchEntry (prevgame);
            if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(),
                                     ie->GetLength()) != OK) {
                return errorResult (ti, "Error reading game file.");
            }
            if (g->Decode (db->bbuf, GAME_DECODE_ALL) != OK) {
                return errorResult (ti, "Error decoding game.");
            }
            g->LoadStandardTags (ie, db->nb);
        }
    }
    const char * s;
    int index = strExactMatch (tagName, options);

    switch (index) {
    case T_Event:
        s = g->GetEventStr();  if (!s) { s = "?"; }
        Tcl_AppendResult (ti, s, NULL);
        break;

    case T_Site:
        s = g->GetSiteStr();  if (!s) { s = "?"; }
        Tcl_AppendResult (ti, s, NULL);
        break;

    case T_Date:
        {
            char dateStr[20];
            date_DecodeToString (g->GetDate(), dateStr);
            Tcl_AppendResult (ti, dateStr, NULL);
        }
        break;

    case T_Year:
        return setUintResult (ti, date_GetYear (g->GetDate()));

    case T_Month:
        return setUintWidthResult (ti, date_GetMonth (g->GetDate()), 2);

    case T_Day:
        return setUintWidthResult (ti, date_GetDay (g->GetDate()), 2);

    case T_Round:
        s = g->GetRoundStr();  if (!s) { s = "?"; }
        Tcl_AppendResult (ti, s, NULL);
        break;

    case T_White:
        s = g->GetWhiteStr();  if (!s) { s = "?"; }
        Tcl_AppendResult (ti, s, NULL);
        break;

    case T_Black:
        s = g->GetBlackStr();  if (!s) { s = "?"; }
        Tcl_AppendResult (ti, s, NULL);
        break;

    case T_Result:
        return setCharResult (ti, RESULT_CHAR[g->GetResult()]);

    case T_WhiteElo:
        return setUintResult (ti, g->GetWhiteElo());

    case T_BlackElo:
        return setUintResult (ti, g->GetBlackElo());

    case T_WhiteRType:
        return setResult (ti, ratingTypeNames[g->GetWhiteRatingType()]);

    case T_BlackRType:
        return setResult (ti, ratingTypeNames[g->GetBlackRatingType()]);

    case T_ECO:
        {
            ecoStringT ecoStr;
            eco_ToExtendedString (g->GetEco(), ecoStr);
            Tcl_AppendResult (ti, ecoStr, NULL);
            break;
        }

    case T_EDate:
        {
            char dateStr[20];
            date_DecodeToString (g->GetEventDate(), dateStr);
            Tcl_AppendResult (ti, dateStr, NULL);
        }
        break;

    case T_EYear:
        return setUintResult (ti, date_GetYear (g->GetEventDate()));

    case T_EMonth:
        return setUintWidthResult (ti, date_GetMonth (g->GetEventDate()), 2);

    case T_EDay:
        return setUintWidthResult (ti, date_GetDay (g->GetEventDate()), 2);

    case T_Extra:
        {
            uint numTags = g->GetNumExtraTags();
            tagT * ptagList = g->GetExtraTags();
            while (numTags > 0) {
                Tcl_AppendResult (ti, ptagList->tag, " \"", ptagList->value,
                                  "\"\n", NULL);
                numTags--;
                ptagList++;
            }
        }
        break;

    default:  // Not a valid tag name.
        return InvalidCommand (ti, "sc_game tags get", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_tags_set:
//    Set the standard tags for this game.
//    Args are: event, site, date, round, white, black, result,
//              whiteElo, whiteRatingType, blackElo, blackRatingType, Eco,
//              eventdate.
//    Last arg is the non-standard tags, a string of lines in the format:
//        [TagName "TagValue"]
int
sc_game_tags_set (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * options[] = {
        "-event", "-site", "-date", "-round", "-white", "-black", "-result",
        "-whiteElo", "-whiteRatingType", "-blackElo", "-blackRatingType",
        "-eco", "-eventdate", "-extra",
        NULL
    };
    enum {
        T_EVENT, T_SITE, T_DATE, T_ROUND, T_WHITE, T_BLACK, T_RESULT,
        T_WHITE_ELO, T_WHITE_RTYPE, T_BLACK_ELO, T_BLACK_RTYPE,
        T_ECO, T_EVENTDATE, T_EXTRA
    };

    int arg = 3;
    if (((argc-arg) % 2) != 0) {
        return errorResult (ti, "Odd number of parameters.");
    }

    // Process each pair of parameters:
    while (arg+1 < argc) {
        int index = strUniqueMatch (argv[arg], options);
        const char * value = argv[arg+1];
        arg += 2;

        switch (index) {
            case T_EVENT: db->game->SetEventStr (value); break;
            case T_SITE: db->game->SetSiteStr (value); break;
            case T_DATE:
                db->game->SetDate (date_EncodeFromString(value));
                break;
            case T_ROUND: db->game->SetRoundStr (value); break;
            case T_WHITE: db->game->SetWhiteStr (value); break;
            case T_BLACK: db->game->SetBlackStr (value); break;
            case T_RESULT: db->game->SetResult (strGetResult(value)); break;
            case T_WHITE_ELO:
                db->game->SetWhiteElo (strGetUnsigned(value)); break;
            case T_WHITE_RTYPE:
                db->game->SetWhiteRatingType (strGetRatingType (value)); break;
            case T_BLACK_ELO:
                db->game->SetBlackElo (strGetUnsigned(value)); break;
            case T_BLACK_RTYPE:
                db->game->SetBlackRatingType (strGetRatingType (value)); break;
            case T_ECO:
                db->game->SetEco (eco_FromString (value)); break;
            case T_EVENTDATE:
                db->game->SetEventDate (date_EncodeFromString(value));
                break;
            case T_EXTRA:
                {
                    // Add all the nonstandard tags:
                    db->game->ClearExtraTags ();
                    int largc;
                    const char ** largv;
                    if (Tcl_SplitList (ti, value, &largc,
                                       (CONST84 char ***) &largv) != TCL_OK) {
                        // Error from Tcl_SplitList!
                        return errorResult (ti, "Error parsing extra tags.");
                    }

                    // Extract each tag-value pair and add it to the game:
                    for (int i=0; i < largc; i++) {
                        char tagStr [1024];
                        char valueStr [1024];
                        //if ( sscanf (largv[i], "%s", tagStr ) == 1 &&
                        //	sscanf (largv[i+1], "%s", valueStr) == 1) {
                        // Usage :: sc_game tags set -extra [ list "Annotator \"boob [sc_pos moveNumber]\"\n" ]
                        if (sscanf (largv[i], "%s \"%[^\"]\"\n", tagStr, valueStr) == 2) {
                            valueStr[255] = 0;
                            db->game->AddPgnTag (tagStr, valueStr);
                        } else {
                            // Invalid line in the list; just ignore it.
                        }
                    }
                    Tcl_Free ((char *) largv);
                }
                break;
            default:
                return InvalidCommand (ti, "sc_game tags set", options);
        }
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_tags_reload:
//    Reloads the tags (White, Black, Event,Site, etc) for a game.
//    Useful when a name that may occur in the current game has been
//    edited.
int
sc_game_tags_reload (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (!db->inUse  ||   db->gameNumber < 0) { return TCL_OK; }
    IndexEntry * ie = db->idx->FetchEntry (db->gameNumber);
    db->game->LoadStandardTags (ie, db->nb);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_game_tags_share:
//    Shares tags between two games, updating one where the other
//    has more complete or better information.
//
//    This is mainly useful for combining the header information
//    of a pair of twins before deleting one of them. For example,
//    one may have a less complete date while the other may have
//    no ratings or an unknown ("?") round value.
//
//    If the subcommand parameter is "check", a list is returned
//    with a multiple of four elements, each set of four indicating
//    a game number, the tag that will be changed, the old value,
//    and the new value. If the parameter is "update", the changes
//    will be made and the empty string is returned.
int
sc_game_tags_share (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage =
        "Usage: sc_game tags share [check|update] <gameNumber1> <gameNumber2>";
    if (argc != 6) { return errorResult (ti, usage); }
    bool updateMode = false;
    if (strIsPrefix (argv[3], "check")) {
        updateMode = false;
    } else if (strIsPrefix (argv[3], "update")) {
        updateMode = true;
    } else {
        return errorResult (ti, usage);
    }
    // Get the two game numbers, which should be different and non-zero.
    uint gn1 = strGetUnsigned (argv[4]);
    uint gn2 = strGetUnsigned (argv[5]);
    if (gn1 == 0) { return TCL_OK; }
    if (gn2 == 0) { return TCL_OK; }
    if (gn1 == gn2) { return TCL_OK; }
    if (gn1 > db->numGames) { return TCL_OK; }
    if (gn2 > db->numGames) { return TCL_OK; }

    // Do nothing if the base is not writable:
    if (!db->inUse  ||  db->fileMode == FMODE_ReadOnly) { return TCL_OK; }

    // Make a local copy of each index entry:
    IndexEntry ie1 = *(db->idx->FetchEntry (gn1 - 1));
    IndexEntry ie2 = *(db->idx->FetchEntry (gn2 - 1));
    bool updated1 = false;
    bool updated2 = false;

    // Share dates if appropriate:
    char dateStr1 [16];
    char dateStr2 [16];
    dateT date1 = ie1.GetDate();
    dateT date2 = ie2.GetDate();
    date_DecodeToString (date1, dateStr1);
    date_DecodeToString (date2, dateStr2);
    strTrimDate (dateStr1);
    strTrimDate (dateStr2);
    if (date1 == 0) { *dateStr1 = 0; }
    if (date2 == 0) { *dateStr2 = 0; }
    // Check if one date is a prefix of the other:
    if (!strEqual (dateStr1, dateStr2)  &&  strIsPrefix (dateStr1, dateStr2)) {
        // Copy date grom game 2 to game 1:
        if (updateMode) {
            ie1.SetDate (date2);
            updated1 = true;
        } else {
            appendUintElement (ti, gn1);
            Tcl_AppendElement (ti, "Date");
            Tcl_AppendElement (ti, dateStr1);
            Tcl_AppendElement (ti, dateStr2);
        }
    }
    if (!strEqual (dateStr1, dateStr2)  &&  strIsPrefix (dateStr2, dateStr1)) {
        // Copy date grom game 1 to game 2:
        if (updateMode) {
            ie2.SetDate (date1);
            updated2 = true;
        } else {
            appendUintElement (ti, gn2);
            Tcl_AppendElement (ti, "Date");
            Tcl_AppendElement (ti, dateStr2);
            Tcl_AppendElement (ti, dateStr1);
        }
    }

    // Check if an event name can be updated:
    idNumberT event1 = ie1.GetEvent();
    idNumberT event2 = ie2.GetEvent();
    const char * eventStr1 = ie1.GetEventName (db->nb);
    const char * eventStr2 = ie2.GetEventName (db->nb);
    bool event1empty = strEqual (eventStr1, "")  ||  strEqual (eventStr1, "?");
    bool event2empty = strEqual (eventStr2, "")  ||  strEqual (eventStr2, "?");
    if (event1empty  && !event2empty) {
        // Copy event from event 2 to game 1:
        if (updateMode) {
            ie1.SetEvent (event2);
            updated1 = true;
        } else {
            appendUintElement (ti, gn1);
            Tcl_AppendElement (ti, "Event");
            Tcl_AppendElement (ti, eventStr1);
            Tcl_AppendElement (ti, eventStr2);
        }
    }
    if (event2empty  && !event1empty) {
        // Copy event from game 1 to game 2:
        if (updateMode) {
            ie2.SetEvent (event1);
            updated2 = true;
        } else {
            appendUintElement (ti, gn2);
            Tcl_AppendElement (ti, "Event");
            Tcl_AppendElement (ti, eventStr2);
            Tcl_AppendElement (ti, eventStr1);
        }
    }

    // Check if a round name can be updated:
    idNumberT round1 = ie1.GetRound();
    idNumberT round2 = ie2.GetRound();
    const char * roundStr1 = ie1.GetRoundName (db->nb);
    const char * roundStr2 = ie2.GetRoundName (db->nb);
    bool round1empty = strEqual (roundStr1, "")  ||  strEqual (roundStr1, "?");
    bool round2empty = strEqual (roundStr2, "")  ||  strEqual (roundStr2, "?");
    if (round1empty  && !round2empty) {
        // Copy round from game 2 to game 1:
        if (updateMode) {
            ie1.SetRound (round2);
            updated1 = true;
        } else {
            appendUintElement (ti, gn1);
            Tcl_AppendElement (ti, "Round");
            Tcl_AppendElement (ti, roundStr1);
            Tcl_AppendElement (ti, roundStr2);
        }
    }
    if (round2empty  && !round1empty) {
        // Copy round from game 1 to game 2:
        if (updateMode) {
            ie2.SetRound (round1);
            updated2 = true;
        } else {
            appendUintElement (ti, gn2);
            Tcl_AppendElement (ti, "Round");
            Tcl_AppendElement (ti, roundStr2);
            Tcl_AppendElement (ti, roundStr1);
        }
    }

    // Check if Elo ratings can be shared:
    eloT welo1 = ie1.GetWhiteElo();
    eloT belo1 = ie1.GetBlackElo();
    eloT welo2 = ie2.GetWhiteElo();
    eloT belo2 = ie2.GetBlackElo();
    if (welo1 == 0  &&  welo2 != 0) {
        // Copy White rating from game 2 to game 1:
        if (updateMode) {
            ie1.SetWhiteElo (welo2);
            updated1 = true;
        } else {
            appendUintElement (ti, gn1);
            Tcl_AppendElement (ti, "WhiteElo");
            appendUintElement (ti, welo1);
            appendUintElement (ti, welo2);
        }
    }
    if (welo2 == 0  &&  welo1 != 0) {
        // Copy White rating from game 1 to game 2:
        if (updateMode) {
            ie2.SetWhiteElo (welo1);
            updated2 = true;
        } else {
            appendUintElement (ti, gn2);
            Tcl_AppendElement (ti, "WhiteElo");
            appendUintElement (ti, welo2);
            appendUintElement (ti, welo1);
        }
    }
    if (belo1 == 0  &&  belo2 != 0) {
        // Copy Black rating from game 2 to game 1:
        if (updateMode) {
            ie1.SetBlackElo (belo2);
            updated1 = true;
        } else {
            appendUintElement (ti, gn1);
            Tcl_AppendElement (ti, "BlackElo");
            appendUintElement (ti, belo1);
            appendUintElement (ti, belo2);
        }
    }
    if (belo2 == 0  &&  belo1 != 0) {
        // Copy Black rating from game 1 to game 2:
        if (updateMode) {
            ie2.SetBlackElo (belo1);
            updated2 = true;
        } else {
            appendUintElement (ti, gn2);
            Tcl_AppendElement (ti, "BlackElo");
            appendUintElement (ti, belo2);
            appendUintElement (ti, belo1);
        }
    }

    // Write changes to the index file:
    if (updateMode) {
        if (updated1) {
            db->idx->WriteEntries (&ie1, gn1 - 1, 1);
        }
        if (updated2) {
            db->idx->WriteEntries (&ie2, gn2 - 1, 1);
        }
        if (updated1  ||  updated2) {
            db->idx->WriteHeader();
        }
    }
    return TCL_OK;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//    resets data used for undos (for example after loading another game)

void sc_game_undo_reset(scidBaseT * base) {
  base->undoMax = -1;
  base->undoIndex = -1;
  base->undoCurrent = -1;
  base->undoCurrentNotAvail = false;
  for (int i = 0 ; i < UNDO_MAX ; i++) {
    if (base->undoGame[i] != NULL) {
      delete base->undoGame[i];
      base->undoGame[i] = NULL;
    }
  }
}

//////////////////////////////////////////////////////////////////////
///  INFO functions

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_info:
//    General Scid Information commands.
int
sc_info (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "asserts", "clipbase", "decimal", "fsize", "gzip",
        "html", "limit", "priority", "ratings",
        "suffix", "tb", "validDate", "version", "language", NULL
    };
    enum {
        INFO_ASSERTS, INFO_CLIPBASE, INFO_DECIMAL, INFO_FSIZE, INFO_GZIP,
        INFO_HTML, INFO_LIMIT, INFO_PRIORITY, INFO_RATINGS,
        INFO_SUFFIX, INFO_TB, INFO_VALIDDATE, INFO_VERSION, INFO_LANGUAGE
    };
    int index = -1;

    if (argc > 1) { index = strUniqueMatch (argv[1], options); }

    switch (index) {
    case INFO_ASSERTS:
        return setUintResult (ti, numAsserts);

    case INFO_CLIPBASE:
        return setUintResult (ti, CLIPBASE_NUM + 1);

    case INFO_DECIMAL:
        if (argc >= 3) {
            decimalPointChar = argv[2][0];
        } else {
            return setCharResult (ti, decimalPointChar);
        }
        break;

    case INFO_FSIZE:
        return sc_info_fsize (cd, ti, argc, argv);

    case INFO_GZIP:
        // Return true if gzip files can be decoded by Scid.
#ifdef NO_ZLIB
        return setBoolResult (ti, false);
#else
        return setBoolResult (ti, true);
#endif
        break;

    case INFO_HTML:
        if (argc >= 3) {
            htmlDiagStyle = strGetUnsigned (argv[2]);
        } else {
            return setUintResult (ti, htmlDiagStyle);
        }
        break;

    case INFO_LIMIT:
        return sc_info_limit (cd, ti, argc, argv);

    case INFO_PRIORITY:
        return sc_info_priority (cd, ti, argc, argv);

    case INFO_RATINGS:   // List of all recognised rating types.
        {
            uint i = 0;
            while (ratingTypeNames[i] != NULL) {
                Tcl_AppendElement (ti, (char *) ratingTypeNames[i]);
                i++;
            }
        }
        break;

    case INFO_SUFFIX:
        return sc_info_suffix (cd, ti, argc, argv);

    case INFO_TB:
        return sc_info_tb (cd, ti, argc, argv);

    case INFO_VALIDDATE:
        if (argc != 3) {
            return errorResult (ti, "Usage: sc_info validDate <datestring>");
        }
        setBoolResult (ti, date_ValidString (argv[2]));
        break;

    case INFO_VERSION:
        if (argc >= 3  &&  strIsPrefix (argv[2], "date")) {
            setResult (ti, SCID_VERSION_DATE);
        } else {
            setResult (ti, SCID_VERSION_STRING);
        }
        break;
    case INFO_LANGUAGE:
      if (argc != 3) {
        return errorResult (ti, "Usage: sc_info language <lang>");
      }
      if ( strcmp(argv[2], "en") == 0) {language = 0;}
      if ( strcmp(argv[2], "fr") == 0) {language = 1;}
      if ( strcmp(argv[2], "es") == 0) {language = 2;}
      if ( strcmp(argv[2], "de") == 0) {language = 3;}
      if ( strcmp(argv[2], "it") == 0) {language = 4;}
      if ( strcmp(argv[2], "ne") == 0) {language = 5;}
      if ( strcmp(argv[2], "cz") == 0) {language = 6;}
      if ( strcmp(argv[2], "hu") == 0) {language = 7;}
      if ( strcmp(argv[2], "no") == 0) {language = 8;}
      if ( strcmp(argv[2], "sw") == 0) {language = 9;}
      if ( strcmp(argv[2], "gr") == 0) {language = 10;}
      if ( strcmp(argv[2], "tr") == 0) {language = 11;}
      if ( strcmp(argv[2], "po") == 0) {language = 12;}

    break;
    default:
        return InvalidCommand (ti, "sc_info", options);
    };

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_info_fsize:
//    Given the name of a .si3, .si, .pgn or .pgn.gz file, this command
//    returns the number of games in that file. For large PGN files,
//    the value returned is only an estimate.
//    To distinguish estimates from correct sizes, an estimate is
//    returned as a negative number.
int
sc_info_fsize (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_info fsize <filename>");
    }
    const char * fname = argv[2];
    const char * lastSuffix = strFileSuffix (fname);
    uint fsize = 0;
    bool isGzipFile = false;
    bool isEpdFile = false;
    bool isRepFile = false;

    if (strAlphaContains (fname, ".epd")) { isEpdFile =  true; }
    if (strAlphaContains (fname, ".sor")) { isRepFile =  true; }
    if (lastSuffix != NULL  &&  strEqual (lastSuffix, GZIP_SUFFIX)) {
        isGzipFile = true;
    }

    if (lastSuffix != NULL  &&  strEqual (lastSuffix, OLD_INDEX_SUFFIX)) {
        fsize = rawFileSize (fname);
        fsize -= OLD_INDEX_HEADER_SIZE;
        fsize = fsize / OLD_INDEX_ENTRY_SIZE;
        return setUintResult (ti, fsize);
    }
    if (lastSuffix != NULL  &&  strEqual (lastSuffix, INDEX_SUFFIX)) {
        fsize = rawFileSize (fname);
        fsize -= INDEX_HEADER_SIZE;
        fsize = fsize / INDEX_ENTRY_SIZE;
        return setUintResult (ti, fsize);
    }

    // Estimate size for PGN files, by reading the first 64 kb
    // of the file and counting the number of games seen:

    if (isGzipFile) {
        fsize = gzipFileSize (fname);
    } else {
        fsize = rawFileSize (fname);
    }

    MFile pgnFile;
    if (pgnFile.Open (fname, FMODE_ReadOnly) != OK) {
        return errorResult (ti, "Error opening file");
    }

    const uint maxBytes = 65536;
    char * buffer =  new char [maxBytes];
    uint bytes = maxBytes - 1;
    if (bytes > fsize) { bytes = fsize; }
    if (pgnFile.ReadNBytes (buffer, bytes) != OK) {
        pgnFile.Close();
        delete[] buffer;
        return errorResult (ti, "Error reading file");
    }
    pgnFile.Close();

    buffer [bytes] = 0;
    const char * s = buffer;
    int ngames = 0;

    for (uint i=0; i < bytes; i++) {
        if (isEpdFile) {
            // EPD file: count positions, one per line.
            if (*s == '\n') { ngames++; }
        } else if (isRepFile) {
            // Repertoire file: count include (+) and exclude (-) lines.
            if (*s == ' '  ||  *s == '\n') {
                if (s[1] == '+'  &&  s[2] == ' ') { ngames++; }
                if (s[1] == '-'  &&  s[2] == ' ') { ngames++; }
            }
        } else {
            // PGN file: count Result tags.
            if (*s == '['  &&  strIsPrefix ("Result ", s+1)) { ngames++; }
        }
        s++;
    }

    // If the file is larger than maxBytes, this was only a sample
    // so return an estimate to the nearest 10 or 100 or 1000 games:
    if (fsize > bytes) {
        ngames = (uint) ((double)ngames * (double)fsize / (double)bytes);
        if (ngames > 10000) {
            ngames = ((ngames + 500) / 1000) * 1000;
        } else if (ngames > 1000) {
            ngames = ((ngames + 50) / 100) * 100;
        } else {
            ngames = ((ngames + 5) / 10) * 10;
        }
        ngames = -ngames;
    }
    delete[] buffer;
    return setIntResult (ti, ngames);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_info limit:
//    Limits that Scid imposes.
int
sc_info_limit (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "clipbase", "elo",       "epd",    "events",
        "games",    "nags",      "players", "rounds",
        "sites",    "treeCache", "year",    NULL
    };
    enum {
        LIM_CLIPBASE, LIM_ELO,       LIM_EPD,     LIM_EVENTS,
        LIM_GAMES,    LIM_NAGS,      LIM_PLAYERS, LIM_ROUNDS,
        LIM_SITES,    LIM_TREECACHE, LIM_YEAR
    };
    int index = -1;
    int result = 0;

    if (argc == 3) { index = strUniqueMatch (argv[2], options); }

    switch (index) {
    case LIM_CLIPBASE:
        result = clipbaseMaxGames;
        break;

    case LIM_ELO:
        result = MAX_ELO;
        break;

    case LIM_EPD:
        result = MAX_EPD;
        break;

    case LIM_EVENTS:
        result = NAME_MAX_ID [NAME_EVENT];
        break;

    case LIM_GAMES:
        result = MAX_GAMES;
        break;

    case LIM_NAGS:
        result = MAX_NAGS;
        break;

    case LIM_PLAYERS:
        result = NAME_MAX_ID [NAME_PLAYER];
        break;

    case LIM_ROUNDS:
        result = NAME_MAX_ID [NAME_ROUND];
        break;

    case LIM_SITES:
        result = NAME_MAX_ID [NAME_SITE];
        break;

    case LIM_TREECACHE:
        result = SCID_TreeCacheSize;
        break;

    case LIM_YEAR:
        result = YEAR_MAX;
        break;

    default:
        return InvalidCommand (ti, "sc_info limit", options);
    }

    return setIntResult (ti, result);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_info_priority:
//   This gets or sets the priority class of a process.
int
sc_info_priority (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
#ifdef _WIN32
    const char * usage = "Usage: sc_info priority <pid> [normal|idle]";
    if (argc < 3  ||  argc > 4) { return errorResult (ti, usage); }

    int pid = strGetInteger(argv[2]);

    if (argc == 4) {
        // For saftey, only normal and idle classes can be set:
        bool idlePriority = false;
        switch (argv[3][0]) {
            case 'i': idlePriority = true;  break;
            case 'n': idlePriority = false; break;
            default: return errorResult (ti, usage);
        }

        // Try to obtain a process handle for setting the priority class:
        HANDLE hProcess = OpenProcess (PROCESS_SET_INFORMATION, false, pid);
        if (hProcess == NULL) {
            return errorResult (ti, "Unable to set process priority.");
        }

        // Set the process class to NORMAL or IDLE:
        SetPriorityClass (hProcess,
                idlePriority ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
        CloseHandle (hProcess);
    }
    // Now return the process priority:
    HANDLE hProcess = OpenProcess (PROCESS_QUERY_INFORMATION, false, pid);
    if (hProcess == NULL) {
        return errorResult (ti, "Unable to get process priority.");
    }
    uint priorityClass = GetPriorityClass (hProcess);
    CloseHandle (hProcess);

    // Convert the priority class number to a name:
    const char * priorityName = "";
    switch (priorityClass) {
        case NORMAL_PRIORITY_CLASS: priorityName = "normal"; break;
        case IDLE_PRIORITY_CLASS:   priorityName = "idle";   break;
        case HIGH_PRIORITY_CLASS:   priorityName = "high";   break;
        default:  priorityName = "unknown";
    }
    Tcl_AppendResult (ti, priorityName, NULL);

#else  // #ifdef _WIN32

    const char * usage = "Usage: sc_info priority <pid> [<priority>]";
    if (argc < 3  ||  argc > 4) { return errorResult (ti, usage); }

    int pid = strGetInteger(argv[2]);

    if (argc == 4) {
        // Try to assign a new priority:
        int newpriority = strGetInteger(argv[3]);
        if (setpriority (PRIO_PROCESS, pid, newpriority) != 0) {
            return errorResult (ti, "Unable to set process priority.");
        }
    }
    // Now return the process priority:
    int priority = getpriority (PRIO_PROCESS, pid);
    appendIntResult (ti, priority);
#endif  // #ifdef _WIN32
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_info suffix:
//    Returns a Scid file suffix for a database file type.
//    The suffix is returned with the leading dot.
int
sc_info_suffix (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "game", "index", "name", "treecache", NULL
    };
    enum {
        SUFFIX_OPT_GAME, SUFFIX_OPT_INDEX, SUFFIX_OPT_NAME, SUFFIX_OPT_TREE
    };
    int index = -1;

    if (argc == 3) { index = strUniqueMatch (argv[2], options); }

    const char * suffix = "";

    switch (index) {
        case SUFFIX_OPT_GAME:  suffix = GFILE_SUFFIX;    break;
        case SUFFIX_OPT_INDEX: suffix = INDEX_SUFFIX;    break;
        case SUFFIX_OPT_NAME:  suffix = NAMEBASE_SUFFIX; break;
        case SUFFIX_OPT_TREE:  suffix = TREEFILE_SUFFIX; break;
        default: return InvalidCommand (ti, "sc_info limit", options);
    }

    return setResult (ti, suffix);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_info_tb:
//   Set up a tablebase directory, or check if a certain
//   tablebase is available.
int
sc_info_tb (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage =
        "Usage: sc_info tb [<directory>|available <material>|cache <size-kb>]";

    if (argc == 2) {
        // Command: sc_info tb
        // Returns whether tablebase support is complied.
        return setBoolResult(ti, scid_TB_compiled());

    } else if (argc == 3) {
        // Command: sc_info_tb <directories>
        // Clears tablebases and registers all tablebases in the
        // specified directories string, which can have more than
        // one directory separated by commas or semicolons.
        return setUintResult (ti, scid_TB_Init (argv[2]));

    } else if (argc == 4  &&  argv[2][0] == 'a') {
        // Command: sc_probe available <material>
        // The material is specified as "KRKN", "kr-kn", etc.
        // Set up the required material:
        matSigT ms = MATSIG_Empty;
        const char * material = argv[3];
        if (toupper(*material) != 'K') { return setBoolResult (ti, false); }
        material++;
        colorT side = WHITE;
        while (1) {
            char ch = toupper(*material);
            material++;
            if (ch == 0) { break; }
            if (ch == 'K') { side = BLACK; continue; }
            pieceT p = piece_Make (side, piece_FromChar (ch));
            if (ch == 'P') { p = piece_Make (side, PAWN); }
            if (p == EMPTY) { continue; }
            ms = matsig_setCount (ms, p, matsig_getCount (ms, p) + 1);
        }
        // Check if a tablebase for this material is available:
        return setBoolResult (ti, scid_TB_Available (ms));
    } else if (argc == 4  &&  argv[2][0] == 'c') {
        // Set the preferred tablebase cache size, to take effect
        // at the next tablebase initialisation.
        uint cachesize = strGetUnsigned (argv[3]);
        scid_TB_SetCacheSize (cachesize * 1024);
        return TCL_OK;
    } else {
        return errorResult (ti, usage);
    }
}

//////////////////////////////////////////////////////////////////////
//  MOVE functions

int
sc_move (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "add", "addSan", "addUCI", "back", "end", "endVar", "startVar", "forward",
        "pgn", "ply", "start", NULL
    };
    enum {
        MOVE_ADD, MOVE_ADDSAN, MOVE_ADDUCI, MOVE_BACK, MOVE_END, MOVE_END_VAR, MOVE_START_VAR, MOVE_FORWARD,
        MOVE_PGN, MOVE_PLY, MOVE_START
    };
    int index = -1;

    if (argc > 1) { index = strUniqueMatch (argv[1], options); }

    switch (index) {
    case MOVE_ADD:
        return sc_move_add (cd, ti, argc, argv);

    case MOVE_ADDSAN:
        return sc_move_addSan (cd, ti, argc, argv);

    case MOVE_ADDUCI:
        return sc_move_addUCI (cd, ti, argc, argv);

    case MOVE_BACK:
        return sc_move_back (cd, ti, argc, argv);

    case MOVE_END:
        db->game->MoveToPly(0);
        {
            errorT err = OK;
            do {
                err = db->game->MoveForward();
            } while (err == OK);
        }
        break;

    case MOVE_END_VAR:
        {
            errorT err = OK;
            do {
                err = db->game->MoveForward();
            } while (err == OK);
        }
        break;

    case MOVE_START_VAR:
        {
            errorT err = OK;
            do {
                err = db->game->MoveBackup();
            } while (err == OK);
        }
        break;

    case MOVE_FORWARD:
        return sc_move_forward (cd, ti, argc, argv);

    case MOVE_PGN:
        return sc_move_pgn (cd, ti, argc, argv);

    case MOVE_PLY:
        if (argc != 3) {
            return errorResult (ti, "Usage: sc_move ply <plynumber>");
        }
        {
            uint ply = strGetUnsigned (argv[2]);
            db->game->MoveToPly (ply);
        }
        break;

    case MOVE_START:
        db->game->MoveToPly (0);
        break;

    default:
        return InvalidCommand (ti, "sc_move", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_move_add: takes a move specified by three parameters
//      (square square promo) and adds it to the game.
int
sc_move_add (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{

    if (argc != 5) {
        return errorResult (ti, "Usage: sc_move add <sq> <sq> <promo>");
    }

    uint sq1 = strGetUnsigned (argv[2]);
    uint sq2 = strGetUnsigned (argv[3]);
    uint promo = strGetUnsigned (argv[4]);
    if (promo == 0) { promo = EMPTY; }

    char s[8];
    s[0] = square_FyleChar (sq1);
    s[1] = square_RankChar (sq1);
    s[2] = square_FyleChar (sq2);
    s[3] = square_RankChar (sq2);
    if (promo == EMPTY) {
        s[4] = 0;
    } else {
        s[4] = piece_Char(promo);
        s[5] = 0;
    }
    simpleMoveT sm;
    Position * pos = db->game->GetCurrentPos();
    errorT err = pos->ReadCoordMove (&sm, s, true);
    if (err == OK) {
        err = db->game->AddMove (&sm, NULL);
        if (err == OK) {
            db->gameAltered = true;
            return TCL_OK;
        }
    }
    return errorResult (ti, "Error adding move.");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_move_addSan:
//    Takes moves in regular SAN (e.g. "e4" or "Nbd2") and adds them
//    to the game. The moves can be in one large string, separate
//    list elements, or a mixture of both. Move numbers are ignored
//    but variations/comments/annotations are parsed and added.
int
sc_move_addSan (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char ** argPtr = &(argv[2]);
    int argsLeft = argc - 2;

    if (argc < 3) { return TCL_OK; }

    PgnParser parser;
    char buf [1000];
    while (argsLeft > 0) {
        parser.Reset (*argPtr);
        parser.SetEndOfInputWarnings (false);
        parser.SetResultWarnings (false);
        errorT err = parser.ParseMoves (db->game, buf, 1000);
        if (err != OK  ||  parser.ErrorCount() > 0) {
            Tcl_AppendResult (ti, "Error reading move(s): ", *argPtr, NULL);
            return TCL_ERROR;
        }
        db->gameAltered = true;
        argPtr++;
        argsLeft--;
    }

    // If we reach here, all moves were successfully added:
    return TCL_OK;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_move_addUCI:
//    Takes moves in engine UCI format (e.g. "g1f3") and adds them
//    to the game. The result is no-longer translated.
//    In case of an error, return the moves that could be converted.
int
sc_move_addUCI (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    char s[8], tmp[10];
    if (argc < 3) { return TCL_OK; }
    char * ptr = (char *) argv[2];

    while (*ptr != 0) {

      // get rid of leading piece
      switch (*ptr) {
        case 'K' :
        case 'Q' :
        case 'R' :
        case 'B' :
        case 'N' : ptr++;
      }

      s[0] = ptr[0];
      s[1] = ptr[1];
      s[2] = ptr[2];
      s[3] = ptr[3];
      if (ptr[4] == ' ') {
        s[4] = 0;
        ptr += 5;
      } else if (ptr[4] == 0) {
        s[4] = 0;
        ptr += 4;
      } else if (ptr[5] == ' ') {
        s[4] = ptr[4];
        s[5] = 0;
        ptr += 6;
      } else { //ptr[5] == 0
        s[4] = ptr[4];
        s[5] = 0;
        ptr += 5;
      }
      simpleMoveT sm;
      Position * pos = db->game->GetCurrentPos();

      if (pos->ReadCoordMove (&sm, s, true) != OK)
        return TCL_ERROR;

      if (db->game->AddMove (&sm, NULL) != OK)
        return TCL_ERROR;

      db->gameAltered = true;
      db->game->GetPrevSAN (tmp);
      // transPieces(tmp);
      Tcl_AppendResult (ti, tmp, " ", NULL);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_move_back:
//    Moves back a specified number of moves (default = 1 move).
int
sc_move_back (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    int numMovesTakenBack = 0;
    int count = 1;
    if (argc > 2) {
        count = strGetInteger (argv[2]);
        // if (count < 1) { count = 1; }
    }

    for (int i = 0; i < count; i++) {
        if (db->game->MoveBackup() != OK) { break; }
        numMovesTakenBack++;
    }

    setUintResult (ti, numMovesTakenBack);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_move_forward:
//    Moves forward a specified number of moves (default = 1 move).
int
sc_move_forward (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    int numMovesMade = 0;
    int count = 1;
    if (argc > 2) {
        count = strGetInteger (argv[2]);
        // Do we want to allow moving forward 0 moves? Yes, I think so.
        // if (count < 1) { count = 1; }
    }

    for (int i = 0; i < count; i++) {
        if (db->game->MoveForward() != OK) { break; }
        numMovesMade++;
    }

    setUintResult (ti, numMovesMade);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_move_pgn:
//    Set the current board to the position closest to
//    the specified place in the PGN output (given as a byte count
//    from the start of the output).
int
sc_move_pgn (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_move pgn <offset>");
    }
    db->tbuf->Empty();
    db->tbuf->SetWrapColumn (99999);

    uint offset = strGetUnsigned (argv[2]);
    db->game->MoveToLocationInPGN (db->tbuf, offset);
    return TCL_OK;
}



//////////////////////////////////////////////////////////////////////
//  POSITION functions

int
sc_pos (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "addNag", "analyze", "bestSquare", "board", "clearNags",
        "fen", "getComment", "getCleanComment", "getNags", "hash", "html",
        "isAt", "isCheck", "isInsufficient", "isLegal", "isPromotion",
        "matchMoves", "moveNumber", "pgnBoard", "pgnOffset", "pieceCount",
        "probe", "setComment", "side", "tex", "moves", "movesUci", "location", NULL
    };
    enum {
        POS_ADDNAG, POS_ANALYZE, POS_BESTSQ, POS_BOARD, POS_CLEARNAGS,
        POS_FEN, POS_GETCOMMENT, POST_CLEANCOMMENT, POS_GETNAGS, POS_HASH, POS_HTML,
        POS_ISAT, POS_ISCHECK, POS_ISINSUFFICENT, POS_ISLEGAL, POS_ISPROMO, POS_MATCHMOVES, POS_MOVENUM,
        POS_PGNBOARD, POS_PGNOFFSET, POS_PIECECOUNT, POS_PROBE,
        POS_SETCOMMENT, POS_SIDE, POS_TEX, POS_MOVES, POS_MOVES_UCI, LOCATION
    };

    char boardStr[200];
    int index = -1;
    if (argc > 1) { index = strUniqueMatch (argv[1], options); }


    const char * tempStr;

    switch (index) {
    case POS_ADDNAG:
        return sc_pos_addNag (cd, ti, argc, argv);

    case POS_ANALYZE:
        return sc_pos_analyze (cd, ti, argc, argv);

    case POS_BESTSQ:
        return sc_pos_bestSquare (cd, ti, argc, argv);

    case POS_BOARD:
        db->game->GetCurrentPos()->MakeLongStr (boardStr);
        Tcl_AppendResult (ti, boardStr, NULL);
        break;

    case POS_CLEARNAGS:
        db->game->ClearNags();
        db->gameAltered = true;
        break;

    case POS_FEN:
        db->game->GetCurrentPos()->PrintFEN (boardStr, FEN_ALL_FIELDS);
        Tcl_AppendResult (ti, boardStr, NULL);
        break;

    case POS_GETCOMMENT:
        tempStr = db->game->GetMoveComment();
        if (tempStr) {
            Tcl_AppendResult (ti, tempStr, NULL);
        }
        break;

    case POST_CLEANCOMMENT:
        tempStr = db->game->GetMoveComment();
        if (tempStr) {
	    char * tempStrClean;
	    tempStrClean = strDuplicate(tempStr);
	    strTrimMarkCodes (tempStrClean);
	    if (tempStrClean && !strIsScore((const char *)tempStrClean)) {
		Tcl_AppendResult (ti, tempStrClean, NULL);
	    }
            delete[] tempStrClean;
        }
        break;

    case POS_GETNAGS:
        return sc_pos_getNags (cd, ti, argc, argv);

    case POS_HASH:
        return sc_pos_hash (cd, ti, argc, argv);

    case POS_HTML:
        return sc_pos_html (cd, ti, argc, argv);

    case POS_ISAT:
        return sc_pos_isAt (cd, ti, argc, argv);

    case POS_ISCHECK:
        return setBoolResult (ti, db->game->GetCurrentPos()->IsKingInCheck());

    case POS_ISINSUFFICENT:
        return sc_pos_isInsufficient (cd, ti, argc, argv);

    case POS_ISLEGAL:
        return sc_pos_isLegal (cd, ti, argc, argv);

    case POS_ISPROMO:
        return sc_pos_isPromo (cd, ti, argc, argv);

    case POS_MATCHMOVES:
        return sc_pos_matchMoves (cd, ti, argc, argv);

    case POS_MOVES:
        return sc_pos_moves (cd, ti, argc, argv);

    case POS_MOVES_UCI:
        return sc_pos_moves_uci (cd, ti, argc, argv);

    case POS_MOVENUM:
        // This used to return:
        //     (db->game->GetCurrentPly() + 2) / 2
        // but that value is wrong for games with non-standard
        // start positions. The correct value to return is:
        //     db->game->GetCurrentPos()->GetFullMoveCount()
        return setUintResult (ti, db->game->GetCurrentPos()->GetFullMoveCount());

    case POS_PGNBOARD:
        return sc_pos_pgnBoard (cd, ti, argc, argv);

    case POS_PGNOFFSET:
        // Optional parameter "next" indicates the next moves offset should
        // be returned, instead of the offset of the move just played:
        if (argc > 2  &&  strIsCasePrefix (argv[2], "next")) {
            setUintResult (ti, db->game->GetPgnOffset(1));
        } else {
            setUintResult (ti, db->game->GetPgnOffset(0));
        }
        break;

    case POS_PIECECOUNT:
        setUintResult (ti, db->game->GetCurrentPos()->TotalMaterial());
        break;

    case POS_PROBE:
        return sc_pos_probe (cd, ti, argc, argv);

    case POS_SETCOMMENT:
        return sc_pos_setComment (cd, ti, argc, argv);

    case POS_SIDE:
        setResult (ti, (db->game->GetCurrentPos()->GetToMove() == WHITE)
                   ? "white" : "black");
        break;

    case POS_TEX:
        {
            bool flip = false;
            if (argc > 2  &&  strIsPrefix (argv[2], "flip")) { flip = true; }
            DString * dstr = new DString;
            db->game->GetCurrentPos()->DumpLatexBoard (dstr, flip);
            Tcl_AppendResult (ti, dstr->Data(), NULL);
            delete dstr;
        }
        break;

    case LOCATION: //TODO: doesn't work for variations (perhaps use sc_pos pgnOffset ??)
        return setUintResult (ti, db->game->GetCurrentPly());

    default:
        return InvalidCommand (ti, "sc_pos", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_addNag:
//    Adds a NAG (annotation symbol) for the current move.
int
sc_pos_addNag (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_pos addNag <nagvalue>");
    }
    const char * nagStr = argv[2];
    byte nag = game_parseNag (nagStr);
    if (nag != 0) {
        db->game->AddNag ((byte) nag);
    }
    db->gameAltered = true;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_analyze:
//    Analyzes the current position for the specified number of
//    milliseconds.
//    Returns a two-element list containing the score in centipawns
//    (from the perspective of the side to move) and the best move.
//    If there are no legal moves, the second element is the empty string.
int
sc_pos_analyze (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_pos analyze [<option> <value> ...]";

    uint searchTime = 1000;   // Default = 1000 milliseconds
    uint hashTableKB = 1024;  // Default: one-megabyte hash table.
    uint pawnTableKB = 32;
    bool postMode = false;
    bool pruning = false;
    uint mindepth = 4; // will not check time until this depth is reached
    uint searchdepth = 0;

    static const char * options [] = {
        "-time", "-hashkb", "-pawnkb", "-post", "-pruning", "-mindepth", "-searchdepth", NULL
    };
    enum {
        OPT_TIME, OPT_HASH, OPT_PAWN, OPT_POST, OPT_PRUNING, OPT_MINDEPTH, OPT_SEARCHDEPTH
    };
    int arg = 2;
    while (arg+1 < argc) {
        const char * option = argv[arg];
        const char * value = argv[arg+1];
        arg += 2;
        int index = strUniqueMatch (option, options);
        switch (index) {
            case OPT_TIME:     searchTime = strGetUnsigned(value);  break;
            case OPT_HASH:     hashTableKB = strGetUnsigned(value); break;
            case OPT_PAWN:     pawnTableKB = strGetUnsigned(value); break;
            case OPT_POST:     postMode = strGetBoolean(value);     break;
            case OPT_PRUNING:  pruning = strGetBoolean(value);      break;
            case OPT_MINDEPTH: mindepth = strGetUnsigned(value);    break;
            case OPT_SEARCHDEPTH: searchdepth = strGetUnsigned(value);    break;
            default:
                return InvalidCommand (ti, "sc_pos analyze", options);
        }
    }
    if (arg != argc) { return errorResult (ti, usage); }

    // Generate all legal moves:
    Position * pos = db->game->GetCurrentPos();
    MoveList mlist;
    pos->GenerateMoves(&mlist);

    // Start the engine:
    Engine * engine = new Engine();
    engine->SetSearchTime (searchTime);
    engine->SetHashTableKilobytes (hashTableKB);
    engine->SetPawnTableKilobytes (pawnTableKB);
    engine->SetMinDepthCheckTime(mindepth);
    if (searchdepth > 0)
      engine->SetSearchDepth(searchdepth);
    engine->SetPosition (pos);
    engine->SetPostMode (postMode);
    engine->SetPruning (pruning);
    int score = engine->Think (&mlist);
    delete engine;
    appendIntResult (ti, score);
    char moveStr[20];
    moveStr[0] = 0;
    if (mlist.Size() > 0) {
        pos->MakeSANString (mlist.Get(0), moveStr, SAN_MATETEST);
    }
    Tcl_AppendElement (ti, moveStr);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_bestSquare:
//    Takes a square and returns the best square that makes a move
//    with the given square. The square can be the from or to part of
//    a move. Used for smart move completion.
//    Returns -1 if no legal moves go to or from the square.
int
sc_pos_bestSquare (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_pos bestSquare <square>");
    }

    Position * pos = db->game->GetCurrentPos();

    // Try to read the square parameter as algebraic ("h8") or numeric (63):
    squareT sq = strGetSquare (argv[2]);
    if (sq == NULL_SQUARE) {
      int sqInt = strGetInteger (argv[2]);
      if (sqInt >= 0  &&  sqInt <= 63) { sq = sqInt; }
    }
    if (sq == NULL_SQUARE) {
        return errorResult (ti, "Usage: sc_pos bestSquare <square>");
    }

    // Generate all legal moves:
    MoveList mlist;
    pos->GenerateMoves(&mlist);

    // Restrict the list of legal moves to contain only those that
    // move to or from the specified square:
    mlist.SelectBySquare (sq);

     // If no matching legal moves, return -1:
    if (mlist.Size() == 0) {
        return setResult (ti, "-1");
    }

    if (mlist.Size() > 1) {
        // We have more than one move to choose from, so first check
        // the ECO openings book (if it is loaded) to see if any move
        // in the list reaches an ECO position. If so, select the move
        // reaching the largest ECO code as the best move. If no ECO
        // position is found, do a small chess engine search to find
        // the best move.

        ecoT bestEco = ECO_None;
        ecoT secondBestEco = ECO_None;
        if (ecoBook != NULL) {
            DString ecoStr;
            for (uint i=0; i < mlist.Size(); i++) {
                ecoT eco = ECO_None;
                pos->DoSimpleMove (mlist.Get(i));
                ecoStr.Clear();
                if (ecoBook->FindOpcode (pos, "eco", &ecoStr) == OK) {
                    eco = eco_FromString (ecoStr.Data());
                }
                pos->UndoSimpleMove (mlist.Get(i));
                if (eco >= bestEco) {
                    secondBestEco = bestEco;
                    bestEco = eco;
                    mlist.MoveToFront (i);
                }
            }
        }

        if (bestEco == ECO_None  ||  bestEco == secondBestEco) {
            // No matching ECO position found, or a tie. So do a short
            // engine search to find the best move; 25 ms (= 1/40 s)
            // is enough to reach a few ply and select reasonable
            // moves but fast enough to seem almost instant. The
            // search promotes the best move to be first in the list.
            Engine * engine = new Engine();
            engine->SetSearchTime (100);    // 25
            engine->SetPosition (pos);
            engine->Think (&mlist);
            delete engine;
        }
    }

    // Now the best move is the first in the list, either because it
    // is the only move, or it reaches the largest ECO code, or because
    // the chess engine search selected it.
    // Find the other square in the best move and return it:

    simpleMoveT * sm = mlist.Get(0);
    ASSERT (sq == sm->from  ||  sq == sm->to);

    if (sm->from == sq) 
      setUintResult (ti, sm->to);
    else
      setUintResult (ti, sm->from);

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_getNags:
//    Get the NAGs for the current move.
int
sc_pos_getNags (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    byte * nag = db->game->GetNags();
    if (nag[0] == 0) {
        return setResult (ti, "0");
    }
    while (*nag) {
        char temp[20];
        game_printNag (*nag, temp, true, PGN_FORMAT_Plain);
        Tcl_AppendResult (ti, temp, " ", NULL);
        nag++;
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_hash:
//   Returns the 32-bit hash value of the current position.
int
sc_pos_hash (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_pos hash [full|pawn]";
    bool pawnHashOnly = false;
    if (argc > 3) { return errorResult (ti, usage); }
    if (argc == 3) {
        switch (argv[2][0]) {
            case 'f': pawnHashOnly = false; break;
            case 'p': pawnHashOnly = true;  break;
            default:  return errorResult (ti, usage);
        }
    }
    Position * pos = db->game->GetCurrentPos();
    uint hash = pos->HashValue();
    if (pawnHashOnly) {
        hash = pos->PawnHashValue();
    }
    return setUintResult (ti, hash);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_html:
//    Returns an HTML table represtentation of the board.
//    There are two styles: 0 (the default), which has
//    40x40 squares and images in a "bitmaps" subdirectory;
//    and style 1 which has 36x35 squares and images in
//    a "bitmaps2" directory.
//    The directory can be overriden with the "-path" command.
int
sc_pos_html (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_pos html [-flip <boolean>] [-path <path>] [<style:0|1>]";
    uint style = htmlDiagStyle;
    bool flip = false;
    int arg = 2;
    const char * path = NULL;

    if (argc > arg+1  && strEqual (argv[arg], "-flip")) {
        flip = strGetBoolean(argv[arg+1]);
        arg += 2;
    }
    if (argc > arg+1  && strEqual (argv[arg], "-path")) {
        path = argv[arg+1];
        arg += 2;
    }
    if (argc < arg ||  argc > arg+1) {
        return errorResult (ti, usage);
    }
    if (argc == arg+1) { style = strGetUnsigned (argv[arg]); }

    DString * dstr = new DString;
    db->game->GetCurrentPos()->DumpHtmlBoard (dstr, style, path, flip);
    Tcl_AppendResult (ti, dstr->Data(), NULL);
    delete dstr;
    return TCL_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_isAt: returns whether the position is at the
//   start or end of a move list, according to the arg value.
//   Valid arguments are: start, end, vstart and vend (or unique
//   abbreviations thereof).
int
sc_pos_isAt (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "start", "end", "vstart", "vend", NULL
    };
    enum {
        OPT_START, OPT_END, OPT_VSTART, OPT_VEND
    };
    int index = -1;
    if (argc == 3) { index = strUniqueMatch(argv[2], options); }

    switch (index) {
    case OPT_START:
        return setBoolResult (ti, db->game->AtStart());

    case OPT_END:
        return setBoolResult (ti, db->game->AtEnd());

    case OPT_VSTART:
        return setBoolResult (ti, db->game->AtVarStart());

    case OPT_VEND:
        return setBoolResult (ti, db->game->AtVarEnd());

    default:
        return errorResult (ti, "Usage: sc_pos isAt start|end|vstart|vend");
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_isPromo:
//    Takes two squares (from and to, in either order) and
//    returns true if they represent a pawn promotion move.
int
sc_pos_isPromo (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 4) {
        return errorResult (ti, "Usage: sc_move isPromo <square> <square>");
    }

    Position * pos = db->game->GetCurrentPos();
    int fromSq = strGetInteger (argv[2]);
    int toSq = strGetInteger (argv[3]);

    if (fromSq < A1  ||  fromSq > H8  ||  toSq < A1  ||  toSq > H8) {
        return errorResult (ti, "Usage: sc_move isPromo <square> <square>");
    }

    setBoolResult (ti, pos->IsPromoMove ((squareT) fromSq, (squareT) toSq));
    return TCL_OK;
}

// Simple insufficient mating material query
int
sc_pos_isInsufficient (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 2)
        return errorResult (ti, "Usage: sc_pos isInsufficient");

    Position * pos = db->game->GetCurrentPos();
    uint numPieces = pos->TotalMaterial();

    // We are ignoring very unlikely mate with odd coloured bishops "8/8/8/8/4b3/7k/8/6BK w - - 1 2"

    if (numPieces < 3 || \
       (numPieces == 3 && ((pos->MaterialValue(WHITE)) == 3 || pos->MaterialValue(BLACK) == 3)) || \
       (numPieces == 4 &&  pos->MaterialValue(WHITE)  == 3 && pos->MaterialValue(BLACK) == 3))
      return setBoolResult (ti, true);
    else
      return setBoolResult (ti, false);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_isLegal: returns true if the move between the two provided
//    squares (either could be the from square) is legal.
int
sc_pos_isLegal (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 4) {
        return errorResult (ti, "Usage: sc_pos isLegal <square> <square>");
    }

    Position * pos = db->game->GetCurrentPos();
    int sq1 = strGetInteger (argv[2]);
    int sq2 = strGetInteger (argv[3]);
    if (sq1 < 0  ||  sq1 > 63  ||  sq2 < 0  ||  sq2 > 63) {
        return setBoolResult (ti, false);
    }

    // Compute all legal moves, then restrict the list to only
    // contain moves that include sq1 and sq2 as to/from squares:
    MoveList mlist;
    pos->GenerateMoves(&mlist);
    mlist.SelectBySquare (sq1);
    mlist.SelectBySquare (sq2);
    bool found = (mlist.Size() > 0);
    return setBoolResult (ti, found);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_matchMoves: Return the list of legal moves matching
//     a specified prefix. Note that any occurence of "x", "=", "+",
//     or "#" is removed from the move text of each move, and the
//     castling moves are "OK" and "OQ" for king and queen-side
//     castling respectively, so that no move is a prefix of
//     any other move. (Bollocks! Castling should be OO - S.A.)
int
sc_pos_matchMoves (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3  &&  argc != 4) {
        return errorResult (ti, "Usage: sc_pos matchMoves <movetext-prefix>");
    }
    bool coordMoves = false;
    const char * prefix = argv[2];
    if (argc == 4) { coordMoves = strGetBoolean (argv[3]); }
    char str[20];
    char str1[20]="";
    char str2[20]="";
    Position * p = db->game->GetCurrentPos();
    sanListT sanList;
    p->CalcSANStrings (&sanList, SAN_NO_CHECKTEST);

    bool kingside_castle=0;
    for (uint i=0; i < sanList.num; i++) {
        strCopyExclude (str, sanList.list[i], "x=+#");
        // O-O comes first in the list
        if (strEqual (str, "O-O")) {
            strCopy (str1, "OO");
            strCopy (str2, "OK");
            kingside_castle=1;
        } else {
           if (strEqual (str, "O-O-O")) {
             if (kingside_castle)
                strCopy (str1, "OQ");
             else {
                strCopy (str1, "OO");
                strCopy (str2, "OQ");
            }
          } else {
            strCopy (str1, str);
          }
        }

        if (*str1 && strIsPrefix (prefix, str1)) {
            Tcl_AppendElement (ti, str1);
            str1[0]=0;
        }
        if (*str2 && strIsPrefix (prefix, str2)) {
            Tcl_AppendElement (ti, str2);
            str2[0]=0;
        }
    }

    // If the prefix string is for b-pawn moves, also look for any
    // Bishop moves that could match, and add them provided they do not
    // clash with a pawn move.
    if (prefix[0] >= 'a'  &&  prefix[0] <= 'h') {
        char * newPrefix = strDuplicate (prefix);
        newPrefix[0] = toupper(newPrefix[0]);
        for (uint i=0; i < sanList.num; i++) {
            strCopyExclude (str, sanList.list[i], "x=+#");
            if (strIsPrefix (newPrefix, str)) {
                Tcl_AppendElement (ti, str);
            }
        }
        delete[] newPrefix;
    }

    // If the prefix string starts with a file (a-h), also add coordinate
    // moves if coordMoves is true:
    if (coordMoves  &&  prefix[0] >= 'a'  &&  prefix[0] <= 'h') {
        MoveList mList;
		p->GenerateMoves(&mList);
        for (uint i=0; i < mList.Size(); i++) {
            simpleMoveT * sm = mList.Get(i);
            str[0] = square_FyleChar (sm->from);
            str[1] = square_RankChar (sm->from);
            str[2] = square_FyleChar (sm->to);
            str[3] = square_RankChar (sm->to);
            if (sm->promote == EMPTY) {
                str[4] = 0;
            } else {
                str[4] = piece_Char (sm->promote);
                str[5] = 0;
            }
            if (strIsPrefix (prefix, str)) {
                Tcl_AppendElement (ti, str);
            }
        }
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_moves: Return the list of legal moves in SAN notation
//
int
sc_pos_moves (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 2) {
        return errorResult (ti, "Usage: sc_pos moves");
    }
    Position * p = db->game->GetCurrentPos();
    sanListT sanList;
    p->CalcSANStrings (&sanList, SAN_NO_CHECKTEST);

    for (uint i=0; i < sanList.num; i++) {
        Tcl_AppendElement (ti, sanList.list[i]);
    }
    return TCL_OK;
}

int
sc_pos_moves_uci (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 2) {
        return errorResult (ti, "Usage: sc_pos moves");
    }
    Position * p = db->game->GetCurrentPos();
    sanListT sanList;
    p->CalcUCIStrings (&sanList);

    for (uint i=0; i < sanList.num; i++) {
        Tcl_AppendElement (ti, sanList.list[i]);
    }
    return TCL_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_pgnBoard:
//    Given a string representing part of a PGN game,
//    returns the board position corresponding to the
//    last position reached in the game.
int
sc_pos_pgnBoard (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_pos pgnBoard <pgn-text>");
    }

    Game * g = scratchGame;

    g->Clear();

    PgnParser parser (argv[2]);

    if ( parser.ParseGame(g) == ERROR_NotFound) {
        // No PGN header tags were found, so try just parsing moves:
        g->Clear();
        char * buf = new char [8000];
        parser.Reset (argv[2]);
        parser.SetEndOfInputWarnings (false);
        parser.SetResultWarnings (false);
        parser.ParseMoves (g, buf, 8000);
        delete[] buf;
    }

    char boardStr [200];
    g->GetCurrentPos()->MakeLongStr (boardStr);

    Tcl_AppendResult (ti, boardStr, NULL);

    return TCL_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_probe:
//    Probes tablebases for the current move.
int
sc_pos_probe (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_pos probe [score|report|optimal|board <sq>]";
    static const char * options[] = {
        "score", "report", "optimal", "board", NULL
    };
    enum { OPT_SCORE, OPT_REPORT, OPT_OPTIMAL, OPT_BOARD };

    int option = OPT_SCORE;  // Default option is to return the score.
    if (argc >= 3) {
        option = strUniqueMatch(argv[2], options);
    }

    if (option == OPT_REPORT) {
        if (argc != 3) { return errorResult (ti, usage); }
        // Command: sc_probe report
        // Tablebase report:
        DString * tbReport = new DString;
        if (probe_tablebase (ti, PROBE_REPORT, tbReport)) {
            Tcl_AppendResult (ti, tbReport->Data(), NULL);
        }
        delete tbReport;
    } else if (option == OPT_OPTIMAL) {
        if (argc != 3) { return errorResult (ti, usage); }
        // Command: sc_probe optimal
        // Optimal moves from tablebase:
        DString * tbOptimal = new DString;
        if (probe_tablebase (ti, PROBE_OPTIMAL, tbOptimal)) {
            Tcl_AppendResult (ti, tbOptimal->Data(), NULL);
        }
        delete tbOptimal;
    } else if (option == OPT_SCORE) {
        int score = 0;
        if (scid_TB_Probe (db->game->GetCurrentPos(), &score) == OK) {
            setIntResult (ti, score);
        }
    } else if (option == OPT_BOARD) {
        return sc_pos_probe_board (cd, ti, argc, argv);
    } else {
        return errorResult (ti, usage);
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_probe_board:
//    Probes tablebases for the current position with one piece
//    (specified by its square) relocated to each empty board square.
int
sc_pos_probe_board (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_pos probe board <square>";

    Position * pos = scratchPos;
    pos->CopyFrom (db->game->GetCurrentPos());

    if (argc != 4) { return errorResult (ti, usage); }

    // Try to read the square parameter as algebraic ("h8") or numeric (63):
    squareT sq = strGetSquare (argv[3]);
    if (sq == NULL_SQUARE) {
      int sqInt = strGetInteger (argv[3]);
      if (sqInt >= 0  &&  sqInt <= 63) { sq = sqInt; }
    }

    if (sq == NULL_SQUARE) {
         return errorResult (ti, usage);
    }

    const pieceT * board = pos->GetBoard();
    if (board[sq] == EMPTY) { return TCL_OK; }

    for (squareT toSq = A1; toSq <= H8; toSq++) {
        const char * result = "";
        if (pos->RelocatePiece (sq, toSq) != OK) {
            result = "X";
        } else {
            int score = 0;
            if (scid_TB_Probe (pos, &score) != OK) {
                result = "?";
            } else {
                if (score > 0) {
                    result = "+";
                } else if (score < 0) {
                    result = "-";
                } else {
                    result = "=";
                }
            }
            pos->RelocatePiece (toSq, sq);
        }
        Tcl_AppendResult (ti, result, NULL);
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_pos_setComment:
//    Set the comment for the current move.
int
sc_pos_setComment (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_pos setComment <comment-text>");
    }
    const char * str = argv[2];
    const char * oldComment = db->game->GetMoveComment();

    if (str[0] == 0  || (isspace((char)str[0]) && str[1] == 0)) {
        // No comment: nullify comment if necessary:
        if (oldComment != NULL) {
            db->game->SetMoveComment (NULL);
            db->gameAltered = true;
        }
    } else {
        // Only set the comment if it has actually changed:
        if (oldComment == NULL  ||  !strEqual (str, oldComment)) {
            db->game->SetMoveComment (str);
            db->gameAltered = true;
        }
    }
    return TCL_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_progressBar:
//    Sets the progress bar. This is typically called once before each
//    search or other slow action that can display a progress bar.
//    It takes either zero (to clear the progress bar) or four
//    arguments: the canvas name, the rectangle object name, the width,
//    and the height. It can also take an optional fifth argument, for the
//    elapsed time text object name in the canvas.
//
//    Actions that update the progress bar set the state to zero when they
//    complete, so the action of setting a progress bar ONLY lasts for
//    the next Scid function that uses it.
int
sc_progressBar (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    switch (argc) {
    case 5:
    case 6:
        progBar.state = true;
        progBar.interrupt = false;
        delete[] progBar.canvName;
        delete[] progBar.rectName;
        delete[] progBar.timeName;
        progBar.canvName = strDuplicate (argv[1]);
        progBar.rectName = strDuplicate (argv[2]);
        progBar.width = strGetInteger (argv[3]);
        progBar.height = strGetInteger (argv[4]);
        progBar.timeName = strDuplicate (argc > 5 ? argv[5] : "");
        break;

    case 1:  // No arguments, just reset progress bar:
        progBar.state = false;
        progBar.interrupt = true;
        break;

    default:
        return errorResult (ti,
                "Usage: sc_progressBar [canvas tagName width height]");
    }
    return TCL_OK;
}


//////////////////////////////////////////////////////////////////////
//   NAME commands
int
sc_name (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "correct", "edit", "info", "match", "plist",
        "ratings", "read", "spellcheck", "retrievename"
        "taglist", NULL
    };
    enum {
        OPT_CORRECT, OPT_EDIT, OPT_INFO, OPT_MATCH, OPT_PLIST,
        OPT_RATINGS, OPT_READ, OPT_SPELLCHECK, OPT_RETRIEVENAME
    };

    int index = -1;
    if (argc > 1) { index = strUniqueMatch (argv[1], options); }

    switch (index) {
    case OPT_CORRECT:
        return sc_name_correct (cd, ti, argc, argv);

    case OPT_EDIT:
        return sc_name_edit (cd, ti, argc, argv);

    case OPT_INFO:
        return sc_name_info (cd, ti, argc, argv);

    case OPT_MATCH:
        return sc_name_match (cd, ti, argc, argv);

    case OPT_PLIST:
        return sc_name_plist (cd, ti, argc, argv);

    case OPT_RATINGS:
        return sc_name_ratings (cd, ti, argc, argv);

    case OPT_READ:
        return sc_name_read (cd, ti, argc, argv);

    case OPT_SPELLCHECK:
        return sc_name_spellcheck (cd, ti, argc, argv);

    case OPT_RETRIEVENAME:
        return sc_name_retrievename (cd, ti, argc, argv);

    default:
        return InvalidCommand (ti, "sc_name", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_name_correct:
//    Corrects specified names in the database.
int
sc_name_correct (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    nameT nt = NAME_INVALID;
    if (argc == 4) { nt = NameBase::NameTypeFromString (argv[2]); }

    if (! NameBase::IsValidNameType(nt)) {
        return errorResult (ti,
                "Usage: sc_name correct p|e|s|r <corrections>");
    }
    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (db->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }

    NameBase * nb = db->nb;
    const char * str = argv[3];
    char oldName [512];
    char newName [512];
    char birth[128];
    char death[128];
    char line [512];
    uint errorCount = 0;
    uint correctionCount = 0;
    uint nameCount = nb->GetNumNames(nt);
    idNumberT * newIDs = new idNumberT [nameCount];
    dateT * startDate = new dateT [nameCount];
    dateT * endDate = new dateT [nameCount];

    for (idNumberT id=0; id < nameCount; id++) {
        newIDs[id] = id;
        startDate[id] = ZERO_DATE;
        endDate[id] = ZERO_DATE;
    }

    while (*str != 0) {
        uint length = 0;
        while (*str != 0  &&  *str != '\n') {
            if (length < 511) { line[length++] = *str; }
            str++;
        }
        line[length] = 0;
        if (*str == '\n') { str++; }
        // Now parse oldName and newName out of line, if the
        // line starts with a double-quote:
        char * s = line;
        if (*s != '"') { continue; }
        birth[0] = 0;
        death[0] = 0;
        int dummyCount = 0;
        if (sscanf (line, "\"%[^\"]\" >> \"%[^\"]\" (%d)  %[0-9.?]--%[0-9.?]",
                    oldName, newName, &dummyCount, birth, death) < 2) {
            continue;
        }

        // Find oldName in the NameBase:
        idNumberT oldID = 0;
        if (nb->FindExactName (nt, oldName, &oldID) != OK) {
            errorCount++;
            continue;
        }

        // Try to add the mapping for this correction:
        idNumberT newIdNumber = 0;
        if (nb->AddName (nt, newName, &newIdNumber) == OK) {
            newIDs [oldID] = newIdNumber;
            startDate[oldID] = date_EncodeFromString (birth);
            endDate[oldID] = date_EncodeFromString (death);
            correctionCount++;
        }
    }

    if (correctionCount == 0) {
        delete[] newIDs;
        delete[] startDate;
        delete[] endDate;

        return setResult (ti, "No valid corrections were found.");
    }

    // Write the name file first, for safety:
    if ((!db->memoryOnly)  &&  db->nb->WriteNameFile() != OK) {
        delete[] newIDs;
        delete[] startDate;
        delete[] endDate;
        return errorResult (ti, "Error writing name file.");
    }

    // Now go through the index making each necessary change:
    IndexEntry * ie;
    IndexEntry newIE;
    uint instanceCount = 0;
    uint datefailedCount = 0;
    for (uint i=0; i < db->numGames; i++) {
        ie = db->idx->FetchEntry (i);
        newIE = *ie;
        bool corrected = false;
        idNumberT oldID, newID;

        switch (nt) {
        case NAME_PLAYER:
            // Check White name first:
            oldID = ie->GetWhite();
            newID = newIDs [oldID];
            if (oldID != newID) {
                dateT date = ie->GetDate();
                // These startDate comparisons check each game for whether it occured before player born
                // but perhaps it's desirable to nominate an age (say 10 years; being uint -5120)
                if (date == ZERO_DATE ||
                    ((startDate[oldID] == ZERO_DATE  ||   date >= startDate[oldID])
                    &&  (endDate[oldID] == ZERO_DATE  ||   date <= endDate[oldID]))) {
                    newIE.SetWhite (newID);
                    corrected = true;
                    instanceCount++;
                } else {
                    datefailedCount++;
                }
            }
            // Now check Black name:
            oldID = ie->GetBlack();
            newID = newIDs [oldID];
            if (oldID != newID) {
                dateT date = ie->GetDate();
                if (date == ZERO_DATE ||
                    ((startDate[oldID] == ZERO_DATE  ||   date >= startDate[oldID])
                    &&  (endDate[oldID] == ZERO_DATE  ||   date <= endDate[oldID]))) {
                    newIE.SetBlack (newID);
                    corrected = true;
                    instanceCount++;
                } else {
                    datefailedCount++;
                }
            }
            break;

        case NAME_EVENT:
            oldID = ie->GetEvent();
            newID = newIDs [oldID];
            if (oldID != newID) {
                newIE.SetEvent (newID);
                corrected = true;
                instanceCount++;
            }
            break;

        case NAME_SITE:
            oldID = ie->GetSite();
            newID = newIDs [oldID];
            if (oldID != newID) {
                newIE.SetSite (newID);
                corrected = true;
                instanceCount++;
            }
            break;

        case NAME_ROUND:
            oldID = ie->GetRound();
            newID = newIDs [oldID];
            if (oldID != newID) {
                newIE.SetRound (newID);
                corrected = true;
                instanceCount++;
            }
            break;

        default:  // Should never happen!
            ASSERT(0);
        }

        // Write the new index entry if it has changed:
        if (corrected) {
            if (db->idx->WriteEntries (&newIE, i, 1) != OK) {
		delete[] newIDs;
		delete[] startDate;
		delete[] endDate;
                return errorResult (ti, "Error writing index file.");
            }
        }
    }

    delete[] newIDs;
    delete[] startDate;
    delete[] endDate;

    if (db->idx->WriteHeader() != OK) {
        return errorResult (ti, "Error writing index file.");
    }

    recalcNameFrequencies (db->nb, db->idx);
    char temp[100];
    sprintf (temp, "%u %s name%s occurring %u time%s in total were corrected.",
             correctionCount,
             NAME_TYPE_STRING[nt],
             strPlural (correctionCount),
             instanceCount, strPlural (instanceCount));
    Tcl_AppendResult (ti, temp, NULL);
    if (datefailedCount > 0) {
      sprintf (temp, "\n\n%u player name changes were ignored because of player age/game date considerations.", datefailedCount);
      Tcl_AppendResult (ti, temp, NULL);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_name_edit: edits a name in the NameBase. This requires
//    writing the entire index file, since the ID number of
//    the edited name will change.
//    A rating, date or eventdate can also be edited.
//
//    1st arg: player|event|site| ound|rating|edate
//    2nd arg: "all" / "filter" / "crosstable" (which games to edit)
//    3rd arg: name to edit.
//    4th arg: new name -- it might already exist in the namebase.
int
sc_name_edit (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_name edit <type> <editSelection> <oldName> <newName> <useStringMatch>";

    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (db->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }

    const char * options[] = {
        "player",   "event",   "site",   "round",   "rating",   "date",   "edate", NULL
    };
    enum {
        OPT_PLAYER, OPT_EVENT, OPT_SITE, OPT_ROUND, OPT_RATING, OPT_DATE, OPT_EVENTDATE
    };

    int option = -1;
    if (argc > 2) { option = strUniqueMatch (argv[2], options); }

    nameT nt = NAME_PLAYER;
    switch (option) {
    case OPT_PLAYER:  nt = NAME_PLAYER;  break;
    case OPT_EVENT:   nt = NAME_EVENT;   break;
    case OPT_SITE:    nt = NAME_SITE;    break;
    case OPT_ROUND:   nt = NAME_ROUND;   break;
    case OPT_RATING:  break;
    case OPT_DATE:    break;
    case OPT_EVENTDATE:   break;
    default:
        return errorResult (ti, usage);
    }

    if (option == OPT_RATING) {
        if (argc != 8) { return errorResult (ti, usage); }
    } else {
        if (argc != 7) { return errorResult (ti, usage); }
    }

    enum { EDIT_ALL, EDIT_FILTER, EDIT_CTABLE };
    int editSelection = EDIT_ALL;
    switch (argv[3][0]) {
    case 'a': editSelection = EDIT_ALL; break;
    case 'f': editSelection = EDIT_FILTER; break;
    case 'c': editSelection = EDIT_CTABLE; break;
    default:
        return errorResult (ti, usage);
    }

    const char * oldName = argv[4];
    const char * newName = argv[5];
    bool useStringMatch;
    dateT oldDate = ZERO_DATE;
    dateT newDate = ZERO_DATE;
    eloT newRating = 0;
    byte newRatingType = 0;
    if (option == OPT_RATING) {
        newRating = strGetUnsigned (argv[5]);
        newRatingType = strGetRatingType (argv[6]);
        useStringMatch =  strGetBoolean (argv[7]);
    } else {
        useStringMatch =  strGetBoolean (argv[6]);
    }

    if (option == OPT_DATE || option == OPT_EVENTDATE) {
        oldDate = date_EncodeFromString (argv[4]);
        newDate = date_EncodeFromString (argv[5]);
    }

    bool glob = ((option == OPT_DATE || option == OPT_EVENTDATE || useStringMatch) && oldName[0]=='*' && oldName[1]==0);
    int match;

    // Find the existing name in the namebase:
    idNumberT oldID = 0;

    // dont' allow globbing player names or EDIT_ALL
    if (glob && (option == OPT_PLAYER || option == OPT_RATING)) {
      Tcl_AppendResult (ti, "Using '*' to match Player names is not allowed.",NULL);
      return TCL_ERROR;
    }
    if (glob && editSelection == EDIT_ALL) {
      Tcl_AppendResult (ti, "Using '*' to match names is potentially harmful, and not allowed with 'All games'." , NULL);
      return TCL_ERROR;
    }

    // Not very useful check but ....

    // skip this check is we useStringMatch or option == OPT_DATE ||  option == OPT_EVENTDATE
    if (! useStringMatch && option != OPT_DATE && option != OPT_EVENTDATE) {
        if (db->nb->FindExactName (nt, oldName, &oldID) != OK) {
            Tcl_AppendResult (ti, "The ", NAME_TYPE_STRING[nt], " name \"", oldName, "\" does not exist.", NULL);
            return TCL_ERROR;
        }
    }

    // Set up crosstable game criteria if necessary:
    idNumberT eventId = 0, siteId = 0;
    dateT firstDate = 0, lastDate = 0, eventDate = 0;
    if (editSelection == EDIT_CTABLE) {
        Game * g = db->game;
        if (db->nb->FindExactName (NAME_EVENT, g->GetEventStr(), &eventId) != OK) {
            return errorResult (ti, "There are no crosstable games.");
        }
        if (db->nb->FindExactName (NAME_SITE, g->GetSiteStr(), &siteId) != OK) {
            return errorResult (ti, "There are no crosstable games.");
        }

        if (date_GetMonthDay (g->GetDate()) == 0) {
          // no month/day, so match games in calender year
	  firstDate = g->GetDate();
	  lastDate = date_AddMonths (g->GetDate(), 12) - 1;
        } else {
	  firstDate = date_AddMonths (g->GetDate(), -3);
	  lastDate = date_AddMonths (g->GetDate(), 3);
        }
        eventDate = g->GetEventDate();
    }

    // Add the new name to the namebase (if necessary)
    idNumberT newID = 0;
    if (option != OPT_RATING  &&  option != OPT_DATE  &&  option != OPT_EVENTDATE) {
        if (db->nb->AddName (nt, newName, &newID) == ERROR_NameBaseFull) {
            return errorResult (ti, "Name file is full; cannot add name.");
        }

        // Write the namebase to disk, if this is not a memory-only database:
        if (! db->memoryOnly  &&  db->nb->WriteNameFile() != OK) {
            return errorResult (ti, "Error writing name file.");
        }
    }

    // Now iterate through the index file making any necessary changes:
    IndexEntry * ie;
    IndexEntry newIE;
    uint numChanges = 0;

    for (uint i=0; i < db->numGames; i++) {
        // Check if this game is a candidate for editing:
        if (editSelection == EDIT_FILTER  &&  db->filter->Get (i) == 0) {
            continue;
        }
        ie = db->idx->FetchEntry (i);
        if (editSelection == EDIT_CTABLE
            && !isCrosstableGame (ie, siteId, eventId, firstDate, lastDate, eventDate)) {
            continue;
        }

        // Fetch the index entry and see if any editing is required:
        newIE = *ie;
        int edits = 0;

        switch (option) {
        case OPT_PLAYER:
          // no globbing allowed
          if (useStringMatch) {
            match = Tcl_StringMatch(db->nb->GetName (NAME_PLAYER, ie->GetWhite()), oldName);
	    if (match) {
		newIE.SetWhite (newID);
		edits++;
		oldID = ie->GetWhite();
            } else {
              // Can't match both white and black in same iteration as no way to decrement both oldIDs
	      match = Tcl_StringMatch(db->nb->GetName (NAME_PLAYER, ie->GetBlack()), oldName);
	      if (match) {
		  newIE.SetBlack (newID);
                  edits++;
                  oldID = ie->GetBlack();
              }
            }
          } else {
            if (ie->GetWhite() == oldID) {
                newIE.SetWhite (newID);
                edits++;
            }
            if (ie->GetBlack() == oldID) {
                newIE.SetBlack (newID);
                edits++;
            }
          }
          break;

        case OPT_EVENT:

            if (useStringMatch) {
                match = Tcl_StringMatch(db->nb->GetName (NAME_EVENT, ie->GetEvent()), oldName);
                if (match) {
                    newIE.SetEvent(newID);
                    edits++;
                    oldID = ie->GetEvent();
                }
            } else if (ie->GetEvent() == oldID) {
                newIE.SetEvent(newID);
                edits++;
            }

            break;

        case OPT_SITE:
            if (useStringMatch) {
                match = Tcl_StringMatch(db->nb->GetName (NAME_SITE, ie->GetSite()), oldName);
                if (match) {
                    newIE.SetSite(newID);
                    edits++;
                    oldID = ie->GetSite();
                }
            } else if (ie->GetSite() == oldID) {
                newIE.SetSite(newID);
                edits++;
            }
            break;

        case OPT_ROUND:

            if (useStringMatch) {
                match = Tcl_StringMatch(db->nb->GetName (NAME_ROUND, ie->GetRound()), oldName);
                if (match) {
                    newIE.SetRound(newID);
                    edits++;
                    oldID = ie->GetRound();
                }
            } else if (ie->GetRound() == oldID) {
                newIE.SetRound(newID);
                edits++;
            }
            break;

        case OPT_DATE:
            if (glob || ie->GetDate() == oldDate) {
                newIE.SetDate (newDate);
                edits++;
            }
            break;

        case OPT_EVENTDATE:
            if (glob || ie->GetEventDate() == oldDate) {
                newIE.SetEventDate (newDate);
                edits++;
            }
            break;

        case OPT_RATING:
            // no globbing allowed
            if (useStringMatch) {
              match = Tcl_StringMatch(db->nb->GetName (NAME_PLAYER, ie->GetWhite()), oldName);
              if (match) {
                  newIE.SetWhiteElo (newRating);
                  newIE.SetWhiteRatingType (newRatingType);
                  edits++;
              }
              match = Tcl_StringMatch(db->nb->GetName (NAME_PLAYER, ie->GetBlack()), oldName);
              if (match) {
                  newIE.SetBlackElo (newRating);
                  newIE.SetBlackRatingType (newRatingType);
                  edits++;
              }
            } else {
              if (ie->GetWhite() == oldID) {
                  newIE.SetWhiteElo (newRating);
                  newIE.SetWhiteRatingType (newRatingType);
                  edits++;
              }
              if (ie->GetBlack() == oldID) {
                  newIE.SetBlackElo (newRating);
                  newIE.SetBlackRatingType (newRatingType);
        	  edits++;
              }
            }
            break;

        default:   // Unreachable:
            ASSERT (0);
        }

        // Write this entry if any edits were made:
        if (edits != 0) {
            if (db->idx->WriteEntries (&newIE, i, 1) != OK) {
                return errorResult (ti, "Error writing index file.");
            }
            if (option != OPT_RATING  &&  option != OPT_DATE && option != OPT_EVENTDATE) {
                db->nb->IncFrequency (nt, newID, edits);
                db->nb->IncFrequency (nt, oldID, -edits);
            }
            numChanges += edits;
        }
    }

    if (db->idx->WriteHeader() != OK) {
        return errorResult (ti, "Error writing index file.");
    }

    char temp[500];
    if (option == OPT_RATING) {
        sprintf (temp, "Edited rating for %u games of \"%s\".",
                 numChanges, oldName);
    } else {
        sprintf (temp, "Changed %u of \"%s\" to \"%s\".",
                 numChanges, oldName, newName);
    }
    Tcl_AppendResult (ti, temp, NULL);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_name_retrievename:
//    Check for the right name in spellcheck and return it.
int
sc_name_retrievename (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usageStr = "Usage: sc_name retrievename <player>";
    SpellChecker * spChecker = spellChecker[NAME_PLAYER];

    if (argc != 3 ) { return errorResult (ti, usageStr); }
    const char * playerName = argv[argc-1];
    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (spChecker != NULL) {
        const char * note = spChecker->Correct (playerName);
        Tcl_AppendResult (ti, (note == NULL)? playerName : note, NULL);
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Returns the difference in length between strlen and Utf length
// (for printf formatting purposes)

int Utf_offset (const char *str) {
    int i;
    i = strlen(str) - Tcl_NumUtfChars(str,-1);
    if (i<0)
      i = 0;

    return i;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_name_info:
//    Prints information given a player name. Reports on the players
//    success rate with white and black, common openings by ECO code,
//    and Elo rating history.
int
sc_name_info (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static char * lastPlayerName = NULL;
    // this needs overhauling S.A
    const char * usageStr = "Usage: sc_name info [-ratings[:YEAR] -elo:ELO|-htext] <player> ";
    uint startYear = 1900;
    uint startElo = 0;
    extern spellingT countryTable[];
    extern spellingT titleTable[];

    if (argc != 3  &&  argc != 4 && argc != 5) { return errorResult (ti, usageStr); }
    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    bool ratingsOnly = false;
    bool ratingsAll  = false;
    bool htextOutput = false;
    bool setFilter = false;   // Set filter to games by this player
    bool setRefine = false; // Refine filter to games by this player
    bool setOpponent = false; // Set filter to games vs this opponent
    bool filter [NUM_COLOR_TYPES][NUM_RESULT_TYPES] =
    {
        { false, false, false, false },
        { false, false, false, false }
    };

    if (argc == 4 || argc == 5) {
        char * opt;
        if (argc == 5) {
            // minimum ELO
            opt = (char*) argv[3];
            if (strIsPrefix ("-elo:", opt)) {
                startElo = strGetUnsigned (opt + 5);
            } else {
                return errorResult (ti, usageStr);
            }
        }
	opt = (char*) argv[2];
        if (strIsPrefix ("-r", opt)) {
            // minimum YEAR
            ratingsOnly = true;
            if (strIsPrefix ("-ratings:", opt) || strIsPrefix ("-rateAll:", opt)) {
                startYear = strGetUnsigned (opt + 9);
            }
            if (strIsPrefix ("-rateAll:", opt)) {
                ratingsAll = true;
            }
        } else if (strIsPrefix ("-h", opt)  &&  strIsPrefix (opt, "-htext")) {
            htextOutput = true;
        } else if (opt[0] == '-'  &&  (opt[1] == 'f' || opt[1] == 'F' || opt[1] == 'o')) {
            if (opt[1] == 'f') {
                setFilter = true;
            } else if (opt[1] == 'F') {
                setFilter = true;
                setRefine = true;
            } else {
                setOpponent = true;
            }
            // Parse filter options: a = all, w = wins, d = draws, l = losses
            // for White, and capitalise those for Black.
            const char * fopt = opt + 2;
            while (*fopt != 0) {
                switch (*fopt) {
                case 'a':  // All White games:
                    filter [WHITE][RESULT_White] = true;
                    filter [WHITE][RESULT_Draw] = true;
                    filter [WHITE][RESULT_Black] = true;
                    filter [WHITE][RESULT_None] = true;
                    break;
                case 'A':  // All Black games:
                    filter [BLACK][RESULT_White] = true;
                    filter [BLACK][RESULT_Draw] = true;
                    filter [BLACK][RESULT_Black] = true;
                    filter [BLACK][RESULT_None] = true;
                    break;
                case 'w':  // White wins:
                    filter [WHITE][RESULT_White] = true;
                    break;
                case 'W':  // Black wins:
                    filter [BLACK][RESULT_White] = true;
                    break;
                case 'd':  // White draws:
                    filter [WHITE][RESULT_Draw] = true;
                    break;
                case 'D':  // Black draws:
                    filter [BLACK][RESULT_Draw] = true;
                    break;
                case 'l':  // White losses:
                    filter [WHITE][RESULT_Black] = true;
                    break;
                case 'L':  // Black losses:
                    filter [BLACK][RESULT_Black] = true;
                    break;
                default:
                    return errorResult (ti, usageStr);
                }
                fopt++;
            }
        } else {
            return errorResult (ti, usageStr);
        }
    }

    // Set up player name:
    const char * playerName = argv[argc-1];
    if (strEqual (playerName, "")) {
        if (lastPlayerName != NULL) { playerName = lastPlayerName; }
    } else {
        if (lastPlayerName != NULL) { delete[] lastPlayerName; }
        lastPlayerName = strDuplicate (playerName);
    }

    if (!playerName[0])
        return TCL_OK;

    // Try to find player name in this database:
    idNumberT id = 0;

    errorT found = db->nb->FindExactName (NAME_PLAYER, playerName, &id);

    if (ratingsAll) {
      Tcl_AppendResult (ti, spellChecker[NAME_PLAYER]->GetAllElo(playerName, startYear, startElo),NULL);
      return TCL_OK;
    }

    if (found != OK && ratingsOnly)
       return TCL_OK;

    char temp  [500];
    const char * newline = (htextOutput ? "<br>" : "\n");
    const char * startHeading = (htextOutput ? "<darkblue>" : "");
    const char * endHeading = (htextOutput ? "</darkblue>" : "");
    const char * startBold = (htextOutput ? "<b>" : "");
    const char * endBold = (htextOutput ? "</b>" : "");

    if (!ratingsOnly) {
      Tcl_AppendResult (ti, startBold, playerName, endBold, newline, newline, NULL);

      // Show title, country, etc if listed in player spellcheck file:
      SpellChecker * spChecker = spellChecker[NAME_PLAYER];
      if (spChecker != NULL) {
	  const char *text;
	  int i;

	  if ((text = spChecker->GetComment (playerName))) {

	    char t_title   [500] = "";
	    char t_country [500] = "";
	    char t_year    [500] = "";
	    char t_elo     [500] = "";
	    char *mark,*mark2;
	    int count;

	    // Comment format is " TITLE COUNTRY{/COUNTRY} [ELO] YEAR-BORN"

	    // For debugging
	    // Tcl_AppendResult (ti, "  ", text, newline, NULL);

	    i = sscanf (text, " %s %s %s %s", t_title , t_country, t_elo, t_year);

	    // title


	    mark = t_title ;
	    count = 0;

	    while (mark) {
	      // extra titles are '+' together, so keep processing till there's no more '+'
	      if ((mark = (char*) strFirstChar (t_title, '+'))) {
		mark[0] = 0;
	      }

	      // lookup in titleTable
	      i = 0;
	      while (titleTable[i].id[0] != 0) {
		if (strCaseCompare( titleTable[i].id , t_title) == 0)
		      break;
		i++;
	      };
	      if (titleTable[i].id[0] != 0) {
		// this is a little confusing, just to get the multiple "titles" on one line
		if (count==0 && mark)
		  Tcl_AppendResult (ti, startHeading, translate (ti,"Title"), ":	", endHeading, titleTable[i].data, NULL);
		else {
		  if (count==0)
		    Tcl_AppendResult (ti, startHeading, translate (ti,"Title"), ":	", endHeading, titleTable[i].data, "  ", t_elo, newline,  NULL);
		  else {
		    if (mark)
		      Tcl_AppendResult (ti, ",  ", titleTable[i].data, NULL);
		    else
		      Tcl_AppendResult (ti, ",  ", titleTable[i].data, "  ", t_elo, newline,  NULL);
		  }
		}
		count++;
	      } else {
		if (t_title[0] == '-') {
		  if ((mark2 = (char*)strFirstChar (t_elo, ']'))) {
		    mark2[0] = 0;
		  }
		  mark2 = t_elo;
		  if (mark2[0] == '[')
		    mark2++;
		  Tcl_AppendResult (ti, startHeading, translate (ti,"Elo"), ":	", endHeading, mark2, newline, NULL);
		} else {
		  Tcl_AppendResult (ti, startHeading, translate (ti,"Title"), ":	", endHeading, t_title, "  ", t_elo, newline, NULL);
		}
	      }

	      if (mark) {
		// Copy from after the '+' back into t_title, and repeat
		// - mark is an index into t_title, so 'strcpy (t_title, mark+1)' isnt very safe
		strcpy (temp, mark+1);
		strcpy (t_title,temp);
	      }
	    }

	    // lookup in countryTable
	    mark = t_country ;
	    count = 0;

	    while (mark) {
	      // extra countries are '/' together, so keep processing till there's no more '/'
	      if ((mark = (char*) strFirstChar (t_country, '/'))) {
		mark[0] = 0;
	      }

	      // lookup in countryTable
	      i = 0;
	      while (countryTable[i].id[0] != 0) {
		if (strCaseCompare( countryTable[i].id , t_country) == 0)
		      break;
		i++;
	      };

	      if (countryTable[i].id[0] != 0) {
		// use proper country name
		strcpy (temp, countryTable[i].data);
	      } else {
		// use three letter code
		strcpy (temp, t_country);
	      }

	      if (t_country[0] != '-') {
		if (count==0 && !mark)
		  Tcl_AppendResult (ti, startHeading, translate (ti,"Country"), ":	", endHeading, temp , newline, NULL);
		else {
		  if (count==0)
		    Tcl_AppendResult (ti, startHeading, translate (ti,"Country"), ":	", endHeading, temp , NULL);
		  else {
		    if (mark)
		      Tcl_AppendResult (ti, ",  ", temp , NULL);
		    else
		      Tcl_AppendResult (ti, ",  ", temp , newline , NULL);
		  }
		}
	      }
	      count++;

	      if (mark) {
		strcpy (temp, mark+1);
		strcpy (t_country,temp);
	      }
	    }

	    // Birthyear
	    if (*t_year) {
	      Tcl_AppendResult (ti, startHeading, "Born:	", endHeading, t_year, newline, NULL);
	    }

	}

	// biography
	const bioNoteT * note = spChecker->GetBioData (playerName);
	if (note != NULL) {
	    Tcl_AppendResult (ti, startHeading, translate (ti, "Biography"), ":", endHeading, newline, NULL);
	    while (note != NULL) {
		Tcl_AppendResult (ti, "	", note->text, newline, NULL);
		note = note->next;
	    }
	}

	if (text && *text) Tcl_AppendResult (ti, newline, NULL);
      }

    }

    if (found != OK) {
      // Tcl_AppendResult (ti, "The name \"", playerName, "\" does not exist in this database.", NULL);
      return TCL_OK;
    }

    // Try to find opponent in this database:
    idNumberT opponentId = 0;
    const char * opponent = NULL;
    if (strEqual (playerName, db->game->GetWhiteStr())) {
        opponent = db->game->GetBlackStr();
    } else if (strEqual (playerName, db->game->GetBlackStr())) {
        opponent = db->game->GetWhiteStr();
    }

    if (opponent != NULL) {
        if (db->nb->FindExactName (NAME_PLAYER, opponent, &opponentId) != OK) {
            opponent = NULL;
        }
    }

    enum {STATS_ALL = 0, STATS_FILTER, STATS_OPP};

    uint whitescore [3][NUM_RESULT_TYPES];
    uint blackscore [3][NUM_RESULT_TYPES];
    uint bothscore [3][NUM_RESULT_TYPES];
    uint whitecount[3] = {0};
    uint blackcount[3] = {0};
    uint totalcount[3] = {0};
    for (uint stat=STATS_ALL; stat <= STATS_OPP; stat++) {
        for (resultT r = 0; r < NUM_RESULT_TYPES; r++) {
            whitescore[stat][r] = 0;
            blackscore[stat][r] = 0;
            bothscore[stat][r] = 0;
        }
    }

    uint ecoCount [NUM_COLOR_TYPES] [50];
    uint ecoScore [NUM_COLOR_TYPES] [50];
    for (uint ecoGroup=0; ecoGroup < 50; ecoGroup++) {
        ecoCount[WHITE][ecoGroup] = ecoCount[BLACK][ecoGroup] = 0;
        ecoScore[WHITE][ecoGroup] = ecoScore[BLACK][ecoGroup] = 0;
    }

    dateT firstGameDate = ZERO_DATE;
    dateT lastGameDate = ZERO_DATE;

    bool seenRating = false;
    const uint monthMax = YEAR_MAX * 12;
    const uint monthMin = startYear * 12;
    eloT * eloByMonth = new eloT [monthMax];
    for (uint month=0; month < monthMax; month++) { eloByMonth[month] = 0; }

    // if (setFilter || setOpponent) { db->dbFilter->Fill (0); }
    if ((setFilter && ! setRefine) || setOpponent) { db->dbFilter->Fill (0); }

    for (uint i=0; i < db->numGames; i++) {
        IndexEntry * ie = db->idx->FetchEntry (i);
        eloT elo = 0;
        ecoT ecoCode = ie->GetEcoCode();
        int ecoClass = -1;
        if (ecoCode != ECO_None) {
            ecoStringT ecoStr;
            eco_ToBasicString (ecoCode, ecoStr);
            if (ecoStr[0] != 0) {
                ecoClass = ((ecoStr[0] - 'A') * 10) + (ecoStr[1] - '0');
                if (ecoClass < 0  ||  ecoClass >= 50) { ecoClass = -1; }
            }
        }

        resultT result = ie->GetResult();
        idNumberT whiteId = ie->GetWhite();
        idNumberT blackId = ie->GetBlack();
        dateT date = ZERO_DATE;

        // Remove from filter if not this person
        if (setRefine && whiteId != id && blackId != id)
            db->dbFilter->Set (i, 0);

        // Track statistics as white and black:
        if (whiteId == id) {
            date = ie->GetDate();
            elo = ie->GetWhiteElo();
            if (ecoClass >= 0) {
                ecoCount[WHITE][ecoClass]++;
                ecoScore[WHITE][ecoClass] += RESULT_SCORE[result];
            }
            whitescore[STATS_ALL][result]++;
            bothscore[STATS_ALL][result]++;
            whitecount[STATS_ALL]++;
            totalcount[STATS_ALL]++;
            if (db->filter->Get(i) > 0) {
                whitescore[STATS_FILTER][result]++;
                bothscore[STATS_FILTER][result]++;
                whitecount[STATS_FILTER]++;
                totalcount[STATS_FILTER]++;
            }
            if (opponent != NULL  &&  blackId == opponentId) {
                whitescore[STATS_OPP][result]++;
                bothscore[STATS_OPP][result]++;
                whitecount[STATS_OPP]++;
                totalcount[STATS_OPP]++;
                if (setOpponent  &&  filter[WHITE][result]) {
                    db->dbFilter->Set (i, 1);
                }
            }
            if (setFilter  &&  filter[WHITE][result]) {
                db->dbFilter->Set (i, 1);
            }
        } else if (blackId == id) {
            date = ie->GetDate();
            elo = ie->GetBlackElo();
            result = RESULT_OPPOSITE[result];
            if (ecoClass >= 0) {
                ecoCount[BLACK][ecoClass]++;
                ecoScore[BLACK][ecoClass] += RESULT_SCORE[result];
            }
            blackscore[STATS_ALL][result]++;
            bothscore[STATS_ALL][result]++;
            blackcount[STATS_ALL]++;
            totalcount[STATS_ALL]++;
            if (db->filter->Get(i) > 0) {
                blackscore[STATS_FILTER][result]++;
                bothscore[STATS_FILTER][result]++;
                blackcount[STATS_FILTER]++;
                totalcount[STATS_FILTER]++;
            }
            if (opponent != NULL  &&  whiteId == opponentId) {
                blackscore[STATS_OPP][result]++;
                bothscore[STATS_OPP][result]++;
                blackcount[STATS_OPP]++;
                totalcount[STATS_OPP]++;
                if (setOpponent  &&  filter[BLACK][result]) {
                    db->dbFilter->Set (i, 1);
                }
            }
            if (setFilter  &&  filter[BLACK][result]) {
                db->dbFilter->Set (i, 1);
            }
        }

        // Keep track of first and last games by this player:
        if (date != ZERO_DATE) {
            if (firstGameDate == ZERO_DATE  ||  date < firstGameDate) {
                firstGameDate = date;
            }
            if (lastGameDate == ZERO_DATE  ||  date > lastGameDate) {
                lastGameDate = date;
            }
        }

        // Track Elo ratings by month:
        if (elo >= startElo) {
            uint year = date_GetYear (date);
            if (year > YEAR_MAX) { year = 0; }
            uint month = date_GetMonth (date);
            if (month > 0) { month--; }
            if (month > 11) { month = 0; }
            ASSERT ((year * 12 + month) < monthMax);
            eloByMonth [year * 12 + month] = elo;
            seenRating = true;
        }
    }
    if (setFilter || setOpponent)
	setMainFilter(db);

    uint score, percent;
    colorT color;

    char trWhite[128];
    char trBlack[128];
    char trTotal[128];
    strcpy (trWhite , translate (ti, "White"));
    strcpy (trBlack , translate (ti, "Black"));
    strcpy (trTotal , translate (ti, "Total"));

    uint wWidth = strLength (trWhite);
    uint bWidth = strLength (trBlack);
    uint tWidth = strLength (trTotal);

    uint wbtWidth = wWidth;
    if (bWidth > wbtWidth) { wbtWidth = bWidth; }
    if (tWidth > wbtWidth) { wbtWidth = tWidth; }

    // German "Weiß", and Polish "Bia³e" are actually length 5 and 6, and breaks the printf :<
    // , so adjust formatting widths to allow for unicode
    wWidth = wbtWidth + Utf_offset(trWhite);
    bWidth = wbtWidth + Utf_offset(trBlack);
    tWidth = wbtWidth + Utf_offset(trTotal);

    //// sprintf format to display won, drawn, lost statistics S.A

    const char * fmt = "%s  %-*s %3u%c%02u%%   +%s%4u%s  -%s%4u%s  =%s%4u%s  %4u%c%c /%s%4u%s";

    char separator_line[100];
    memset (separator_line, '-', 100);
    if (wbtWidth + 44 < 100) 
      separator_line[wbtWidth + 44] = 0;
    else
      separator_line[99] = 0;

    if (ratingsOnly) { goto doRatings; }

    if (totalcount[STATS_ALL] == 0) {
      Tcl_AppendResult (ti, "0 ", translate (ti, "games"), NULL);
      return TCL_OK;
    }

    sprintf (temp, "%s%u %s%s",
             htextOutput ? "<green><run sc_name info -faA {}; ::playerInfoRefresh>" : "",
             totalcount[STATS_ALL],
             (totalcount[STATS_ALL] == 1 ?
              translate (ti, "game") : translate (ti, "games")),
             htextOutput ? "</run></green>" : "");
    Tcl_AppendResult (ti, temp, NULL);

    if (firstGameDate != ZERO_DATE) {
        date_DecodeToString (firstGameDate, temp);
        strTrimDate (temp);
        Tcl_AppendResult (ti, "	", temp, NULL);
    }
    if (lastGameDate > firstGameDate) {
        date_DecodeToString (lastGameDate, temp);
        strTrimDate (temp);
        Tcl_AppendResult (ti, "  to  ", temp, NULL);
    }

    Tcl_AppendResult (ti, newline, NULL);

    if (totalcount[STATS_FILTER] != totalcount[STATS_ALL]) {
      sprintf (temp, "%s: %s%u %s%s",
	       translate (ti, "Filter"),
	       htextOutput ? "<green><run sc_name info -F {}; ::playerInfoRefresh>" : "",
	       totalcount[STATS_FILTER],
	       (totalcount[STATS_FILTER] == 1 ?
		translate (ti, "game") : translate (ti, "games")),
	       htextOutput ? "</run></green>" : "");
      Tcl_AppendResult (ti, temp, NULL);
    }

    // Won , Drawn, Lost heading
    // (hmmm... no translations)
    sprintf (temp, " %s%s%*s%7s %7s%s%s",
            startHeading,
	    htextOutput ? "<tt>" : "",
	    wbtWidth+18,
	    "Won",
	    "Lost",
	    "Drawn",
	    htextOutput ? "</tt>" : "",
            endHeading);

    Tcl_AppendResult (ti, newline, temp, NULL);

    // Print stats for all games:

    sprintf (temp, "<b>%s</b>", translate (ti, "PInfoAll"));
    if (! htextOutput) { strTrimMarkup (temp); }
    Tcl_AppendResult (ti, newline, startHeading, temp,
                      endHeading, newline, NULL);

    score = percent = 0;
    if (totalcount[STATS_ALL] > 0) {
        score = bothscore[STATS_ALL][RESULT_White] * 2
            + bothscore[STATS_ALL][RESULT_Draw]
            + bothscore[STATS_ALL][RESULT_None];
        percent = score * 5000 / totalcount[STATS_ALL];
    }
    sprintf (temp, fmt,
             htextOutput ? "<tt>" : "",
             tWidth,
             trTotal,
             percent / 100, decimalPointChar, percent % 100,
             htextOutput ? "<green><run sc_name info -fwW {}; ::playerInfoRefresh>" : "",
             bothscore[STATS_ALL][RESULT_White],
             htextOutput ? "</run></green>" : "",
             htextOutput ? "<green><run sc_name info -flL {}; ::playerInfoRefresh>" : "",
             bothscore[STATS_ALL][RESULT_Black],
             htextOutput ? "</run></green>" : "",
             htextOutput ? "<green><run sc_name info -fdD {}; ::playerInfoRefresh>" : "",
             bothscore[STATS_ALL][RESULT_Draw],
             htextOutput ? "</run></green>" : "",
             score / 2, decimalPointChar, score % 2 ? '5' : '0',
             htextOutput ? "<green><run sc_name info -faA {}; ::playerInfoRefresh>" : "",
             totalcount[STATS_ALL],
             htextOutput ? "</run></green></tt>" : "");
    Tcl_AppendResult (ti, temp, newline, NULL);

    if (htextOutput) {
      Tcl_AppendResult (ti, "<tt>  " , separator_line , "</tt>", newline, NULL);
    }

    score = percent = 0;
    if (whitecount[STATS_ALL] > 0) {
        score = whitescore[STATS_ALL][RESULT_White] * 2
            + whitescore[STATS_ALL][RESULT_Draw]
            + whitescore[STATS_ALL][RESULT_None];
        percent = score * 5000 / whitecount[STATS_ALL];
    }

    sprintf (temp, fmt,
             htextOutput ? "<tt>" : "",
             wWidth,
             trWhite,
             percent / 100, decimalPointChar, percent % 100,
             htextOutput ? "<green><run sc_name info -fw {}; ::playerInfoRefresh>" : "",
             whitescore[STATS_ALL][RESULT_White],
             htextOutput ? "</run></green>" : "",
             htextOutput ? "<green><run sc_name info -fl {}; ::playerInfoRefresh>" : "",
             whitescore[STATS_ALL][RESULT_Black],
             htextOutput ? "</run></green>" : "",
             htextOutput ? "<green><run sc_name info -fd {}; ::playerInfoRefresh>" : "",
             whitescore[STATS_ALL][RESULT_Draw],
             htextOutput ? "</run></green>" : "",
             score / 2, decimalPointChar, score % 2 ? '5' : '0',
             htextOutput ? "<green><run sc_name info -fa {}; ::playerInfoRefresh>" : "",
             whitecount[STATS_ALL],
             htextOutput ? "</run></green></tt>" : "");
    Tcl_AppendResult (ti, temp, newline, NULL);

    score = percent = 0;
    if (blackcount[STATS_ALL] > 0) {
        score = blackscore[STATS_ALL][RESULT_White] * 2
            + blackscore[STATS_ALL][RESULT_Draw]
            + blackscore[STATS_ALL][RESULT_None];
        percent = score * 5000 / blackcount[STATS_ALL];
    }
    sprintf (temp, fmt,
             htextOutput ? "<tt>" : "",
             bWidth,
             trBlack,
             percent / 100, decimalPointChar, percent % 100,
             htextOutput ? "<green><run sc_name info -fW {}; ::playerInfoRefresh>" : "",
             blackscore[STATS_ALL][RESULT_White],
             htextOutput ? "</run></green>" : "",
             htextOutput ? "<green><run sc_name info -fL {}; ::playerInfoRefresh>" : "",
             blackscore[STATS_ALL][RESULT_Black],
             htextOutput ? "</run></green>" : "",
             htextOutput ? "<green><run sc_name info -fD {}; ::playerInfoRefresh>" : "",
             blackscore[STATS_ALL][RESULT_Draw],
             htextOutput ? "</run></green>" : "",
             score / 2, decimalPointChar, score % 2 ? '5' : '0',
             htextOutput ? "<green><run sc_name info -fA {}; ::playerInfoRefresh>" : "",
             blackcount[STATS_ALL],
             htextOutput ? "</run></green></tt>" : "");
    Tcl_AppendResult (ti, temp, newline, NULL);

// Now print stats for games in the filter (if any games are filtered)

if (db->numGames == db->filter->Count()) {
    Tcl_AppendResult (ti, newline, NULL);
} else {

    sprintf (temp, "<b>%s</b>", translate (ti, "PInfoFilter"));
    if (! htextOutput) { strTrimMarkup (temp); }
    Tcl_AppendResult (ti, startHeading, temp,
                      endHeading, newline, NULL);

    score = percent = 0;
    if (totalcount[STATS_FILTER] > 0) {
        score = bothscore[STATS_FILTER][RESULT_White] * 2
            + bothscore[STATS_FILTER][RESULT_Draw]
            + bothscore[STATS_FILTER][RESULT_None];
        percent = score * 5000 / totalcount[STATS_FILTER];
    }
    sprintf (temp, fmt,
             htextOutput ? "<tt>" : "",
             tWidth,
             trTotal,
             percent / 100, decimalPointChar, percent % 100,
             "", bothscore[STATS_FILTER][RESULT_White], "",
             "", bothscore[STATS_FILTER][RESULT_Black], "",
             "", bothscore[STATS_FILTER][RESULT_Draw], "",
             score / 2, decimalPointChar, score % 2 ? '5' : '0',
             htextOutput ? "<green><run sc_name info -F {}; ::playerInfoRefresh>" : "",
             totalcount[STATS_FILTER],
             htextOutput ? "</run></green></tt>" : "");
    Tcl_AppendResult (ti, temp, newline, NULL);

    if (htextOutput) {
      Tcl_AppendResult (ti, "<tt>  " , separator_line , "</tt>", newline, NULL);
    }

    score = percent = 0;
    if (whitecount[STATS_FILTER] > 0) {
        score = whitescore[STATS_FILTER][RESULT_White] * 2
            + whitescore[STATS_FILTER][RESULT_Draw]
            + whitescore[STATS_FILTER][RESULT_None];
        percent = score * 5000 / whitecount[STATS_FILTER];
    }
    sprintf (temp, fmt,
             htextOutput ? "<tt>" : "",
             wWidth,
             trWhite,
             percent / 100, decimalPointChar, percent % 100,
             "", whitescore[STATS_FILTER][RESULT_White], "",
             "", whitescore[STATS_FILTER][RESULT_Black], "",
             "", whitescore[STATS_FILTER][RESULT_Draw], "",
             score / 2, decimalPointChar, score % 2 ? '5' : '0',
             "", whitecount[STATS_FILTER],
             htextOutput ? "</tt>" : "");
    Tcl_AppendResult (ti, temp, newline, NULL);

    score = percent = 0;
    if (blackcount[STATS_FILTER] > 0) {
        score = blackscore[STATS_FILTER][RESULT_White] * 2
            + blackscore[STATS_FILTER][RESULT_Draw]
            + blackscore[STATS_FILTER][RESULT_None];
        percent = score * 5000 / blackcount[STATS_FILTER];
    }
    sprintf (temp, fmt,
             htextOutput ? "<tt>" : "",
             bWidth,
             trBlack,
             percent / 100, decimalPointChar, percent % 100,
             "", blackscore[STATS_FILTER][RESULT_White], "",
             "", blackscore[STATS_FILTER][RESULT_Black], "",
             "", blackscore[STATS_FILTER][RESULT_Draw], "",
             score / 2, decimalPointChar, score % 2 ? '5' : '0',
             "", blackcount[STATS_FILTER],
             htextOutput ? "</tt>" : "");
    Tcl_AppendResult (ti, temp, newline, NULL);
}

    // Now print stats for games against the current opponent:

    if (opponent != NULL) {
        Tcl_AppendResult (ti, startHeading,
                          translate (ti, "PInfoAgainst"), " ",
                          startBold, opponent, endBold,
                          endHeading, newline, NULL);

        score = percent = 0;
        if (totalcount[STATS_OPP] > 0) {
            score = bothscore[STATS_OPP][RESULT_White] * 2
                + bothscore[STATS_OPP][RESULT_Draw]
                + bothscore[STATS_OPP][RESULT_None];
            percent = score * 5000 / totalcount[STATS_OPP];
	}
        sprintf (temp, fmt,
                 htextOutput ? "<tt>" : "",
                 tWidth,
                 trTotal,
                 percent / 100, decimalPointChar, percent % 100,
                 htextOutput ? "<green><run sc_name info -owW {}; ::playerInfoRefresh>" : "",
                 bothscore[STATS_OPP][RESULT_White],
                 htextOutput ? "</run></green>" : "",
                 htextOutput ? "<green><run sc_name info -olL {}; ::playerInfoRefresh>" : "",
                 bothscore[STATS_OPP][RESULT_Black],
                 htextOutput ? "</run></green>" : "",
                 htextOutput ? "<green><run sc_name info -odD {}; ::playerInfoRefresh>" : "",
                 bothscore[STATS_OPP][RESULT_Draw],
                 htextOutput ? "</run></green>" : "",
                 score / 2, decimalPointChar, score % 2 ? '5' : '0',
                 htextOutput ? "<green><run sc_name info -oaA {}; ::playerInfoRefresh>" : "",
                 totalcount[STATS_OPP],
                 htextOutput ? "</run></green></tt>" : "");
        Tcl_AppendResult (ti, temp, newline, NULL);

	if (htextOutput) {
	  Tcl_AppendResult (ti, "<tt>  " , separator_line , "</tt>", newline, NULL);
        }

        score = percent = 0;
        if (whitecount[STATS_OPP] > 0) {
            score = whitescore[STATS_OPP][RESULT_White] * 2
            + whitescore[STATS_OPP][RESULT_Draw]
            + whitescore[STATS_OPP][RESULT_None];
            percent = score * 5000 / whitecount[STATS_OPP];
        }
        sprintf (temp, fmt,
                 htextOutput ? "<tt>" : "",
                 wWidth,
                 trWhite,
                 percent / 100, decimalPointChar, percent % 100,
                 htextOutput ? "<green><run sc_name info -ow {}; ::playerInfoRefresh>" : "",
                 whitescore[STATS_OPP][RESULT_White],
                 htextOutput ? "</run></green>" : "",
                 htextOutput ? "<green><run sc_name info -ol {}; ::playerInfoRefresh>" : "",
                 whitescore[STATS_OPP][RESULT_Black],
                 htextOutput ? "</run></green>" : "",
                 htextOutput ? "<green><run sc_name info -od {}; ::playerInfoRefresh>" : "",
                 whitescore[STATS_OPP][RESULT_Draw],
                 htextOutput ? "</run></green>" : "",
                 score / 2, decimalPointChar, score % 2 ? '5' : '0',
                 htextOutput ? "<green><run sc_name info -oa {}; ::playerInfoRefresh>" : "",
                 whitecount[STATS_OPP],
                 htextOutput ? "</run></green></tt>" : "");
        Tcl_AppendResult (ti, temp, newline, NULL);

        score = percent = 0;
        if (blackcount[STATS_OPP] > 0) {
            score = blackscore[STATS_OPP][RESULT_White] * 2
                + blackscore[STATS_OPP][RESULT_Draw]
                + blackscore[STATS_OPP][RESULT_None];
            percent = score * 5000 / blackcount[STATS_OPP];
        }
        sprintf (temp, fmt,
                 htextOutput ? "<tt>" : "",
                 bWidth,
                 trBlack,
                 percent / 100, decimalPointChar, percent % 100,
                 htextOutput ? "<green><run sc_name info -oW {}; ::playerInfoRefresh>" : "",
                 blackscore[STATS_OPP][RESULT_White],
                 htextOutput ? "</run></green>" : "",
                 htextOutput ? "<green><run sc_name info -oL {}; ::playerInfoRefresh>" : "",
                 blackscore[STATS_OPP][RESULT_Black],
                 htextOutput ? "</run></green>" : "",
                 htextOutput ? "<green><run sc_name info -oD {}; ::playerInfoRefresh>" : "",
                 blackscore[STATS_OPP][RESULT_Draw],
                 htextOutput ? "</run></green>" : "",
                 score / 2, decimalPointChar, score % 2 ? '5' : '0',
                 htextOutput ? "<green><run sc_name info -oA {}; ::playerInfoRefresh>" : "",
                 blackcount[STATS_OPP],
                 htextOutput ? "</run></green></tt>" : "");
        Tcl_AppendResult (ti, temp, newline, NULL);

    }

    // Now print common openings played:

    for (color = WHITE; color <= BLACK; color++) {
        for (uint count = 0; count < 6; count++) {
            int mostPlayedIdx = -1;
            uint mostPlayed = 0;
            for (uint i=0; i < 50; i++) {
                if (ecoCount[color][i] > mostPlayed) {
                    mostPlayedIdx = i;
                    mostPlayed = ecoCount[color][i];
                }
            }
            if (mostPlayed > 0) {
                if (count == 0) {
                    const char * s = (color == WHITE ? "PInfoMostWhite" :
                                      "PInfoMostBlack");
                    Tcl_AppendResult (ti, newline, startHeading,
                                      translate (ti, s),
                                      endHeading, newline, NULL);
                } else if (count == 3) {
                    Tcl_AppendResult (ti, newline, NULL);
                }
                Tcl_AppendResult (ti, "   ", NULL);

                temp[0] = mostPlayedIdx / 10 + 'A';
                temp[1] = mostPlayedIdx % 10 + '0';
                temp[2] = 0;
                if (htextOutput) {
                    Tcl_AppendResult (ti, "<blue><run ::windows::eco::Refresh ",
                                      temp, ">", NULL);
                }
                Tcl_AppendResult (ti, temp, NULL);
                if (htextOutput) {
                    Tcl_AppendResult (ti, "</run></blue>", NULL);
                }
                sprintf (temp, ":%3u (%u%%)", mostPlayed,
                         ecoScore[color][mostPlayedIdx] * 50 / mostPlayed);
                Tcl_AppendResult (ti, temp, NULL);
                ecoCount[color][mostPlayedIdx] = 0;
            }
        }
    }

  doRatings:
    if (seenRating) {
        if (! ratingsOnly) {
            Tcl_AppendResult (ti, newline, newline, startHeading,
                              translate (ti, "PInfoRating"),
                              endHeading, NULL);
        }
        eloT previousElo = 0;
        uint count = 0;
        for (uint i = monthMin; i < monthMax; i++) {
            eloT elo = eloByMonth [i];
            if (elo != 0) {
                uint year = i / 12;
                uint month = 1 + (i % 12);
                if (ratingsOnly) {
                    sprintf (temp, "%4u.%02u", year, (month - 1) * 100 / 12);
                    Tcl_AppendElement (ti, temp);
                    sprintf (temp, "%4u", elo);
                    Tcl_AppendElement (ti, temp);
                } else {
                    if (previousElo != elo) {
                        previousElo = elo;
                        count++;
                        if (count % 2) {
                            Tcl_AppendResult (ti, newline, NULL);
                        } else {
                            Tcl_AppendResult (ti, "   ", NULL);
                        }
                        sprintf (temp, "    %4u.%02u   %4u", year, month, elo);
                        Tcl_AppendResult (ti, temp, NULL);
                    }
                }
            }
        }
    }
    delete[] eloByMonth;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_name_match: returns the first N matching names,
//    or fewer if there are not N matches, given a substring
//    to search for.
//    Output is a Tcl list, to be read in pairs: the first element of
//    each pair is the frequency, the second contains the name.
//
//    1st arg: "p" (player) / "e" (event) / "s" (site) / "r" (round)
//    2nd arg: prefix string to search for.
//    3rd arg: maximum number of matches to return.
//    Example: sc_nameMatch player "Speel" 10
int
sc_name_match (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = \
        "Usage: sc_name match [-elo] <nameType> <prefix> <maxMatches>";

    int arg = 2;
    int argsleft = argc - 2;
    bool eloMode = false;  // In elo mode, return player peak ratings.
    if (argsleft < 3) { return errorResult (ti, usage); }
    if (argv[arg][0] == '-'  &&  strIsPrefix (argv[arg], "-elo")) {
        eloMode = true;
        arg++;
        argsleft--;
    }
    if (argsleft != 3) {
        return errorResult (ti, usage);
    }

    nameT nt = NameBase::NameTypeFromString (argv[arg++]);
    if (nt == NAME_INVALID) {
        return errorResult (ti, usage);
    }

    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    const char * prefix = argv[arg++];
    uint maxMatches = strGetUnsigned (argv[arg++]);
    if (maxMatches == 0) { return TCL_OK; }
    idNumberT * array = new idNumberT [maxMatches];
    uint matches = db->nb->GetFirstMatches (nt, prefix, maxMatches, array);
    for (uint i=0; i < matches; i++) {
        uint freq = db->nb->GetFrequency (nt, array[i]);
        const char * str = db->nb->GetName (nt, array[i]);
        appendUintElement (ti, freq);
        Tcl_AppendElement (ti, str);
        if (nt == NAME_PLAYER  &&  eloMode) {
            appendUintElement (ti, db->nb->GetElo (array[i]));
        }
    }
    delete[] array;

    return TCL_OK;
}


typedef const nameNodeT * namebaseNodeT;

struct SortNamebaseNodes
{
    static int
    compareNames(namebaseNodeT lhs, namebaseNodeT rhs)
    {
	const char * name1 = lhs->name;
	const char * name2 = rhs->name;

	// If equal, resolve by comparing names, first case-insensitive and
	// then case-sensitively if still tied:
	int compare = strCaseCompare (name1, name2);
	if (compare == 0) { compare = strCompare (name1, name2); }
	return compare;
    }

    static int
    compareCountry(namebaseNodeT lhs, namebaseNodeT rhs)
    {
	const char * name1 = lhs->data.country;
	const char * name2 = rhs->data.country;

	int compare = strCompare (name1, name2);
	return compare;
    }

    static int
    compareElo(namebaseNodeT lhs, namebaseNodeT rhs)
    {
	int compare = rhs->data.maxElo - lhs->data.maxElo;
	return compare ? compare : compareNames(lhs, rhs);
    }

    static int
    compareGames(namebaseNodeT lhs, namebaseNodeT rhs)
    {
	int compare = rhs->data.frequency - lhs->data.frequency;
	return compare ? compare : compareNames(lhs, rhs);
    }

    static int
    compareOldest(namebaseNodeT lhs, namebaseNodeT rhs)
    {
	// Sort by oldest game year in ascending order:
	int compare = lhs->data.firstDate - rhs->data.firstDate;
	return compare ? compare : compareNames(lhs, rhs);
    }

    static int
    compareNewest(namebaseNodeT lhs, namebaseNodeT rhs)
    {
	// Sort by newest game date in descending order:
	int compare = date_GetYear(rhs->data.firstDate) - date_GetYear(lhs->data.firstDate);
	return compare ? compare : compareNames(lhs, rhs);
    }

    static int
    comparePhoto(namebaseNodeT lhs, namebaseNodeT rhs)
    {
	int compare = (int)rhs->data.hasPhoto - (int)lhs->data.hasPhoto;
	return compare ? compare : compareNames(lhs, rhs);
    }

    typedef int (*Comp) (namebaseNodeT lhs, namebaseNodeT rhs);
    Comp comp;

    SortNamebaseNodes (Comp comp_) :comp(comp_) {}

    // std::multiset expects this less operator, it returns whether
    // lhs is less than rhs.

    bool operator () (namebaseNodeT lhs, namebaseNodeT rhs) const
    {
	return comp(lhs, rhs) < 0;
    }
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_name_plist:
//   Returns a list of play data matching selected criteria.
int
sc_name_plist (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usage = "Usage: sc_name plist [-<option> <value> ...]";

    const char * namePrefix = "";
    const char * country = "";
    bool searchCountry = 0;
    uint minGames = 0;
    uint maxGames = db->numGames;
    uint minElo = 0;
    uint maxElo = MAX_ELO;
    uint maxListSize = db->nb->GetNumNames(NAME_PLAYER);
    extern spellingT countryTable[];

    if (! db->inUse) { return errorResult (ti, errMsgNotOpen(ti)); }
    if (db->numGames == 0) { return TCL_OK; }

    static const char * options [] = {
        "-name", "-minElo", "-maxElo", "-minGames", "-maxGames", "-size", "-sort", "-country", NULL
    };
    enum {
        OPT_NAME, OPT_MINELO, OPT_MAXELO, OPT_MINGAMES, OPT_MAXGAMES, OPT_SIZE, OPT_SORT, OPT_COUNTRY
    };

    // Valid sort types:
    static const char * sortModes [] = {
        "elo", "games", "oldest", "newest", "name", "photo", "country", NULL
    };
    enum {
        SORT_ELO, SORT_GAMES, SORT_OLDEST, SORT_NEWEST, SORT_NAME, SORT_PHOTO, SORT_COUNTRY
    };

    int sortMode = SORT_NAME;

    // Read parameters in pairs:
    int arg = 2;
    while (arg+1 < argc) {
        const char * option = argv[arg];
        const char * value = argv[arg+1];
        arg += 2;
        int index = strUniqueMatch (option, options);
        switch (index) {
            case OPT_NAME:     namePrefix = value;                   break;
            case OPT_MINELO:   minElo = strGetUnsigned (value);      break;
            case OPT_MAXELO:   maxElo = strGetUnsigned (value);      break;
            case OPT_MINGAMES: minGames = strGetUnsigned (value);    break;
            case OPT_MAXGAMES: maxGames = strGetUnsigned (value);    break;
            case OPT_SIZE:     maxListSize = strGetUnsigned (value); break;
            case OPT_COUNTRY:
               searchCountry = !(strEqual(value,"NO"));
               // 'yes' leaves country = "";
               if (!strEqual(value,"YES")) country = value;
               break;
            case OPT_SORT:
               sortMode = strUniqueMatch (value, sortModes);
               break;
            default:
                return InvalidCommand (ti, "sc_name plist", options);
        }
    }

    if (arg != argc) { return errorResult (ti, usage); }

    // C++: 'auto' will deduce the correct type from assigned value.
    auto comp = SortNamebaseNodes::compareNewest;

    switch (sortMode) {
        case SORT_ELO:    comp = SortNamebaseNodes::compareElo;    break;
        case SORT_GAMES:  comp = SortNamebaseNodes::compareGames;  break;
        case SORT_OLDEST: comp = SortNamebaseNodes::compareOldest; break;
        case SORT_NEWEST: comp = SortNamebaseNodes::compareNewest; break;
        case SORT_NAME:   comp = SortNamebaseNodes::compareNames;  break;
        case SORT_PHOTO:  comp = SortNamebaseNodes::comparePhoto;  break;
        case SORT_COUNTRY: comp = SortNamebaseNodes::compareCountry; break;
        default:
            return InvalidCommand (ti, "sc_name plist -sort", sortModes);
    }

    typedef std::multiset<namebaseNodeT, SortNamebaseNodes> NamebaseNodeSet;

    NameBase * nb = db->nb;
    uint nPlayers = nb->GetNumNames(NAME_PLAYER);

    SortNamebaseNodes sortOp(comp);
    NamebaseNodeSet pset(sortOp);
    SpellChecker * spChecker = spellChecker[NAME_PLAYER];
    const char *text;
    char tmp[16];

    for (uint id = 0; id < nPlayers; id++) {
        namebaseNodeT node = nb->GetNode(NAME_PLAYER, id);

        ASSERT(node);

        const char * name = node->name;
        uint nGames = node->data.frequency;
        eloT elo = node->data.maxElo;
        if (nGames < minGames  ||  nGames > maxGames) { continue; }
        if (elo < minElo  ||  elo > maxElo) { continue; }
        if (! strIsCasePrefix (namePrefix, name)) { continue; }

        if (searchCountry) {
          if (spChecker && (node->data.country[0] == 0) && (text = spChecker->GetComment(node->name))) {
              strcpy ((char*)node->data.country, SpellChecker::GetLastCountry(text));
              // We use strIsCasePrefix to match ""
              if (! strIsCasePrefix (country, node->data.country)) { continue; }
          } else {
              if (node->data.country[0] == 0) {
                  strcpy  ((char*)node->data.country, "~~~");
                  if (! strIsCasePrefix (country, "~~~")) { continue; }
              } else {
                  if (! strIsCasePrefix (country, node->data.country)) { continue; }
              }
          }
        }

        // Check if this player has a photo:
        //if (Tcl_GetVar2 (ti, "photo", (char *)name, TCL_GLOBAL_ONLY) != NULL) {
        //    nb->SetHasPhoto (id, true);
        //}

        // Insert this player into the ordered set if necessary:
        pset.insert(node);
    }

    // Generate the list of player data:
    Tcl_DString * ds = new Tcl_DString;
    Tcl_DStringInit (ds);

    maxListSize = std::min(maxListSize, uint(pset.size()));
    uint count = 0;

    // C++: 'auto' will deduce the correct type from assigned value. In this
    // C++: case it will be a iterator (concrete type is NamebaseNodeSet::const_iterator).
    // C++: Note that in C++ pre-increment is preferred over post-increment, this means
    // C++: '++i' is better than 'i++' (pre-increment returns the modified object, but
    // C++: post-increment must return a copy of the object before modification).

    for (auto i = pset.cbegin(), e = pset.cend(); i != e; ++i) {
        Tcl_DStringStartSublist (ds);
        namebaseNodeT node = *i; // C++: '*' is dereferencing the object from iterator
        sprintf (tmp, "%u", node->data.frequency);
        Tcl_DStringAppendElement(ds, tmp);
        sprintf (tmp, "%u", date_GetYear(node->data.firstDate));
        Tcl_DStringAppendElement(ds, tmp);
        sprintf (tmp, "%u", date_GetYear(node->data.lastDate));
        Tcl_DStringAppendElement(ds, tmp);

        if (searchCountry) 
            Tcl_DStringAppendElement(ds, node->data.country);

        sprintf (tmp, "%u", node->data.maxElo);
        Tcl_DStringAppendElement(ds, tmp);
        //strCopy (tmp, nb->HasPhoto(pset[p]) ? "1" : "0");
        //Tcl_DStringAppendElement(ds, tmp);
        Tcl_DStringAppendElement(ds, node->name);
        Tcl_DStringEndSublist (ds);

	if (++count == maxListSize) {
	    break;
	}
    }
    Tcl_DStringResult (ti, ds);
    Tcl_DStringFree (ds);
    delete ds;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_name_ratings:
//   Scan the current database for games with unrated players who
//   have Elo rating information in the spellcheck file, and fill
//   in the missing Elo ratings.
//
//   Boolean options:
//      -nomonth (default=true): indicates whether games with no month
//           should still have ratings allocated, assuming the month
//           to be January.
//      -update (default=true): indicates whether the database should
//           be updated; if it is false, no actual changes are made.
//      -debug (default=false): for debugging; it dumps all detected
//           rating changes, one per line, to stdout.
//      -test (default=false): tests whether a sspelling file with
//           rating info is loaded and can be used on this database.
//      -overwrite (default=false): if true, existing ratings can be
//           changed.
//
//   Returns a two-integer list: the number of changed ratings, and
//   the number of changed games.
int
sc_name_ratings (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * options[] = {
        "-nomonth", "-update", "-debug", "-test", "-change", "-filter" };
    enum {
        OPT_NOMONTH, OPT_UPDATE, OPT_DEBUG, OPT_TEST, OPT_CHANGE, OPT_FILTER
    };

    bool showProgress = startProgressBar();
    bool doGamesWithNoMonth = true;
    bool updateIndexFile = true;
    bool printEachChange = false;
    bool testOnly = false;
    bool overwrite = false;
    bool filterOnly = false;

    int arg = 2;
    while (arg+1 < argc) {
        int option = strUniqueMatch (argv[arg], options);
        bool value = strGetBoolean (argv[arg+1]);
        switch (option) {
            case OPT_NOMONTH:   doGamesWithNoMonth = value; break;
            case OPT_UPDATE:    updateIndexFile = value;    break;
            case OPT_DEBUG:     printEachChange = value;    break;
            case OPT_TEST:      testOnly = value;           break;
            case OPT_CHANGE:    overwrite = value;          break;
            case OPT_FILTER:    filterOnly = value;         break;
            default: return InvalidCommand (ti, "sc_name ratings", options);
        }
        arg += 2;
    }

    uint numChangedRatings = 0;
    uint numChangedGames = 0;
    SpellChecker * sp = spellChecker[NAME_PLAYER];

    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (db->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }
    if (sp == NULL) {
        Tcl_AppendResult (ti, "A spellcheck file has not been loaded.\n\n",
                          "You can load one from the Options menu.", NULL);
        return TCL_ERROR;
    }
    if (! sp->HasEloData()) {
        Tcl_AppendResult (ti, "The current spelling file does not have Elo ratings.\n\n",
                          "Please load \"ratings.ssp\" (available from the Scid website) ",
                          "as your spellcheck file.", NULL);

        return TCL_ERROR;
    }

    if (testOnly) { return TCL_OK; }

    uint updateStart = 1000;
    uint update = updateStart;

    for (uint gnum=0; gnum < db->numGames; gnum++) {
        if (showProgress) {  // Update the percentage done bar:
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, gnum, db->numGames);
                if (interruptedProgress()) break;
            }
        }
        if (filterOnly  &&  db->filter->Get(gnum) == 0) { continue; }

        bool changed = false;
        bool exact = false;
        eloT newWhite = 0;
        eloT newBlack = 0;
        IndexEntry * ie = db->idx->FetchEntry (gnum);
        dateT date = ie->GetDate();
        if (date_GetMonth(date) == 0  &&  !doGamesWithNoMonth) { continue; }
        eloT oldWhite = ie->GetWhiteElo();
        if (overwrite  &&  oldWhite != 0) { exact = true; }
        if (overwrite  ||  oldWhite == 0) {
            const char * name = ie->GetWhiteName (db->nb);
            eloT rating = sp->GetElo (name, date, exact);
            if (rating != 0) {
                if (printEachChange) {
                    printf ("%4u  %4u.%02u  %s\n", rating,
                            date_GetYear(date), date_GetMonth(date), name);
                }
                newWhite = rating;
                changed = true;
                numChangedRatings++;
            }
        }
        eloT oldBlack = ie->GetBlackElo();
        exact = false;
        if (overwrite  &&  oldBlack != 0) { exact = true; }
        if (overwrite  ||  oldBlack == 0) {
            const char * name = ie->GetBlackName (db->nb);
            eloT rating = sp->GetElo (name, date, exact);
            if (rating != 0) {
                if (printEachChange) {
                    printf ("%4u  %4u.%02u  %s\n", rating,
                            date_GetYear(date), date_GetMonth(date), name);
                }
                newBlack = rating;
                changed = true;
                numChangedRatings++;
            }
        }
        if (changed) {
            numChangedGames++;
            if (updateIndexFile) {
                IndexEntry newIE = *ie;
                if (newWhite != 0) {
                    newIE.SetWhiteElo (newWhite);
                    newIE.SetWhiteRatingType (RATING_Elo);
                }
                if (newBlack != 0) {
                    newIE.SetBlackElo (newBlack);
                    newIE.SetBlackRatingType (RATING_Elo);
                }
                if (db->idx->WriteEntries (&newIE, gnum, 1) != OK) {
                    return errorResult (ti, "Error writing index file.");
                }
            }
        }
    }
    appendUintElement (ti, numChangedRatings);
    appendUintElement (ti, numChangedGames);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_name_read:
//   Reads a Scid name spelling file into memory, and returns a list of
//   four integers: the number of player, event, site and round names in
//   the file.
//   If there is no filename argument, sc_name_read just returns the same
//   list for the current spellchecker status without reading a new file.
int
sc_name_read (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc == 2) {
        for (nameT nt = NAME_FIRST; nt <= NAME_LAST; nt++) {
            uint numNames = 0;
            if (spellChecker[nt] != NULL) {
                numNames = spellChecker[nt]->NumCorrectNames();
            }
            appendUintElement (ti, numNames);
        }
        return TCL_OK;
    }

    if (argc > 5) {
        return errorResult (ti, "Usage: sc_name read <spellcheck-file> [-checkPlayerOrder <bool>]");
    }
    const char * filename = argv[2];
    bool checkPlayerOrder = false;
    if (argc == 5) {
        checkPlayerOrder = strGetBoolean (argv[4]);
    }

    for (nameT nt = NAME_FIRST; nt <= NAME_LAST; nt++) {
        SpellChecker * temp_spellChecker = new SpellChecker;
        temp_spellChecker->SetNameType (nt);

        // ReadSpellCheckFile gets called four times for some reason S.A &&&
        // But it doesn't seem too bad, as the OS caches the file after the first open.
        //
        // src/spellchk.cpp(sc_name_read): if (fp.Open (filename, FMODE_ReadOnly)
        // src/mfile.cpp: Handle = fopen (name, modeStr);

        if (temp_spellChecker->ReadSpellCheckFile (filename, checkPlayerOrder) != OK) {
            delete temp_spellChecker;
            Tcl_ResetResult (ti);
            return errorResult (ti, "Error reading name spellcheck file.");
        }
        if (spellChecker[nt] != NULL) { delete spellChecker[nt]; }
        spellChecker[nt] = temp_spellChecker;
        appendUintElement (ti, spellChecker[nt]->NumCorrectNames());
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// strPromoteSurname:
//    Copies source to target, rearranging it so the text after the
//    final space is moved to the start of the string and has that
//    space after it. Example: "aaa bbb ccc" -> "ccc aaa bbb". Useful
//    for promoting a surname from the end of a name to the start, as
//    long as the surname has no spaces in it.
void
strPromoteSurname (char * target, const char * source)
{
    const char * lastSpace = strLastChar (source, ' ');
    if (lastSpace == NULL) {
        strCopy (target, source);
        return;
    }
    const char * from = lastSpace + 1;
    while (*from != 0) {
        *target++ = *from++;
    }
    *target++ = ' ';
    from = source;
    while (from != lastSpace) {
        *target++ = *from++;
    }
    *target = 0;
    return;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_name_spellcheck:
//   Scan the current database for spelling corrections.
int
sc_name_spellcheck (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    nameT nt = NAME_INVALID;
    bool doSurnames = false;
    bool ambiguous = true;
    const char * usage = "Usage: sc_name spellcheck [-surnames <boolean>] [-ambiguous <boolean>] players|events|sites|rounds";

    const char * options[] = {
        "-surnames", "-ambiguous", NULL
    };
    enum {
        OPT_SURNAMES, OPT_AMBIGUOUS
    };

    int arg = 2;
    while (arg+1 < argc) {
        const char * option = argv[arg];
        const char * value = argv[arg+1];
        arg += 2;
        int index = -1;
        if (option[0] == '-') { index = strUniqueMatch (option, options); }

        switch (index) {
        case OPT_SURNAMES:
            doSurnames = strGetBoolean (value);
            break;
        case OPT_AMBIGUOUS:
            ambiguous = strGetBoolean (value);
            break;
        default:
            return InvalidCommand (ti, "sc_name spellcheck", options);
        }
    }
    if (arg+1 != argc) { return errorResult (ti, usage); }
    nt = NameBase::NameTypeFromString (argv[arg]);

    if (! NameBase::IsValidNameType (nt)) {
        return errorResult (ti, usage);
    }

    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (db->fileMode == FMODE_ReadOnly) {
        return errorResult (ti, errMsgReadOnly(ti));
    }
    if (spellChecker[nt] == NULL) {
        Tcl_AppendResult (ti, "A spellcheck file has not been loaded.\n\n",
                          "You can load one from the Options menu.", NULL);
        return TCL_ERROR;
    }

    NameBase * nb = db->nb;
    DString * dstr = new DString;
    char tempStr[1024];
    char tempName [1024];
    const char * prevCorrection = "";
    idNumberT id = 0;
    uint correctionCount = 0;

    // Check every name of the specified type:

    nb->IterateStart (nt);

    // progressbar
    bool showProgress = startProgressBar();
    uint updateStart, update;
    updateStart = update = 1000;  // Update progress bar every 1000 games
    idNumberT namesCount = 0;
    idNumberT namesTotal = nb->GetNumNames(nt);

    while (nb->Iterate (nt, &id) == OK) {
        namesCount++;
        if (showProgress) {  // Update the percentage done bar:
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, namesCount, namesTotal);
                if (interruptedProgress()) break;
            }
        }

        uint frequency = nb->GetFrequency (nt, id);
        // Do not bother trying to correct unused names:
        if (frequency == 0) { continue; }

        const char * name = nb->GetName (nt, id);
        const char * origName = name;

        if (nt == NAME_PLAYER  &&  !doSurnames  && strIsSurnameOnly (name)) {
            continue;
        }

        // First, check for a general prefix or suffix correction:
        int offset = 0;
        const char * replace;

        replace = spellChecker[nt]->CorrectPrefix (name, &offset);
        if (replace != NULL) {
                strCopy (tempName, replace);
                strAppend (tempName, &(name[offset]));
                sprintf (tempStr, "%s\"%s\"\t>> \"%s\" (%u)\n",
                         *origName == *prevCorrection ? "" : "\n",
                         origName, tempName, frequency);
                dstr->Append (tempStr);
                prevCorrection = origName;
            correctionCount++;
            continue;
        }

        replace = spellChecker[nt]->CorrectSuffix (name, &offset);
        if (replace != NULL) {
                strCopy (tempName, name);
                strCopy (tempName + offset, replace);
                sprintf (tempStr, "%s\"%s\"\t>> \"%s\" (%u)\n",
                         *origName == *prevCorrection ? "" : "\n",
                         origName, tempName, frequency);
                dstr->Append (tempStr);
                prevCorrection = origName;
            correctionCount++;
            continue;
        }

        int replacedLength = 0;
        replace = spellChecker[nt]->CorrectInfix (name, &offset, &replacedLength);
        if (replace != NULL) {
                strCopy (tempName, name);
                strCopy (tempName + offset, replace);
                strAppend (tempName, &(name[offset + replacedLength]));
                sprintf (tempStr, "%s\"%s\"\t>> \"%s\" (%u)\n",
                         *origName == *prevCorrection ? "" : "\n",
                         origName, tempName, frequency);
                dstr->Append (tempStr);
                prevCorrection = origName;
            correctionCount++;
            continue;
        }

        // If spellchecking names, remove any country code like " (USA)"
        // in parentheses at the end of the name:
        if (nt == NAME_PLAYER) {
            uint len = strLength (name);
            if (len > 6  &&  name[len-6] == ' '  &&  name[len-5] == '('  &&
                isupper(name[len-4])  &&  isupper(name[len-3])  &&
                isupper(name[len-2])  &&  name[len-1] == ')') {
                strCopy (tempName, name);
                tempName[len-6] = 0;
                name = tempName;
            }
        }

        const uint maxAmbiguous = 10;
        uint count = 0;
        const char * corrections [maxAmbiguous];
        count = spellChecker[nt]->Corrections (name, corrections, maxAmbiguous);

        if (nt == NAME_PLAYER  &&  count < maxAmbiguous) {
            // If correcting player names, also try the name with the text
            // after the last space moved to the start, e.g. "R. J. Fischer"
            // converted to "Fischer R. J.":
            strPromoteSurname (tempStr, name);
            count += spellChecker[nt]->Corrections (tempStr,
                          &(corrections[count]), maxAmbiguous - count);
            // The above step can cause duplicated corrections, so
            // remove them:
            for (uint i=1; i < count; i++) {
                if (strEqual (corrections[i], corrections[i-1])) {
                    for (uint j=i+1; j < count; j++) {
                        corrections[j-1] = corrections[j];
                    }
                    count--;
                }
            }
        }

        if (count > 1  &&  !ambiguous) { count = 0; }

        for (uint i=0; i < count; i++) {
            if (strEqual (origName, corrections[i])) { continue; }
            correctionCount++;

            // Add correction to output, with a blank line first if the
            // correction starts with a different character to the
            // previous correction:
            sprintf (tempStr, "%s%s\"%s\"\t>> \"%s\" (%u)",
                     *origName == *prevCorrection ? "" : "\n",
                     count > 1 ? "Ambiguous: " : "",
                     origName, corrections[i], frequency);
            dstr->Append (tempStr);
            if (nt == NAME_PLAYER) {
                // Look for a player birthdate:
                    const char * text =
                        spellChecker[nt]->GetCommentExact (corrections[i]);
                    dateT birthdate = SpellChecker::GetBirthdate(text);
                    dateT deathdate = SpellChecker::GetDeathdate(text);
                    if (birthdate != ZERO_DATE  ||  deathdate != ZERO_DATE) {
                        dstr->Append ("  ");
                        if (birthdate != ZERO_DATE) {
                            date_DecodeToString (birthdate, tempStr);
                            dstr->Append(tempStr);
                        }
                        dstr->Append ("--");
                        if (deathdate != ZERO_DATE) {
                            date_DecodeToString (deathdate, tempStr);
                            dstr->Append(tempStr);
                        }
                    }
                }
                dstr->Append ("\n");
                prevCorrection = origName;

        }
    }

    if (showProgress) { updateProgressBar (ti, 1, 1); }

    // Generate message

    sprintf (tempStr, "Scid found %u %s name correction%s.",
             correctionCount, NAME_TYPE_STRING[nt],
             strPlural (correctionCount));
    Tcl_AppendResult (ti, tempStr, NULL);
    Tcl_AppendResult (ti, "\n", dstr->Data(), NULL);

    delete dstr;
    return TCL_OK;
}



//////////////////////////////////////////////////////////////////////
//  OPENING/PLAYER REPORT functions

static uint
avgGameLength (resultT result)
{
    uint sum = 0;
    uint count = 0;
    for (uint i=0; i < db->numGames; i++) {
        IndexEntry * ie = db->idx->FetchEntry(i);
        if (result == ie->GetResult()) {
            count++;
            sum += ((ie->GetNumHalfMoves() + 1) / 2);
        }
    }
    if (count == 0) { return 0; }
    return (sum + (count/2)) / count;
}

int
sc_report (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "avgLength", "best", "counts", "create", "eco", "elo",
        "endmaterial", "format", "frequency", "line", "max", "moveOrders",
        "notes", "players", "print", "score", "select", "themes", NULL
    };
    enum {
        OPT_AVGLENGTH, OPT_BEST, OPT_COUNTS, OPT_CREATE, OPT_ECO, OPT_ELO,
        OPT_ENDMAT, OPT_FORMAT, OPT_FREQ, OPT_LINE, OPT_MAX, OPT_MOVEORDERS,
        OPT_NOTES, OPT_PLAYERS, OPT_PRINT, OPT_SCORE, OPT_SELECT, OPT_THEMES
    };

    static const char * usage =
        "Usage: sc_report opening|player <command> [<options...>]";
    OpTable * report = NULL;
    if (argc < 2) {
        return errorResult (ti, usage);
    }
    switch (argv[1][0]) {
        case 'O': case 'o':  report = reports[REPORT_OPENING]; break;
        case 'P': case 'p':  report = reports[REPORT_PLAYER]; break;
        default:
            return errorResult (ti, usage);
    }

    DString * dstr = NULL;
    int index = strUniqueMatch (argv[2], options);

    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (index >= 0  &&  index != OPT_CREATE  &&  report == NULL) {
        return errorResult (ti, "No report has been created yet.");
    }

    if (index != OPT_CREATE && report->GetBase() != (uint) currentBase) {
      return errorResult (ti, "sc_report: please select report base.");
      // Automatically switching requires updating gui
      // db = &(dbList[report->GetBase()]);
    }

    switch (index) {
    case OPT_AVGLENGTH:
        if (argc != 4) {
            return errorResult (ti, "Usage: sc_report player|opening avgLength 1|=|0|*");
        } else {
            resultT result = strGetResult (argv[3]);
            appendUintElement (ti, report->AvgLength (result));
            appendUintElement (ti, avgGameLength (result));
        }
        break;

    case OPT_BEST:
        if (argc != 5) {
            return errorResult (ti, "Usage: sc_report opening|player best w|b|a|o|n <count>");
        }
        dstr = new DString;
        report->BestGames (dstr, strGetUnsigned(argv[4]), argv[3]);
        Tcl_AppendResult (ti, dstr->Data(), NULL);
        break;

    case OPT_COUNTS:
        appendUintElement (ti, report->GetTotalCount());
        appendUintElement (ti, report->GetTheoryCount());
        break;

    case OPT_CREATE:
        return sc_report_create (cd, ti, argc, argv);

    case OPT_ECO:
        if (argc > 3) {
            dstr = new DString();
            report->TopEcoCodes (dstr, strGetUnsigned(argv[3]));
            Tcl_AppendResult (ti, dstr->Data(), NULL);
        } else {
            Tcl_AppendResult (ti, report->GetEco(), NULL);
        }
        break;

    case OPT_ELO:
        if (argc != 4) {
            return errorResult (ti, "Usage: sc_report opening|player elo white|black");
        } else {
            colorT color = WHITE;
            uint count = 0;
            uint pct = 0;
            uint perf = 0;
            if (argv[3][0] == 'B'  ||  argv[3][0] == 'b') { color = BLACK; }
            uint avg = report->AvgElo (color, &count, &pct, &perf);
            appendUintElement (ti, avg);
            appendUintElement (ti, count);
            appendUintElement (ti, pct);
            appendUintElement (ti, perf);
        }
        break;

    case OPT_ENDMAT:
        dstr = new DString;
        report->EndMaterialReport (dstr,
                       translate (ti, "OprepReportGames", "Report games"),
                       translate (ti, "OprepAllGames", "All games"));
        Tcl_AppendResult (ti, dstr->Data(), NULL);
        break;

    case OPT_FORMAT:
        if (argc != 4) {
            return errorResult (ti, "Usage: sc_report opening|player format latex|html|text|ctext");
        }
        report->SetFormat (argv[3]);
        break;

    case OPT_FREQ:
        if (argc != 4) {
            return errorResult (ti, "Usage: sc_report opening|player frequency 1|=|0|*");
        } else {
            resultT result = strGetResult (argv[3]);
            appendUintElement (ti, report->PercentFreq (result));
            uint freq = db->stats.nResults[result] * 1000;
            freq = freq / db->numGames;
            appendUintElement (ti, freq);
        }
        break;

    case OPT_LINE:
        dstr = new DString;
        report->PrintStemLine (dstr);
        Tcl_AppendResult (ti, dstr->Data(), NULL);
        break;

    case OPT_MAX:
        if (argc == 4  &&  argv[3][0] == 'g') {
            return setUintResult (ti, OPTABLE_MAX_TABLE_LINES);
        } else if (argc == 4  &&  argv[3][0] == 'r') {
            return setUintResult (ti, OPTABLE_MAX_ROWS);
        }
        return errorResult (ti, "Usage: sc_report opening|player max games|rows");

    case OPT_MOVEORDERS:
        if (argc != 4) {
            return errorResult (ti, "Usage: sc_report opening|player moveOrders <count>");
        }
        dstr = new DString;
        report->PopularMoveOrders (dstr, strGetUnsigned(argv[3]));
        Tcl_AppendResult (ti, dstr->Data(), NULL);
        break;

    case OPT_NOTES:
        if (argc < 4  ||  argc > 5) {
            return errorResult (ti, "Usage: sc_report opening|player notes <0|1> [numrows]");
        }
        report->ClearNotes ();
        if (strGetBoolean (argv[3])  &&  report->GetNumLines() > 0) {
            report->GuessNumRows ();
            if (argc > 4) {
                uint nrows = strGetUnsigned (argv[4]);
                if (nrows > 0) { report->SetNumRows (nrows); }
            }
            dstr = new DString;
            // Print the table just to set up notes, but there is
            // no need to return the result:
            report->PrintTable (dstr, "", "");
        }
        break;

    case OPT_PLAYERS:
        if (argc != 5) {
            return errorResult (ti, "Usage: sc_report opening|player players w|b <count>");
        } else {
            colorT color = WHITE;
            if (argv[3][0] == 'B'  ||  argv[3][0] == 'b') { color = BLACK; }
            dstr = new DString;
            report->TopPlayers (dstr, color, strGetUnsigned(argv[4]));
            Tcl_AppendResult (ti, dstr->Data(), NULL);
        }
        break;

    case OPT_PRINT:
        if (argc < 3  ||  argc > 6) {
            return errorResult (ti, "Usage: sc_report opening|players print [numrows] [title] [comment]");
        }
        report->GuessNumRows ();
        if (argc > 3) {
            uint nrows = strGetUnsigned (argv[3]);
            if (nrows > 0) { report->SetNumRows (nrows); }
        }
        dstr = new DString;
        report->PrintTable (dstr, argc > 4 ? argv[4] : "",
                             argc > 5 ? argv[5] : "");
        Tcl_AppendResult (ti, dstr->Data(), NULL);
        break;

    case OPT_SCORE:
        appendUintElement (ti, report->PercentScore());
        {
            uint percent = db->stats.nResults[RESULT_White] * 2;
            percent += db->stats.nResults[RESULT_Draw];
            percent = percent * 500;
            uint sum = (db->stats.nResults[RESULT_White] +
                                 db->stats.nResults[RESULT_Draw] +
                                 db->stats.nResults[RESULT_Black]);
            if (sum != 0)
            	percent = percent / sum;
            	else
            	percent = 0;
            appendUintElement (ti, percent);
        }
        break;

    case OPT_SELECT:
        return sc_report_select (cd, ti, argc, argv);

    case OPT_THEMES:
        dstr = new DString;
        report->ThemeReport (dstr, argc - 3, (const char **) argv + 3);
        Tcl_AppendResult (ti, dstr->Data(), NULL);
        break;

    default:
        return InvalidCommand (ti, "sc_report", options);
    }

    if (dstr != NULL) { delete dstr; }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_report_create:
//    Creates a new opening table.
//    NOTE: It assumes the filter contains the results
//    of a tree search for the current position, so
//    the Tcl code that calls this need to ensure that
//    is done first.
int
sc_report_create (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    uint maxThemeMoveNumber = 20;
    uint maxExtraMoves = 1;
    // maxTableLines corresponds to maxTableGames in tcl, maxLines corresponds to maxGames.
    uint maxTableLines = OPTABLE_MAX_TABLE_LINES;
    uint maxLines = OPTABLE_MAX_LINES;
    static const char * usage =
        "Usage: sc_report opening|player create [maxExtraMoves] [maxTableGames] [maxGames] [excludeMove]";

    uint reportType = 0;
    if (argc < 2) {
        return errorResult (ti, usage);
    }

    switch (argv[1][0]) {
        case 'O': case 'o':  reportType = REPORT_OPENING; break;
        case 'P': case 'p':  reportType = REPORT_PLAYER; break;
        default:
            return errorResult (ti, usage);
    }

    if (argc > 3) {
        maxExtraMoves = strGetUnsigned (argv[3]);
    }
    if (argc > 4) {
        maxTableLines = strGetUnsigned (argv[4]);
        if (maxTableLines > OPTABLE_MAX_TABLE_LINES) {
            printf ("maxTableLines %u reduced to limit of %u\n",maxTableLines,OPTABLE_MAX_TABLE_LINES);
            maxTableLines = OPTABLE_MAX_TABLE_LINES;
        }
        if (maxTableLines == 0) { maxTableLines = 1; }
    }
    if (argc > 5) {
        maxLines = strGetUnsigned (argv[5]);
        if (maxLines > OPTABLE_MAX_LINES) {
            printf ("maxLines %u reduced to limit of %u\n",maxLines,OPTABLE_MAX_LINES);
            maxLines = OPTABLE_MAX_LINES;
        }
        if (maxLines == 0) { maxLines = 1; }
    }
    const char * excludeMove = "";
    if (argc > 6) { excludeMove = argv[6]; }

    if (strcmp (excludeMove, "none") == 0) { excludeMove = ""; }

    bool showProgress = startProgressBar();
    if (reports[reportType] != NULL) {
        delete reports[reportType];
    }
    OpTable * report = new OpTable (currentBase, reportTypeName[reportType], db->game, ecoBook);
    reports[reportType] = report;
    report->SetMaxTableLines (maxTableLines);
    report->SetMaxLines (maxLines);
    report->SetExcludeMove (excludeMove);
    report->SetDecimalChar (decimalPointChar);
    report->SetMaxThemeMoveNumber (maxThemeMoveNumber);

    uint updateStart, update;
    updateStart = update = 2000;  // Update progress bar every 2000 games

    for (uint gnum=0; gnum < db->numGames; gnum++) {
        if (showProgress) {  // Update the percentage done bar:
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, gnum, db->numGames);
                if (interruptedProgress()) { break; }
            }
        }
        byte ply = db->dbFilter->Get(gnum);
        IndexEntry * ie = db->idx->FetchEntry (gnum);
        if (ply != 0) {
            if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(), ie->GetLength()) != OK) {
                return errorResult (ti, "Error reading game file.");
            }
            if (scratchGame->Decode (db->bbuf, GAME_DECODE_ALL) != OK) {
                return errorResult (ti, "Error decoding game.");
            }
            scratchGame->LoadStandardTags (ie, db->nb);
            scratchGame->MoveToPly (ply - 1);
            if (scratchGame->AtEnd()) {
                ply = 0;
                db->dbFilter->Set (gnum, 0);
            }
            if (ply != 0) {
                uint moveOrderID = report->AddMoveOrder (scratchGame);
                OpLine * line = new OpLine (scratchGame, ie, gnum+1, maxExtraMoves, maxThemeMoveNumber);
                report->Add (line);
                line->SetMoveOrderID (moveOrderID);
            }
        }
        report->AddEndMaterial (ie->GetFinalMatSig(), (ply != 0));
    }
    if (showProgress) { updateProgressBar (ti, 1, 1); }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_report_select:
//    Restricts the filter to only contain games
//    in the opening report matching the specified
//    opening/endgame theme or note number.
int
sc_report_select (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * usage =
        "Usage: sc_report opening|player select <eg|note|mo|theme|all> <number>";
    if (argc != 5) {
        return errorResult (ti, usage);
    }
    OpTable * report = NULL;
    switch (argv[1][0]) {
        case 'O': case 'o':  report = reports[REPORT_OPENING]; break;
        case 'P': case 'p':  report = reports[REPORT_PLAYER]; break;
        default:
            return errorResult (ti, usage);
    }

    char type = tolower (argv[3][0]);
    uint number = strGetUnsigned (argv[4]);

    uint * matches = report->SelectGames (type, number);
    uint * match = matches;
    db->dbFilter->Fill (0);
    while (*match != 0) {
        uint gnum = *match - 1;
        match++;
        uint ply = *match + 1;
        match++;
        db->dbFilter->Set (gnum, ply);
    }
    setMainFilter(db);

    delete[] matches;

    return TCL_OK;
}


//////////////////////////////////////////////////////////////////////
//  SEARCH and TREE functions

int
sc_tree (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "best", "move", "positions", "search", "size", "time",  "write", "free",
        "cachesize", "cacheinfo", "clean", NULL
    };
    enum {
        TREE_BEST, TREE_MOVE, TREE_POSITIONS, TREE_SEARCH, TREE_SIZE,
        TREE_TIME, TREE_WRITE, TREE_FREE, TREE_CACHESIZE, TREE_CACHEINFO, TREE_CLEAN
    };

    int index = -1;
    if (argc > 1) { index = strUniqueMatch (argv[1], options); }

    switch (index) {
    case TREE_BEST:
        return sc_tree_best (cd, ti, argc, argv);

    case TREE_CLEAN:
        return sc_tree_clean (cd, ti, argc, argv);

    case TREE_MOVE:
        return sc_tree_move (cd, ti, argc, argv);

    case TREE_POSITIONS:
        // Return the number of positions cached:
        return setUintResult (ti, db->treeCache->UsedSize());

    case TREE_SEARCH:
        return sc_tree_search (cd, ti, argc, argv);

    case TREE_SIZE:
        return setUintResult (ti, db->treeCache->Size());

    case TREE_TIME:
        return sc_tree_time (cd, ti, argc, argv);

    case TREE_WRITE:
        return sc_tree_write (cd, ti, argc, argv);

    case TREE_FREE:
        return sc_tree_free (cd, ti, argc, argv);

    case TREE_CACHESIZE:
        return sc_tree_cachesize (cd, ti, argc, argv);

    case TREE_CACHEINFO:
        return sc_tree_cacheinfo (cd, ti, argc, argv);

    default:
        return InvalidCommand (ti, "sc_tree", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_tree_best:
//    Returns a list of the best games in the current tree filter.
int
sc_tree_best (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 7) {
        return errorResult (ti, "Usage: sc_tree best <baseNum> <count> <results> <sort> <formatStr>");
    }

    scidBaseT * base = db;
    int baseNum = strGetInteger (argv[2]);
    if (baseNum >= 1  &&  baseNum <= MAX_BASES) {
        base = &(dbList[baseNum - 1]);
    }
    if (!base->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    bool results [NUM_RESULT_TYPES] = { false };

    uint maxGames = strGetUnsigned (argv[3]);
    if (maxGames > 1000) { maxGames = 1000; }
    if (maxGames == 0) { return TCL_OK; }

    const char * rstr = argv[4];
    if (strContains (rstr, "1-0")) { results[RESULT_White] = true; }
    if (strContains (rstr, "0-1")) { results[RESULT_Black] = true; }
    if (strContains (rstr, "1/2")) { results[RESULT_Draw] = true; }
    if (strContains (rstr, "="))   { results[RESULT_Draw] = true; }
    if (strContains (rstr, "*"))   { results[RESULT_None] = true; }
    if (strCaseEqual (rstr, "all")) {
        results[RESULT_White] = results[RESULT_Black] = true;
        results[RESULT_Draw] = results[RESULT_None] = true;
    }

    // "Best" or "Order"
    bool sortBest = (argv[5][0] == 'B') ;

    uint count = 0;
    uint * bestIndex = new uint [maxGames];
    uint * bestElo = new uint [maxGames];
    uint tmp;
    IndexEntry * ie;
    uint insert;

    if (sortBest) {
        for (uint gnum=0; gnum < base->numGames; gnum++) {
            if (base->treeFilter->Get(gnum) == 0) { continue; }
            ie = base->idx->FetchEntry (gnum);
            if (! results [ie->GetResult()]) { continue; }
            eloT welo = ie->GetWhiteElo();
            eloT belo = ie->GetBlackElo();
            if (welo == 0) { welo = base->nb->GetElo (ie->GetWhite()); }
            if (belo == 0) { belo = base->nb->GetElo (ie->GetBlack()); }
            uint avg = (welo + belo) / 2;

            // Start at end of best list and work up as far as possible:
            insert = count;
            while (insert > 0) {
                if (bestElo[insert-1] >= avg) { break; }
                insert--;
            }
            if (insert < maxGames) {
                // Move all lower-rated games down one place:
                for (tmp=count; tmp > insert; tmp--) {
                    if (tmp >= maxGames) { continue; }
                    bestElo[tmp] = bestElo[tmp-1];
                    bestIndex[tmp] = bestIndex[tmp-1];
                }
                // Add details for this game:
                bestElo[insert] = avg;
                bestIndex[insert] = gnum;
                count++;
                if (count > maxGames) { count = maxGames; }
            }
        }
    } else {
        insert = 0;
        // Insert the last maxGames games
        // then reverse the order so they appear the same as in the gamelist
        for (uint gnum = base->numGames ; gnum > 0 ; ) {
            gnum--;
            if (base->treeFilter->Get(gnum) == 0)
              continue;
            ie = base->idx->FetchEntry (gnum);
            if (! results [ie->GetResult()])
              continue;
            bestIndex[insert++] = gnum;
            count++;
            if (count == maxGames)
              break;
        }
        for (uint i=0; i < count/2; i++) {
            tmp = bestIndex[i];
            bestIndex[i] = bestIndex[count-1-i];
            bestIndex[count-1-i] = tmp;
        }
    }

    // Now generate the Tcl list of best game details:
    const char * formatStr = argv[6];
    bool nextMoves = 0;

    // '!' prefix in formatStr indicates nextMoves should be processed.
    if (formatStr[0] == '!') {
        nextMoves = 1;
        formatStr++;
    }

    char temp[2048];
    char tempStr[128];

    DString * moveStr = new DString;
    Game * g = scratchGame;
    // We are using ply from current game (which may not be in the same db)
    uint gamePly = db->game->GetCurrentPly();
    bool gameNonStd = db->game->HasNonStandardStart();

    for (uint i=0; i < count; i++) {
        ie = base->idx->FetchEntry (bestIndex[i]);

        // We need the gamenumber for the tree(bestList) and gbrowser
        sprintf (tempStr, "%u ", bestIndex[i] + 1);

	if (nextMoves) {
        if (ie->GetLength() == 0) {
            // todo
        } else {
            if (base->gfile->ReadGame (base->bbuf, ie->GetOffset(), ie->GetLength()) != OK) {
                printf ("mybadbest handling game %u\n", bestIndex[i]);
            } else {
                g->Clear();
                if (g->Decode (base->bbuf, GAME_DECODE_NONE) != OK) {
                  printf ("mybadbest decoding game %u\n", bestIndex[i]);
                } else {
                  moveStr->Clear();
                  // Copied from "Cases to consider" above

                  uint index = bestIndex[i];
                  uint ply;

		  if ((int)index == base->gameNumber) {
		    ply = gamePly;
                  } else {
                    ply = base->treeFilter->Get(index) - 1;
                    if (!(gameNonStd || ie->GetStartFlag() || ply == gamePly)) {
                        // Check for repetition - move to gamePly and check if position is the same
                        Position tempPos;
                        tempPos.CopyFrom(base->game->GetCurrentPos());
                        g->MoveToPly(gamePly);
                        if (gamePly == g->GetCurrentPly() &&  \
                            g->GetCurrentPos()->Compare(&tempPos) == EQUAL_TO) {
                          ply = gamePly;
                        }
                     }
                  }

                  // Show arbitary 3 moves (6 ply)
                  g->GetPartialMoveList (moveStr, ply, 6);
                }
            }
        }
	}

        // This seems solid, but we should be wary, as in sc_game_list PrintGameInfo
        // is only used on current base, but here we are using it for any open base
	ie->PrintGameInfo (temp, 0, bestIndex[i]+1, base->nb, formatStr, moveStr->Data());
	Tcl_AppendResult (ti, tempStr, temp, "\n", NULL);
    }

    delete moveStr;
    delete[] bestIndex;
    delete[] bestElo;

    return TCL_OK;
}


int
sc_tree_clean (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc < 3 || argc > 4) {
        return errorResult (ti, "Usage: sc_tree clean <baseNum> [updateFilter]");
    }

    scidBaseT * base = db;
    int baseNum = strGetInteger (argv[2]);
    if (baseNum >= 1  &&  baseNum <= MAX_BASES) {
        base = &(dbList[baseNum - 1]);
    }
    if (!base->inUse) {
        return setResult (ti, errMsgNotOpen(ti));
    }

    if (argc == 4) {
      // This should really be in it's own proc, but easier to overload sc_tree_clean
      // Copy treeFilter back to dbFilter only

      if(base->treeFilter && base->dbFilter != base->filter) {
	for (uint i=0; i < base->numGames; i++) {
	  base->dbFilter->Set(i,base->filter->Get(i));
	}
	return TCL_OK;
      }
      return setResult (ti, errMsgNotOpen(ti));
    }

    if(base->treeFilter) {
        delete base->treeFilter;
        base->treeFilter = NULL;
    }

    if( base->dbFilter && base->dbFilter != base->filter) {
        for (uint i=0; i < base->numGames; i++)
            base->filter->Set( i, base->dbFilter->Get(i));

        delete base->dbFilter;
        base->dbFilter = base->filter;
    }

    return TCL_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_tree_move:
//    Returns the move for a tree line.
//    Arg can be in the range [1.. numTreeLines].
//    It can also be "random" to request a random move selected
//    according to the frequency of each move in the tree.
int
sc_tree_move (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 4) {
        return errorResult (ti, "Usage: sc_tree move <baseNum> <lineNum>");
    }

    scidBaseT * base = db;
    int baseNum = strGetInteger (argv[2]);
    if (baseNum >= 1  &&  baseNum <= MAX_BASES) {
        base = &(dbList[baseNum - 1]);
    }
    if (!base->inUse) {
        return setResult (ti, errMsgNotOpen(ti));
    }

    int selection = strGetInteger (argv[3]);
    if (argv[3][0] == 'r'  &&  strIsPrefix (argv[3], "random")) {
        uint total = base->tree.totalCount;
        if (total == 0) { return TCL_OK; }
        uint r = random32() % total;
        uint sum = 0;
        for (uint i=0; i < base->tree.moveCount; i++) {
            sum += base->tree.node[i].total;
            if (r <= sum) {
                selection = i + 1;
                break;
            }
        }
    }

    if (selection < 1  ||  selection > (int)(base->tree.moveCount)) {
        // Not a valid selection. We ignore it (e.g. the user clicked on a
        // line with no move on it).
        return TCL_OK;
    }

    treeNodeT * node = &(base->tree.node[selection - 1]);

    // If the san string first char is not a letter, it is the
    // empty move (e.g. "[end]") so we do NOT add a move:
    if (! isalpha(node->san[0])) {
        return TCL_OK;
    }

    Tcl_AppendResult (ti, node->san, NULL);
    return TCL_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_tree_time:
//    Returns the elapsed time in millliseconds for the last tree search.
int
sc_tree_time (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * base = db;
    if (argc == 3) {
        int baseNum = strGetInteger (argv[2]);
        if (baseNum >= 1  &&  baseNum <= MAX_BASES) {
            base = &(dbList[baseNum - 1]);
        }
    }

    return setUintResult (ti, base->treeSearchTime);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_tree_write:
//    Writes the tree cache file for the specified database, which
//    defaults to the current base.
int
sc_tree_write (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    scidBaseT * base = db;
    if (argc == 3) {
        int baseNum = strGetInteger (argv[2]);
        if (baseNum >= 1  &&  baseNum <= MAX_BASES) {
            base = &(dbList[baseNum - 1]);
        }
    }

    if (!base->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }
    if (base->memoryOnly) {
        // Memory-only file, so ignore.
        return TCL_OK;
    }

    if (base->treeCache->WriteFile (base->fileName) != OK) {
        return errorResult (ti, "Error writing Scid tree cache file.");
    }
    return setUintResult (ti, fileSize (base->fileName, TREEFILE_SUFFIX));
}

// Enumeration of possible move-sorting methods for tree mode:
enum moveSortE { SORT_ALPHA, SORT_ECO, SORT_FREQUENCY, SORT_SCORE };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sortTreeMoves():
//    Sorts the moves of a tree node according to a specified order.
//    Slow sort method, but the typical number of moves is under 20
//    so it is easily fast enough.
void
sortTreeMoves (treeT * tree, int sortMethod, colorT toMove)
{
    // Only sort if there are at least two moves in the tree node:
    if (tree->moveCount <= 1) { return; }

    for (uint outer=0; outer < tree->moveCount - 1; outer++) {
        for (uint inner=outer+1; inner < tree->moveCount; inner++) {
            int result = 0;

            switch (sortMethod) {
            case SORT_FREQUENCY:  // Most frequent moves first:
                result = tree->node[outer].total - tree->node[inner].total;
                break;

            case SORT_ALPHA:  // Alphabetical order:
                result = strCompare (tree->node[inner].san,
                                     tree->node[outer].san);
                break;

            case SORT_ECO:  // ECO code order:
                result = tree->node[outer].ecoCode - tree->node[inner].ecoCode;
                break;

            case SORT_SCORE:  // Order by success:
                result = tree->node[outer].score - tree->node[inner].score;
                if (toMove == BLACK) { result = -result; }
                break;

            default:  // Unreachable:
                return;
            }

            if (result < 0) {
                // Swap the nodes:
                treeNodeT temp = tree->node[outer];
                tree->node[outer] = tree->node[inner];
                tree->node[inner] = temp;
            }
        }  // for (inner)
    } // for (outer)
    return;
}

// Returns the tree for the current position

int
sc_tree_search (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * usageStr =
	"Usage: sc_tree search [-hideMoves <0|1>] [-sort alpha|eco|frequency|score] [-time <0|1>] [-epd <0|1>] [-list <0|1>] [-fastmode <0|1>] [-adjust <0|1>] [-short <0|1>] [-base <baseNumber>]";

      // Sort options: these should match the moveSortE enumerated type.
      static const char * sortOptions[] = {
	  "alpha", "eco", "frequency", "score", NULL
      };

      char tempTrans[10];

      bool hideMoves = false;
      bool shortDisplay = false;
      //  bool showTimeStats = true; // Stats are disabled at the moment
      bool showEpdData = true;
      bool listMode = false;
      bool adjustMode = false;
      bool fastMode = false;
      int sortMethod = SORT_FREQUENCY; // default move order: frequency

      scidBaseT * base = db;
      db->bbuf->Empty();

      static std::set<scidBaseT**> search_pool;

      // Check that there is an even number of optional arguments and
      // parse them as option-value pairs:
      int arg = 2;
      int argsLeft = (argc - arg);
      if (argsLeft % 2 != 0) { return errorResult (ti, usageStr); }

      while (arg < argc) {
	if (strIsPrefix (argv[arg], "-sort")) {
	    sortMethod = strUniqueMatch (argv[arg+1], sortOptions);
	} else if (strIsPrefix (argv[arg], "-hideMoves")) {
	    hideMoves = strGetBoolean (argv[arg+1]);
	} else if (strIsPrefix (argv[arg], "-shortDisplay")) {
            shortDisplay = strGetBoolean (argv[arg+1]);
        } else if (strIsPrefix (argv[arg], "-base")) {
            int baseNum = strGetInteger (argv[arg+1]);
            if (baseNum >= 1  &&  baseNum <= MAX_BASES) {
                base = &(dbList[baseNum - 1]);
            }
        } else if (strIsPrefix (argv[arg], "-time")) {
            // showTimeStats = strGetBoolean (argv[arg+1]); // Stats are turned off at the moment
        } else if (strIsPrefix (argv[arg], "-epd")) {
            showEpdData = strGetBoolean (argv[arg+1]);
        } else if (strIsPrefix (argv[arg], "-list")) {
            listMode = strGetBoolean (argv[arg+1]);
        } else if (strIsPrefix (argv[arg], "-adjust")) {
            adjustMode = strGetBoolean (argv[arg+1]);
        } else if (strIsPrefix (argv[arg], "-fastmode")) {
            fastMode = strGetBoolean (argv[arg+1]);
        } else if (strIsPrefix (argv[arg], "-cancel")) {
            search_pool.clear();
            return TCL_OK;
        } else {
            return errorResult (ti, usageStr);
        }
        arg += 2;
    }

    if (sortMethod < 0) { return errorResult (ti, usageStr); }
    if (!base->inUse) { return setResult (ti, errMsgNotOpen(ti)); }

    search_pool.insert(&base);

    if( base->treeFilter == NULL)
    {
	    base->dbFilter = base->filter->Clone();
	    base->treeFilter = new Filter( base->numGames);
    }

    bool showProgress = startProgressBar();

    Timer timer;  // Start timing this search.

    uint skipcount = 0;
    cachedTreeT * pct;
    bool foundInCache = false;

    // 1. Cache Search

    // Check if there is a TreeCache file to open
    base->treeCache->ReadFile (base->fileName);

    // Lookup the cache before searching
    pct = base->treeCache->Lookup (db->game->GetCurrentPos());
    if (pct != NULL) {
    	// It was in the cache! Use it to save time:
    	if (pct->cfilter->Size() == base->numGames) {
            if (pct->cfilter->UncompressTo (base->treeFilter) == OK) {
                base->tree = pct->tree;
                foundInCache = true;
            }
	}
    }

    // Lookup the backup cache which is useful for storing recent nodes when the main disk-file cache is full
    if (! foundInCache) {
	pct = base->backupCache->Lookup (db->game->GetCurrentPos());
	if (pct != NULL) {
	    // It was in the backup cache! Use it to save time
	    if (pct->cfilter->Size() == base->numGames) {
		if (pct->cfilter->UncompressTo (base->treeFilter) == OK) {
		    base->tree = pct->tree;
		    foundInCache = true;
		}
	    }
	}
    }

    // we went back before the saved filter data
    if (base->treeFilter->oldDataTreePly > db->game->GetCurrentPly()) {
	base->treeFilter->isValidOldDataTree = false;
    }

    if ( foundInCache && fastMode ) {
	base->treeFilter->saveFilterForFastMode (db->game->GetCurrentPly());
    }

    if (!foundInCache) {
    	// Not in the cache, so do the search

    	// 2. Set vars
    	Position* pos = db->game->GetCurrentPos();
    	treeT* tree = &(base->tree);
    	tree->moveCount = tree->totalCount = 0;
    	matSigT msig = matsig_Make (pos->GetMaterial());
    	uint hpSig = pos->GetHPSig();
    	simpleMoveT sm;
    	base->treeFilter->Fill (0); // Reset the filter to be empty
    	skipcount = 0;
    	uint updateStart = 5000;  // Update progress bar every 5000 games
    	uint update = 1;

    	// 3. Set up the stored line code matches
    	StoredLine stored_line(pos);

    	// 4. Search through each game
    	for (uint i=0; i < base->numGames; i++) {
	    if (showProgress) {
		    update--;
		    if (update == 0) {
			    update = updateStart;
			    // updateProgressBar does a full update here to keep UI responsive
			    updateProgressBar (ti, i, base->numGames);
			    if (interruptedProgress()) {
				    Tcl_SetResult (ti, (char *) "interrupted", TCL_STATIC);
				    return TCL_OK;
			    }
			    if (search_pool.count(&base) == 0) {
				    Tcl_SetResult (ti, (char *) "canceled", TCL_STATIC);
				    return TCL_OK;
			    }
		    }
	    }

	    const byte * oldFilterData = base->treeFilter->GetOldDataTree();

	    // if the game is not already in the filter, continue
	    if (fastMode && base->treeFilter->isValidOldDataTree && oldFilterData[i] == 0)
		continue;

	    IndexEntry* ie = base->idx->FetchEntry (i);
	    if (ie->GetLength() == 0) {
		skipcount++;
		continue;
	    }

	    // We do not skip deleted games, so next line is commented out:
	    // if (ie->GetDeleteFlag()) { skipcount++; continue; }

	    bool foundMatch = false;
	    uint ply = 0;

	    // Check the stored line result for this game:
	    if (! stored_line.CanMatch(ie->GetStoredLineCode(), &ply, &sm)) {
		    skipcount++;
		    continue;
	    }
	    if (ply > 0) foundMatch = true;

	    pieceT * bd = pos->GetBoard();
	    bool isStartPos =
		   (bd[A1]==WR  &&  bd[B1]==WN  &&  bd[C1]==WB  &&  bd[D1]==WQ  &&
		    bd[E1]==WK  &&  bd[F1]==WB  &&  bd[G1]==WN  &&  bd[H1]==WR  &&
		    bd[A2]==WP  &&  bd[B2]==WP  &&  bd[C2]==WP  &&  bd[D2]==WP  &&
		    bd[E2]==WP  &&  bd[F2]==WP  &&  bd[G2]==WP  &&  bd[H2]==WP  &&
		    bd[A7]==BP  &&  bd[B7]==BP  &&  bd[C7]==BP  &&  bd[D7]==BP  &&
		    bd[E7]==BP  &&  bd[F7]==BP  &&  bd[G7]==BP  &&  bd[H7]==BP  &&
		    bd[A8]==BR  &&  bd[B8]==BN  &&  bd[C8]==BB  &&  bd[D8]==BQ  &&
		    bd[E8]==BK  &&  bd[F8]==BB  &&  bd[G8]==BN  &&  bd[H8]==BR);

	    if (!isStartPos  &&  ie->GetNumHalfMoves() == 0) {
		    skipcount++;
		    continue;
	    }

	    if (!foundMatch  &&  ! ie->GetStartFlag()) {
		    // Speedups that only apply to standard start games:
		    if (hpSig != HPSIG_StdStart) { // Not the start mask
			    if (! hpSig_PossibleMatch(hpSig, ie->GetHomePawnData())) {
				    skipcount++;
				    continue;
			    }
		    }
	    }

	    if (!foundMatch  &&  msig != MATSIG_StdStart
		   &&  !matsig_isReachable (msig, ie->GetFinalMatSig(),
					    ie->GetPromotionsFlag(),
					    ie->GetUnderPromoFlag()))
	    {
		    skipcount++;
		    continue;
	    }

	    char errorStr[100];
	    if (! foundMatch) {
		    if (base->gfile->ReadGame (base->bbuf, ie->GetOffset(), ie->GetLength()) != OK) {
			    search_pool.erase(&base);
			    sprintf (errorStr, "Tree search: error reading game %u.", i+1);
			    return errorResult (ti, errorStr);
		    }
		    Game *g = scratchGame;
		    if (g->ExactMatch (pos, base->bbuf, &sm)) {
			    ply = g->GetCurrentPly() + 1;
			    //if (ply > 255) { ply = 255; }
			    foundMatch = true;
			    //base->filter->Set (i, (byte) ply);
		    }
	    }

	    // If match was found, add it to the list of found moves
	    if (foundMatch) {
		    if (ply > 255) { ply = 255; }
		    base->treeFilter->Set (i, (byte) ply);
		    uint search;
		    treeNodeT* node = tree->node;
		    for (search = 0; search < tree->moveCount; search++, node++) {
			    if (sm.from == node->sm.from
				&&  sm.to == node->sm.to
				&&  sm.promote == node->sm.promote) {
				    break;
			    }
		    }

		    // Now node is the node to update or add.
		    // Check for exceeding max number of nodes:
		    if (search >= MAX_TREE_NODES) {
			    search_pool.erase(&base);
			    return errorResult (ti, "Too many moves.");
		    }

		    if (search == tree->moveCount) {
			    // A new move to add:
			    initTreeNode (node);
			    node->sm = sm;
			    if (sm.from == NULL_SQUARE) {
				    strCopy(node->san, "[end]");
			    } else {
				    pos->MakeSANString (&sm, node->san, SAN_CHECKTEST);
			    }
			    tree->moveCount++;
		    }
		    node->total++;
		    node->freq[ie->GetResult()]++;
		    eloT elo = 0;
		    eloT oppElo = 0;
		    uint year = ie->GetYear();
		    if (pos->GetToMove() == WHITE) {
			    elo = ie->GetWhiteElo();
			    oppElo = ie->GetBlackElo();
		    } else {
			    elo = ie->GetBlackElo();
			    oppElo = ie->GetWhiteElo();
		    }
		    if (elo > 0) {
			    node->eloSum += elo;
			    node->eloCount++;
		    }
		    if (oppElo > 0) {
			    node->perfSum += oppElo;
			    node->perfCount++;
		    }
		    if (year != 0) {
			    node->yearSum += year;
			    node->yearCount++;
		    }
		    tree->totalCount++;
	    } // end: if (foundMatch) ...
    	} // end: for
    }

    // Update the normal filter (if desired) S.A.
    if (adjustMode)
	updateMainFilter(base);

    //// No longer update the main filter for ply.
    //// Game ply is got from treeFilter in sc_game_load
    // else updateMainFilter2(base);

    // Now we generate the score of each move: it is the expected score per
    // 1000 games. Also generate the ECO code of each move.

    DString dstr;
    treeT* tree = &(base->tree);
    treeNodeT* node = tree->node;
    for (uint i=0; i < tree->moveCount; i++, node++) {
        node->score = (node->freq[RESULT_White] * 2 + node->freq[RESULT_Draw] + node->freq[RESULT_None])
		* 500 / node->total;

        node->ecoCode = 0;
        if (ecoBook != NULL) {
            scratchPos->CopyFrom (db->game->GetCurrentPos());
            if (node->sm.from != NULL_SQUARE) {
                scratchPos->DoSimpleMove (&(node->sm));
            }
            dstr.Clear();
            if (ecoBook->FindOpcode (scratchPos, "eco", &dstr) == OK) {
                node->ecoCode = eco_FromString (dstr.Data());
            }
        }
    }

    // Now we sort the move list:
    sortTreeMoves (tree, sortMethod, db->game->GetCurrentPos()->GetToMove());

    // If it wasn't in the cache, maybe it belongs there, but only add to the cache if not in fastmode
    if (!foundInCache  && !fastMode) {
    	base->treeCache->Add   (db->game->GetCurrentPos(), tree, base->treeFilter);
    	base->backupCache->Add (db->game->GetCurrentPos(), tree, base->treeFilter);
        base->treeFilter->saveFilterForFastMode(db->game->GetCurrentPly());
    }

    if (showProgress) {
        updateProgressBar (ti, 1, 1, true);
    }
    search_pool.erase(&base);

    DString * output = new DString;
    char temp [200];
    if (! listMode) {
	// This row sets line length only.
	const char *titleRow = "      Move        Frequency    Score  %Draws AvElo Perf AvYear ECO ";
        if (shortDisplay) {
	  titleRow = translate (ti, "TreeTitleRowShort", titleRow);
        } else {
	  titleRow = translate (ti, "TreeTitleRow", titleRow);
        }
        output->Append (titleRow);
        if (showEpdData) {
            for (int epdID = 0; epdID < MAX_EPD; epdID++) {
                if (pbooks[epdID] != NULL) {
                    const char * name = pbooks[epdID]->GetFileName();
                    const char * lastSlash = strrchr (name, '/');
                    if (lastSlash != NULL) { name = lastSlash + 1; }
                    strCopy (temp, name);
                    strTrimFileSuffix (temp);
                    strPad (temp, temp, 5, ' ');
                    output->Append ("  ");
                    output->Append (temp);
                }
            }
        }
    }

    // Now we print the list into the return string:
    node = tree->node;
    for (uint count=0; count < tree->moveCount; count++, node++) {
        ecoStringT ecoStr;
        eco_ToExtendedString (node->ecoCode, ecoStr);
        uint avgElo = 0;
        if (node->eloCount >= 10) {
            // bool foundInCache = false;
            avgElo = node->eloSum / node->eloCount;
        }
        uint perf = 0;
        if (node->perfCount >= 10) {
            perf = node->perfSum / node->perfCount;
            uint score = (node->score + 5) / 10;
            if (db->game->GetCurrentPos()->GetToMove() == BLACK) { score = 100 - score; }
            perf = Crosstable::Performance (perf, score);
        }
        unsigned long long avgYear = 0;
        if (node->yearCount > 0) {
            avgYear = (node->yearSum + (node->yearCount/2)) / node->yearCount;
        }
        node->san[6] = 0;

        strcpy(tempTrans, node->san);
        transPieces(tempTrans);

        if (listMode) {
            if (ecoStr[0] == 0) { strCopy (ecoStr, "{}"); }
            sprintf (temp, "%2u %-6s%7u %3d%c%1d %3d%c%1d",
                     count + 1,
                     hideMoves ? "---" : tempTrans,//node->san,
                     node->total,
                     100 * node->total / tree->totalCount,
                     decimalPointChar,
                     (1000 * node->total / tree->totalCount) % 10,
                     node->score / 10,
                     decimalPointChar,
                     node->score % 10);
            output->Append (temp);
        } else {
            sprintf (temp, "\n%2u: %-6s%7u:%3d%c%1d%%  %3d%c%1d%%",
                     count + 1,
                     hideMoves ? "---" : tempTrans,//node->san,
                     node->total,
                     100 * node->total / tree->totalCount,
                     decimalPointChar,
                     (1000 * node->total / tree->totalCount) % 10,
                     node->score / 10,
                     decimalPointChar,
                     node->score % 10);
            output->Append (temp);
        }
        uint pctDraws = node->freq[RESULT_Draw] * 1000 / node->total;
        sprintf (temp, " %3d%%", (pctDraws + 5) / 10);
        output->Append (temp);

      if (!shortDisplay) {
        if (avgElo == 0) {
            strCopy (temp, listMode ? " {}" : "      ");
        } else {
            sprintf (temp, "  %4u", avgElo);
        }
        output->Append (temp);

        if (perf == 0) {
            strCopy (temp, listMode ? " {}" : "      ");
        } else {
            sprintf (temp, "  %4u", perf);
        }
        output->Append (temp);
        if (avgYear == 0) {
            strCopy (temp, listMode ? " {}" : "      ");
        } else {
#ifdef _WIN32
	    sprintf (temp, "  %4I64u", avgYear);
#else
	    sprintf (temp, "  %4llu", avgYear);
#endif
        }
        output->Append (temp);

        if (showEpdData && !listMode) {
            for (int epdID = 0; epdID < MAX_EPD; epdID++) {
                PBook * pb = pbooks[epdID];
                if (pb != NULL) {
                    scratchPos->CopyFrom (base->game->GetCurrentPos());
                    if (node->sm.from != NULL_SQUARE) {
                        scratchPos->DoSimpleMove (&(node->sm));
                    }
                    const char * s = "";
                    dstr.Clear();
                    if (pb->FindSummary (scratchPos, &dstr) == OK) {
                        s = dstr.Data();
                    }
                    strPad (temp, s, 5, ' ');
                    output->Append ("   ");
                    output->Append (temp);
                }
            }
        }

	sprintf (temp, " %-5s", hideMoves ? "{}" : ecoStr);
        output->Append (temp);
      }
        if (listMode) {
            Tcl_AppendElement (ti, (char *) output->Data());
            output->Clear();
        }
    }

    // Print a totals line as well, if there are any moves in the tree:

    if (tree->moveCount > 0) {
        int totalScore = 0;
        unsigned long long eloSum = 0;
        unsigned long long eloCount = 0;
        unsigned long long perfSum = 0;
        unsigned long long perfCount = 0;
        unsigned long long yearCount = 0;
        unsigned long long yearSum = 0;
        uint nDraws = 0;
        node = tree->node;
        for (uint count=0; count < tree->moveCount; count++, node++) {
            totalScore += node->freq[RESULT_White] * 2;
            totalScore += node->freq[RESULT_Draw] + node->freq[RESULT_None];
            eloCount += node->eloCount;
            eloSum += node->eloSum;
            perfCount += node->perfCount;
            perfSum += node->perfSum;
            yearCount += node->yearCount;
            yearSum += node->yearSum;
            nDraws += node->freq[RESULT_Draw];
        }
        totalScore = totalScore * 500 / tree->totalCount;
        unsigned long long avgElo = 0;
        if (eloCount >= 10) {
            avgElo = eloSum / eloCount;
        }
        uint perf = 0;
        if (perfCount >= 10) {
            perf = perfSum / perfCount;
            uint score = (totalScore + 5) / 10;
            if (db->game->GetCurrentPos()->GetToMove() == BLACK) { score = 100 - score; }
            perf = Crosstable::Performance (perf, score);
        }

        if (listMode) {
            /* malformed but unused. (listMode is for pocket i think S.A.)

            sprintf (temp, "%2u %-6s%7u %3d%c%1d %3d%c%1d",
                     0,
                     "TOTAL",
                     "{}",
                     tree->totalCount,
                     100, decimalPointChar, 0,
                     totalScore / 10, decimalPointChar, totalScore % 10);
            output->Append (temp);

            */
        } else {
            const char * totalString = translate (ti, "TreeTotal:", "TOTAL:");
            if (shortDisplay)
              output->Append ("\n___________________________________________\n");
            else
              output->Append ("\n__________________________________________________________________\n");
            sprintf (temp, "%-10s%7u:100%c0%%  %3d%c%1d%%",
                     totalString, tree->totalCount, decimalPointChar,
                     totalScore / 10, decimalPointChar, totalScore % 10);
            output->Append (temp);
        }
        uint pctDraws = nDraws * 1000 / tree->totalCount;
        sprintf (temp, " %3d%%", (pctDraws + 5) / 10);
        output->Append (temp);

      if (!shortDisplay) {
        if (avgElo == 0) {
            output->Append (listMode ? " {}" : "      ");
        } else {
#ifdef _WIN32
            sprintf (temp, "  %4I64u", avgElo);
#else
            sprintf (temp, "  %4llu", avgElo);
#endif
            output->Append (temp);
        }
        if (perf == 0) {
            output->Append (listMode ? " {} " : "      ");
        } else {
            sprintf (temp, "  %4u", perf);
            output->Append (temp);
        }
        if (yearCount == 0) {
            output->Append (listMode ? " {}" : "      ");
        } else {
#ifdef _WIN32
            sprintf (temp, "  %4I64u", (yearSum + (yearCount/2)) / yearCount);
#else
            sprintf (temp, "  %4llu", (yearSum + (yearCount/2)) / yearCount);
#endif
            output->Append (temp);
        }
      }
        if (listMode) {
            Tcl_AppendElement (ti, (char *) output->Data());
            output->Clear();
        } else {
            output->Append ("\n");
        }
    }

    // Print timing and other information:
#if 0
    if (showTimeStats  &&  !listMode) {
        int csecs = timer.CentiSecs();
        sprintf (temp, "\n  Time: %d%c%02d s",
                 csecs / 100, decimalPointChar, csecs % 100);
        output->Append (temp);

        if (foundInCache) {
            output->Append ("  (Found in cache)");
        } else {
#ifdef SHOW_SKIPPED_STATS
            output->Append ("  Skipped: ", skipcount, " games.");
#endif
        }
    }
#endif

    if (! listMode) {
    	Tcl_AppendResult (ti, output->Data(), NULL);
    }
    delete output;
    base->treeSearchTime = timer.MilliSecs();

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_tree_free:
//    Frees memory allocated for SortedLines
//    only useful for Pocket PC (1MB of memory to be freed)
int
sc_tree_free (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
  StoredLine::FreeStoredLine();
  return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_tree_cachesize:
//    set cache size
int
sc_tree_cachesize (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
  if (argc != 4) {
    return errorResult (ti, "Usage: sc_tree cachesize <base> <size>");
  }
  scidBaseT * base = NULL;
  int baseNum = strGetInteger (argv[2]);
  if (baseNum >= 1  &&  baseNum <= MAX_BASES) {
   base = &(dbList[baseNum - 1]);
  }
  uint size = strGetUnsigned(argv[3]);
  if (base->inUse)
    base->treeCache->CacheResize(size);
  return TCL_OK;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_tree_cacheinfo:
//    returns a list of 2 values : used slots and max cache size
int
sc_tree_cacheinfo (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
  if (argc != 3) {
    return errorResult (ti, "Usage: sc_tree cacheinfo <base>");
  }
  scidBaseT * base = NULL;
  int baseNum = strGetInteger (argv[2]);
  if (baseNum >= 1  &&  baseNum <= MAX_BASES) {
   base = &(dbList[baseNum - 1]);
  }
  if (base->inUse) {
    appendUintElement (ti, base->treeCache->UsedSize());
    appendUintElement (ti, base->treeCache->Size());
  } else {
    appendUintElement (ti, 0);
    appendUintElement (ti, 0);
  }
  return TCL_OK;
}

//    Search function interface

int
sc_search (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "board", "moves", "cql", "header", "material", NULL
    };
    enum { OPT_BOARD, OPT_MOVES, OPT_CQL, OPT_HEADER, OPT_MATERIAL };

    int index = -1;
    if (argc > 1) { index = strUniqueMatch (argv[1], options); }
    int ret = TCL_OK;

    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    switch (index) {
    case OPT_BOARD:
	ret = sc_search_board (cd, ti, argc, argv);
	break;

    case OPT_MOVES:
	ret = sc_search_moves (cd, ti, argc, argv);
	break;

    case OPT_CQL:
	ret = sc_search_cql (cd, ti, argc, argv);
	break;

    case OPT_HEADER:
	ret = sc_search_header (cd, ti, argc, argv);
	break;

    case OPT_MATERIAL:
	ret = sc_search_material (cd, ti, argc, argv);
	break;

    default:
        return InvalidCommand (ti, "sc_search", options);
    }

    // Update the normal filter
    // (Board search does so inline because it may be searching other dbs)

    if (index != OPT_BOARD)
      setMainFilter(db);

    return ret;
}


inline uint
startFilterSize (scidBaseT * base, filterOpT filterOp)
{
    // &&& if( base->dbFilter == NULL)
    // &&&     initDbFilter( base, 1);

    if (filterOp == FILTEROP_AND) {
        return base->dbFilter->Count();
    }
    return base->numGames;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_search_board:
//    Searches for exact match for the current position.
//    if <base> is present, search for current position in base <base>,
//    and sets <base> filter accordingly
int
sc_search_board (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usageStr =
	"Usage: sc_search board <filterOp> <searchType> <searchInVars> <flip> [<base>]";
    bool showProgress = startProgressBar();
    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    if (argc < 6  ||  argc > 7) { return errorResult (ti, usageStr); }

    filterOpT filterOp = strGetFilterOp (argv[2]);

    bool useHpSigSpeedup = false;
    gameExactMatchT searchType = GAME_EXACT_MATCH_Exact;

    switch (argv[3][0]) {
    case 'E':
        searchType = GAME_EXACT_MATCH_Exact;
        useHpSigSpeedup = true;
        break;
    case 'P':
        searchType = GAME_EXACT_MATCH_Pawns;
        useHpSigSpeedup = true;
        break;
    case 'F':
        searchType = GAME_EXACT_MATCH_Fyles;
        break;
    case 'M':
        searchType = GAME_EXACT_MATCH_Material;
        break;
    default:
        return errorResult (ti, usageStr);
    }

    bool searchInVars = strGetBoolean (argv[4]);
    bool flip = false;
    // if (argc == 6) { flip = strGetBoolean (argv[5]); }
    flip = strGetBoolean (argv[5]);

    // &&&    if( db->dbFilter == NULL) {
    //         initDbFilter (db);
    //             }

    Position * pos = db->game->GetCurrentPos();

    int baseNum = -1;
    int oldCurrentBase = currentBase;
    bool searchInRefBase = false;
    if (argc == 7) {
//                   0        1          2                        3
//       set str [sc_search board $::search::filter::operation $sBoardSearchType $searchInVars $sBoardIgnoreCols $base]
      baseNum = strGetUnsigned( argv[6] );
      searchInRefBase = true;
      currentBase = baseNum - 1;
      db = &(dbList[currentBase]);
    }

    Timer timer;  // Start timing this search.

    Position * posFlip =  NULL;
    matSigT msig = matsig_Make (pos->GetMaterial());
    matSigT msigFlip = 0;
    uint hpSig = pos->GetHPSig();
    uint hpSigFlip = 0;

    if (flip) {
        posFlip = new Position;
        char cboard [40];
        pos->PrintCompactStrFlipped (cboard);
        posFlip->ReadFromCompactStr ((byte *) cboard);
        hpSigFlip = posFlip->GetHPSig();
        msigFlip = matsig_Make (posFlip->GetMaterial());
    }

    uint skipcount = 0;
    uint updateStart, update;
    updateStart = update = 5000;  // Update progress bar every 5000 games

    // If filter operation is to reset the filter, reset it:
    if (filterOp == FILTEROP_RESET) {
        filter_reset (db, 1);
        filterOp = FILTEROP_AND;
    }
    uint startFilterCount = startFilterSize (db, filterOp);

    // Here is the loop that searches on each game:
    IndexEntry * ie;
    Game * g = scratchGame;
    uint gameNum;
    for (gameNum=0; gameNum < db->numGames; gameNum++) {
        if (showProgress) {  // Update the percentage done bar:
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, gameNum, db->numGames);
                if (interruptedProgress()) { break; }
            }
        }
        // First, apply the filter operation:
        if (filterOp == FILTEROP_AND) {  // Skip any games not in the filter:
            if (db->dbFilter->Get(gameNum) == 0) {
                skipcount++;
                continue;
            }
        } else /* filterOp==FILTEROP_OR*/ { // Skip any games in the filter:
            if (db->dbFilter->Get(gameNum) != 0) {
                skipcount++;
                continue;
            } else {
                // OK, this game is NOT in the filter.
                // Add it so filterCounts are kept up to date:
                db->dbFilter->Set (gameNum, 1);
            }
        }

        ie = db->idx->FetchEntry (gameNum);
        if (ie->GetLength() == 0) {
            // Skip games with no gamefile record:
            db->dbFilter->Set (gameNum, 0);
            skipcount++;
            continue;
        }

        // Set "useVars" to true only if the search specified searching
        // in variations, AND this game has variations:
        bool useVars = searchInVars && ie->GetVariationsFlag();

        bool possibleMatch = true;
        bool possibleFlippedMatch = flip;

        // Apply speedups if we are not searching in variations:
        if (! useVars) {
            if (! ie->GetStartFlag()) {
                // Speedups that only apply to standard start games:
                if (useHpSigSpeedup  &&  hpSig != 0xFFFF) {
                    const byte * hpData = ie->GetHomePawnData();
                    if (! hpSig_PossibleMatch (hpSig, hpData)) {
                        possibleMatch = false;
                    }
                    if (possibleFlippedMatch) {
                        if (! hpSig_PossibleMatch (hpSigFlip, hpData)) {
                            possibleFlippedMatch = false;
                        }
                    }
                }
            }

            // If this game has no promotions, check the material of its final
            // position, since the searched position might be unreachable:
            if (possibleMatch) {
                if (!matsig_isReachable (msig, ie->GetFinalMatSig(),
                                         ie->GetPromotionsFlag(),
                                         ie->GetUnderPromoFlag())) {
                        possibleMatch = false;
                    }
            }
            if (possibleFlippedMatch) {
                if (!matsig_isReachable (msigFlip, ie->GetFinalMatSig(),
                                         ie->GetPromotionsFlag(),
                                         ie->GetUnderPromoFlag())) {
                        possibleFlippedMatch = false;
                    }
            }
        }

        if (!possibleMatch  &&  !possibleFlippedMatch) {
            db->dbFilter->Set (gameNum, 0);
            skipcount++;
            continue;
        }

        // At this point, the game needs to be loaded:
        if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(),
                                 ie->GetLength()) != OK) {
            return errorResult (ti, "Error reading game file.");
        }
        uint ply = 0;
        if (useVars) {
            g->Decode (db->bbuf, GAME_DECODE_NONE);
            // Try matching the game without variations first:
            if (ply == 0  &&  possibleMatch) {
                if (g->ExactMatch (pos, NULL, NULL, searchType)) {
                    ply = g->GetCurrentPly() + 1;
                }
            }
            if (ply == 0  &&  possibleFlippedMatch) {
                if (g->ExactMatch (posFlip, NULL, NULL, searchType)) {
                    ply = g->GetCurrentPly() + 1;
                }
            }
            if (ply == 0  &&  possibleMatch) {
                g->MoveToPly (0);
                if (g->VarExactMatch (pos, searchType)) {
                    ply = g->GetCurrentPly() + 1;
                }
            }
            if (ply == 0  &&  possibleFlippedMatch) {
                g->MoveToPly (0);
                if (g->VarExactMatch (posFlip, searchType)) {
                    ply = g->GetCurrentPly() + 1;
                }
            }
        } else {
            // No searching in variations:
            if (possibleMatch) {
                if (g->ExactMatch (pos, db->bbuf, NULL, searchType)) {
                    // Set its auto-load move number to the matching move:
                    ply = g->GetCurrentPly() + 1;
                }
            }
            if (ply == 0  &&  possibleFlippedMatch) {
                db->bbuf->BackToStart();
                if (g->ExactMatch (posFlip, db->bbuf, NULL, searchType)) {
                    ply = g->GetCurrentPly() + 1;
                }
            }
        }
        if (ply > 255) { ply = 255; }
        db->dbFilter->Set (gameNum, ply);
    }

    if (showProgress) { updateProgressBar (ti, 1, 1); }
    if (flip) { delete posFlip; }

    // Now print statistics and time for the search:
    char temp[200];
    int centisecs = timer.CentiSecs();
    if (gameNum != db->numGames) {
        Tcl_AppendResult (ti, errMsgSearchInterrupted(ti), "  ", NULL);
    }
    sprintf (temp, "%d / %d  (%d%c%02d s)",
             db->dbFilter->Count(), startFilterCount,
             centisecs / 100, decimalPointChar, centisecs % 100);
    Tcl_AppendResult (ti, temp, NULL);
#ifdef SHOW_SKIPPED_STATS
    sprintf(temp, "  Skipped %u games.", skipcount);
    Tcl_AppendResult (ti, temp, NULL);
#endif

    setMainFilter(db);
    if (searchInRefBase ) {
      currentBase = oldCurrentBase;
      db = &(dbList[currentBase]);
    }

    return TCL_OK;
}

// Searches for a move or move combos
// stevenaaus September 2012

int
sc_search_moves (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    const char * usageStr =
	"Usage: sc_search moves <filterOp> <moves> <checkTest> <sideToMove>";
    bool showProgress = startProgressBar();
    char **m_argv;
    int m_argc;

    if (!db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    if (argc != 6) { return errorResult (ti, usageStr); }

    filterOpT filterOp = strGetFilterOp (argv[2]);

    // argv[3] is move list
    if (Tcl_SplitList (ti, argv[3], &m_argc, (CONST84 char ***) &m_argv) != TCL_OK) {
	Tcl_AppendResult (ti, "Error splitting movelist.", NULL);
	return TCL_ERROR;
    }

    int checkTest = strGetUnsigned (argv[4]);

    bool wToMove = false;
    if (strFirstChar (argv[5], 'w')  || strFirstChar (argv[5], 'W')) {
	wToMove = true;
    }
    bool bToMove = false;
    if (strFirstChar (argv[5], 'b')  || strFirstChar (argv[5], 'B')) {
	bToMove = true;
    }

    // &&&    if( db->dbFilter == NULL) {
    //         initDbFilter (db);
    //             }

    Timer timer;  // Start timing this search.

    uint skipcount = 0;
    uint updateStart, update;
    updateStart = update = 5000;  // Update progress bar every 5000 games

    // If filter operation is to reset the filter, reset it:
    if (filterOp == FILTEROP_RESET) {
        filter_reset (db, 1);
        filterOp = FILTEROP_AND;
    }
    uint startFilterCount = startFilterSize (db, filterOp);

    // Here is the loop that searches on each game:
    IndexEntry * ie;
    Game * g = scratchGame;
    uint gameNum;
    uint ply;

    for (gameNum=0; gameNum < db->numGames; gameNum++) {
        if (showProgress) {  // Update the percentage done bar:
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, gameNum, db->numGames);
                if (interruptedProgress()) { break; }
            }
        }
        // First, apply the filter operation:
        if (filterOp == FILTEROP_AND) {  // Skip any games not in the filter:
            if (db->dbFilter->Get(gameNum) == 0) {
                skipcount++;
                continue;
            }
        } else /* filterOp==FILTEROP_OR*/ { // Skip any games in the filter:
            if (db->dbFilter->Get(gameNum) != 0) {
                skipcount++;
                continue;
            } else {
                // OK, this game is NOT in the filter.
                // Add it so filterCounts are kept up to date:
                db->dbFilter->Set (gameNum, 1);
            }
        }

        ie = db->idx->FetchEntry (gameNum);
        if (ie->GetLength() == 0) {
            // Skip games with no gamefile record:
            db->dbFilter->Set (gameNum, 0);
            skipcount++;
            continue;
        }

	// todo : search in Vars using 'MoveIntoVariation'
	// todo : allow user to state max/min ply depth

        // At this point, the game needs to be loaded:
        if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(),
                                 ie->GetLength()) != OK) {
            return errorResult (ti, "Error reading game file.");
        }

	g->Decode (db->bbuf, GAME_DECODE_NONE);

	if (g->MoveMatch (m_argc, m_argv, 255, wToMove, bToMove, checkTest)) {
	    // Set its auto-load move number to the matching move:
	    ply = g->GetCurrentPly() + 2;
	    if (ply > 255) { ply = 255; }
	} else {
            ply = 0;
        }

        db->dbFilter->Set (gameNum, ply);
    }

    if (showProgress) { updateProgressBar (ti, 1, 1); }

    // Now print statistics and time for the search:
    char temp[200];
    int centisecs = timer.CentiSecs();
    if (gameNum != db->numGames) {
        Tcl_AppendResult (ti, errMsgSearchInterrupted(ti), "  ", NULL);
    }
    sprintf (temp, "%d / %d  (%d%c%02d s)",
             db->dbFilter->Count(), startFilterCount,
             centisecs / 100, decimalPointChar, centisecs % 100);
    Tcl_AppendResult (ti, temp, NULL);
#ifdef SHOW_SKIPPED_STATS
    sprintf(temp, "  Skipped %u games.", skipcount);
    Tcl_AppendResult (ti, temp, NULL);
#endif
    Tcl_Free((char *) m_argv);

    return TCL_OK;
}

// Search on CQL syntax.
// Lionel Hampton --  October 2017

int
sc_search_cql (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    uint gameNum;
    const char * usageStr =
      "Usage: sc_search cql <filterOp> <stripSwitch> <silentSwitch> <syntax>";
    bool showProgress = startProgressBar();

    if (argc != 6)
      return errorResult (ti, usageStr);

    if (!db->inUse)
      return errorResult (ti, errMsgNotOpen(ti));

    filterOpT filterOp = strGetFilterOp (argv[2]);

    bool stripSwitch = (argv[3][0] == '1');

    // This corresponds to the 'Allow Comments' user option.
    bool silentSwitch = (argv[4][0] != '1');

    // If the base is readonly, just run the query in silent mode.
    if (db->fileMode==FMODE_ReadOnly) silentSwitch = true;
    // This will let the engine run slightly more efficiently.
    // The larger effect of silentSwitch is that we will simply not replace the game.
    extern bool CqlSilent;
    CqlSilent = silentSwitch;

    // argv[5] is CQL syntax
    // printf("CQL Syntax:\n%s\n", argv[5]);

    // All cql based asserts leave an error msg in the cqlErrMsg global.
    // However, the assert is not the only path to longjmp(), and the
    // error msg is not always set. Hence, we null out the pointer before
    // nesting our way down the call stack.
    extern char *cqlErrMsg;
    cqlErrMsg = NULL;

    // Set to true to show some useful info on <stdout>.
    extern bool CqlShowLex, CqlShowParse, CqlDebug;
    CqlShowLex = false;
    CqlShowParse = false;
    CqlDebug = false;

    // We believe it is safe to employ longjmp(), since all CQL destructors are trivial.
    // Nonetheless, TODO: maybe should use C++ exception handling here...
    extern std::jmp_buf jump_buffer;
    void CqlReset();
    if (setjmp(jump_buffer) != 0 ) {
      CqlReset();
      if (cqlErrMsg) Tcl_AppendResult (ti, cqlErrMsg, NULL);
      else Tcl_AppendResult (ti, "Error reported back from CQL engine", NULL);
      return TCL_OK;
    }

    // Parse the CQL script.
    bool CqlParseBuffer(char*);
    CqlParseBuffer((char *)argv[5]);

    Timer timer;  // Start timing this search.

    uint skipcount = 0;
    uint updateStart, update;
    // this is a tradeoff... some queries are EXTREMELY compute-intensive
    updateStart = update = 1000;  // Update progress bar every 1000 games

    // If filter operation is to reset the filter, reset it:
    if (filterOp == FILTEROP_RESET) {
        filter_reset (db, 1);
        filterOp = FILTEROP_AND;
    }
    uint startFilterCount = startFilterSize (db, filterOp);

    // Here is the loop that searches on each game:
    IndexEntry * ie;
    Game * g = scratchGame;  // TODO: how is this NOT a memory leak?
    bool dirtyFlag = false;  // has a game been replaced?
    bool jumpFlag = false;   // have we caught a longjump()?

    for (gameNum=0; gameNum < db->numGames; gameNum++) {
        // If we're just finishing out after a longjump(), remove all remaining
        // games from the filter.
        if (jumpFlag) {
          db->dbFilter->Set (gameNum, 0); // remove the game from the filter
          continue;
        }

        // Update the percentage done bar:
        if (showProgress) {
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, gameNum, db->numGames);
                if (interruptedProgress()) { break; }
            }
        }

        // First, apply the filter operation:
        if (filterOp == FILTEROP_AND) {  // Skip any games not in the filter:
            if (db->dbFilter->Get(gameNum) == 0) {
                skipcount++;
                continue;
            }
        } else /* filterOp==FILTEROP_OR*/ { // Skip any games in the filter:
            if (db->dbFilter->Get(gameNum) != 0) {
                skipcount++;
                continue;
            } else {
                // OK, this game is NOT in the filter.
                // Add it so filterCounts are kept up to date:
                db->dbFilter->Set (gameNum, 1);
            }
        }

        // Skip games with no gamefile record:
        // TODO: What does this mean???
        // Thought it would be non-standard openings with no moves (just
        // a FEN tag), but no...
        ie = db->idx->FetchEntry (gameNum);
        if (ie->GetLength() == 0) {
            db->dbFilter->Set (gameNum, 0);
            skipcount++;
            continue;
        }

        // Load the game.
        if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(), ie->GetLength()) != OK) {
            return errorResult (ti, "Error reading game file.");
        }

        g->Decode (db->bbuf, GAME_DECODE_ALL); // variations, comments and non-standards
        g->LoadStandardTags (ie, db->nb); // for metadata matches
        g->SetNumber (gameNum + 1); // for game range tests
        g->SetAltered(false);  // the CQL engine will set this flag when adding comments

        char *comment, *match;
        int countMatch;

        // Strip existing MATCH markers left by a previous query.
        // Save the cpu cycles if we're running silent.
        if (stripSwitch && !silentSwitch) {
          countMatch = 0;
          g->MoveToPly(0);
          do {
            if ((comment = g->GetMoveComment())) {
              if ((match = strstr(comment, "MATCH"))) {
                if (match == comment) g->SetMoveComment(NULL);
                else match[0] = 0;  // truncate the mark and all that follows
                countMatch++;
              }
            }
          } while (g->MoveForward() == OK);
          if (countMatch) g->SetAltered(true);
        }

        if (setjmp(jump_buffer) != 0 ) {
          printf("CQL longjump() while matching gamenumber %d\n", gameNum+1);
          //db->gameNumber = gameNum;
          jumpFlag = true;
          db->dbFilter->Set (gameNum, g->GetCurrentPly() + 1);
          continue;
        }

        // See if the game matches.
        bool CqlMatchGame(Game *);  // this is our surrogate in the cql code... see comment
        extern uint CqlMatchPlyFirst; // set by the cql engine
        if ( CqlMatchGame(g) ) {
          if (CqlMatchPlyFirst > 254) { CqlMatchPlyFirst = 254; } // is 254 enough cushion?
          db->dbFilter->Set (gameNum, CqlMatchPlyFirst + 1);
        } else {
          db->dbFilter->Set (gameNum, 0); // remove the game from the filter
        }

        // Do not replace the game if we are in silent mode,
        // regardless of whether or not the game has been altered.
        if (g->GetAltered() && !silentSwitch) {
          if (sc_savegame (ti, g, gameNum+1, db) != OK) { return TCL_ERROR; }
          dirtyFlag = true;
        }
    }

    // Reset the engine.
    CqlReset();

    // If necessary, update index and name files:
    if (dirtyFlag) {
        db->gfile->FlushAll();
        if (db->idx->WriteHeader() != OK) {
            return errorResult (ti, "Error writing index file.");
        }
        // Not sure this is absolutely necessary, only that counts are
        // bumped up and down when game is replaced.  But why should that
        // matter... it's a wash.
        if (! db->memoryOnly  &&  db->nb->WriteNameFile() != OK) {
            return errorResult (ti, "Error writing name file.");
        }
    }

    if (showProgress) { updateProgressBar (ti, 1, 1); }

    // Now print statistics and time for the search:
    // If the jump flag is set, we're falling out with an exception.
    if (jumpFlag) {
      if (cqlErrMsg) Tcl_AppendResult (ti, cqlErrMsg, NULL);
      else Tcl_AppendResult (ti, "Error reported back from CQL engine", NULL);
    } else {
      char temp[200];
      int centisecs = timer.CentiSecs();
      if (gameNum != db->numGames) {
          Tcl_AppendResult (ti, errMsgSearchInterrupted(ti), "  ", NULL);
      }
      sprintf (temp, "%d / %d  (%d%c%02d s)",
             db->dbFilter->Count(), startFilterCount,
             centisecs / 100, decimalPointChar, centisecs % 100);
      Tcl_AppendResult (ti, temp, NULL);
#ifdef SHOW_SKIPPED_STATS
      sprintf(temp, "  Skipped %u games.", skipcount);
      Tcl_AppendResult (ti, temp, NULL);
#endif
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// addPattern():
//    Called by the parameter parsing section of sc_search_material()
//    to add a pattern to a pattern list.
//    Returns the new head of the pattern list.
patternT *
addPattern (patternT * pattHead, patternT * addPatt)
{
    // Create a new pattern structure:
    patternT * newPatt = new patternT;

    // Initialise it:
    newPatt->flag = addPatt->flag;
    newPatt->pieceMatch = addPatt->pieceMatch;
    newPatt->fyleMatch = addPatt->fyleMatch;
    newPatt->rankMatch = addPatt->rankMatch;

    // Add to the head of the list of patterns, and return:
    newPatt->next = pattHead;
    return newPatt;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// freePatternList():
//    Frees the memory used by a list of patterns.
void
freePatternList (patternT * patt)
{
    patternT * nextPatt;
    while (patt) {
        nextPatt = patt->next;
        delete patt;
        patt = nextPatt;
    }
}

void
flipPattern (patternT * patt)
{
    if (patt->rankMatch != NO_RANK) {
        patt->rankMatch = (RANK_1 + RANK_8) - patt->rankMatch;
    }
    patt->pieceMatch = PIECE_FLIP[patt->pieceMatch];
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// parsePattern:
//    Called by sc_search_material to extract the details of
//    a pattern parameter (e.g "no wp c ?" for no White pawn on
//    the d file).
errorT
parsePattern (const char * str, patternT * patt)
{
    ASSERT (str != NULL  &&  patt != NULL);

    // Set up pointers to the four whitespace-separated pattern
    // parameter values in the string:
    str = strFirstWord (str);
    const char * flagStr = str;
    str = strNextWord (str);
    const char * colorPieceStr = str;
    str = strNextWord (str);
    const char * fyleStr = str;
    str = strNextWord (str);
    const char * rankStr = str;

    // Parse the color parameter: "w", "b", or "?" for no pattern.
    if (*colorPieceStr == '?') {
        // Empty pattern:
        patt->pieceMatch = EMPTY;
        return OK;
    }

    colorT color = WHITE;
    switch (tolower(*colorPieceStr)) {
        case 'w': color = WHITE; break;
        case 'b': color = BLACK; break;
        default: return ERROR;
    }

    // Parse the piece type parameter for this pattern:
    pieceT p = EMPTY;
    switch (tolower(colorPieceStr[1])) {
        case 'k': p = KING; break;
        case 'q': p = QUEEN; break;
        case 'r': p = ROOK; break;
        case 'b': p = BISHOP; break;
        case 'n': p = KNIGHT; break;
        case 'p': p = PAWN; break;
        default: return ERROR;
    }

    patt->pieceMatch = piece_Make (color, p);
    patt->flag = strGetBoolean (flagStr);

    // Parse the fyle parameter for this pattern:
    char ch = *fyleStr;
    if (ch == '?') {
        patt->fyleMatch = NO_FYLE;
    } else if (ch >= 'a'  &&  ch <= 'h') {
        patt->fyleMatch = A_FYLE + (ch - 'a');
    } else {
        return ERROR;
    }

    // Parse the rank parameter for this pattern:
    ch = *rankStr;
    if (ch == '?') {
        patt->rankMatch = NO_RANK;
    } else if (ch >= '1'  &&  ch <= '8') {
        patt->rankMatch = RANK_1 + (ch - '1');
    } else {
        return ERROR;
    }
    return OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_search_material:
//    Searches by material and/or pattern.
int
sc_search_material (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = startProgressBar();
    if (! db->inUse) {
        return errorResult (ti, "Not an open database.");
    }

    uint minMoves = 0;
    uint minPly = 0;
    uint maxPly = 999;
    uint matchLength = 1;
    byte min[MAX_PIECE_TYPES] = {0};
    byte minFlipped[MAX_PIECE_TYPES] = {0};
    byte max[MAX_PIECE_TYPES] = {0};
    byte maxFlipped[MAX_PIECE_TYPES] = {0};
    max[WM] = max[BM] = 9;
    int matDiff[2];
    matDiff[0] = -40;
    matDiff[1] = 40;
    filterOpT filterOp = FILTEROP_RESET;
    bool flip = false;
    bool matchEndOnly = false;
    bool oppBishops = true;
    bool sameBishops = true;
    uint hpExcludeMask = HPSIG_Empty;
    uint hpExMaskFlip = HPSIG_Empty;
    patternT * patt = NULL;
    patternT * flippedPatt = NULL;
    patternT tempPatt;

    const char * options[] = {
        "wq", "bq", "wr", "br", "wb", "bb", "wn", "bn",
        "wp", "bp", "wm", "bm", "flip", "filter", "range",
        "length", "bishops", "diff", "pattern", "matchendonly", NULL
    };
    enum {
        OPT_WQ, OPT_BQ, OPT_WR, OPT_BR, OPT_WB, OPT_BB, OPT_WN, OPT_BN,
        OPT_WP, OPT_BP, OPT_WM, OPT_BM, OPT_FLIP, OPT_FILTER, OPT_RANGE,
        OPT_LENGTH, OPT_BISHOPS, OPT_DIFF, OPT_PATTERN, OPT_MATCHENDONLY
    };

    int arg = 2;
    while (arg+1 < argc) {
        const char * option = argv[arg];
        const char * value = argv[arg+1];
        arg += 2;
        int index = -1;
        if (option[0] == '-') {
            index = strUniqueMatch (&(option[1]), options);
        }
        uint counts [2] = {0, 0};
        if (index >= OPT_WQ  &&  index <= OPT_BM) {
            strGetUnsigneds (value, counts, 2);
        }

        switch (index) {

        case OPT_WQ:  min[WQ] = counts[0];  max[WQ] = counts[1];  break;
        case OPT_BQ:  min[BQ] = counts[0];  max[BQ] = counts[1];  break;
        case OPT_WR:  min[WR] = counts[0];  max[WR] = counts[1];  break;
        case OPT_BR:  min[BR] = counts[0];  max[BR] = counts[1];  break;
        case OPT_WB:  min[WB] = counts[0];  max[WB] = counts[1];  break;
        case OPT_BB:  min[BB] = counts[0];  max[BB] = counts[1];  break;
        case OPT_WN:  min[WN] = counts[0];  max[WN] = counts[1];  break;
        case OPT_BN:  min[BN] = counts[0];  max[BN] = counts[1];  break;
        case OPT_WP:  min[WP] = counts[0];  max[WP] = counts[1];  break;
        case OPT_BP:  min[BP] = counts[0];  max[BP] = counts[1];  break;
        case OPT_WM:  min[WM] = counts[0];  max[WM] = counts[1];  break;
        case OPT_BM:  min[BM] = counts[0];  max[BM] = counts[1];  break;

        case OPT_FLIP:
            flip = strGetBoolean (value);
            break;

        case OPT_FILTER:
            filterOp = strGetFilterOp (value);
            break;

        case OPT_RANGE:
            strGetUnsigneds (value, counts, 2);
            minPly = counts[0];  maxPly = counts[1];
            break;

        case OPT_LENGTH:
            matchLength = strGetUnsigned (value);
            if (matchLength < 1) { matchLength = 1; }
            break;

        case OPT_BISHOPS:
            switch (toupper(value[0])) {
                case 'S': oppBishops = false; sameBishops = true;  break;
                case 'O': oppBishops = true;  sameBishops = false; break;
                default:  oppBishops = true;  sameBishops = true;  break;
            }
            break;

        case OPT_DIFF:
            strGetIntegers (value, matDiff, 2);
            break;

        case OPT_PATTERN:
            if (parsePattern (value, &tempPatt) != OK) {
                Tcl_AppendResult (ti, "Invalid pattern: ", value, NULL);
                return TCL_ERROR;
            }
            // Only add to lists if a pattern was specified:
            if (tempPatt.pieceMatch == EMPTY) { break; }
            // Update home-pawn exclude masks if appropriate:
            if (!tempPatt.flag
                &&  piece_Type(tempPatt.pieceMatch) == PAWN
                &&  tempPatt.rankMatch == NO_RANK
                &&  tempPatt.fyleMatch != NO_FYLE) {
                colorT color = piece_Color (tempPatt.pieceMatch);
                colorT flipColor = (color == WHITE ? BLACK : WHITE);
                fyleT fyle = tempPatt.fyleMatch;
                hpExcludeMask = hpSig_AddPawn (hpExcludeMask, color, fyle);
                hpExMaskFlip = hpSig_AddPawn (hpExMaskFlip, flipColor, fyle);
            }
            // Add the pattern and its flipped equivalent:
            patt = addPattern (patt, &tempPatt);
            flipPattern (&tempPatt);
            flippedPatt = addPattern (flippedPatt, &tempPatt);
            break;

        case OPT_MATCHENDONLY:
            matchEndOnly = strGetBoolean (value);
            break;

        default:
            return InvalidCommand (ti, "sc_search material", options);
        }
    }
    if (arg != argc) { return errorResult (ti, "Odd number of parameters."); }

    // Sanity check of values:
    if (max[WQ] < min[WQ]) { max[WQ] = min[WQ]; }
    if (max[BQ] < min[BQ]) { max[BQ] = min[BQ]; }
    if (max[WR] < min[WR]) { max[WR] = min[WR]; }
    if (max[BR] < min[BR]) { max[BR] = min[BR]; }
    if (max[WB] < min[WB]) { max[WB] = min[WB]; }
    if (max[BB] < min[BB]) { max[BB] = min[BB]; }
    if (max[WN] < min[WN]) { max[WN] = min[WN]; }
    if (max[BN] < min[BN]) { max[BN] = min[BN]; }
    if (max[WP] < min[WP]) { max[WP] = min[WP]; }
    if (max[BP] < min[BP]) { max[BP] = min[BP]; }
    // Minor piece range should be at least the sum of the Bishop
    // and Knight minimums, and at most the sum of the maximums:
    if (min[WM] < min[WB]+min[WN]) { min[WM] = min[WB] + min[WN]; }
    if (min[BM] < max[BB]+min[BN]) { min[BM] = min[BB] + min[BN]; }
    if (max[WM] > max[WB]+max[WN]) { max[WM] = max[WB] + max[WN]; }
    if (max[BM] > max[BB]+max[BN]) { max[BM] = max[BB] + max[BN]; }

    // Swap material difference range values if necessary:
    if (matDiff[0] > matDiff[1]) {
        int temp = matDiff[0]; matDiff[0] = matDiff[1]; matDiff[1] = temp;
    }

    // Set up flipped piece counts if necessary:
    if (flip) {
        minFlipped[WQ] = min[BQ];  maxFlipped[WQ] = max[BQ];
        minFlipped[WR] = min[BR];  maxFlipped[WR] = max[BR];
        minFlipped[WB] = min[BB];  maxFlipped[WB] = max[BB];
        minFlipped[WN] = min[BN];  maxFlipped[WN] = max[BN];
        minFlipped[WP] = min[BP];  maxFlipped[WP] = max[BP];
        minFlipped[WM] = min[BM];  maxFlipped[WM] = max[BM];
        minFlipped[BQ] = min[WQ];  maxFlipped[BQ] = max[WQ];
        minFlipped[BR] = min[WR];  maxFlipped[BR] = max[WR];
        minFlipped[BB] = min[WB];  maxFlipped[BB] = max[WB];
        minFlipped[BN] = min[WN];  maxFlipped[BN] = max[WN];
        minFlipped[BP] = min[WP];  maxFlipped[BP] = max[WP];
        minFlipped[BM] = min[WM];  maxFlipped[BM] = max[WM];
    }

    if (matchEndOnly) {
      // Ignore the given ranges and min/max ply
      matchLength = 1;
      minMoves = 1;
      minPly = 1;
      /* Setting these three 1s to 0 enables matching zero length games
         But the issue is, it can also mismatch games at gameEnd-1 ply.
         Could maybe be fixed by extensive hacking around */
      maxPly = 999 * 2;
    } else {
      // Convert move numbers to halfmoves (ply counts):
      minMoves = minPly;
      minPly = minPly * 2 - 1;
      maxPly = maxPly * 2;
    }

    // Set up the material Sig: it is the signature of the MAXIMUMs.
    matSigT msig, msigFlipped;
    int checkMsig = 1;
    if (max[WQ] > 3  ||  max[BQ] > 3  ||  max[WR] > 3 ||  max[BR] > 3 ||
        max[WB] > 3  ||  max[BB] > 3  ||  max[WN] > 3 ||  max[BN] > 3) {
        // It is an unusual search, we cannot use material sig!
        checkMsig = 0;
    }
    msig = matsig_Make (max);
    msigFlipped = MATSIG_FlipColor(msig);

    Timer timer;  // Start timing this search.

    uint skipcount = 0;
    char temp [250];
    Game * g = scratchGame;
    IndexEntry * ie;
    uint updateStart, update;
    updateStart = update = 1000;  // Update progress bar every 1000 games

    // If filter operation is to reset the filter, reset it:
    if (filterOp == FILTEROP_RESET) {
        filter_reset (db, 1);
        filterOp = FILTEROP_AND;
    }
    uint startFilterCount = startFilterSize (db, filterOp);

    // Here is the loop that searches on each game:
    uint gameNum;
    for (gameNum = 0; gameNum < db->numGames; gameNum++) {
        if (showProgress) {  // Update the percentage done bar:
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, gameNum, db->numGames);
                if (interruptedProgress()) {
                    break;
                }
            }
        }
        // First, apply the filter operation:
        if (filterOp == FILTEROP_AND) {  // Skip any games not in the filter:
            if (db->dbFilter->Get(gameNum) == 0) {
                skipcount++;
                continue;
            }
        } else /* filterOp == FILTEROP_OR*/ { // Skip any games in the filter:
            if (db->dbFilter->Get(gameNum) != 0) {
                skipcount++;
                continue;
            }
            // OK, this game is NOT in the filter.
            // Add it so filterCounts are kept up to date:
            db->dbFilter->Set (gameNum, 1);
        }

        ie = db->idx->FetchEntry (gameNum);
        if (ie->GetLength() == 0) {  // Skip games with no gamefile record
            db->dbFilter->Set (gameNum, 0);
            skipcount++;
            continue;
        }

        if (ie->GetNumHalfMoves() < minMoves  &&  ! ie->GetStartFlag()) {
            // Skip games without enough moves to match, if they
            // have the standard starting position:
            db->dbFilter->Set (gameNum, 0);
            skipcount++;
            continue;
        }

        bool possibleMatch = true;
        bool possibleFlippedMatch = flip;

        // First, eliminate games that cannot match from their final
        // material signature:
        if (checkMsig  &&  !matsig_isReachable (msig, ie->GetFinalMatSig(),
                                                ie->GetPromotionsFlag(),
                                                ie->GetUnderPromoFlag()))
        {
            possibleMatch = false;
        }
        if (flip  &&  checkMsig
                &&  !matsig_isReachable (msigFlipped, ie->GetFinalMatSig(),
                                         ie->GetPromotionsFlag(),
                                         ie->GetUnderPromoFlag()))
        {
            possibleFlippedMatch = false;
        }

        // If the game has a final home pawn that cannot appear in the
        // patterns, exclude it. For example, a White IQP search has no
        // white c or e pawns, so any game that ends with a c2 or e2 pawn
        // at home need not be loaded:

        if (possibleMatch  &&  hpExcludeMask != HPSIG_Empty) {
            uint gameFinalHP = hpSig_Final (ie->GetHomePawnData());
            // If any bit is set in both, this game cannot match:
            if ((gameFinalHP & hpExcludeMask) != 0) {
                possibleMatch = false;
            }
        }
        if (possibleFlippedMatch  &&  hpExMaskFlip != HPSIG_Empty) {
            uint gameFinalHP = hpSig_Final (ie->GetHomePawnData());
            // If any bit is set in both, this game cannot match:
            if ((gameFinalHP & hpExMaskFlip) != 0) {
                possibleFlippedMatch = false;
            }
        }

        if (!possibleMatch  &&  !possibleFlippedMatch) {
            db->dbFilter->Set (gameNum, 0);
            skipcount++;
            continue;
        }

        // Now, the game must be loaded and searched:

        if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(),
                                 ie->GetLength()) != OK) {
            continue;
        }

        bool result = false;

        if (!matchEndOnly) {
	    if (possibleMatch) {
		result = g->MaterialMatch (db->bbuf, min, max, patt,
					   minPly, maxPly, matchLength,
					   oppBishops, sameBishops,
					   matDiff[0], matDiff[1]);
	    }
	    if (result == 0  &&  possibleFlippedMatch) {
		db->bbuf->BackToStart();
		result = g->MaterialMatch (db->bbuf, minFlipped, maxFlipped,
					   flippedPatt, minPly, maxPly,
					   matchLength, oppBishops, sameBishops,
					   matDiff[0], matDiff[1]);
	    }
        } else {
            // Check the end position only - S.A
	    //
	    // Can we use DecodeVariation ? ... but it is private
	    // Or is there a better way to do this

	    g->Decode (db->bbuf, GAME_DECODE_ALL);
	    while (g->MoveForward() == OK) {} ;
	    g->MoveBackup();

	    if (possibleMatch) {
		result = g->MaterialMatch (NULL, min, max, patt,
					   minPly, maxPly, matchLength,
					   oppBishops, sameBishops,
					   matDiff[0], matDiff[1]);
	    }
	    if (result == 0  &&  possibleFlippedMatch) {
		while (g->MoveForward() == OK) {} ;
		g->MoveBackup();
		result = g->MaterialMatch (NULL, minFlipped, maxFlipped,
					   flippedPatt, minPly, maxPly,
					   matchLength, oppBishops, sameBishops,
					   matDiff[0], matDiff[1]);
	    }
        }

        if (result) {
            // update the filter value to the current ply:
            uint plyOfMatch = g->GetCurrentPly() + 1 - matchLength;
            byte b = (byte) (plyOfMatch + 1);
            if (b == 0) { b = 1; }
            db->dbFilter->Set (gameNum, b);
        } else {
            // This game did NOT match:
            db->dbFilter->Set (gameNum, 0);
        }
    }

    freePatternList (patt);
    freePatternList (flippedPatt);
    if (showProgress) { updateProgressBar (ti, 1, 1); }

    int centisecs = timer.CentiSecs();

    if (gameNum != db->numGames) {
        Tcl_AppendResult (ti, errMsgSearchInterrupted(ti), "  ", NULL);
    }
    sprintf (temp, "%d / %d  (%d%c%02d s)",
             db->dbFilter->Count(), startFilterCount,
             centisecs / 100, decimalPointChar, centisecs % 100);
    Tcl_AppendResult (ti, temp, NULL);
#ifdef SHOW_SKIPPED_STATS
    sprintf(temp, "  Skipped %u games.", skipcount);
    Tcl_AppendResult (ti, temp, NULL);
#endif

    return TCL_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// matchGameFlags():
//    Called by sc_search_header to test a particular game against the
//    specified index flag restrictions, for example, excluding
//    deleted games or games without comments.
bool
matchGameFlags (IndexEntry * ie, flagT fStdStart, flagT fPromos, flagT fUnderPromo,
                flagT fComments, flagT fVars, flagT fNags, flagT fDelete,
                flagT fWhiteOp, flagT fBlackOp, flagT fMiddle,
                flagT fEndgame, flagT fNovelty, flagT fPawn,
                flagT fTactics, flagT fKside, flagT fQside,
                flagT fBrill, flagT fBlunder, flagT fUser,
                flagT fCustom1, flagT fCustom2, flagT fCustom3,
                flagT fCustom4, flagT fCustom5, flagT fCustom6 )
{
    bool flag;

    flag = ie->GetStartFlag();
    if ((flag && !flag_Yes(fStdStart))  ||  (!flag && !flag_No(fStdStart))) {
        return false;
    }

    flag = ie->GetUnderPromoFlag();
    if ((flag && !flag_Yes(fUnderPromo))  ||  (!flag && !flag_No(fUnderPromo))) {
        return false;
    }

    flag = ie->GetPromotionsFlag();
    if ((flag && !flag_Yes(fPromos))  ||  (!flag && !flag_No(fPromos))) {
        return false;
    }

    flag = ie->GetCommentsFlag();
    if ((flag && !flag_Yes(fComments))  ||  (!flag && !flag_No(fComments))) {
        return false;
    }

    flag = ie->GetVariationsFlag();
    if ((flag && !flag_Yes(fVars))  ||  (!flag && !flag_No(fVars))) {
        return false;
    }

    flag = ie->GetNagsFlag();
    if ((flag && !flag_Yes(fNags))  ||  (!flag && !flag_No(fNags))) {
        return false;
    }

    flag = ie->GetDeleteFlag();
    if ((flag && !flag_Yes(fDelete))  ||  (!flag && !flag_No(fDelete))) {
        return false;
    }

    flag = ie->GetWhiteOpFlag();
    if ((flag && !flag_Yes(fWhiteOp))  ||  (!flag && !flag_No(fWhiteOp))) {
        return false;
    }

    flag = ie->GetBlackOpFlag();
    if ((flag && !flag_Yes(fBlackOp))  ||  (!flag && !flag_No(fBlackOp))) {
        return false;
    }

    flag = ie->GetMiddlegameFlag();
    if ((flag && !flag_Yes(fMiddle))  ||  (!flag && !flag_No(fMiddle))) {
        return false;
    }

    flag = ie->GetEndgameFlag();
    if ((flag && !flag_Yes(fEndgame))  ||  (!flag && !flag_No(fEndgame))) {
        return false;
    }

    flag = ie->GetNoveltyFlag();
    if ((flag && !flag_Yes(fNovelty))  ||  (!flag && !flag_No(fNovelty))) {
        return false;
    }

    flag = ie->GetPawnStructFlag();
    if ((flag && !flag_Yes(fPawn))  ||  (!flag && !flag_No(fPawn))) {
        return false;
    }

    flag = ie->GetTacticsFlag();
    if ((flag && !flag_Yes(fTactics))  ||  (!flag && !flag_No(fTactics))) {
        return false;
    }

    flag = ie->GetKingsideFlag();
    if ((flag && !flag_Yes(fKside))  ||  (!flag && !flag_No(fKside))) {
        return false;
    }

    flag = ie->GetQueensideFlag();
    if ((flag && !flag_Yes(fQside))  ||  (!flag && !flag_No(fQside))) {
        return false;
    }

    flag = ie->GetBrilliancyFlag();
    if ((flag && !flag_Yes(fBrill))  ||  (!flag && !flag_No(fBrill))) {
        return false;
    }

    flag = ie->GetBlunderFlag();
    if ((flag && !flag_Yes(fBlunder))  ||  (!flag && !flag_No(fBlunder))) {
        return false;
    }

    flag = ie->GetUserFlag();
    if ((flag && !flag_Yes(fUser))  ||  (!flag && !flag_No(fUser))) {
        return false;
    }

    flag = ie->GetCustomFlag(1);
    if ((flag && !flag_Yes(fCustom1))  ||  (!flag && !flag_No(fCustom1))) {
      return false;
    }

    flag = ie->GetCustomFlag(2);
    if ((flag && !flag_Yes(fCustom2))  ||  (!flag && !flag_No(fCustom2))) {
      return false;
    }

    flag = ie->GetCustomFlag(3);
    if ((flag && !flag_Yes(fCustom3))  ||  (!flag && !flag_No(fCustom3))) {
      return false;
    }

    flag = ie->GetCustomFlag(4);
    if ((flag && !flag_Yes(fCustom4))  ||  (!flag && !flag_No(fCustom4))) {
      return false;
    }

    flag = ie->GetCustomFlag(5);
    if ((flag && !flag_Yes(fCustom5))  ||  (!flag && !flag_No(fCustom5))) {
      return false;
    }

    flag = ie->GetCustomFlag(6);
    if ((flag && !flag_Yes(fCustom6))  ||  (!flag && !flag_No(fCustom6))) {
      return false;
    }

    // If we reach here, the game matched all flag restrictions.
    return true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// matchGameHeader():
//    Called by sc_search_header to test a particular game against the
//    header search criteria.
bool
matchGameHeader (IndexEntry * ie, NameBase * nb,
                 bool * mWhite, bool * mBlack,
                 bool * mEvent, bool * mSite, bool *mRound,
                 dateT dateMin, dateT dateMax, bool * results,
                 int weloMin, int weloMax, int beloMin, int beloMax,
                 int diffeloMin, int diffeloMax,
                 ecoT ecoMin, ecoT ecoMax, bool ecoNone,
                 uint halfmovesMin, uint halfmovesMax,
                 bool wToMove, bool bToMove)
{
    // First, check the numeric ranges:

    if (!results[ie->GetResult()]) { return false; }

    uint halfmoves = ie->GetNumHalfMoves();
    if (halfmoves < halfmovesMin  ||  halfmoves > halfmovesMax) {
        return false;
    }
    if ((halfmoves % 2) == 0) {
        // This game ends with White to move:
        if (! wToMove) { return false; }
    } else {
        // This game ends with Black to move:
        if (! bToMove) { return false; }
    }


    dateT date = ie->GetDate();
    if (date < dateMin  ||  date > dateMax) { return false; }

    // Check Elo ratings:
    int whiteElo = (int) ie->GetWhiteElo();
    int blackElo = (int) ie->GetBlackElo();
    if (whiteElo == 0) { whiteElo = nb->GetElo (ie->GetWhite()); }
    if (blackElo == 0) { blackElo = nb->GetElo (ie->GetBlack()); }

    int diffElo = whiteElo - blackElo;
    // Elo difference used to be absolute difference, but now it is
    // just white rating minus black rating, so leave next line commented:
    //if (diffElo < 0) { diffElo = -diffElo; }

    if (whiteElo < weloMin  ||  whiteElo > weloMax) { return false; }
    if (blackElo < beloMin  ||  blackElo > beloMax) { return false; }
    if (diffElo < diffeloMin  ||  diffElo > diffeloMax) { return false; }

    ecoT ecoCode = ie->GetEcoCode();
    if (ecoCode == ECO_None) {
        if (!ecoNone) { return false; }
    } else {
        if (ecoCode < ecoMin  ||  ecoCode > ecoMax) { return false; }
    }

    // Now check the event, site and round fields:
    if (mEvent != NULL  &&  !mEvent[ie->GetEvent()]) { return false; }
    if (mSite != NULL  &&  !mSite[ie->GetSite()]) { return false; }
    if (mRound != NULL  &&  !mRound[ie->GetRound()]) { return false; }

    // Last, we check the players
    if (mWhite != NULL  &&  !mWhite[ie->GetWhite()]) { return false; }
    if (mBlack != NULL  &&  !mBlack[ie->GetBlack()]) { return false; }

    // If we reach here, this game matches all criteria.
    return true;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// strIsWildcardSearch:
//     Returns true if the specified string starts and ends with a
//     double quote (") character, indicating it is a wildcard search.
bool
strIsWildcardSearch (const char * str)
{
    if (str == NULL  ||  str[0] != '"') { return false; }
    uint len = strLength (str);
    if (len < 2) { return false; }
    if (str[len-1] != '"') { return false; }
    return true;
}

const uint NUM_TITLES = 8;
enum {
    TITLE_GM, TITLE_IM, TITLE_FM, TITLE_CM, TITLE_WGM, TITLE_WIM, TITLE_WFM, TITLE_NONE
};
const char * titleStr [NUM_TITLES] = {
    "gm", "im", "fm", "cm", "wgm", "wim", "wfm", "none"
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// parseTitles:
//    Called from sc_search_header to parse a list
//    of player titles to be searched for. The provided
//    string should have some subset of the elements
//    gm, im, fm, wgm, wim, w and none, each separated
//    by whitespace. Example: "gm wgm" would indicate
//    to only search for games by a GM or WIM.
bool *
parseTitles (const char * str)
{
    bool * titles = new bool [NUM_TITLES];
    for (uint t=0; t < NUM_TITLES; t++) { titles[t] = false; }

    str = strFirstWord (str);
    while (*str != 0) {
        for (uint i=0; i < NUM_TITLES; i++) {
            if (strIsCasePrefix (titleStr[i], str)) {
                titles[i] = true;
                break;
            }
        }
        str = strNextWord (str);
    }
    return titles;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_search_header:
//    Searches by header information. (also pgn text, and checkmate/stalemate)
int
sc_search_header (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool showProgress = startProgressBar();
    if (! db->inUse) {
        return errorResult (ti, errMsgNotOpen(ti));
    }

    char * sWhite = NULL;
    char * sBlack = NULL;
    char * sEvent = NULL;
    char * sSite  = NULL;
    char * sRound = NULL;
    char * sTag   = NULL;
    char * sTagValue = NULL;

    bool * mWhite = NULL;
    bool * mBlack = NULL;
    bool * mEvent = NULL;
    bool * mSite = NULL;
    bool * mRound = NULL;

    dateT dateRange[2];
    dateRange[0] = ZERO_DATE;
    dateRange[1] = DATE_MAKE (YEAR_MAX, 12, 31);

    bool results [NUM_RESULT_TYPES];
    bool resultsF [NUM_RESULT_TYPES];  // Flipped results for ignore-colors.
    results[RESULT_White] = results[RESULT_Black] = true;
    results[RESULT_Draw] = results[RESULT_None] = true;

    uint wEloRange [2];   // White rating range
    uint bEloRange [2];   // Black rating range
    int  dEloRange [2];   // Rating difference (White minus Black) range
    wEloRange[0] = bEloRange[0] = 0;
    wEloRange[1] = bEloRange[1] = MAX_ELO;
    dEloRange[0] = - (int)MAX_ELO;
    dEloRange[1] = MAX_ELO;

    bool * wTitles = NULL;
    bool * bTitles = NULL;

    bool wToMove = true;
    bool bToMove = true;

    uint halfMoveRange[2];
    halfMoveRange[0] = 0;
    halfMoveRange[1] = 999;

    ecoT ecoRange [2];    // ECO code range.
    ecoRange[0] = eco_FromString ("A00");
    ecoRange[1] = eco_FromString ("E99");
    bool ecoNone = true;  // Whether to include games with no ECO code.

    // gameNumRange: a range of game numbers to search.
    int gameNumRange[2];
    gameNumRange[0] = 1;   // Default: start searching at 1st game.
    gameNumRange[1] = -1;  // Default: stop searching at last game.

    bool ignoreColors = false;
    bool preComment = false;
    bool postComment = false;
    filterOpT filterOp = FILTEROP_RESET;

    flagT fStdStart = FLAG_BOTH;
    flagT fPromotions = FLAG_BOTH;
    flagT fUnderPromo = FLAG_BOTH;
    flagT fComments = FLAG_BOTH;
    flagT fVariations = FLAG_BOTH;
    flagT fAnnotations = FLAG_BOTH;
    flagT fDelete = FLAG_BOTH;
    flagT fWhiteOp = FLAG_BOTH;
    flagT fBlackOp = FLAG_BOTH;
    flagT fMiddlegame = FLAG_BOTH;
    flagT fEndgame = FLAG_BOTH;
    flagT fNovelty = FLAG_BOTH;
    flagT fPawnStruct = FLAG_BOTH;
    flagT fTactics = FLAG_BOTH;
    flagT fKside = FLAG_BOTH;
    flagT fQside = FLAG_BOTH;
    flagT fBrilliancy = FLAG_BOTH;
    flagT fBlunder = FLAG_BOTH;
    flagT fUser = FLAG_BOTH;
    flagT fCustom1 = FLAG_BOTH;
    flagT fCustom2 = FLAG_BOTH;
    flagT fCustom3 = FLAG_BOTH;
    flagT fCustom4 = FLAG_BOTH;
    flagT fCustom5 = FLAG_BOTH;
    flagT fCustom6 = FLAG_BOTH;

    int pgnTextCount = 0;
    char ** sPgnText = NULL;
    bool ignoreCase = 0;
    char gameEnd = 'A'; // Any

    const char * options[] = {
        "white", "black", "event", "site", "round",
        "date", "results", "welo", "belo", "delo",
        "wtitles", "btitles", "toMove",
        "eco", "length", "gameNumber", "flip", "filter",
        "fStdStart", "fPromotions", "fUnderPromo", "fComments", "fVariations",
        "fAnnotations", "fDelete", "fWhiteOpening", "fBlackOpening",
        "fMiddlegame", "fEndgame", "fNovelty", "fPawnStructure",
        "fTactics", "fKingside", "fQueenside", "fBrilliancy", "fBlunder",
        "fUser", "fCustom1" , "fCustom2" , "fCustom3" , "extratag", "tagvalue",
        "fCustom4" , "fCustom5" , "fCustom6" , "pgn", "ignoreCase", "gameend", "preComment", "postComment", NULL
    };
    enum {
        OPT_WHITE, OPT_BLACK, OPT_EVENT, OPT_SITE, OPT_ROUND,
        OPT_DATE, OPT_RESULTS, OPT_WELO, OPT_BELO, OPT_DELO,
        OPT_WTITLES, OPT_BTITLES, OPT_TOMOVE,
        OPT_ECO, OPT_LENGTH, OPT_GAMENUMBER, OPT_FLIP, OPT_FILTER,
        OPT_FSTDSTART, OPT_FPROMOTIONS, OPT_FUNDERPROMO, OPT_FCOMMENTS, OPT_FVARIATIONS,
        OPT_FANNOTATIONS, OPT_FDELETE, OPT_FWHITEOP, OPT_FBLACKOP,
        OPT_FMIDDLEGAME, OPT_FENDGAME, OPT_FNOVELTY, OPT_FPAWNSTRUCT,
        OPT_FTACTICS, OPT_FKSIDE, OPT_FQSIDE, OPT_FBRILLIANCY, OPT_FBLUNDER,
        OPT_FUSER, OPT_FCUSTOM1, OPT_FCUSTOM2, OPT_FCUSTOM3, OPT_TAG, OPT_TAGVALUE,
        OPT_FCUSTOM4,  OPT_FCUSTOM5, OPT_FCUSTOM6, OPT_PGN, OPT_PGNCASE, OPT_GAMEEND, OPT_PRECOMMENT, OPT_POSTCOMMENT
    };

    int arg = 2;
    while (arg+1 < argc) {
        const char * option = argv[arg];
        const char * value = argv[arg+1];
        arg += 2;
        int index = -1;
        if (option[0] == '-') {
            index = strUniqueMatch (&(option[1]), options);
        }

        switch (index) {
        case OPT_WHITE:
            sWhite = strDuplicate (value);
            break;

        case OPT_BLACK:
            sBlack = strDuplicate (value);
            break;

        case OPT_EVENT:
            sEvent = strDuplicate (value);
            break;

        case OPT_SITE:
            sSite = strDuplicate (value);
            break;

        case OPT_ROUND:
            sRound = strDuplicate (value);
            break;

        case OPT_TAG:
            sTag = strDuplicate (value);
            break;

        case OPT_TAGVALUE:
            sTagValue = strDuplicate (value);
            break;

        case OPT_DATE:
            // Extract two whitespace-separated dates:
            value = strFirstWord (value);
            dateRange[0] = date_EncodeFromString (value);
            value = strNextWord (value);
            dateRange[1] = date_EncodeFromString (value);
            break;

        case OPT_RESULTS:
            // Extract four results in the order 1-0, draw, 0-1, none:
            value = strFirstWord (value);
            results[RESULT_White] = strGetBoolean (value);
            value = strNextWord (value);
            results[RESULT_Draw] = strGetBoolean (value);
            value = strNextWord (value);
            results[RESULT_Black] = strGetBoolean (value);
            value = strNextWord (value);
            results[RESULT_None] = strGetBoolean (value);
            break;

        case OPT_WELO:
            strGetUnsigneds (value, wEloRange, 2);
            break;

        case OPT_BELO:
            strGetUnsigneds (value, bEloRange, 2);
            break;

        case OPT_DELO:
            strGetIntegers (value, dEloRange, 2);
            break;

        case OPT_WTITLES:
            wTitles = parseTitles (value);
            break;

        case OPT_BTITLES:
            bTitles = parseTitles (value);
            break;

        case OPT_TOMOVE:
            wToMove = false;
            if (strFirstChar (value, 'w')  || strFirstChar (value, 'W')) {
                wToMove = true;
            }
            bToMove = false;
            if (strFirstChar (value, 'b')  || strFirstChar (value, 'B')) {
                bToMove = true;
            }
            break;

        case OPT_ECO:
            // Extract two whitespace-separated ECO codes then a boolean:
            value = strFirstWord (value);
            ecoRange[0] = eco_FromString (value);
            value = strNextWord (value);
            ecoRange[1] = eco_FromString (value);
            value = strNextWord (value);
            ecoNone = strGetBoolean (value);
            break;

        case OPT_LENGTH:
            strGetUnsigneds (value, halfMoveRange, 2);
            break;

        case OPT_GAMENUMBER:
            strGetIntegers (value, gameNumRange, 2);
            break;

        case OPT_FLIP:
            ignoreColors = strGetBoolean (value);
            break;

        case OPT_FILTER:
            filterOp = strGetFilterOp (value);
            break;

        case OPT_FSTDSTART:    fStdStart    = strGetFlag (value); break;
        case OPT_FPROMOTIONS:  fPromotions  = strGetFlag (value); break;
        case OPT_FUNDERPROMO:  fUnderPromo  = strGetFlag (value); break;
        case OPT_FCOMMENTS:    fComments    = strGetFlag (value); break;
        case OPT_FVARIATIONS:  fVariations  = strGetFlag (value); break;
        case OPT_FANNOTATIONS: fAnnotations = strGetFlag (value); break;
        case OPT_FDELETE:      fDelete      = strGetFlag (value); break;
        case OPT_FWHITEOP:     fWhiteOp     = strGetFlag (value); break;
        case OPT_FBLACKOP:     fBlackOp     = strGetFlag (value); break;
        case OPT_FMIDDLEGAME:  fMiddlegame  = strGetFlag (value); break;
        case OPT_FENDGAME:     fEndgame     = strGetFlag (value); break;
        case OPT_FNOVELTY:     fNovelty     = strGetFlag (value); break;
        case OPT_FPAWNSTRUCT:  fPawnStruct  = strGetFlag (value); break;
        case OPT_FTACTICS:     fTactics     = strGetFlag (value); break;
        case OPT_FKSIDE:       fKside       = strGetFlag (value); break;
        case OPT_FQSIDE:       fQside       = strGetFlag (value); break;
        case OPT_FBRILLIANCY:  fBrilliancy  = strGetFlag (value); break;
        case OPT_FBLUNDER:     fBlunder     = strGetFlag (value); break;
        case OPT_FCUSTOM1:     fCustom1     = strGetFlag (value); break;
        case OPT_FCUSTOM2:     fCustom2     = strGetFlag (value); break;
        case OPT_FCUSTOM3:     fCustom3     = strGetFlag (value); break;
        case OPT_FCUSTOM4:     fCustom4     = strGetFlag (value); break;
        case OPT_FCUSTOM5:     fCustom5     = strGetFlag (value); break;
        case OPT_FCUSTOM6:     fCustom6     = strGetFlag (value); break;
        case OPT_FUSER:        fUser        = strGetFlag (value); break;

        case OPT_PGN:
            if (Tcl_SplitList (ti, (char *)value, &pgnTextCount,
                               (CONST84 char ***) &sPgnText) != TCL_OK) {
                return TCL_ERROR;
            }
            break;

        case OPT_PGNCASE:
            ignoreCase = strGetBoolean (value);
            break;

        case OPT_GAMEEND:
            gameEnd = value[0];
            break;

        case OPT_PRECOMMENT:
            preComment = strGetBoolean (value);
            break;

        case OPT_POSTCOMMENT:
            postComment = strGetBoolean (value);
            break;

        default:
            return InvalidCommand (ti, "sc_search header", options);
        }
    }
    if (arg != argc) { return errorResult (ti, "Odd number of parameters."); }

    // Set up White name matches array:
    if (sWhite != NULL  &&  sWhite[0] != 0) {
        char * search = sWhite;
        bool wildcard = false;
        if (strIsWildcardSearch (search)) {
            wildcard = true;
            search[strLength(search) - 1] = 0;
            search++;
        }
        // Search players for match on White name:
        idNumberT numNames = db->nb->GetNumNames(NAME_PLAYER);
        mWhite = new bool [numNames];
        for (idNumberT i=0; i < numNames; i++) {
            const char * name = db->nb->GetName (NAME_PLAYER, i);
            if (wildcard) {
                mWhite[i] = (Tcl_StringMatch (name, search) ? true : false);
            } else {
                mWhite[i] = strAlphaContains (name, search);
            }
        }
    }
    if (wTitles != NULL  &&  spellChecker[NAME_PLAYER] != NULL) {
        bool allTitlesOn = true;
        for (uint t=0; t < NUM_TITLES; t++) {
            if (! wTitles[t]) { allTitlesOn = false; break; }
        }
        if (! allTitlesOn) {
            idNumberT i;
            idNumberT numNames = db->nb->GetNumNames(NAME_PLAYER);
            if (mWhite == NULL) {
                mWhite = new bool [numNames];
                for (i=0; i < numNames; i++) { mWhite[i] = true; }
            }
            for (i=0; i < numNames; i++) {
                if (! mWhite[i]) { continue; }
                const char * name = db->nb->GetName (NAME_PLAYER, i);
                const char * text =
                    spellChecker[NAME_PLAYER]->GetCommentExact (name);
                const char * title = SpellChecker::GetTitle (text);
                if ((!wTitles[TITLE_GM]  &&  strEqual(title, "gm"))
                    || (!wTitles[TITLE_GM]  &&  strEqual(title, "hgm"))
                    || (!wTitles[TITLE_IM]  &&  strEqual(title, "im"))
                    || (!wTitles[TITLE_FM]  &&  strEqual(title, "fm"))
                    || (!wTitles[TITLE_CM]  &&  strEqual(title, "cm"))
                    || (!wTitles[TITLE_WGM]  &&  strEqual(title, "wgm"))
                    || (!wTitles[TITLE_WIM]  &&  strEqual(title, "wim"))
                    || (!wTitles[TITLE_WFM]  &&  strEqual(title, "wfm"))
                    || (!wTitles[TITLE_NONE]  &&  strEqual(title, ""))
                    || (!wTitles[TITLE_NONE]  &&  strEqual(title, "cgm"))
                    || (!wTitles[TITLE_NONE]  &&  strEqual(title, "cim"))) {
                    mWhite[i] = false;
                }
            }
        }
    }

    // Set up Black name matches array:
    if (sBlack != NULL  &&  sBlack[0] != 0) {
        char * search = sBlack;
        bool wildcard = false;
        if (strIsWildcardSearch (search)) {
            wildcard = true;
            search[strLength(search) - 1] = 0;
            search++;
        }
        // Search players for match on Black name:
        idNumberT numNames = db->nb->GetNumNames(NAME_PLAYER);
        mBlack = new bool [numNames];
        for (idNumberT i=0; i < numNames; i++) {
            const char * name = db->nb->GetName (NAME_PLAYER, i);
            if (wildcard) {
                mBlack[i] = (Tcl_StringMatch (name, search) ? true : false);
            } else {
                mBlack[i] = strAlphaContains (name, search);
            }
        }
    }
    if (bTitles != NULL  &&  spellChecker[NAME_PLAYER] != NULL) {
        bool allTitlesOn = true;
        for (uint t=0; t < NUM_TITLES; t++) {
            if (!bTitles[t]) { allTitlesOn = false; break; }
        }
        if (! allTitlesOn) {
            idNumberT i;
            idNumberT numNames = db->nb->GetNumNames(NAME_PLAYER);
            if (mBlack == NULL) {
                mBlack = new bool [numNames];
                for (i=0; i < numNames; i++) { mBlack[i] = true; }
            }
            for (i=0; i < numNames; i++) {
                if (! mBlack[i]) { continue; }
                const char * name = db->nb->GetName (NAME_PLAYER, i);
                const char * text =
                    spellChecker[NAME_PLAYER]->GetCommentExact (name);
                const char * title = SpellChecker::GetTitle (text);
                if ((!bTitles[TITLE_GM]  &&  strEqual(title, "gm"))
                    || (!bTitles[TITLE_GM]  &&  strEqual(title, "hgm"))
                    || (!bTitles[TITLE_IM]  &&  strEqual(title, "im"))
                    || (!bTitles[TITLE_FM]  &&  strEqual(title, "fm"))
                    || (!bTitles[TITLE_CM]  &&  strEqual(title, "cm"))
                    || (!bTitles[TITLE_WGM]  &&  strEqual(title, "wgm"))
                    || (!bTitles[TITLE_WIM]  &&  strEqual(title, "wim"))
                    || (!bTitles[TITLE_WFM]  &&  strEqual(title, "wfm"))
                    || (!bTitles[TITLE_NONE]  &&  strEqual(title, ""))
                    || (!bTitles[TITLE_NONE]  &&  strEqual(title, "cgm"))
                    || (!bTitles[TITLE_NONE]  &&  strEqual(title, "cim"))) {
                    mBlack[i] = false;
                }
            }
        }
    }

    // Set up Event name matches array:
    if (sEvent != NULL  &&  sEvent[0] != 0) {
        char * search = sEvent;
        bool wildcard = false;
        if (strIsWildcardSearch (search)) {
            wildcard = true;
            search[strLength(search) - 1] = 0;
            search++;
        }
        // Search players for match on Event name:
        idNumberT numNames = db->nb->GetNumNames(NAME_EVENT);
        mEvent = new bool [numNames];
        for (idNumberT i=0; i < numNames; i++) {
            const char * name = db->nb->GetName (NAME_EVENT, i);
            if (wildcard) {
                mEvent[i] = (Tcl_StringMatch (name, search) ? true : false);
            } else {
                mEvent[i] = strAlphaContains (name, search);
            }
        }
    }

    // Set up Site name matches array:
    if (sSite != NULL  &&  sSite[0] != 0) {
        char * search = sSite;
        bool wildcard = false;
        if (strIsWildcardSearch (search)) {
            wildcard = true;
            search[strLength(search) - 1] = 0;
            search++;
        }
        // Search players for match on Site name:
        idNumberT numNames = db->nb->GetNumNames(NAME_SITE);
        mSite = new bool [numNames];
        for (idNumberT i=0; i < numNames; i++) {
            const char * name = db->nb->GetName (NAME_SITE, i);
            if (wildcard) {
                mSite[i] = (Tcl_StringMatch (name, search) ? true : false);
            } else {
                mSite[i] = strAlphaContains (name, search);
            }
        }
    }

    // Set up Round name matches array:
    if (sRound != NULL  &&  sRound[0] != 0) {
        char * search = sRound;
        bool wildcard = false;
        if (strIsWildcardSearch (search)) {
            wildcard = true;
            search[strLength(search) - 1] = 0;
            search++;
        }
        // Search players for match on Event name:
        idNumberT numNames = db->nb->GetNumNames(NAME_ROUND);
        mRound = new bool [numNames];
        for (idNumberT i=0; i < numNames; i++) {
            const char * name = db->nb->GetName (NAME_ROUND, i);
            if (wildcard) {
                mRound[i] = (Tcl_StringMatch (name, search) ? true : false);
            } else {
                mRound[i] = strAlphaContains (name, search);
            }
        }
    }

    // Set up flipped results flags for ignore-colors option:
    resultsF[RESULT_White] = results[RESULT_Black];
    resultsF[RESULT_Draw]  = results[RESULT_Draw];
    resultsF[RESULT_Black] = results[RESULT_White];
    resultsF[RESULT_None]  = results[RESULT_None];

    // Swap rating difference values if necesary:
    if (dEloRange[0] > dEloRange[1]) {
        int x = dEloRange[0]; dEloRange[0] = dEloRange[1]; dEloRange[1] = x;
    }

    // Set eco maximum to be the largest subcode, for example,
    // "B07" -> "B07z4" to make sure subcodes are included in the range:
    ecoRange[1] = eco_LastSubCode (ecoRange[1]);

    // Set up game number range:
    // Note that a negative number means a count from the end,
    // so -1 = last game, -2 = second to last, etc.
    // Convert any negative values to positive:
    if (gameNumRange[0] < 0) { gameNumRange[0] += db->numGames + 1; }
    if (gameNumRange[1] < 0) { gameNumRange[1] += db->numGames + 1; }
    if (gameNumRange[0] < 0) { gameNumRange[0] = 0; }
    if (gameNumRange[1] < 0) { gameNumRange[1] = 0; }
    uint gameNumMin = (uint) gameNumRange[0];
    uint gameNumMax = (uint) gameNumRange[1];
    if (gameNumMin > db->numGames) { gameNumMin = db->numGames; }
    if (gameNumMax > db->numGames) { gameNumMax = db->numGames; }
    // Swap them if necessary so min <= max:
    if (gameNumMin > gameNumMax) {
        uint temp = gameNumMin; gameNumMin = gameNumMax; gameNumMax = temp;
    }

    Timer timer;  // Start timing this search.

    uint skipcount = 0;
    uint startFilterCount = startFilterSize (db, filterOp);
    char temp[250];
    IndexEntry * ie;
    uint updateStart, update;
    updateStart = update = 5000;  // Update progress bar every 5000 games

    // If filter operation is to reset the filter, reset it:
    if (filterOp == FILTEROP_RESET) {
        filter_reset (db, 1);
        filterOp = FILTEROP_AND;
    }

    bool readGame = ((sTag && sTag[0]) || pgnTextCount > 0 || gameEnd == 'S' || gameEnd == 'C' || preComment || postComment);

    // Here is the loop that searches on each game:
    for (uint i=0; i < db->numGames; i++) {
        if (showProgress) {  // Update the percentage done bar:
            update--;
            if (update == 0) {
                update = updateStart;
                updateProgressBar (ti, i, db->numGames);
                if (interruptedProgress()) {
                    break;
                }
            }
        }
        // First, apply the filter operation:
        if (filterOp == FILTEROP_AND) {  // Skip any games not in the filter:
            if (db->dbFilter->Get(i) == 0) {
                skipcount++;
                continue;
            }
        } else /* filterOp == FILTEROP_OR*/ { // Skip any games in the filter:
            if (db->dbFilter->Get(i) != 0) {
                skipcount++;
                continue;
            } else {
                // OK, this game is NOT in the filter.
                // Add it so filterCounts are kept up to date:
                db->dbFilter->Set (i, 1);
            }
        }

        // Skip games outside the specified game number range:
        if (i+1 < gameNumMin  ||  i+1 > gameNumMax) {
            db->dbFilter->Set (i, 0);
            skipcount++;
            continue;
        }

        ie = db->idx->FetchEntry (i);
        if (ie->GetLength() == 0) {  // Skip games with no gamefile record
            db->dbFilter->Set (i, 0);
            skipcount++;
            continue;
        }

        bool match = false;

        if (matchGameFlags (ie, fStdStart, fPromotions, fUnderPromo,
                            fComments, fVariations, fAnnotations, fDelete,
                            fWhiteOp, fBlackOp, fMiddlegame, fEndgame,
                            fNovelty, fPawnStruct, fTactics, fKside,
                            fQside, fBrilliancy, fBlunder, fUser,
                            fCustom1, fCustom2, fCustom3, fCustom4, fCustom5, fCustom6
                           )) {
            if (matchGameHeader (ie, db->nb, mWhite, mBlack,
                                 mEvent, mSite, mRound,
                                 dateRange[0], dateRange[1], results,
                                 wEloRange[0], wEloRange[1],
                                 bEloRange[0], bEloRange[1],
                                 dEloRange[0], dEloRange[1],
                                 ecoRange[0], ecoRange[1], ecoNone,
                                 halfMoveRange[0], halfMoveRange[1],
                                 wToMove, bToMove)) {
                match = true;
            }

            // Try with inverted players/ratings/results if ignoring colors:

            if (!match  &&  ignoreColors  &&
                matchGameHeader (ie, db->nb, mBlack, mWhite,
                                 mEvent, mSite, mRound,
                                 dateRange[0], dateRange[1], resultsF,
                                 bEloRange[0], bEloRange[1],
                                 wEloRange[0], wEloRange[1],
                                 -dEloRange[1], -dEloRange[0],
                                 ecoRange[0], ecoRange[1], ecoNone,
                                 halfMoveRange[0], halfMoveRange[1],
                                 bToMove, wToMove)) {
                match = true;
            }
        }


        // Stalemate or Checkmate ??
        // - or -
        // Now try to match the comment text if applicable:
        // Note that it is not worth using a faster staring search
        // algorithm like Boyer-Moore or Knuth-Morris-Pratt since
        // profiling showed most that most of the time is spent
        // generating the PGN representation of each game.

        if (match  &&  readGame) {
            if (db->gfile->ReadGame (db->bbuf, ie->GetOffset(), ie->GetLength()) != OK) {
                match = false;
            }

            // Extra Tag search feature S.A.

            if (match && sTag && sTag[0]) {
                scratchGame->Clear();
                if (scratchGame->DecodeTags (db->bbuf, GAME_DECODE_ALL) != OK)
                    match = false;
                if (match) {
                    match = false;
                    uint numtags = scratchGame->GetNumExtraTags();
                    tagT *tag = scratchGame->GetExtraTags();

                    for (uint j=0; j<numtags; j++, tag++) {
                        if (!strcmp(tag->tag, sTag)) {
                            match = true;
                            // We have a match for the tag, now does it match the second pgnText
                            if (sTagValue && sTagValue[0]) {
                                // tag search does a prefix search for ascii, but exact search for numbers
                                if (isdigit(sTagValue[0]))
				    match = (strcmp(tag->value, sTagValue) == 0);
                                else
				    match = strIsPrefix(sTagValue, tag->value);
                            }
                            break;
                        }
                    }
                }
		scratchGame->Clear();
		db->bbuf->BackToStart();
            }


            if (match  &&  scratchGame->Decode (db->bbuf, GAME_DECODE_ALL) != OK) {
                match = false;
            }


            if (match) {

	      if (gameEnd == 'S' || gameEnd == 'C') {
                // Game is actually at endPos, so this line should be removed
		while (scratchGame->MoveForward() == OK) {} ;
		Position * p = scratchGame->GetCurrentPos();
		if (gameEnd == 'S') {
		     match = (p->IsStaleMate());
		} else {
		     match = (p->IsKingInMate());
		}
	      }

	      if (match && (preComment || postComment)) {
		char * tmpComment;
		if (postComment) {
		  tmpComment = scratchGame->GetMoveComment();
                  // Don't match comments starting with [ and ending with ]
		  match = ((tmpComment != NULL) && (tmpComment[0] != '[' || tmpComment[strlen(tmpComment) - 1] != ']'));
		}
		if (match && preComment) {
		  scratchGame->MoveToPly(0);
		  tmpComment = scratchGame->GetMoveComment();
		  match = ((tmpComment != NULL) && (tmpComment[0] != '[' || tmpComment[strlen(tmpComment) - 1] != ']'));
		}
	      }

              if (match && pgnTextCount > 0) {
                db->tbuf->Empty();
                db->tbuf->SetWrapColumn (99999);
                scratchGame->LoadStandardTags (ie, db->nb);
                scratchGame->ResetPgnStyle ();
                scratchGame->AddPgnStyle (PGN_STYLE_TAGS);
                scratchGame->AddPgnStyle (PGN_STYLE_COMMENTS);
                scratchGame->AddPgnStyle (PGN_STYLE_VARS);
                scratchGame->AddPgnStyle (PGN_STYLE_SYMBOLS);
                scratchGame->SetPgnFormat (PGN_FORMAT_Plain);
                scratchGame->WriteToPGN(db->tbuf);
                const char * buf = db->tbuf->GetBuffer();
                if (ignoreCase)
		  for (int m=0; m < pgnTextCount; m++) {
		     if (match) { match = strCaseContains (buf, sPgnText[m]); }
		  }
                else
		  for (int m=0; m < pgnTextCount; m++) {
		     if (match) { match = strContains (buf, sPgnText[m]); }
		  }
	    }
            }
        }

        if (match) {
            // Game matched, so update the filter value. Only change it
            // to 1 if it is currently 0:
            if (db->dbFilter->Get(i) == 0) {
                db->dbFilter->Set (i, 1);
            }
        } else {
            // This game did NOT match:
            db->dbFilter->Set (i, 0);
        }
    }
    if (sWhite != NULL) { delete[] sWhite; }
    if (sBlack != NULL) { delete[] sBlack; }
    if (sEvent != NULL) { delete[] sEvent; }
    if (sSite  != NULL) { delete[] sSite;  }
    if (sRound != NULL) { delete[] sRound; }
    if (mWhite != NULL) { delete[] mWhite; }
    if (mBlack != NULL) { delete[] mBlack; }
    if (mEvent != NULL) { delete[] mEvent; }
    if (mSite  != NULL) { delete[] mSite;  }
    if (mRound != NULL) { delete[] mRound; }
    if (wTitles != NULL) { delete[] wTitles; }
    if (bTitles != NULL) { delete[] bTitles; }

    Tcl_Free ((char *) sPgnText);

    if (showProgress) { updateProgressBar (ti, 1, 1); }
    if (interruptedProgress()) {
        Tcl_AppendResult (ti, errMsgSearchInterrupted(ti), "  ", NULL);
    }
    int centisecs = timer.CentiSecs();
    sprintf (temp, "%d / %d  (%d%c%02d s)",
             db->dbFilter->Count(), startFilterCount,
             centisecs / 100, decimalPointChar, centisecs % 100);
    Tcl_AppendResult (ti, temp, NULL);
#ifdef SHOW_SKIPPED_STATS
    sprintf(temp, "  Skipped %u games.", skipcount);
    Tcl_AppendResult (ti, temp, NULL);
#endif

    return TCL_OK;
}


// sc_search_repertoire
//    Searches according to an opening repertoire.

// The Repertoire was removed a while ago - S.A.

//////////////////////////////////////////////////////////////////////
//  VARIATION creation/deletion/navigation functions.

int
sc_var (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "count", "number", "create", "delete", "deletefree", "enter", "exit", "first",
        "level", "list", "moveInto", "promote", NULL
    };
    enum {
        VAR_COUNT, VAR_NUMBER, VAR_CREATE, VAR_DELETE, VAR_DELETEFREE, VAR_ENTER, VAR_EXIT, VAR_FIRST,
        VAR_LEVEL, VAR_LIST, VAR_MOVEINTO, VAR_PROMOTE
    };
    int index = -1;

    if (argc > 1) { index = strUniqueMatch (argv[1], options); }

    switch (index) {
    case VAR_COUNT:
        return setUintResult (ti, db->game->GetNumVariations());

    case VAR_NUMBER:
        return setUintResult (ti, db->game->GetVarNumber());

    case VAR_CREATE:
	if (db->game->AtVarStart()) {
          if (db->game->AtVarEnd())
            break;
          else
	    // We at a var marker, "sc_var exit" to allign the new var alongside this one - S.A
	    db->game->MoveExitVariation();
        }
	db->game->MoveForward();
	db->game->AddVariation();
	db->gameAltered = true;
        break;

    case VAR_DELETE:
        return sc_var_delete (cd, ti, argc, argv);

    case VAR_DELETEFREE:
        return sc_var_delete_free (cd, ti, argc, argv);

    case VAR_ENTER:
        return sc_var_enter (cd, ti, argc, argv);

    case VAR_EXIT:
        db->game->MoveExitVariation();
        break;

    case VAR_FIRST:
        return sc_var_first (cd, ti, argc, argv);

    case VAR_LEVEL:
        return setUintResult (ti, db->game->GetVarLevel());

    case VAR_LIST:
        return sc_var_list (cd, ti, argc, argv);

    case VAR_MOVEINTO:
        return sc_var_enter (cd, ti, argc, argv);

    case VAR_PROMOTE:
        return sc_var_promote (cd, ti, argc, argv);

    default:
        return InvalidCommand (ti, "sc_var", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_var_delete:
//    Deletes a specified variation and free moves
int
sc_var_delete_free (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{

    if (argc != 3) {
        return errorResult (ti, "Usage: sc_var deletefree <number>");
    }

    uint varNumber = strGetUnsigned (argv[2]);
    if (varNumber >= db->game->GetNumVariations()) {
        return errorResult (ti, "No such variation!");
    }
    db->game->DeleteVariationAndFree (varNumber);
    db->gameAltered = true;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_var_delete:
//    Deletes a specified variation.
int
sc_var_delete (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{

    if (argc != 3) {
        return errorResult (ti, "Usage: sc_var delete <number>");
    }

    uint varNumber = strGetUnsigned (argv[2]);
    if (varNumber >= db->game->GetNumVariations()) {
        return errorResult (ti, "No such variation!");
    }
    db->game->DeleteVariation (varNumber);
    db->gameAltered = true;
    return TCL_OK;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_var_first:
//    Promotes the specified variation of the current to be the
//    first in the list.
int
sc_var_first (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_var first <number>");
    }

    uint varNumber = strGetUnsigned (argv[2]);
    if (varNumber >= db->game->GetNumVariations()) {
        return errorResult (ti, "No such variation!");
    }
    db->game->FirstVariation (varNumber);
    db->gameAltered = true;
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_var_list:
//    Returns a Tcl list of the variations for the current move.
int
sc_var_list (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    bool uci = (argc > 2) && ! strCompare("UCI", argv[2]);
    uint varCount = db->game->GetNumVariations();
    char s[100];
    for (uint varNumber = 0; varNumber < varCount; varNumber++) {
        db->game->MoveIntoVariation (varNumber);
        if (uci) db->game->GetNextMoveUCI (s);
        else db->game->GetSAN (s);
        // if (s[0] == 0) { strCopy (s, "(empty)"); }
        Tcl_AppendElement (ti, s);
        db->game->MoveExitVariation ();
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_var_enter:
//    Moves into a specified variation.
int
sc_var_enter (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_var enter <number>");
    }

    uint varNumber = strGetUnsigned (argv[2]);
    if (varNumber >= db->game->GetNumVariations()) {
        return errorResult (ti, "No such variation!");
    }

    db->game->MoveIntoVariation (varNumber);
    // Should moving into a variation also automatically play
    // the first variation move? Maybe it should depend on
    // whether there is a comment before the first move.
    // Uncomment the following line to auto-play the first move:
    db->game->MoveForward();

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_var_main:
//    Promotes the specified variation of the current to be the
//    mainline.
int
sc_var_promote (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_var promote <number>");
    }

    uint varNumber = strGetUnsigned (argv[2]);
    if (varNumber >= db->game->GetNumVariations()) {
        return errorResult (ti, "No such variation!");
    }
    db->game->MainVariation (varNumber);
    db->gameAltered = true;
    return TCL_OK;
}
//////////////////////////////////////////////////////////////////////
///  BOOK functions

int
sc_book (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    static const char * options [] = {
        "load", "close", "moves", "positions", "movesupdate", "update", NULL
    };
    enum {
        BOOK_LOAD,    BOOK_CLOSE, BOOK_MOVE, BOOK_POSITIONS, BOOK_MOVES_UPDATE, BOOK_UPDATE,
    };
    int index = -1;

    if (argc > 1) { index = strUniqueMatch (argv[1], options);}

    switch (index) {
    case BOOK_LOAD:
        return sc_book_load (cd, ti, argc, argv);

    case BOOK_CLOSE:
        return sc_book_close (cd, ti, argc, argv);

    case BOOK_MOVE:
        return sc_book_moves (cd, ti, argc, argv);

    case BOOK_POSITIONS:
        return sc_book_positions (cd, ti, argc, argv);

    case BOOK_UPDATE:
        return sc_book_update (cd, ti, argc, argv);

    case BOOK_MOVES_UPDATE:
        return sc_book_movesupdate (cd, ti, argc, argv);

    default:
        return InvalidCommand (ti, "sc_book", options);
    }

    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_book_load:
//    Opens and loads a .bin book (fruit format)
int
sc_book_load (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 4) {
        return errorResult (ti, "Usage: sc_book load bookfile slot");
    }

    uint slot = strGetUnsigned (argv[3]);

    int bookstate = polyglot_open(argv[2], slot);

    if (bookstate == -1 ) {
       return errorResult (ti, "Unable to load book");
    }
    if (bookstate  >  0 ) {
       // state == 1: book is read only
       return setIntResult (ti, bookstate);
    }
    return TCL_OK;

//--//    if (polyglot_open(argv[2], slot) == -1 ) {
//--//			return errorResult (ti, "Unable to load book");
//--//		}
//--//    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_book_close:
//    Closes the previously loaded book
int
sc_book_close (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 3) {
        return errorResult (ti, "Usage: sc_book close slot");
    }
    uint slot = strGetUnsigned (argv[2]);
    if (polyglot_close(slot) == -1 ) {
        return errorResult (ti, "Error closing book");
    }
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_book_moves:
//    returns a list of all moves contained in opened book and their probability in a TCL list
int
sc_book_moves (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    char moves[200] = "";
    char boardStr[100];

    if (argc != 3) {
        return errorResult (ti, "Usage: sc_book moves slot");
    }
    uint slot = strGetUnsigned (argv[2]);
    db->game->GetCurrentPos()->PrintFEN (boardStr, FEN_ALL_FIELDS);
    polyglot_moves(moves, (const char *) boardStr, slot);
    Tcl_AppendResult (ti, moves, NULL);
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_positions:
//    returns a TCL list of moves to a position in the book
int
sc_book_positions (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    char moves[200] = "";
    char boardStr[100];

    if (argc != 3) {
        return errorResult (ti, "Usage: sc_book positions slot");
    }
    uint slot = strGetUnsigned (argv[2]);
    db->game->GetCurrentPos()->PrintFEN (boardStr, FEN_ALL_FIELDS);
    polyglot_positions(moves, (const char *) boardStr, slot);
    Tcl_AppendResult (ti, moves, NULL);
    return TCL_OK;
}

// *** Seems unused - S.A. ***
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_book_update:
//    updates the opened book with probability values
int
sc_book_update (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 4) {
        return errorResult (ti, "Usage: sc_book update <probs> slot");
    }
    uint slot = strGetUnsigned (argv[3]);
    scid_book_update( (char*) argv[2], slot );
    return TCL_OK;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sc_book_movesupdate:
//    updates the opened book with moves and probability values
int
sc_book_movesupdate (ClientData cd, Tcl_Interp * ti, int argc, const char ** argv)
{
    if (argc != 6) {
        return errorResult (ti, "Usage: sc_book movesupdate <moves> <probs> slot tempfile");
    }
    uint slot = strGetUnsigned (argv[4]);
    scid_book_movesupdate( (char*) argv[2], (char*) argv[3], slot, (char*) argv[5] );
    return TCL_OK;
}

//////////////////////////////////////////////////////////////////////
/// END of tkscid.cpp
//////////////////////////////////////////////////////////////////////
