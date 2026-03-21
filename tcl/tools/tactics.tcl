###
###
### tactics.tcl (and opening.tcl)
### Copyright (C) 2007  Pascal Georges
###
######################################################################
### Solve tactics (mate in n moves for example)
# use Site token in pgn notation to store progress
#
# S.A: Updated a little 6 Sept, 2010
# "Show Solution" checkbox now adds solution as PGN for browsing, and pauses main loop
# ... I still havent checked the "Win Won" functionality

namespace eval tactics {

  set infoEngineLabel ""

  set basePath $::scidBasesDir

  set baseList {}
  set solved "problem solved"
  set failed "problem failed"
  set prevScore 0
  set prevLine ""
  set nextEngineMove ""
  set matePending 0
  set cancelScoreReset 0
  set askToReplaceMoves_old 0
  set showSolution 0
  set labelSolution {}
  # set labelSolution {. . . . . . }
  set lastGameLoaded 0
  set prevFen ""

  # Don't try to find the exact best move but to win a won game (that is a mate in 5 is ok even if there was a pending mate in 2)
  set winWonGame 0


  ### Current base must contain games with Tactics flag and special **** markers
  # (as prepared by the Find Best Move in analysis annotation).
  # The first var should contain the best move (the next best move
  # is at least 1.0 point away.
  # TODO preset the filter on flag == Tactics to speed up searching

  proc findBestMove {} {
    bind .main.board  <Double-Button-1> ::tactics::findBestMove
    set ::gameInfo(hideNextMove) 1
    if {[winfo exists .pgnWin]} {
      destroy .pgnWin
    }

    set found 0

    if {![sc_base inUse] || [sc_base numGames] == 0} {
      tk_messageBox -type ok -icon info -title {Find Best Move} -message "No games in database"
      return
    }

    busyCursor .
    update

    # Try to find in current game, from current pos (exit vars first)
    catch {
      # if gamenumber == 0, sc_game flag T returns no boolean
      if {[sc_game flag T [sc_game number]]} {
	while {[sc_var level] != 0} { sc_var exit }
	  if {[llength [gotoNextTacticMarker] ] != 0} {
	    set found 1
	  }
      }
    }

    if {!$found} {
      # Then search other 'T' flagged games in DB
      for {set g [expr [sc_game number] +1] } { $g <= [sc_base numGames]} { incr g} {
        if {![sc_game flag T $g]} {
          continue
        }
        sc_game load $g
        # go through all moves and look for tactical markers ****D->
        if {[llength [gotoNextTacticMarker] ] != 0} {
          set found 1
          break
        } else {
          # A tactical flagged game (without ****D-> markers) with non-standard start position,
	  # begins probably with a tactical position.
          if {[sc_game startBoard]} {
	    sc_move start
            set found 1
            break
          }
        }
      }
    }

    unbusyCursor .

    if { ! $found } {
      tk_messageBox -type ok -icon info -title {Find Best Move} -message "No (more) relevant games found."
      sc_game load 1
    } else  {
      sideToMoveAtBottom
    }
    updateBoard -pgn
    ::windows::gamelist::Refresh
    updateTitle
  }

  ################################################################################
  # returns a list with depth score prevscore
  # or an empty list if marker not found

  proc gotoNextTacticMarker {} {
    while {![sc_pos isAt end]} {
      sc_move forward
      set cmt [sc_pos getComment]

      if {[string match {*\*\*\*\*D*->*} $cmt]} {
        # anything non-null
        return $cmt
      }
    }
    return {}
  }


  ### Configuration dialog for Mate in N puzzle
  # (Had some associated core dumps here, possibly when scidBasesDir is wrongly set in config S.A)

  proc config {} {
    global ::tactics::basePath ::tactics::baseList ::tactics::baseDesc tr
    set basePath $::scidBasesDir

    if {[winfo exists .tacticsWin]} {
      destroy .tacticsWin
    }

    set w .configTactics
    if {[winfo exists $w]} {
      raiseWin $w
      return
    }

    update
    toplevel $w
    wm title $w $tr(ConfigureTactics)
    setWinLocation $w

    if {[sc_base count free] == 0} {
      tk_messageBox -type ok -icon info -title Scid -message "Too many databases are open; close one first"
      return
    }

    set prevBase [sc_base current]
    # go through all bases and take descriptions
    set baseList {}
    set baseDesc {}
    set fileList [  lsort -dictionary [ glob -nocomplain -directory $basePath *.si4 ] ]
    foreach file  $fileList {
      if {[sc_base slot $file] == 0} {
        sc_base open [file rootname $file]
        set wasOpened 0
      } else  {
        sc_base switch [sc_base slot $file]
        set wasOpened 1
      }

      set solvedCount 0
      for {set g 1 } { $g <= [sc_base numGames]} { incr g} {
        sc_game load $g
        if {[sc_game tags get "Site"] == $::tactics::solved} { incr solvedCount }
      }
      lappend baseList "$file" "[sc_base description]  ($solvedCount/[sc_base numGames])"
      lappend baseDesc [sc_base description]
      if {! $wasOpened } {
        sc_base switch $prevBase
        sc_base close [sc_base slot $file]
      }
    }

    updateMenuStates
    updateStatusBar
    updateTitle

    frame $w.fconfig -relief raised -borderwidth 1
    label $w.fconfig.l1 -text $tr(ChooseTrainingBase)
    pack $w.fconfig.l1 -pady 3

    frame $w.fconfig.flist

    listbox $w.fconfig.flist.lb -selectmode single -exportselection 0 \
        -yscrollcommand "$w.fconfig.flist.ybar set" -height 10 -width 30
    bind $w.fconfig.flist.lb <Double-Button-1> "::tactics::start $w"

    scrollbar $w.fconfig.flist.ybar -command "$w.fconfig.flist.lb yview"
    pack $w.fconfig.flist.lb $w.fconfig.flist.ybar -side left -fill y
    for {set i 1} {$i<[llength $baseList]} {incr i 2} {
      $w.fconfig.flist.lb insert end [lindex $baseList $i]
    }
    $w.fconfig.flist.lb selection set 0

    frame $w.fconfig.reset
    button $w.fconfig.reset.button -text $tr(ResetScores) -command {
      set current [.configTactics.fconfig.flist.lb curselection]
      set name [lindex $::tactics::baseList [expr $current * 2 ] ]
      set desc [lindex $::tactics::baseDesc $current]
      if {[tk_messageBox -type yesno -parent .configTactics -icon question \
	     -title {Confirm Reset} -message "Confirm resetting \"$desc\" database"] == {yes}} {
	::tactics::resetScores $name .configTactics
      }
    }
    pack $w.fconfig.reset.button

    # in order to limit CPU usage, limit time for analysis (this prevents noise on laptops)
    frame $w.fconfig.flimit
    label $w.fconfig.flimit.blimit -text "$tr(limitanalysis) ($tr(seconds))" -relief flat
    scale $w.fconfig.flimit.analysisTime -orient horizontal -from 1 -to 30 -length 120 \
      -variable ::tactics::analysisTime -resolution 1
    pack $w.fconfig.flimit.blimit -side top
    pack $w.fconfig.flimit.analysisTime -side bottom

    frame $w.fconfig.fbutton
    dialogbutton $w.fconfig.fbutton.ok -text Ok -command "::tactics::start $w"
    dialogbutton $w.fconfig.fbutton.help -text $tr(Help) -command {helpWindow TacticsTrainer}
    dialogbutton $w.fconfig.fbutton.cancel -text $tr(Cancel) -command "focus .main ; destroy $w"
    pack $w.fconfig.fbutton.ok $w.fconfig.fbutton.help $w.fconfig.fbutton.cancel -expand yes -side left -padx 10 -pady 2
    pack $w.fconfig $w.fconfig.flist $w.fconfig.reset -side top

    addHorizontalRule $w.fconfig

    pack $w.fconfig.flimit -pady 5 -side top

    addHorizontalRule $w.fconfig

    pack $w.fconfig.fbutton -pady 5 -side bottom
    bind $w <Configure> "recordWinSize $w"
    bind $w <F1> {helpWindow TacticsTrainer}
  }


