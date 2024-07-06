  Scid vs. PC
  Chess Database and Toolkit

  ____________________________________________________________

  Table of Contents


  1. introduction
  2. features
        2..1 New and Improved features

  3. download
        3..1 Other resources

  4. installation
        4..1 Linux , Unix
        4..2 Windows
        4..3 Mac OS X

  5. news
  6. miscellaneous
     6.1 docked windows
     6.2 how to play
     6.3 todo
     6.4 known issues
     6.5 bugs
     6.6 thanks
     6.7 scid's history

  7. changes
        7..1 Scid vs. PC 4.25

  8. contact
  9. links


  ______________________________________________________________________


  1.  introduction


  Shane's Chess Information Database is a powerful Chess Toolkit, with
  which one can create huge databases, run chess engines, and play
  casual games against the computer or online with the Free Internet
  Chess Server. It was originally written by Shane Hudson , and has
  received strong contribution from Pascal Georges and others.

  Scid vs. PC <http://scidvspc.sourceforge.net/> began with bug-fixes
  for the computer-versus-player features of Scid (hence the name), but
  has evolved into a solid alternative with many new features and
  interfaces. The project is authored by ``Stevenaaus'' and numerous
  contributors ``(thanks)''.

  2.  features

  See ``changes'' for a comprehensive changelog, or the gallery
  <https://sourceforge.net/apps/gallery/scidvspc/index.php> for some
  screenshots..

  2.0.1.  New and Improved features

  o  Overhauled and customizable interface.

  o  Engine versus engine computer tournaments.

  o  Extra search features, including move, end-of-game, and
     stalemate/checkmate searches.

  o  Drag+Drop file opens for Windows and Linux.

  o  Rewritten Gamelist widget with convenient context menus and
     buttons, and integrated Database Switcher.

  o  Improved Computer Game and FICS features, including premove, and
     simultaneous observed games.

  o  Many chess engine improvements, including max-ply option, an
     unlimited number of engines running, and the function hot-keys can
     be explicitly set.

  o  New EPD search and analyze features.

  o  Tri-coloured Tree bar-graphs, and options for more or less
     statistics.

  o  Ratings Graph can show multiple players, and Score graph is an
     attractive bar graph.

  o  Improved Book windows, including book compare, and remove move
     features.

  o  Redone Button and Tool bars.

  o  The Chessboard/Pieces config widget has been overhauled, and
     includes support for custom tiles and pieces.

  o  Browse multiple games.

  o  Recent Game and Player-info histories.

  o  Bug tested Undo and Redo features.

  o  The Help index is meaningful to new users, with links to the game's
     main features.

  o  Clickable Variation Arrows, and Paste Variation feature.

  o  A user friendly Annotation feature, with search-to-depth feature.

  o  Better support for UTF and Latin character sets in PGN
     export/imports.

  o  Improved and more powerful Tree Mask feature.

  o

  o  Chess 960 / Fischer Chess is now supported (only by a patch).

  3.  download

  Source scid_vs_pc-4.25.tgz
  <http://sourceforge.net/projects/scidvspc/files/source/scid_vs_pc-4.25.tgz/download>

  Windows Scid vs PC-4.25.exe
  <http://sourceforge.net/projects/scidvspc/files/windows/Scid%20vs%20PC-4.25.exe/download>

  Windows 64 bit Scid vs PC-4.25.x64.exe
  <http://sourceforge.net/projects/scidvspc/files/windows-64bit/Scid%20vs%20PC-4.25.x64.exe/download>

  Mac ScidvsMac-4.25.dmg
  <http://sourceforge.net/projects/scidvspc/files/mac/ScidvsMac-4.25.dmg/download>

  Mac 64bit (beta) ScidvsMac-4.25.x64.dmg
  <https://sourceforge.net/projects/scidvspc/files/mac-64bit-
  unsupported/ScidvsMac-4.25.x64.dmg/download>

  3.0.1.  Other resources

  The latest code is available from subversion
  <https://sourceforge.net/p/scidvspc/code/HEAD/tree/>

  Other project files
  <https://sourceforge.net/project/showfiles.php?group_id=263836>
  including german / deutsch versions

  4.  installation

  4.0.1.  Linux , Unix

  Installing from source is reccommended, though there exists deb packages of some versions (from third parties) in the linux packages
  <https://sourceforge.net/projects/scidvspc/files/linux%20packages/>

  Scid vs. PC requires Wish (Tcl/Tk) 8.5 or later and a C++ compiler.
  Example packages required include "tcl, tk, tcl-dev, tk-dev" and
  "gcc, g++ , libstdc++"; but will vary with your distribution.

  The default installation directory is /usr/local, which is generally
  empty, but any version of Scid here will be overwritten. To install
  into /usr (for eg) use ./configure BINDIR=/usr/bin/
  SHAREDIR=/usr/share/scid/

  Installing from source:

  ______________________________________________________________________
  tar -xzf scid_vs_pc-4.25.tgz
  cd scid_vs_pc-4.25
  ./configure
  make
  sudo make install
  scid
  ______________________________________________________________________


  Extra chess pieces (such as Berlin) are now enabled by default for
  Wish 8.6, but 8.5 requires installing TkImg. Sound support requires
  the buggy library Snack.
  If your distro does not provide these packages ("tkimg", "tcl-snack"),
  you can install from source using these links (both of which have
  fixes applied).
  TkImg
  <https://sourceforge.net/project/downloading.php?group_id=263836&filename=tkimg1.3.scidvspc.tar.bz2>,
  Snack
  <http://sourceforge.net/projects/scidvspc/files/support%20files/snack2.2.10.scidvspc.tgz/download>.

  Note Wish 8.5.10 has severe bugs, and many versions of Tk-8.6.x have
  *severe* memory leaks.  To avoid many of these leaks, or simply for a
  performance boost, compile with Gregor's tk::text
  (patches/gregors_tktext_inline.patch).

  4.0.2.  Windows

  Windows installation simply requires downloading the ``executable'',
  and following the prompts.

  The configuration files, including the chess engine list, are stored
  in the Scid-vs-PC\bin\config directory, and may be copied over from
  old versions to make upgrading easier. If the app is installed in
  "Program Files" On Windows 7, the config files are mirrored in
  C:\Users\[USERNAME]\AppData\Local\VirtualStore\Program Files\Scid vs
  PC

  Our main windows build system is MinGW and Makefile.mingw. We also
  have a Makefile.vc for visual studio, but it does not get updated too
  often.

  4.0.3.  Mac OS X

  The ``ScidvsMac-4.25 app'' should include everything you need. Simply
  drag and drop the App into /Applications (or similar). It cannot be
  run from the dmg disk image.

  Due to technical build reasons, only Mac OSX 10.10 and later
  are supported. Previous versions (including Snow Leopard) should install Scid vs Mac 4.16).
  Additionally, Newer macOS have removed 32-bit app support, and are only supported by our beta ScidvsMac-x64 app, which is not as robust as the 32 bit version.

  To compile from source - once you have XCode installed - please read
  ScidvsMac-HowTo.rtfd in the source tarball for some older information.

  Users upgrading may have to remove (or edit)
  $HOME/.scidvspc/config/engines.dat to properly configure the chess
  engines.

  5.  news

   July 2017

  Thanks to (our best bug reporter) Ileano for finding a bug in the new
  Best-Games/Browser feature. I've made a 4.18.1 point release to
  include the fix. This version also includes a minor PGN import fix.

   October 2016

  4.17 has a bit of a tough life, and is being re-released with a bug
  fix 28th October.  Otherwise, it is an exciting release for hackers,
  with patches for chess960 and Gregors speedy tkText rewrite.

   April 2015

  Exciting to have some usability fixes for the Tree Mask. Hopefully
  this feature will start to get some real use.

   December 2013

  4.11 is out, with large board sizes, and a fix for the annoying
  filter/tree issue.

   April 2013

  Finally have a docked windows feature. It's a damn complicated thing,
  but i am fond of it.  Scid vs PC 4.9 is coming :)

   March 2012

  Big effort by Gregor to write Drag and Drop support for Scidb and
  ScidvsPC. Thanks. FICS is looking great too.

   September 29, 2011

  Jiri Pavlovsky has made a windows installer. It's a nice piece of
  software :) Big thank-you. And we now have undo and redo features.

   July 8, 2011

  Thanks to Gilles for the web page restructure and OSX testing. Gregor
  Cramer  <http://scidb.sf.net> has contributed a PGN figurine feature.

   April 19, 2011

  A belated thanks to Lee from Sourceforge for this article
  <http://sourceforge.net/blog/after-scid-row-a-new-chess-app-is-born/>.

   December 10, 2010

  Scid vs. PC 4.2 includes support for Scid's si4 db format.

   July 3, 2010

  For the tenth release I've adopted verison number 4.0 . It includes a
  new Computer Tournament feature (thanks to some UCI snippets from
  Fulvio) and the Gamelist Widget is finally up-to-speed for large
  databases.

   April 19, 2010

  Release 3.6.26.9 includes a Fics accept/decline offers widget.

   December 20, 2009

  Thanks to Dorothy for making me a Mac DMG package with this release ,
  3.6.26.8.

   August 16, 2009

  With 3.6.26.6 I've fixed Phalanx's illegal castling. There is also
  changes to the Setup board and Toolbar configuration widgets.

   July 17, 2009

  3.6.26.5 - New Gamelist widget, and re-fashioned main buttons.
  Project's looking quite solid :->

   June 23, 2009

  The monkey on my back has really been having a good time. This release
  includes changes to the Gameinfo, Comment Editor, and Board Style
  widgets, some new chess pieces, colour schemes and tiles. Thanks to
  Michal and Alex for feedback.

   June 4, 2009

  Well, the html is up, and i've got a couple of files in the downloads
  section. My project is fairly modest fork of Scid ... just rewriting
  Tk widgets when i get the urge.

  6.  miscellaneous

  6.1.  docked windows

  This powerful feature is maturing. See the Docked Windows help item
  for more info.

  o  Window focus automatically follows the mouse around, and also
     impacts which keyboard bindings are active. Most bindings are
     active when the mouse is over the main board.

  o  In the event of Scid failing to start, restart the program with the
     -nodock option.

  o  Windows undocked from within docking mode can have glitches.
     Mainly: drag and drop game copies in the switcher don't work.

  o  OS X support is not great.

     Make sure to check out the new Theme Options, which affect how the
     Docked Windows (and Gamelist) look and feel.

  6.2.  how to play


     Playing against the Computer
        The main Computer vs. Player feature is accessed from
        Play->Computer. Here you'll find options to play against Phalanx
        (a flexible computer opponent whose skill you can select), or
        any installed UCI engine.

     Playing on the Internet
        Playing on the Internet is done via the the Play->Internet menu
        item. I recommend visiting the Fics <http://www.freechess.org>
        website to create a user account, but it is also possible to
        play anonymously. To start a game, press the Login as Guest
        button, then watch the available games as they are announced in
        the console. Enter play [game number] to accept a game
        challenge.


  There is more information about the Fics and Tactical Game features in
  the Scid Help menus.

  6.3.  todo


  o  The TCL sound package, Snack, needs fixing and a maintainer.

  o  Openseal is an open source version of FICS Timeseal. It needs some
     rewriting to work with Scid, though it is a small program.

  o  FICS could be adapted to work with the ICC. The work involves
     analysing the differences in the strings used by the two programs.
     (for example, for FICS we have this line to ackowledge successful
     log-in if {string match "*Starting FICS session*" $line]} { The two
     servers do have many similarities i think, and examining xboard's
     "backend.c" (or some other client) for "FICS" particularities

  o  Translation updates are always welcome

  o  There are two engine types - UCI and XBoard. Pascal's UCI code is
     in some ways inferior to Shane's Xboard code.  Though this is
     mostly mitigated by the speed of modern processors, it'd be nice to
     make use of the UCI ponder feature in analysis mode (Comp mode
     already does so).

  o  Verify/update the Novag Citrine drivers and interface, and
     Correspondence / XFCC feature.

  o  Our Windows port needs a little overhaul to properly use Users home
     directory to store all the various data.  I will get around to
     doing it one day though.

  o  A lot of people use chessbase books (.ctg). Scid can only read
     polyglot opening books, but inlining Scidb's support for ctg may
     not be too hard.

  6.4.  known issues


  o  macOS Catalina and later are only supported by the buggy and slow
     64 bit version.

  o  Linux sound playback is buggy/broken. The Wish sound package
     (Snack) needs maintenance.

  o  Tcl/Tk-8.5.10 is buggy.

  o  Some versions of Tk-8.6.x have severe memory leaks. To avoid,
     compile with Gregor's tk::text
     (patches/gregors_tktext_inline.patch).

  o  PGN Window slow-downs (with huge games) can also be fixed using
     Gregor's tk::text.

  o  OS X docked mode has some issues due to it's poor Tcl/Tk.

  o  Focus Issues. KDE users can allow Tcl apps to properly raise
     themselves by configuring desktop > window behavior > advanced >
     focus stealing prevention set to "none"

  o  Sometimes Scid vs. PC may have very loud/wrong colours.  This can
     be caused by the Window Manager exporting their color schemes, and
     can normally be switched off somewhere in the window manager's
     colour settings.

  6.5.  bugs


  o  See the known issues about Tcl/Tk (above).

  o  Importing huge PGN archives can sometimes fail. The command line
     utility pgnscid is a more reliable way to create large databases
     from pgn. Typing "pgnscid somefile.pgn" creates a new database
     "somefile.si4".

  o  Using the '+' operator and clicking on 'Find' in the gamelist
     widget can be very slow. The code needs to be moved from
     gamelist.tcl to tkscid.cpp.

  o  Windows only:

     o  Using "ALT+F"... etc key bindings to access the menus is badly
        broke. This is a Tcl/Tk issue.

     o  Window focus/raise issues, another Tcl/Tk issue.

     o  Screenshots are broken/disabled.

  6.6.  thanks

  Thanks to Shane Hudson and the authors of Tcl/Tk <http://tcl.tk>.  To
  Gregor Cramer <http://scidb.sf.net> for new features and technical
  support.  Ozkan for his Win64 builds and knowledgable help.  Christian
  Opitz for his comprehensive German translation.  Sourceforge.net
  <http://sourceforge.net> for their great hosting, Jiri Pavlovsky for
  the windows installer <http://www.jrsoftware.org/isinfo.php>.  Thanks
  to Pascal Georges for his many technical contributions to mainline
  Scid, and also Fulvio, Gerd and the language translators.  To Gilles,
  Dorothy and Steve for OS X support, to Michal Rudolf for early
  encouragement, and H.G.Muller
  <http://home.hccnet.nl/h.g.muller/chess.html> for technical feedback.

  6.7.  scid's history

  Scid is a huge project, with an interesting history. Originally
  authored by Shane Hudson from New Zealand, it combined the power of
  Tk's GUI and the speed of C, to produce a free Chess Database
  application with Opening Reports, Tree Analysis, and Tablebase
  support.  It gained quite some attention, as it was arguably the first
  project of it's kind; but after writing over a hundred thousand lines
  of code, in 2004 development stopped and Shane never contributed to
  Scid again.

  Two new versions of Scid appeared around 2006. The first was ChessDB
  authored by Dr. David Kirby. With some good documentation and the
  ability to automatically download games from several web portals, it
  became popular. But at the same time Pascal Georges from France was
  making strong technical improvements to Scid. Frustrated with Scid's
  dormancy, and because of disagreements with ChessDB's author, Pascal
  released his own tree, Scid-pg, which included UCI support and
  numerous Player versus Computer features.

  But subtley, and with some controversy, he began to adopt the name
  Scid as his own. Some people objected, especially Dr. Kirby, with whom
  a flame war began, but Pascal's efforts to gain ownership of the
  Sourceforge Scid project eventually succeeded.

  Under Pascal, and with the help of numerous contributors, Scid again
  strode forward. Pascal wrote a Tree Mask feature, and in 2009 he
  upgraded the database format to si4, all the time making speed and
  technical improvements to the neglect of the interface. In 2010,
  Pascal ceased contributing at all, and shortly after Scid 4.3 was
  released.  Since then, Scid has had widespread technical changes by
  Fulvio Benini, who is a long-standing contributor.

  Currently there exist several Scid related projects.

  Chessx was originally by Michal Rudolf (a longtime Scid contributor
  from Germany) and named Newscid. Now led by Jens Nissen, with a 1.0
  release, it is an attractive Chess GUI but with a small feature set
  and without si4 support.

  Scid vs. PC was started in 2009 by Steven Atkinson from Australia.
  Forked from Scid-3.6.26, it began as an effort to consolidate many of
  Pascal's new features, and has since matured into a capable Scid
  successor.

  The Android app, Scid on the Go, supports the si4 database format, and
  is the only mobile Scid related project.

  Another huge project, now without a maintainer, is Scidb by Gregor
  Cramer from Germany. It is an ambitious chess database program
  inspired by Scid, with heavy utilization of C++ classes and customized
  Tk widgets. It also supports Chessbase databases and many chess
  variants.

  7.  changes

  Scid vs. PC 4.25

    Extra Tags - bulk add Extra Tag feature 
    Enable pattern matching in the Name Editor 
    Show Comments in Game Preview/Browser windows, change colours to match PGN, and ControlKey+Load does not close the Browser window 
    LaTex Game Export - draw Marks in chess diagrams, indent variations, and other minor changes (from Bruno) 
    Crosstable: add clickable column headings for Nationality, Rating and Score 
    Engine Annotation - Score last move in case of checkmate (and stalemate) 
    Tree mask - Options to hide Markers (default), disable tooltips, and make the mask move colour steelblue 

    Engine - Right-clicking AddAllVars adds all first moves only 
    Comment Editor - Remove 'Apply' button, instead automatically apply miniboard changes. Add wheelmouse move-forward/back bindings 
    Player Info - don't show 'filtered games' stats if they duplicate the normal stats 
    Make the Tree Bargraph height similar to the font height (for high def displays) 
    Add a few Keypad bindings, re suggestion from Patrick 
    Pressing Home-key inside a variation moves to var start instead of game start 

  General Bugs  

    Twin Game checker minor fixes 
    Some spinboxes (annotate(blunder)) had erroneous error checking and threw exceptions 
    Some graphs didn't show the correct final/current year, gah! 
    Minor optable.tcl fixes/clean-up. Opening Table max games is now 100,000 (was 25,000) 
    Add current decade to Opening report Current popularity 
    EPD analysis hardening 
    For a few widgets - dont scroll text windows when using control-scroll to alter font size 
    Update stats/etc on Game quickSave 

  8.  contact

  Scid vs. PC mailing list
  <https://lists.sourceforge.net/lists/listinfo/scidvspc-users>

  Stevenaaus <email://stevenaaus at yahoo dot com> is a uni graduate in
  math and computer science, who programs as a hobby in tcl/tk, bash and
  C. He lives and works in rural australia.

  9.  links


  o  Scid vs. PC  <http://scidvspc.sourceforge.net/>

  o  Project page  <http://sourceforge.net/projects/scidvspc>

  o  Online documentation
     <http://scidvspc.sourceforge.net/doc/Contents.htm>

  o  Cassabase database  <http://caissabase.co.uk/>

  o  Player Information resources
     <http://sourceforge.net/projects/scid/files/Player Data/>

  o  FICS  <http://www.freechess.org>

  o  FICS game archives  <http://ficsgames.org>

  o  Kayvan's Cross platform 'docker' images
     <https://github.com/ksylvan/scidvspc>

  o  Chess Tech blog, with some ScidvsPC tutorials
     <https://www.chesstech.info/> Part 1
     <https://www.chesstech.info/2019/02/scid-vs-pc-players-pictures-
     part-1.html>

  o  Scid vs. 960/variants (Chess960 support)
     <https://github.com/brittonf/scid-vs-variants>

  o  Debian/Mint/Ubuntu installation how-to
     <http://www.linuxx.eu/2012/11/scid-vs-pc-installation-guide-
     ubuntu.html>

  o  Ed Collins' Scid vs. PC page
     <http://edcollins.com/chess/scidvspc/index.html>

  o  Gorgonian's custom pieces  <http://gorgonian.weebly.com/scid-vs-
     pc.html>

  o  The PGN and EPD standards
     <http://www.saremba.de/chessgml/standards/pgn/pgn-complete.htm>

  o  Pgn of players  <http://www.pgnmentor.com/files.html#players>

  o  Pgn of events  <http://www.pgnmentor.com/files.html#events>

  o  Inno setup  <http://www.jrsoftware.org/isinfo.php> (used to make
     windows installer)

  o  Professional quality chess icons  <www.virtualpieces.net>

  o  Tango icons  <http://tango.freedesktop.org/Tango_Desktop_Project>

  o  Mailing list subscribe (must be a member to post to list)
     <https://lists.sourceforge.net/lists/listinfo/scidvspc-users>

  o  Mailing list archive
     <http://sourceforge.net/mailarchive/forum.php?forum_name=scidvspc-
     users>

  o  Programmer's reference
     <http://scidvspc.sourceforge.net/doc/progref.html>

  o  UCI engine protocol  <http://wbec-
     ridderkerk.nl/html/UCIProtocol.html>

  o  Xboard engine protocol  <http://www.open-
     aurec.com/wbforum/WinBoard/engine-intf.html>


