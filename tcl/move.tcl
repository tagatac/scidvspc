### move.tcl
### Functions for moving within a game.

namespace eval ::move {}

proc ::move::checkVarArrows {} {
  global analysis
  # also check Engine is not drawing arrows on main board
  if {($analysis(boardEngine) > -1 && $analysis(analyzeMode$analysis(boardEngine))) || !$::showVarArrows || $::autoplayMode} {
    return 0
  }

  set bDrawArrow 0
  set varList [sc_var list UCI]

  if {$varList != ""} {
    set move [sc_game info nextMoveUCI]
    if {$move != ""} { set varList [linsert $varList 0 $move] }
    foreach move $varList {
      set bDrawn 0
      set sq_start [::board::sq [string range $move 0 1]]
      set sq_end [::board::sq [string range $move 2 3]]
        foreach mark $::board::_mark(.main.board) {
          if { [lindex $mark 0] == "arrow" ||[string match {var*} [lindex $mark 0]] } {
            if {[lindex $mark 1] == $sq_start && [lindex $mark 2] == $sq_end} {
              set bDrawn 1
              break
            }
        }
      }
      if {!$bDrawn} {
        set bDrawArrow 1
        break
      }
    }
  }

  return $bDrawArrow
}

proc ::move::showVarArrows {} {
  global analysis
  # don't show var arrows if engine running and drawing on main board
  if {$analysis(boardEngine) > -1 && $analysis(analyzeMode$analysis(boardEngine))} {
    return
  }
  set move [sc_game info nextMoveUCI]
  set varList [sc_var list UCI]

  set var 0

  if {$move != ""} {
    set sq_start [ ::board::sq [ string range $move 0 1 ] ]
    set sq_end [ ::board::sq [ string range $move 2 3 ] ]
    ::board::mark::add .main.board var$var $sq_start $sq_end $::maincolor
    incr var
  }

  foreach move $varList {
    set sq_start [ ::board::sq [ string range $move 0 1 ] ]
    set sq_end [ ::board::sq [ string range $move 2 3 ] ]
    ::board::mark::add .main.board var$var $sq_start $sq_end $::varcolor
    incr var
  }
}

proc ::move::Start {{var 0}} {
  if {$::fics::playing == 2} {
      ::fics::writechan {backward 256} noecho
      return
  }
  set ::pause 1
  if {$var && ![sc_pos isAt vstart]} {
    sc_move startVar
  } else {
    sc_move start
  }
  updateBoard
  if {[winfo exists .pgnWin.text]} {
    .pgnWin.text see 0.0
  }
  if {[::move::checkVarArrows]} {
    ::move::showVarArrows
  }
}

proc ::move::End {{var 0}} {
  if {$::fics::playing == 2} {
      ::fics::writechan {forward 256} noecho
      return
  }
  set ::pause 1
  if {$var && ![sc_pos isAt vend]} {
    sc_move endVar
  } else {
    sc_move end
  }
  updateBoard
  if {[::move::checkVarArrows]} {::move::showVarArrows}
}

proc ::move::ExitVar {} {
  sc_var exit;
  updateBoard -animate;
  # Do comments work properly ?
  if {[::move::checkVarArrows]} {::move::showVarArrows}
}

set ::fics::playing 0 ; # predefine

proc ::move::Back {{count 1}} {
  if {[sc_pos isAt start]} {
    if {$::fics::playing == 2} {
      ::fics::writechan backward noecho
    }
    return
  }
  if {[sc_pos isAt vstart]} {
     ::move::ExitVar
     return
   }

  ### if playing, remove this move from hash array S.A

  set ::lFen [lrange $::lFen 0 end-[expr {$count+1}]]
  pauseGame

  sc_move back $count

  if {$count == 1} {
    # Do animation and speech:
    updateBoard -animate
    ::utils::sound::AnnounceBack
  } else {
    updateBoard
  }
  if {[::move::checkVarArrows]} {::move::showVarArrows}
}

proc ::move::Forward {{count 1}} {
  global autoplayMode

  if {[sc_pos isAt end]  ||  [sc_pos isAt vend]} {
    if {$::fics::playing == 2} {
      ::fics::writechan forward noecho
    }
    return
  }
  set ::pause 1
  set bArrows [::move::checkVarArrows]

  set move [sc_game info next]
  if {$count == 1} {
    if {[sc_var count] != 0 && ! $autoplayMode && $::showVarPopup} {
      ::commenteditor::storeComment
      showVars
      set bArrows $::showVarArrows
    } else  {
      if {!$bArrows} {
        sc_move forward
        ::utils::sound::AnnounceForward $move
      }
    }

    updateBoard -animate
  } else {
    if {!$bArrows} {sc_move forward $count}
    updateBoard
  }
  if {$bArrows} {::move::showVarArrows}
}