  proc start {parent} {
    global ::tactics::analysisEngine ::askToReplaceMoves ::tactics::askToReplaceMoves_old tr

    set current [.configTactics.fconfig.flist.lb curselection]
    set base [lindex $::tactics::baseList [expr $current * 2]]
    set desc [lindex $::tactics::baseDesc $current]

    if {![::tactics::loadBase [file rootname $base] $parent]} {
      return
    }

    destroy $parent

    set ::gameInfo(hideNextMove) 1
    if {[winfo exists .pgnWin]} {
      destroy .pgnWin
    }

    set ::tactics::lastGameLoaded 0

    if { ![::tactics::launchengine] } { return }

    set askToReplaceMoves_old $askToReplaceMoves
    set askToReplaceMoves 0

    set w .tacticsWin
    if {[winfo exists $w]} {
      raiseWin $w
      return
    }

    toplevel $w
    wm title $w $desc
    setWinLocation $w
    # because sometimes the 2 buttons at the bottom are hidden
    wm minsize $w 170 170
    frame $w.f1 -relief groove -borderwidth 1
    label $w.f1.labelInfo -textvariable ::tactics::infoEngineLabel -bg linen
    checkbutton $w.f1.cbWinWonGame -text $tr(WinWonGame) -variable ::tactics::winWonGame
    pack $w.f1.labelInfo $w.f1.cbWinWonGame -expand yes -fill both -side top

    frame $w.clock
    ::gameclock::new $w.clock 1 80 0
    ::gameclock::reset 1
    ::gameclock::start 1

    frame $w.f2 -relief groove
    checkbutton $w.f2.solution -text $tr(ShowSolution) -variable ::tactics::showSolution \
      -command ::tactics::toggleSolution
    label $w.f2.solved -textvariable ::tactics::labelSolution -wraplength 120
    pack $w.f2.solution $w.f2.solved -expand yes -fill both -side top

    frame $w.buttons -relief groove -borderwidth 1
    pack $w.f1 -expand yes -fill both
    pack $w.clock
    pack $w.f2 $w.buttons -expand yes -fill both

    setInfoEngine $tr(LoadingBase)

    button $w.buttons.next -textvar tr(Next) -command {
      ::tactics::stopAnalyze
      # mark game as solved if solution shown
      if {$::tactics::showSolution} {
	sc_game tags set -site $::tactics::solved
	sc_game save [sc_game number]
      }
      ::tactics::loadNextGame
    }
    button $w.buttons.close -textvar tr(Quit) -command ::tactics::endTraining
    pack $w.buttons.next $w.buttons.close -expand yes -fill both -padx 20 -pady 10
    bind $w <Destroy> { ::tactics::endTraining }
    bind $w <Configure> "recordWinSize $w"
    bind $w <F1> { helpWindow TacticsTrainer }

    setInfoEngine "---"
    ::tactics::loadNextGame
    ::tactics::mainLoop
  }
  ################################################################################
  #
  ################################################################################
  proc endTraining {} {
    set w .tacticsWin
    bind $w <Destroy> {}
    ::tactics::stopAnalyze
    after cancel ::tactics::mainLoop
    ::file::Close

    set ::askToReplaceMoves $::tactics::askToReplaceMoves_old
    focus .main
    destroy $w

    set ::gameInfo(hideNextMove) 0

    catch { ::uci::closeUCIengine $::tactics::engineSlot }
  }

  proc toggleSolution {} {
    global ::tactics::showSolution ::tactics::labelSolution ::tactics::analysisEngine

    if {$showSolution} {
      # pause main loop
      after cancel ::tactics::mainLoop
      if {![sc_pos isAt start]} {
	# not sure why...but have to move back one
	sc_move back
      }

      # add solution
      sc_move addSan $analysisEngine(moves)

      sc_move start

      set labelSolution $analysisEngine(moves)
      if {$analysisEngine(score) != {-327.0}} {
	append labelSolution "\n(score $analysisEngine(score))"
      }
    } else  {
      # restart this game
      sc_game load $::tactics::lastGameLoaded
      after 1000  ::tactics::mainLoop
      set labelSolution {}
    }
    updateBoard -pgn
    update
  }

  proc resetScores {name parent} {
    global ::tactics::cancelScoreReset ::tactics::baseList

    set base [file rootname $name]

    set wasOpened 0

    if {[sc_base count free] == 0} {
      tk_messageBox -type ok -icon info -title Scid -message "Too many databases are opened\nClose one first" -parent $parent
      return
    }
    # check if the base is already opened
    if {[sc_base slot $name] != 0} {
      sc_base switch [sc_base slot $name]
      set wasOpened 1
    } else  {
      if { [catch { sc_base open $base }] } {
        tk_messageBox -type ok -icon warning -title Scid -message "Unable to open base" -parent $parent
        return
      }
    }
    if {[sc_base isReadOnly]} {
        tk_messageBox -type ok -icon warning -title Scid -message "Base $base is read-only" -parent $parent
        return
    }

    # reset site tag for each game
    progressWindow Scid $::tr(ResettingScore) $::tr(Cancel) "::tactics::sc_progressBar"
    set numGames [sc_base numGames]
    set cancelScoreReset 0
    for {set g 1} { $g <= $numGames } { incr g} {
      if { $cancelScoreReset } { break }
      sc_game load $g
      if { [sc_game tags get "Site"] != ""} {
        sc_game tags set -site ""
        sc_game save [sc_game number]
      }
      if { [expr $g % 100] == 0 } {
        updateProgressWindow $g $numGames
      }
    }
    closeProgressWindow
    if { ! $wasOpened } {
      sc_base close
    }
    # update listbox
    set w .configTactics
    set cs [$w.fconfig.flist.lb curselection]
    set idx [expr $cs * 2 +1]
    set tmp [lindex $baseList $idx]
    regsub "\[(\]\[0-9\]+/" $tmp "(0/" tmp
    lset baseList $idx $tmp
    $w.fconfig.flist.lb delete 0 end
    for {set i 1} {$i<[llength $baseList]} {incr i 2} {
      $w.fconfig.flist.lb insert end [lindex $baseList $i]
    }
    $w.fconfig.flist.lb selection set $cs
  }
  ################################################################################
  # cancel score reset loading
  ################################################################################
  proc sc_progressBar {} {
    set ::tactics::cancelScoreReset 1
  }
  ################################################################################
  #
  ################################################################################
  proc loadNextGame {} {
    ::tactics::resetValues
    setInfoEngine $::tr(LoadingGame)
    set newGameFound 0
    # find a game with site tag != problem solved
    for {set g [ expr $::tactics::lastGameLoaded +1 ] } { $g <= [sc_base numGames]} { incr g} {
      sc_game load $g
      set tag [sc_game tags get "Site"]
      if {$tag != $::tactics::solved} { set newGameFound 1 ; break }
    }
    # it seems we finished the serial
    if {! $newGameFound } {
      tk_messageBox -title Scid -icon info -type ok -message $::tr(AllExercisesDone)
      return
    }
    set ::tactics::lastGameLoaded $g

    sideToMoveAtBottom

    ::gameclock::reset 1
    ::gameclock::start 1

    updateBoard -pgn
    set ::tactics::prevFen [sc_pos fen]
    ::tree::refresh
    ::windows::stats::Refresh
    updateMenuStates
    updateTitle
    updateStatusBar
    ::tactics::startAnalyze
    ::tactics::mainLoop
  }
  ################################################################################
  # flips the board if necessary so the side to move is at the bottom
  ################################################################################
  proc sideToMoveAtBottom {} {
    if { [sc_pos side] == "white" && [::board::isFlipped] || [sc_pos side] == "black" &&  ![::board::isFlipped] } {
      toggleRotateBoard
    }
  }

  ################################################################################
  #
  ################################################################################

  # We should probably disable "flip board" button, as it breaks game
  proc isPlayerTurn {} {
    if { [sc_pos side] == "white" &&  ![::board::isFlipped] || \
         [sc_pos side] == "black" &&  [::board::isFlipped] } {
      return 1
    } else {
      return 0
    }
  }

  ################################################################################
  #
  ################################################################################
  proc exSolved {} {
    ::tactics::stopAnalyze
    ::gameclock::stop 1
    sc_game tags set -site $::tactics::solved
    sc_game save [sc_game number]
    if {$::tactics::showSolution} {
      return
    }
    tk_messageBox -title Scid -icon info -type ok -message $::tr(MateFound)
    ::tactics::loadNextGame
  }
  ################################################################################
  # Handle the case where position was changed not during normal play but certainly with
  # move back / forward / rewind commands
  ################################################################################
  proc abnormalContinuation {} {
    ::tactics::stopAnalyze
    ::tactics::resetValues
    ::tree::refresh
    ::windows::stats::Refresh
    updateMenuStates
    updateTitle
    updateStatusBar
    if { [sc_pos side] == "white" && [::board::isFlipped] \
      || [sc_pos side] == "black" &&  ![::board::isFlipped] } {
      ::board::flip .main.board
    }
    updateBoard -pgn
    set ::tactics::prevFen [sc_pos fen]
    ::tactics::startAnalyze
    ::tactics::mainLoop
  }

  ################################################################################
  # waits for the user to play and check the move played
  ################################################################################
  proc mainLoop {} {
    global ::tactics::prevScore ::tactics::prevLine ::tactics::analysisEngine ::tactics::nextEngineMove

    after cancel ::tactics::mainLoop

    if {[sc_pos fen] != $::tactics::prevFen && [sc_pos isAt start]} {
      ::tactics::abnormalContinuation
      return
    }

    # is this player's turn (which always plays from bottom of the board)
    if { [::tactics::isPlayerTurn] } {
      after 1000  ::tactics::mainLoop
      return
    }

    set ::tactics::prevFen [sc_pos fen]

    # check if player's move is a direct mate : no need to wait for engine analysis in this case
    set move_done [sc_game info previousMove]
    if { [string index $move_done end] == "#"} { ::tactics::exSolved; return }

    # if the engine is still analyzing, wait the end of it
    if {$analysisEngine(analyzeMode)} { vwait ::tactics::analysisEngine(analyzeMode) }

    if {[sc_pos fen] != $::tactics::prevFen  && [sc_pos isAt start]} {
      ::tactics::abnormalContinuation
      return
    }

    # the player moved and analysis is over : check if his move was as good as expected
    set prevScore $analysisEngine(score)
    set prevLine $analysisEngine(moves)
    ::tactics::startAnalyze

    # now wait for the end of analyzis
    if {$analysisEngine(analyzeMode)} { vwait ::tactics::analysisEngine(analyzeMode) }
    if {[sc_pos fen] != $::tactics::prevFen  && [sc_pos isAt start]} {
      ::tactics::abnormalContinuation
      return
    }

    # compare results
    set res [::tactics::foundBestLine]
    if {  $res != ""} {
      tk_messageBox -title Scid -icon info -type ok -message "$::tr(BestSolutionNotFound)\n$res"
      # take back last move so restore engine status
      set analysisEngine(score) $prevScore
      set analysisEngine(moves) $prevLine
      sc_game tags set -site $::tactics::failed
      sc_game save [sc_game number]
      sc_move back
      updateBoard -pgn
      set ::tactics::prevFen [sc_pos fen]
    } else  {
      catch { sc_move addSan $nextEngineMove }
      set ::tactics::prevFen [sc_pos fen]
      updateBoard -pgn
      if { $::tactics::matePending } {
        # continue until end of game
      } else  {
        setInfoEngine $::tr(GoodMove)
        sc_game tags set -site $::tactics::solved
        sc_game save [sc_game number]
      }
    }

    after 1000 ::tactics::mainLoop
  }
  ################################################################################
  # Returns "" if the user played the best line, otherwise an explanation about the missed move :
  # - guessed the same next move as engine
  # - mate found in the minimal number of moves
  # - combinaison's score is close enough (within 0.5 point)
  ################################################################################
  proc foundBestLine {} {
    global ::tactics::analysisEngine ::tactics::prevScore ::tactics::prevLine ::tactics::nextEngineMove ::tactics::matePending
    set score $analysisEngine(score)
    set line $analysisEngine(moves)

    set s [ regsub -all "\[\.\]{3} " $line "" ]
    set s [ regsub -all "\[0-9\]+\[\.\] " $s "" ]
    set nextEngineMove [ lindex [ split $s ] 0 ]
    set ply [ llength [split $s] ]

    # check if the player played the same move predicted by engine
    set s [ regsub -all "\[\.\]{3} " $prevLine "" ]
    set s [ regsub -all "\[0-9\]+\[\.\] " $s "" ]
    set prevBestMove [ lindex [ split $s ] 1 ]
    if { [sc_game info previousMoveNT] == $prevBestMove} {
      return ""
    }

    # Case of mate
    if { [string index $prevLine end] == "#"} {
      set matePending 1
      #  Engine may find a mate then put a score != 300 but rather 10
      if {[string index $line end] != "#"} {
        if {! $::tactics::winWonGame } {
          return $::tr(MateNotFound)
        } else  {
          # win won game but still have to find a mate
          if {[sc_pos side] == "white" && $score < -300 || [sc_pos side] == "black" && $score > 300} {
            return ""
          } else  {
            return $::tr(MateNotFound)
          }
        }
      }
      # Engine found a mate, search in how many plies
      set s [ regsub -all "\[\.\]{3} " $prevLine "" ]
      set s [ regsub -all "\[0-9\]+\[\.\] " $s "" ]
      set prevPly [ llength [ split $s ] ]
      if { $ply > [ expr $prevPly - 1 ] && ! $::tactics::winWonGame } {
        return $::tr(ShorterMateExists)
      } else  {
        return ""
      }
    } else  {
      # no mate case
      set matePending 0
      set threshold 0.5
      if {$::tactics::winWonGame} {
        # Only alert when the advantage clearly changes side
        if {[sc_pos side] == "white" && $prevScore < 0 && $score >= $threshold  || \
              [sc_pos side] == "black" &&  $prevScore >= 0 && $score < [expr 0 - $threshold]  } {
          return "$::tr(ScorePlayed) $score\n$::tr(Expected) $prevScore"
        } else  {
          return ""
        }
      }
      if {[ expr abs($prevScore) ] > 3.0 } { set threshold 1.0 }
      if {[ expr abs($prevScore) ] > 5.0 } { set threshold 1.5 }
      # the player moved : score is from opponent side
      if {[sc_pos side] == "white" && $score < [ expr $prevScore + $threshold ] || \
            [sc_pos side] == "black" && $score > [ expr $prevScore - $threshold ] } {
        return ""
      } else  {
        return "$::tr(ScorePlayed) $score\n$::tr(Expected) $prevScore"
      }
    }
  }

  ################################################################################
  # Loads a base bundled with Scid (in ./bases directory)
  ################################################################################
  proc loadBase {name parent} {

    if {[sc_base count free] == 0} {
      tk_messageBox -type ok -icon info -title Scid -message "Too many databases are open; close one first" -parent $parent
      return 0
    }
    # check if the base is already opened
    if {[sc_base slot $name] != 0} {
      sc_base switch [sc_base slot $name]
    } else  {
      if { [catch { sc_base open $name }] } {
        tk_messageBox -type ok -icon warning -title Scid -message "Unable to open base" -parent $parent
        return 0
      }
    }
    if {[sc_base isReadOnly]} {
        tk_messageBox -type ok -icon warning -title Scid -message "Base $name is read-only" -parent $parent
        return 0
    }

    ::tree::refresh
    ::windows::stats::Refresh
    updateMenuStates
    updateBoard -pgn
    updateTitle
    updateStatusBar
    return 1
  }

  ################################################################################
  ## resetValues
  #   Resets global data.
  ################################################################################
  proc resetValues {} {
    set ::tactics::prevScore 0
    set ::tactics::prevLine ""
    set ::tactics::nextEngineMove ""
    set ::tactics::matePending 0
    set ::tactics::showSolution 0
    set ::tactics::labelSolution ""
    set ::tactics::prevFen ""
  }

  ################################################################################
  #
  ################################################################################
  proc  restoreAskToReplaceMoves {} {
    set ::askToReplaceMoves $::tactics::askToReplaceMoves_old
  }

  ################################################################################
  #
  ################################################################################
  proc setInfoEngine { s { color linen } } {
    set ::tactics::infoEngineLabel $s
    .tacticsWin.f1.labelInfo configure -background $color
  }

  ################################################################################
  #  Will start engine
  # in case of an error, return 0, or 1 if the engine is ok
  ################################################################################
  proc launchengine {} {
    global ::tactics::analysisEngine

    set analysisEngine(analyzeMode) 0

    # Use Toga
    set index 0
    foreach e $::engines(list) {
      if { [string equal -nocase -length 4 [lindex $e 0] "toga" ] } {
	# Start engine in analysis mode
        set ::tactics::engineSlot $index
	::uci::startSilentEngine $index
	return 1
      }
      incr index
    }

    # failsafe only ???
    set ::tactics::engineSlot 0

    tk_messageBox -type ok -icon warning -parent . -title Scid \
      -message "Unable to find engine.\nPlease configure engine with Toga as name"
    return 0

  }

  # ======================================================================
  # sendToEngine:
  #   Send a command to a running analysis engine.
  # ======================================================================
  proc sendToEngine {text} {
    ::uci::sendToEngine $::tactics::engineSlot $text
  }

  # ======================================================================
  # startAnalyzeMode:
  #   Put the engine in analyze mode
  # ======================================================================
  proc startAnalyze {} {
    global ::tactics::analysisEngine ::tactics::analysisTime
    setInfoEngine "$::tr(Thinking) ..." PaleVioletRed
    .tacticsWin.f2.solution configure -state disabled

    # Check that the engine has not already had analyze mode started:
    if {$analysisEngine(analyzeMode)} {
      ::tactics::sendToEngine  "exit"
    }

    set analysisEngine(analyzeMode) 1
    after cancel ::tactics::stopAnalyze
    ::tactics::sendToEngine "position fen [sc_pos fen]"
    ::tactics::sendToEngine "go infinite"
    after [expr 1000 * $analysisTime] ::tactics::stopAnalyze
  }

  # ======================================================================
  # stopAnalyzeMode:
  #   Stop the engine analyze mode
  # ======================================================================
  proc stopAnalyze {} {
    global ::tactics::analysisEngine ::tactics::analysisTime
    # Check that the engine has already had analyze mode started:
    if {!$analysisEngine(analyzeMode)} { return }

    set pv [lindex $::analysis(multiPV$::tactics::engineSlot) 0]
    set analysisEngine(score) [lindex $pv 1]
    set analysisEngine(moves) [lindex $pv 2]

    set analysisEngine(analyzeMode) 0
    ::tactics::sendToEngine  "stop"
    setInfoEngine $::tr(AnalyzeDone) PaleGreen3
    .tacticsWin.f2.solution configure -state normal
  }

}

###
### End of file: tactics.tcl
###
# ------------------------------------------------------------------------------------------
### opening.tcl: part of Scid.
### Copyright (C) 2007  Pascal Georges
### Copyright (C) 2026  stevenaaus
###

namespace eval opening {
  set repBase -1
  # list of elements of type fenMovesEvalList (fen move1 nag1 .... movei nagi)
  set allLinesFenList {}
  set fenMovesEvalList {}
  # list of hash lists, one list per game (a game = a line)
  set allLinesHashList {}
  set hashList {}

  set playerBestMove 1
  set opBestMove 1
  set onlyFlaggedLines 0
  set repColor "w"
  set resetStats 0

  set movesLoaded 0
  set fenLastUpdate 0
  set fenLastStatsUpdate 0
  set lastMainLoopFen 0
  set lastMainLoopFlipped [sc_pos side]

  # parameters for opening trainer window
  set tCM {}
  set lastCMFen {}
  set lastCMGames {}
  set lastCM -1
  set listStats {} ;# list of {fen x y z t} where x:good move played, y:dubious move, z:move out of rep, t:position played

  ################################################################################
  # Configuration
  ################################################################################
  proc config {} {
    global tr ::opening::playerBestMove ::opening::opBestMove ::opening::repColor ::opening::maxDepth ::opening::removeGames

    set w .openingConfig
    if {[winfo exists $w]} {
      raiseWin $w
      return
    }
    if {[winfo exists .openingWin]} {
      raiseWin .openingWin
      return
    }

    toplevel $w
    wm title $w $tr(Repertoiretrainingconfiguration)
    setWinLocation $w
    frame $w.f0 -relief groove

    radiobutton $w.f0.rbRepColorW -value w -variable ::opening::repColor -text $tr(White)
    radiobutton $w.f0.rbRepColorB -value b -variable ::opening::repColor -text $tr(Black)
    radiobutton $w.f0.rbRepColorWB -value wb -variable ::opening::repColor -text $tr(Both)
    pack $w.f0.rbRepColorW $w.f0.rbRepColorB $w.f0.rbRepColorWB -side left  -expand yes -fill both

    frame $w.f1
    checkbutton $w.f1.cbPlayerBestMove -text $tr(PlayerBestMove) -variable ::opening::playerBestMove
    checkbutton $w.f1.cbOpBestMove -text $tr(OpponentBestMove) -variable ::opening::opBestMove
    checkbutton $w.f1.cbRemoveGames -text $tr(RemoveGames) -variable ::opening::removeGames
    checkbutton $w.f1.cbOnlyFlaggedLines -text $tr(OnlyFlaggedLines) -variable ::opening::onlyFlaggedLines
    checkbutton $w.f1.cbResetStats -text $tr(resetStats) -variable ::opening::resetStats
    frame       $w.f1.maxDepth
    label       $w.f1.maxDepth.label -textvar ::tr(MaxPly)
    entry       $w.f1.maxDepth.entry -textvar ::opening::maxDepth -width 4 -justify center
    pack        $w.f1.maxDepth.label $w.f1.maxDepth.entry -padx 5 -anchor w -side left
    pack $w.f1.cbPlayerBestMove $w.f1.cbOpBestMove $w.f1.cbRemoveGames $w.f1.cbOnlyFlaggedLines $w.f1.cbResetStats $w.f1.maxDepth -anchor w -side top

    frame $w.f2
    dialogbutton $w.f2.ok -text Ok -command "destroy $w ; ::opening::openRep"
    dialogbutton $w.f2.help -text $tr(Help) -command {helpWindow OpeningTrainer}
    dialogbutton $w.f2.cancel -text $tr(Cancel) -command "focus . ; destroy $w"
    pack $w.f2.ok $w.f2.help $w.f2.cancel -expand yes -side left -padx 20 -pady 10

    pack $w.f0 $w.f1 $w.f2 -side top -fill both

    bind $w <F1> {helpWindow OpeningTrainer}
    bind $w <Escape> "destroy $w"
    bind $w <Destroy> ""
    bind $w <Configure> "recordWinSize $w"
  }
  ################################################################################
  # Open a repertoire
  ################################################################################
  proc openRep {} {
    global ::windows::switcher::base_types ::opening::repColor ::opening::repBase ::opening::fenLastUpdate
    global ::opening::allLinesHashList ::opening::allLinesFenList ::opening::lastCMFen ::opening::lastCM
    global ::opening::CMGames ::opening::lastCMGames
    set allLinesHashList {}
    set allLinesFenList
    set lastCMFen {}
    set lastCM -1
    set tCM {}
    set lastCMGames {}

    set fenLastUpdate 0
    if {$::opening::resetStats} {
      set ::opening::listStats {}
    } else  {
      loadStats
    }

    set repBase -1
    set typeW [lsearch $base_types {Openings for White} ]
    set typeB [lsearch $base_types {Openings for Black} ]
    set typeWB [lsearch $base_types {Openings for either color} ]

    for {set x 1} {$x <= [ expr [sc_base count total]-1 ]} {incr x} {
      if {![sc_base inUse $x]} {
        continue
      }
      set type [sc_base type $x]
      if {$type == $typeW && $repColor == "w" || $type == $typeB && $repColor == "b" || $type == $typeWB && $repColor == "wb"} {
        set repBase  $x
        break
      }
    }

    if {$repBase == -1} {
      tk_messageBox -title $::tr(Repertoirenotfound) -type ok -icon warning -message $::tr(NoRepertoireFound)
      return
    }

    sc_base switch $repBase
    loadRep "$repBase - [sc_base filename $repBase]" "[sc_base description]"
    sc_base switch clipbase

    if {$::opening::movesLoaded == 0} {
      tk_messageBox -title $::tr(Repertoirenotfound) -type ok -icon error -message "No moves loaded."
    }

    # add a blank game for training in clipbase
    sc_game new
    sc_game tags set -event $::tr(Openingtrainer)
    sc_game save 0
    # flip board if Black Repo S.A
    if {$repColor == "b" && ![::board::isFlipped]} {
      ::board::flip .main.board
    }
    updateBoard -pgn -animate
    ::windows::gamelist::Refresh
    updateTitle

    ::opening::openingWin
    ::opening::mainLoop
  }
  ################################################################################
  # Loads a repertoire
  # Go through all games and variations and build a tree of positions encountered
  ################################################################################
  proc loadRep {name desc} {
    global ::opening::repBase ::opening::fenMovesEvalList ::opening::allLinesFenList \
        ::opening::allLinesHashList ::opening::hashList ::opening::onlyFlaggedLines \
        ::opening::movesLoaded ::opening::lastCMFen

    set movesLoaded 0
    set allLinesFenList {}
    set allLinesHashList {}
    set lastCMFen {}
    set hashList {}
    set fenMovesEvalList {}

    # This progressWindow is broken for big databases S.A
    # Maybe it's just cpu busy because of "lsort -unique $hashList" below

    set ::interrupt 0
    progressWindow Scid $::tr(Loadingrepertoire) $::tr(Cancel) "set ::interrupt 1"
    busyCursor .
    update idletasks

    set numGames [sc_filter count]
    set g [sc_filter first]
    while {$g > 0} {
      if {$::interrupt} {
        set ::interupt 0
        break
      } ; # broke?
      changeProgressWindow "$::tr(Movesloaded) $movesLoaded"
      updateProgressWindow $g $numGames

      if {$onlyFlaggedLines && ![sc_game flag WhiteOpFlag $g] && ![sc_game flag BlackOpFlag $g]} {
        continue
      }
      set fenMovesEvalList {}
      set hashList  {}
      sc_game load $g
      sc_move start
      parseGame
      lappend allLinesFenList $fenMovesEvalList
      set hashList [lsort -unique $hashList]
      lappend allLinesHashList $hashList
      set g [sc_filter next]
    }

    closeProgressWindow
    unbusyCursor .
  }

  ################################################################################
  # parse one game and fill the list
  ################################################################################
  proc parseGame {} {
    while {![sc_pos isAt vend] && [sc_pos location] < $::opening::maxDepth} {
      fillFen
      # Go through all variants
      for {set v 0} {$v < [sc_var count]} {incr v} {
        # enter each var (beware the first move is played)
        sc_var enter $v
        parseVar
      }
      # now treat the main line
      sc_move forward
    }
  }
  ################################################################################
  # parse recursively variants.
  ################################################################################
  proc parseVar {} {
    while {![sc_pos isAt vend] && [sc_pos location] < $::opening::maxDepth} {
      fillFen
      # Go through all variants
      for {set v 0} {$v < [sc_var count]} {incr v} {
        sc_var enter $v
        fillFen
        # we are at the start of a var, before the first move : start recursive calls
        parseVar
      }
      sc_move forward
    }
    # at the end of a var : exit it
    sc_var exit
  }
  ################################################################################
  # fill the tree with repertoire information
  # we are at a given position :
  # - fill hash list in order to speed up searches
  # - fill fenMovesEvalList with {fen {move eval} {move eval} .... }
  ################################################################################
  proc fillFen {} {
    global ::opening::fenMovesEvalList ::opening::hashList ::opening::movesLoaded

    if {[sc_pos isAt vend] && [sc_var count] == 0 } {
      return
    }

    set fen [lrange [sc_pos fen] 0 3]
    set newFen {}
    set moves {}
    set newIndex -1
    incr movesLoaded

    lappend hashList [sc_pos hash]

    # check if the fen already exists in the list
    for {set i 0} { $i < [llength $fenMovesEvalList]} {incr i} {
      set f [lindex $fenMovesEvalList $i]
      if {[lindex $f 0] == $fen} {
        set newFen $fen
        set moves [lindex $f 1]
        set newIndex $i
        break
      }
    }
    set newFen $fen

    # the main move
    if {! [sc_pos isAt vend] } {
      set m [sc_game info nextMove]
      sc_move forward
      set nag [sc_pos getNags]
      sc_move back
      if {[lsearch $moves $m] == -1 } {
        lappend moves $m $nag
      } else  {
        # the move already exists : check if NAG values are coherent
        set lmoves [lsearch -all $moves $m]
        foreach i $lmoves {
          if {[lindex $moves [expr $i +1]] != $nag} {
            puts "redundancy and incoherence $m $nag for $newFen"
          }
        }
      }
    }
    # Go through all variants
    for {set v 0} {$v < [sc_var count]} {incr v} {
      sc_var enter $v
      set nag [sc_pos getNags]
      set m [sc_game info previousMove]
      if {[lsearch $moves $m] == -1 } {
        lappend moves $m $nag
      } else  {
        # the move already exists : check if NAG values are coherent
        set lmoves [lsearch -all $moves $m]
        foreach i $lmoves {
          if {[lindex $moves [expr $i +1]] != $nag} {
            puts "var redundancy and incoherence $m $nag for $newFen"
          }
        }
      }
      sc_var exit
    }

    # put the newFen in the list
    if {$newIndex == -1} {
      lappend fenMovesEvalList [list $fen $moves ]
    } else  {
      lset fenMovesEvalList $newIndex [list $fen $moves]
    }
  }
  ################################################################################
  # main loop called every second to trigger playing
  ################################################################################
  proc mainLoop {} {
    global ::opening::allLinesHashList ::opening::allLinesFenList

    after cancel ::opening::mainLoop

    # Handle case of player's turn (which always plays from bottom of the board)
    if {[sc_pos side] == "white" &&  ![::board::isFlipped] || [sc_pos side] == "black" &&  [::board::isFlipped]} {
      # it is player's turn : update UI
      ::opening::updateCMdisplay
      ::opening::updateStats
      after 1000 ::opening::mainLoop
      return
    }

    # check the position has not been treated already
    if {[sc_pos fen] == $::opening::lastMainLoopFen && [::board::isFlipped] == $::opening::lastMainLoopFlipped} {
      after 1000 ::opening::mainLoop
      return
    }

    # the player moved : check if his move was in the repertoire and as good as expected
    set move_done [sc_game info previousMove]
    if { $move_done != "" } {
      sc_move back
      set cm [getCM]
      sc_move forward
      # No move available : reached the end of a line
      if { [llength $cm] == 0 } {
        ::opening::updateCMdisplay
        ::opening::updateStats
        after 1000 ::opening::mainLoop
        return
      }

      # we know there are some CM
      set l [lsearch -all $cm $move_done]
      # move not in repertoire
      if {[llength $l] == 0} {
        tk_messageBox -type ok -message $::tr(Movenotinrepertoire) -icon info -parent .main.board
        sc_move back
        sc_game truncate
        addStats -good 0 -dubious 0 -absent 1 -total 1
        ::opening::updateCMdisplay
        ::opening::updateStats
        updateBoard -pgn -animate
        after 1000 ::opening::mainLoop
        return
      }

      # The move played is in repertoire !
      set moveOK 1

      if {$::opening::playerBestMove} {
        foreach i $l {
          if {! [ ::opening::isGoodMove [ lindex $cm [expr $i+1] ] ] } {
            addStatsPrev -good 0 -dubious 1 -absent 0 -total 1
            set moveOK 0
            set nag [ lindex $cm [expr $i+1] ]
            break
          }
        }

        # The move is not good : offer to take back
        if {!$moveOK} {
          # addStatsPrev -good 0 -dubious 0 -absent 1 -total 0
          set answer [tk_messageBox -icon question -title $::tr(OutOfOpening) -type yesno \
              -message "$::tr(yourmoveisnotgood) ($nag) \n $::tr(DoYouWantContinue)" ]
          if {$answer == "no"} {
            sc_move back
            updateBoard -pgn
            after 1000 ::opening::mainLoop
            return
          }
        } else  { ;# the move is a good one
          addStatsPrev -good 1 -dubious 0 -absent 0 -total 1
        }
      } else  { ;# player is allowed to play bad moves
        foreach i $l {
          set goodMove 1
          if {! [ ::opening::isGoodMove [ lindex $cm [expr $i+1] ] ] } {
            set goodMove 0
            break
          }
        }
        if {$goodMove} {
          addStatsPrev -good 1 -dubious 0 -absent 0 -total 1
        } else  {
          addStatsPrev -good 1 -dubious 1 -absent 0 -total 1
        }
      }

    }
    # end of player's move check
    # now it is computer's turn
    set cm [getCM]

    if {[llength $cm] != 0} {
      ::opening::play $cm
    }
    set ::opening::lastMainLoopFen [sc_pos fen]
    set ::opening::lastMainLoopFlipped [::board::isFlipped]

    ::opening::updateCMdisplay
    ::opening::updateStats
    after 1000 ::opening::mainLoop
  }
  ################################################################################
  # isGoodMove : returns true if the nag list in parameter is empty or contains !? ! !!
  ################################################################################
  proc isGoodMove { n } {
    if { [lsearch -exact $n "?"] != -1 || [lsearch -exact $n "?!"] != -1 || [lsearch -exact $n "??"] != -1} {
      return 0
    }
    return 1
  }
  ################################################################################
  # get all candidate moves in the repertoire from current position
  # the list returned is of the form {move1 nag1 move2 nag2 ....}
  # the moves are not unique
  ################################################################################
  proc getCM {} {
    global ::opening::allLinesHashList ::opening::allLinesFenList ::opening::lastCMFen ::opening::lastCM
    global ::opening::CMGames ::opening::lastCMGames ;# dynamic var in sync with getCM which shows which game a move is from S.A.

    set fen [sc_pos fen]
    # avoids calculation
    if {$fen == $lastCMFen} {
      return $lastCM
    }
    set fen [lrange $fen 0 3]

    set cm {}
    set CMGames {}
    # Find the position in hash lists
    for {set i 0} {$i < [llength $allLinesHashList]} {incr i} {
      if {[lsearch -sorted [lindex $allLinesHashList $i] [sc_pos hash]] != -1} {

	foreach f [lindex $allLinesFenList $i] {
	  if {[lindex $f 0] == $fen} {
	    set cm [concat $cm [lindex $f 1]]
            lappend CMGames $i
	  }
	}

      }
    }

    set lastCM $cm
    set lastCMFen $fen
    if {$CMGames != {}} {
      set lastCMGames $CMGames
    }

    return $cm
  }
  ################################################################################
  # play one of the candidate moves
  ################################################################################
  proc play {cm} {
    # addStatsPrev -good 0 -dubious 0 -absent 0 -total 1
    set r [expr int(rand()*[llength $cm]/2)]
    set m [lindex $cm [expr $r * 2]]

    if {[sc_pos moveNumber] == 1 && [sc_pos side] == "white"} {
      ::game::Clear
    }

    if {![catch {sc_move addSan [::untrans $m] }]} {
      # wtf S.A.
    }
    updateBoard -pgn -animate
  }
  ################################################################################
  # Main Opening Trainer window
  ################################################################################
  proc openingWin {} {
    global tr ::opening::displayCM ::opening::displayCMValue ::opening::tCM ::opening::fenLastUpdate

    set w .openingWin
    if {[winfo exists $w]} {
      raiseWin $w
      return
    }
    toplevel $w

    wm title $w "$::tr(Openingtrainer) ($tr(Depth) $::opening::maxDepth)"
    setWinLocation $w
    frame $w.f1
    frame $w.f2
    frame $w.f3

    checkbutton $w.f1.cbDisplayCM  -text $tr(DisplayCM) -variable ::opening::displayCM -relief flat -command {
      set fenLastUpdate 0
      ::opening::updateCMdisplay 1
    }
    checkbutton $w.f1.cbDisplayCMValue  -text $tr(DisplayCMValue) -variable ::opening::displayCMValue -relief flat -command {
      set fenLastUpdate 0
      ::opening::updateCMdisplay 1
    }
    label $w.f1.lCM -textvariable ::opening::tCM
    pack $w.f1.cbDisplayCM $w.f1.cbDisplayCMValue -anchor w -side top
    pack $w.f1.lCM -side top -anchor center

    checkbutton $w.f2.cbDisplayStats  -text $tr(DisplayOpeningStats) -variable ::opening::displayOpeningStats -relief flat \
        -command "::opening::updateStats 1"
    label $w.f2.lStats1 -textvariable ::opening::lStats1 -width 4 -anchor center -background green
    label $w.f2.lStats2 -textvariable ::opening::lStats2 -width 4 -anchor center -background yellow
    label $w.f2.lStats3 -textvariable ::opening::lStats3 -width 4 -anchor center -background red
    label $w.f2.lStats4 -textvariable ::opening::lStats4 -width 4 -anchor center -background white

    label $w.f2.lStats1exp -text $tr(NumberOfGoodMovesPlayed)
    label $w.f2.lStats2exp -text $tr(NumberOfDubiousMovesPlayed)
    label $w.f2.lStats3exp -text $tr(NumberOfMovesPlayedNotInRepertoire)
    label $w.f2.lStats4exp -text $tr(NumberOfTimesPositionEncountered)

    grid $w.f2.cbDisplayStats -row 0 -column 0 -columnspan 2
    grid $w.f2.lStats4 -row 1 -column 0 -sticky w -padx 5
    grid $w.f2.lStats1 -row 2 -column 0 -sticky w -padx 5
    grid $w.f2.lStats2 -row 3 -column 0 -sticky w -padx 5
    grid $w.f2.lStats3 -row 4 -column 0 -sticky w -padx 5

    grid $w.f2.lStats4exp -row 1 -column 1 -sticky w -padx 5
    grid $w.f2.lStats1exp -row 2 -column 1 -sticky w -padx 5
    grid $w.f2.lStats2exp -row 3 -column 1 -sticky w -padx 5
    grid $w.f2.lStats3exp -row 4 -column 1 -sticky w -padx 5

    button $w.f3.next   -textvar tr(NextGame) -command {
      ::game::Clear
      ::windows::gamelist::Refresh

      if {$::opening::removeGames} {
	# Remove this game for nexttime
        # Bit of a hack but easiest way to implement to play-each-game-once feature
	set i [lindex $::opening::lastCMGames 0]
	set hash {}
	set fen  {}
	if {$i > 0} {
	  set hash [lrange $::opening::allLinesHashList 0 $i-1]
	  set fen [lrange $::opening::allLinesFenList 0 $i-1]
	}
	if {$i < [llength $::opening::allLinesHashList]} {
	  set hash [concat $hash [lrange $::opening::allLinesHashList $i+1 end]]
	  set fen  [concat $fen  [lrange $::opening::allLinesFenList $i+1 end]]
	}
	set ::opening::allLinesHashList $hash
	set ::opening::allLinesFenList $fen
	if {[llength $::opening::allLinesHashList] == 0} {
	  tk_messageBox -type ok -icon info -title {Scid} -message "All games finished"
          ::opening::endTraining
          return
	}
      }
      ::opening::mainLoop
    }
    button $w.f3.report -textvar tr(ShowReport) -command ::opening::report
    button $w.f3.close -textvar tr(Quit) -command ::opening::endTraining

    pack $w.f3.next -side top -anchor center -fill x -pady 10
    pack $w.f3.report $w.f3.close -side top -anchor center -fill x
    pack $w.f1 $w.f2 $w.f3 -fill x -pady 8 -padx 5

    bind $w <F1> {helpWindow OpeningTrainer}
    bind $w <Escape> ::opening::endTraining
    bind $w <Destroy> ::opening::endTraining
    bind $w <Configure> "recordWinSize $w"
    wm minsize $w 45 0
  }
  ################################################################################
  #
  ################################################################################
  proc endTraining {} {
    bind .openingWin <Destroy> ""
    after cancel ::opening::mainLoop
    saveStats
    focus .
    destroy .openingWin
  }
  ################################################################################
  # display the candidate moves list (with NAG values)
  ################################################################################
  proc  updateCMdisplay { {forceUpdate 0} } {
    global ::opening::displayCM ::opening::displayCMValue ::opening::tCM ::opening::fenLastUpdate

    # If current fen is the same as the one used during latest update call, do nothing
    if {$fenLastUpdate == [sc_pos fen] && ! $forceUpdate} {
      return
    }

    set cm [getCM]

    if { [llength $cm] == 0 } {
      .openingWin.f1.lCM configure -bg LightCoral
      set tCM $::tr(EndOfVar)
      set fenLastUpdate [sc_pos fen]
      ::opening::updateStats
      after idle {after cancel ::opening::mainLoop}
      return
    }

    if {!$displayCM} {
      set tCM ""
      set fenLastUpdate 0
      return
    }

    .openingWin.f1.lCM configure -bg linen

    set tmp ""

    for {set x 0} {$x < [llength $cm]} {incr x 2} {
      set m [lindex $cm $x]
      # if the move already found, skip it, even if it has other nags : to be corrected ?
      if {[string first $m $tmp] != -1} {
        continue
      }
      append tmp  $m " "
      set nlist [lindex $cm [expr $x+1] ]
      if {$nlist == 0} {
        continue
      }
      if {$displayCMValue} {
        foreach n $nlist {
          append tmp $n " "
        }
      }
      # go to new line every 3 (moves,nags)
      if {[expr $x % 3] == 2} { append tmp "\n" }
    }

    set fenLastUpdate [sc_pos fen]
    set tCM $tmp
  }
  ################################################################################
  #
  ################################################################################
  proc loadStats {} {
    set optionsFile [scidConfigFile optrainer]
    if {[catch {source $optionsFile} ]} {
      ::splash::add "Unable to find the options file: [file tail $optionsFile]"
    } else {
      ::splash::add "Your options file \"[file tail $optionsFile]\" was found and loaded."
    }
  }
  ################################################################################
  #
  ################################################################################
  proc saveStats {} {
    set optrainerFile [scidConfigFile optrainer]
    if {[catch {open $optrainerFile w} f]} {
      return 0
    }
    puts $f "set ::opening::listStats { $::opening::listStats }"
    close $f
    return 1
  }
  ################################################################################
  # getStats
  # returns a list containing the 4 stats values for current pos
  # or an empty list if the stats are not available for current position
  ################################################################################
  proc getStats {} {
    # set s [split [sc_pos fen]]
    # set fen "[lindex $s 0] [lindex $s 1] [lindex $s 2] [lindex $s 3]"
    set fen [lrange [sc_pos fen] 0 3]
    set found 0
    set idx 0
    foreach l $::opening::listStats {
      if {[lindex $l 0] == $fen} {
        set found 1
        break
      }
      incr idx
    }
    if {$found} {
      return [lindex $l 1]
    }
    return {}
  }

  ################################################################################
  # addStats
  # x = success best moves only, y = success all moves z = failures t = coverage by computer
  ################################################################################
  proc addStats { args } {
    set dx 0
    set dy 0
    set dz 0
    set dt 0

    for {set i 0 } {$i < [llength $args]} {incr i 2} {
      if {[lindex $args $i] == "-good"} { set dx [lindex $args [expr $i + 1] ] ; continue }
      if {[lindex $args $i] == "-dubious"} { set dy [lindex $args [expr $i + 1] ] ; continue }
      if {[lindex $args $i] == "-absent"} { set dz [lindex $args [expr $i + 1] ] ; continue }
      if {[lindex $args $i] == "-total"} { set dt [lindex $args [expr $i + 1] ] ; continue }
    }

    # set s [split [sc_pos fen]]
    # set fen "[lindex $s 0] [lindex $s 1] [lindex $s 2] [lindex $s 3]"
    set fen [lrange [sc_pos fen] 0 3]
    set found 0
    set idx 0
    foreach l $::opening::listStats {
      if {[lindex $l 0] == $fen} {
        set found 1
        break
      }
      incr idx
    }

    if {$found} {
      set lval [lindex $l 1]
      set ::opening::listStats [ lreplace $::opening::listStats $idx $idx [list $fen [list \
          [expr [lindex $lval 0]+$dx] \
          [expr [lindex $lval 1]+$dy] \
          [expr [lindex $lval 2]+$dz] \
          [expr [lindex $lval 3]+$dt] \
          ] ] ]
    } else  {
      lappend ::opening::listStats [list $fen [list $dx $dy $dz $dt] ]
    }
    updateStats 1
  }
  ################################################################################
  #
  ################################################################################
  proc addStatsPrev { args } {
    if {[sc_pos isAt vstart] } { return }
    if {![catch {sc_move back}]} {
      eval addStats $args
      sc_move forward
    }
  }
  ################################################################################
  #
  ################################################################################
  proc updateStats { {force 0} } {
    global ::opening::fenLastStatsUpdate

    # If current fen is the same as the one used during latest update call, do nothing
    if {$fenLastStatsUpdate == [sc_pos fen] && !$force} {
      return
    }

    set fenLastStatsUpdate [sc_pos fen]

    if { $::opening::displayOpeningStats } {
      set gs [getStats]
      set ::opening::lStats1 [lindex $gs 0]
      set ::opening::lStats2 [lindex $gs 1]
      set ::opening::lStats3 [lindex $gs 2]
      set ::opening::lStats4 [lindex $gs 3]
    } else  {
      set ::opening::lStats1 " "
      set ::opening::lStats2 " "
      set ::opening::lStats3 " "
      set ::opening::lStats4 " "
    }
  }
  ################################################################################
  # shows a repertoire report (how much of the rep was trained)
  ################################################################################
  proc report {} {
    global ::opening::listStats ::opening::allLinesFenList
    set w ".openingWin.optrainerreport"
    if {[winfo exists $w]} {
      raiseWin $w
      return
    }

    toplevel $w
    wm title $w $::tr(Openingtrainer)
    setWinLocation $w

    frame $w.ft
    text $w.ft.text -height 10 -width 40 -wrap word -background white
    pack $w.ft.text
    pack $w.ft

    frame $w.fclose
    button $w.fclose.close -textvar ::tr(Close) -command "destroy $w"
    pack $w.fclose.close

    # builds stats report
    set posNotPlayed 0
    set posTotalPlayed 0
    set success 0
    set dubMoves 0
    set outOfRep 0
    set totalPos 0
    foreach line $allLinesFenList {
      incr totalPos [llength $line]
      foreach pos $line {
        set fenLine [lindex $pos 0]
        set idx 0
        set found 0
        foreach l $listStats {
          if {$fenLine == [lindex $l 0]} {
            set found 1
            break
          }
          incr idx
        }
        if { $found } {
          set stats [lindex [ lindex $listStats $idx ] 1]
          if { $stats != "" } {
            incr success [lindex $stats 0]
            incr dubMoves [lindex $stats 1]
            incr outOfRep [lindex $stats 2]
            incr posTotalPlayed [lindex $stats 3]
          }
        } else {
          incr posNotPlayed
        }
      }
    }
    $w.ft.text insert end "$::tr(PositionsInRepertoire) $totalPos\n"
    $w.ft.text insert end "$::tr(PositionsNotPlayed) $posNotPlayed\n"
    $w.ft.text insert end "$::tr(PositionsPlayed) [expr $totalPos - $posNotPlayed]\n"
    $w.ft.text insert end "$::tr(Success) $success\n"
    $w.ft.text insert end "$::tr(DubiousMoves) $dubMoves\n"
    $w.ft.text insert end "$::tr(OutOfRepertoire) $outOfRep\n"

    $w.ft.text configure -state disabled

    bind $w <F1> { helpWindow OpeningTrainer }
    bind $w <Escape> "destroy $w"
    bind $w <Destroy> ""
    bind $w <Configure> "recordWinSize $w"
    # wm minsize $w 45 0
  }
  ################################################################################
  #
  ################################################################################
}
###
### End of file: opening.tcl
###
