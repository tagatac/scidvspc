### start of edit.tcl

set defaultFen {rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1}

proc fenErrorDialog {{msg {}}} {

  if {[winfo exists .setup]} {
    tk_messageBox -icon info -type ok -title "Scid: Invalid FEN" -message $msg -parent .setup
  } else {
    tk_messageBox -icon info -type ok -title "Scid: Invalid FEN" -message $msg 
  }

}

proc copyFEN {} {
  setClipboard [sc_pos fen]
}

# Bypasses the board setup window and tries to paste the current text selection as the setup position

proc pasteFEN {} {

  set confirm [::game::ConfirmDiscard]
  if {$confirm == 2} { return }
  if {$confirm == 0} {
    sc_game save [sc_game number]
  }
  setTrialMode 0
  sc_game new

  set fen1 {}
  set fen2 {}
  catch {set fen1 [selection get -selection PRIMARY]} 
  catch {set fen2 [selection get -selection CLIPBOARD]}
  # Remove unicode chars, and a leading text \uxxxx which firefox can insert somehow S.A.
  catch {
    regsub -all {[\u0080-\uffff]} $fen1 "" fen1
    regsub -all {\\u....} $fen1 "" fen1
    regsub -all {[\u0080-\uffff]} $fen2 "" fen2
    regsub -all {\\u....} $fen2 "" fen2
  }

  ### Strip any "FEN:" prefix
  if {[string match -nocase fen:* $fen1]} { set fen1 [string trim [string range $fen1 4 end]] }
  if {[string match -nocase fen:* $fen2]} { set fen2 [string trim [string range $fen2 4 end]] }

  if {$fen1 == {}} {
    set fen $fen2
  } else {
    sc_game push
    # use PRIMARY (fen1) unless it looks funny
    if {[catch {sc_game startBoard $fen1}] && $fen2 != ""} {
      set fen $fen2
    } else {
      set fen $fen1
    }
    sc_game pop
  }

  catch {set fen [sanityCheckFENCastling [string trim $fen]]}

  if {$fen == ""} {
    fenErrorDialog "The clipboard is empty.\n"
    return
  }

  ### If the first arg ends with "/", then remove it, Some people seem to use this
  # (eg 8/3r1pk1/2p1b3/2p3p1/2P1P3/1P3P2/4KB2/2R5/ B)
  # and lowercase the second letter.
  catch {
      set s1 [lindex $fen 0]
      if {[string index $s1 end] == "/"} { 
        set s1 [string range $s1 0 end-1]
      }
      set s2 [lindex $fen 1]
      if {$s2 == "W" || $s2 == "B"} {
	set s2 [string tolower $s2]
      }
      set fen "$s1 $s2 [lrange $fen 2 end]"
  }

  if {[catch {sc_game startBoard $fen}]} {
    if {[string length $fen] > 80} {
      set fen [string range $fen 0 80]
      append fen "..."
    }
    # set fen [string map {\n { }} $fen]

    fenErrorDialog "\"$fen\" is not a valid FEN.

FEN is the standard representation of a chess position, for example:
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

  }
  updateBoard -pgn
}

proc copyGame {} {
  catch {sc_clipbase copy}
  set ::glistFlipped([sc_info clipbase]) [::board::isFlipped .main.board] 
  # is updateBoard needed ?
  updateBoard
  ::windows::switcher::Refresh
}


proc pasteGame {} {
  sc_clipbase paste
  if {$::glistFlipped([sc_info clipbase]) != [::board::isFlipped .main.board]} { 
    ::board::flip .main.board
  } 
  updateBoard -pgn

  ## Seems best
  # refreshWindows
  ::tools::graphs::score::Refresh
}

proc setSetupBoardToFen {} {
  global setupFen setupboardSize setupBd

  # Called from ".setup.fencombo" FEN combo S.A

  sc_game push
  if {[catch {sc_game startBoard $setupFen} err]} {
    fenErrorDialog $err
  } else {
    # ::utils::history::AddEntry setupFen $setupFen
    set setupBd [sc_pos board]
    setBoard .setup.l.bd $setupBd $setupboardSize
  }
  sc_game pop
}

############################################################
### Board setup window:

set setupBd {}
set setupFen {}

# makeSetupFen:
#    Reconstructs the FEN string from the current settings in the
#    setupBoard dialog. Check to see if the position is
#    acceptable (a position can be unacceptable by not having exactly
#    one King per side, or by having more than 16 pieces per side).
#

proc makeSetupFen {args} {
  global setupFen setupBd moveNum pawnNum toMove castling epFile
  set fen ""
  set errorStr [validateSetup]
  if {$errorStr != ""} {
    set setupFen "Invalid board: $errorStr"
    return
  }
  for {set bRow 56} {$bRow >= 0} {incr bRow -8} {
    if {$bRow < 56} { append fen "/" }
    set emptyRun 0
    for {set bCol 0} {$bCol < 8} {incr bCol} {
      set sq [expr {$bRow + $bCol} ]
      set piece [string index $setupBd $sq]
      if {$piece == "."} {
        incr emptyRun
      } else {
        if {$emptyRun > 0} {
          append fen $emptyRun
          set emptyRun 0
        }
        append fen $piece
      }
    }
    if {$emptyRun > 0} { append fen $emptyRun }
  }
  append fen " " [string tolower [string index $toMove 0]] " "
  if {$castling == ""} {
    append fen "- "
  } else {
    append fen $castling " "
  }
  if {$epFile == ""  ||  $epFile == "-"} {
    append fen "-"
  } else {
    append fen $epFile
    if {$toMove == "White"} {
      append fen "6"
    } else {
      append fen "3"
    }
  }
  # We assume a halfmove clock of zero:
  # append fen " 0 " $moveNum

  if {[string is integer -strict $pawnNum]} {
      append fen " $pawnNum " $moveNum
  } else {
      append fen " 0 " $moveNum
  }

  set setupFen $fen
}

# validateSetup:
#   Called by makeSetupFen to check that the board is sensible: that is,
#   that there is one king per side and there are at most 16 pieces per
#   side.
#
proc validateSetup {} {
  global setupBd
  set wkCount 0; set bkCount 0; set wCount 0; set bCount 0
  set wpCount 0; set bpCount 0
  for {set i 0} {$i < 64} {incr i} {
    set p [string index $setupBd $i]
    if {$p == "."} {
    } elseif {$p == "P"} { incr wCount; incr wpCount
    } elseif {$p == "p"} { incr bCount; incr bpCount
    } elseif {$p in {N B R Q}} {
      incr wCount
    } elseif {$p in {n b r q}} {
      incr bCount
    } elseif {$p == "K"} { incr wCount; incr wkCount
    } elseif {$p == "k"} { incr bCount; incr bkCount
    } else { return "Invalid piece: $p" }
  }
  if {$wkCount != 1} { return "There must be one white king"
  } elseif {$bkCount != 1} { return "There must be one black king"
  } elseif {$wCount > 16} { return "Too many white pieces"
  } elseif {$bCount > 16} { return "Too many black pieces"
  } elseif {$wpCount > 8} { return "Too many white pawns"
  } elseif {$bpCount > 8} { return "Too many black pawns" }
  return ""
}

proc setupBoardRightClick {square} {
  global setupBd pastePiece 

  set temp [string index $setupBd $square]
  if {$temp != "."} {
    set pastePiece $temp
  } else {
    if {[string is upper $pastePiece]} {
      set pastePiece [string tolower $pastePiece]
    } else {
      set pastePiece [string toupper $pastePiece]
    }
  }
}

#    Called by setupBoard to set or clear a square when it is clicked on.
#    Sets that square to containing the active piece (stored in pastePiece)
#    unless it already contains that piece, in which case the square is
#    cleared to be empty.

proc setupBoardPiece {square {clear 0}} {
  global setupBd pastePiece setupboardSize setupFen
  set oldState $setupBd
  set setupBd {}
  set piece $pastePiece

  if {[string index $oldState $square] == $pastePiece || $clear } {
    if {$clear} {
      set temp [string index $oldState $square]
      if {$temp != "."} {
	set pastePiece $temp
      }
    }
    set piece "."
  }
  if {$piece == "P"  ||  $piece == "p"} {
    if {$square < 8  ||  $square >= 56} {
      set setupBd $oldState
      unset oldState
      return
    }
  }
  append setupBd \
    [string range $oldState 0 [expr {$square - 1} ]] \
    $piece \
    [string range $oldState [expr {$square + 1} ] 63]
  unset oldState
  setBoard .setup.l.bd $setupBd $setupboardSize
  makeSetupFen
}

proc copyBoardPiece {square} {
  global setupBd pastePiece

  set temp [string index $setupBd $square]
  if {$temp != "."} {
    set pastePiece $temp
  }
}

# switchPastePiece:
#   Changes the active piece selection in the board setup dialog to the
#   next or previous piece in order.
#
proc switchPastePiece { switchType } {
  global pastePiece
  array set nextPiece { K Q Q R R B B N N P P k k q q r r b b n n p p K}
  array set prevPiece { K p Q K R Q B R N B P N k P q k r q b r n b p n}
  if {$switchType == "next"} {
    set pastePiece $nextPiece($pastePiece)
  } else {
    set pastePiece $prevPiece($pastePiece)
  }
}

proc exitSetupBoard {} {

  # called when "OK" button hit

  global setupFen selectedSq

  bind .setup <Destroy> {}
  set selectedSq -1

  # We always always creating a new game before entering setup board, so no point making undoPoint
  # sc_game undoPoint

  set setupFen [sanityCheckFENCastling $setupFen]

  if {$setupFen == $::defaultFen} {
    sc_game new
    destroy .setup
    updateBoard -pgn
  } else {
    if {[catch {sc_game startBoard $setupFen} err]} {
      fenErrorDialog $err
      bind .setup <Destroy> cancelSetupBoard

      # Ideally, "$err" should be more specific than "Invalid FEN", but
      # procedural flow is a little complicated S.A.
    } else {
      ::utils::history::AddEntry setupFen $setupFen
      destroy .setup
      updateBoard -pgn
    }
  }
}


### Do a sanity check on castling
### Helpful because illegal FENs crash engines and we could also have one for enpassant

proc sanityCheckFENCastling {fen} {

  # Castling part of the fen
  set c [lindex $fen 2]

  # todo: missing space (K1w) breaks it
  # r1rbn1k1/2qb1p1p/3p4/5P2/p1p5/P1B4P/1PBQ1PP1/R3R1K1w KQkq - 0 1

  if {![validatePiece $fen r 1 1]} {set c [string map {q {}} $c]}
  if {![validatePiece $fen k 5 1]} {set c [string map {k {} q {}} $c]}
  if {![validatePiece $fen r 8 1]} {set c [string map {k {}} $c]}

  if {![validatePiece $fen R 1 8]} {set c [string map {Q {}} $c]}
  if {![validatePiece $fen K 5 8]} {set c [string map {K {} Q {}} $c]}
  if {![validatePiece $fen R 8 8]} {set c [string map {K {}} $c]}

  if {$c == {}} {set c {-}}
  return "[lreplace $fen 2 2 $c]"
}


proc validatePiece {fen piece x y} {

  # Check fen and return true if "$piece" resides at square x,y. S.A

  set pos [expr $x - 1 + ($y - 1) * 8]
  set square 0
  set i      0
  while {1} {
    # process each char in the Fen until we get past where the piece should be

    set ch [string index $fen $i]
    incr i

    if {$ch == {/}}		{continue}
    if {$square == $pos}	{return [expr {$ch == $piece}]}
    if {$square  > $pos}	{return 0}
    if {[string is digit -strict $ch]} {
      incr square $ch
    } else {
      incr square
    }
  }
}
proc cancelSetupBoard {} {

  # When FEN strings are previewed, the gameboard state is changed, but *not*
  # drawn in the main window. This means that while the game state can be
  # restored in the event of user hitting "cancel", game history has been lost
  # This behaviour is necessary to enable FEN previewing.

  global origFen selectedSq

  bind .setup <Destroy> {}

  # restore old gamestate if changed

  if {$origFen != "[sc_pos fen]"} {
    catch {sc_game startBoard $origFen}
    updateBoard -pgn
  }
  set selectedSq -1
  destroy .setup
}


# Global variables for entry of the start position:
set epFile {}          ;# legal values are empty, or "a"-"h".
set moveNum 1          ;# legal values are 1-999.
set pawnNum 0
set castling KQkq      ;# will be empty or some combination of KQkq letters.
set toMove White       ;# side to move, "White" or "Black".
set pastePiece P       ;# Piece being pasted, "K", "k", "Q", "q", etc.

# Traces to keep entry values sensible:

proc check_moveNum {a b c} {
  ::utils::validate::Integer 999 0 $a $b $c
  makeSetupFen
}
proc check_epFile {a b c} {
  ::utils::validate::Regexp {^(-|[a-h])?$} $a $b $c
  makeSetupFen
}
proc check_castling {a b c} {
  ::utils::validate::Regexp {^(-|[KQkq]*)$} $a $b $c
  makeSetupFen
}
trace variable moveNum w check_moveNum
trace variable pawnNum w check_moveNum
trace variable epFile w check_epFile
trace variable castling w check_castling

# setupBoard:
#   The main procedure for creating the dialog for setting the start board.
#   Calls switchPastePiece and makeSetupFen.
#   On "Setup" button press, calls sc_pos startBoard to try to set the
#   starting board.

#   todo: perhaps ensure all engines have stopped before doing this S.A.

proc setupBoard {} {

  global boardSizes boardSize setupboardSize setupBd pastePiece \
         toMove epFile moveNum pawnNum castling setupFen highcolor origFen borderwidth selectedSq 

  set w .setup
  if {[winfo exists $w]} {
    raiseWin $w
    return
  }

  set confirm [::game::ConfirmDiscard]
  if {$confirm == 2} { return }
  if {$confirm == 0} {
    sc_game save [sc_game number]
  }
  setTrialMode 0
  set origFen [sc_pos fen]
  set setupBd [sc_pos board]

  sc_game new
  if {$origFen != $::defaultFen} {
    sc_game startBoard $origFen
  }

  updateBoard -pgn

  toplevel $w
  wm title $w "Setup Board"
  setWinLocation $w

  set selectedSq -1

  # Fenframe is a gridded frame at bottom of screen
  frame $w.fenframe
  pack $w.fenframe -side bottom -fill x -padx 5 -pady 5

  set sl $w.l
  set sr $w.r
  set sbd $sl.bd

  frame $sl
  frame $sr
  pack $sl -side left -expand 1 -fill both
  pack $sr -side right -expand 1 -fill y

  # make the setup board a couple of sizes smaller
  set setupboardSize [boardSize_plus_n -3]
  set psize $setupboardSize

  # We now use ::board for the Setup board 
  # We probably change selectedSq (etc) from a global to something board specific
  ::board::new $sbd $setupboardSize 0

  # Show double coords if main board has them
  if {$::boardCoords == 2} {
    set ::board::_coords($sbd) 2
  } else {
    set ::board::_coords($sbd) 1
  }
  ::board::coords $sbd

  if { [::board::isFlipped .main.board] } {
     ::board::flip $sbd
  }

  # border not implemented yet
  set border $borderwidth
  set bsize [expr $psize * 8 + $border * 9 + 1]

  ### Main setup board/canvas

  if {!$::macOS} {
  frame $sl.hints
  label $sl.hints.label2 -text {Left button - Paste} -font font_SmallItalic
  label $sl.hints.label3 -text {Middle - Select} -font font_SmallItalic
  label $sl.hints.label4 -text {Right button - Clear} -font font_SmallItalic
  pack $sl.hints -side top -fill x
  pack $sl.hints.label2 $sl.hints.label3 $sl.hints.label4 -side left -expand yes -fill x
  }

  pack $sbd -padx 10 -pady 10

  for {set i 0} {$i < 64} {incr i} {
    $sbd.bd bind p$i <ButtonPress-1> "set ::selectedSq $i ; ::board::setDragSquare $sbd $i"
    $sbd.bd bind p$i <ButtonPress-2> "copyBoardPiece $i"
    $sbd.bd bind p$i <ButtonPress-3> "setupBoardPiece $i 1"
  }
  bind $sbd.bd <B1-Motion> "::board::dragPiece $sbd %x %y"
  bind $sbd.bd <ButtonRelease-1> "releaseSetupSquare $sbd %x %y"
  bind $w <ButtonPress-4> "switchPastePiece next"
  bind $w <ButtonPress-5> "switchPastePiece prev"

  pack [frame $sl.w] -side bottom -padx 8 -pady 8
  pack [frame $sl.b] -side bottom -padx 8 -pady 8

  setBoard $sbd $setupBd $setupboardSize

  ### Piece Buttons

  set setupboardSize2 [boardSize_plus_n -4]
  foreach i {p n b r q k} {
    foreach color {w b} value "[string toupper $i] $i" {
      if {$::macOS} {
	ttk::radiobutton $sl.$color.$i -image $color$i$setupboardSize2 \
	  -variable pastePiece -value $value 
      } else {
	radiobutton $sl.$color.$i -image $color$i$setupboardSize2 -indicatoron 0 \
	  -variable pastePiece -value $value -activebackground $highcolor
      }
	# -relief raised -activebackground grey75 -selectcolor rosybrown
      pack $sl.$color.$i -side left ;# -expand yes -fill x -padx 5
    }
  }

  ### Side to move frame.

  frame $sr.tomove
  label $sr.tomove.label -textvar ::tr(SideToMove)
  frame $sr.tomove.buttons
  radiobutton $sr.tomove.buttons.w -text $::tr(White) -variable toMove -value White \
    -command makeSetupFen
  radiobutton $sr.tomove.buttons.b -text $::tr(Black) -variable toMove -value Black \
    -command makeSetupFen

  pack $sr.tomove -pady 5
  pack $sr.tomove.label -side top -pady 2
  pack $sr.tomove.buttons -side top
  pack $sr.tomove.buttons.w $sr.tomove.buttons.b -side left

  set toMove [lindex $origFen 1]
  if {$toMove == "b" || $toMove == "B"} {
    set toMove Black
  } else {
    set toMove White
  }

  set pawnNum [lindex $origFen end-1]
  if {![string is integer -strict $pawnNum]} {
    set pawnNum 0
  }

  set moveNum [lindex $origFen end]
  if {![string is integer -strict $moveNum]} {
    set moveNum 1
  }

  pack [frame $sr.mid] -padx 5 -pady 5

  ### Move number

  frame $sr.mid.movenum
  label $sr.mid.movenum.label -textvar ::tr(MoveNumber)
  entry $sr.mid.movenum.e -width 3 -textvariable moveNum

  pack $sr.mid.movenum -pady 5 -expand yes -fill x
  pack $sr.mid.movenum.label -side left -anchor w
  pack $sr.mid.movenum.e -side right

  ### Moves since capture/pawn move

  frame $sr.mid.pawnnum
  label $sr.mid.pawnnum.label -textvar ::tr(HalfMoves)
  entry $sr.mid.pawnnum.e -width 3 -textvariable pawnNum

  pack $sr.mid.pawnnum -pady 5 -expand yes -fill x
  pack $sr.mid.pawnnum.label -side left -anchor w
  pack $sr.mid.pawnnum.e -side right

  ### Castling 

  frame $sr.mid.castle
  label $sr.mid.castle.label -textvar ::tr(Castling)
  ttk::combobox $sr.mid.castle.e -width 5 -textvariable castling -values {KQkq KQ kq K Q k q -}

  set castling [lindex $origFen 2]

  pack $sr.mid.castle -pady 5 -expand yes -fill x
  pack $sr.mid.castle.label -side left -anchor w
  pack $sr.mid.castle.e -side right

  ### En Passant file

  frame $sr.mid.ep
  label $sr.mid.ep.label -textvar ::tr(EnPassantFile)
  ttk::combobox $sr.mid.ep.e -width 2 -textvariable epFile -values {- a b c d e f g h}

  set epFile [string index [lindex $origFen 3] 0]

  pack $sr.mid.ep -pady 5 -expand yes -fill x
  pack $sr.mid.ep.label $sr.mid.ep.e -side left -anchor w -expand yes -fill x

  # Set bindings so the Fen string is updated at any change. The "after idle"
  # is needed to ensure any keypress which causes a text edit is processed
  # before we regenerate the FEN text.

  foreach i "$sr.mid.ep.e $sr.mid.castle.e $sr.mid.movenum.e $sr.mid.pawnnum.e" {
    bind $i <Any-KeyPress> {after idle makeSetupFen}
    bind $i <FocusOut> {after idle makeSetupFen}
  }

  ### Buttons: Clear Board and Initial Board.

  frame $sr.b

  button $sr.b.clear -textvar ::tr(EmptyBoard) -command {
    set setupBd \
      "..............K..................................k.............."
    setBoard .setup.l.bd $setupBd $setupboardSize
    set castling {}
    makeSetupFen
  } -width 10

  button $sr.b.initial -textvar ::tr(InitialBoard) -command {
    set setupBd \
      "RNBQKBNRPPPPPPPP................................pppppppprnbqkbnr"
    setBoard .setup.l.bd $setupBd $setupboardSize
    set castling KQkq
    set pawnNum 0
    set moveNum 1
    makeSetupFen
  } -width 10

  button $sr.b.flip -text [lindex [tr FlipBoard] 0] -command {
    makeSetupFen
    ::board::flip .setup.l.bd
    ::board::update .setup.l.bd $setupBd
  } -width 10

  # Are these bullet-proof and correct ? 
  # but make them a little smaller, as they are hardly used

  button $sr.b.swap -text {Swap Colours} -command {
    set tmp {}
    foreach char [split $setupBd {}] {
      append tmp [::utils::string::invertcase $char]
    }
    set setupBd $tmp

    set tmp {}
    foreach char [split $castling {}] {
      append tmp [::utils::string::invertcase $char]
    }
    set castling $tmp

    setBoard .setup.l.bd $setupBd $setupboardSize

    if {$toMove != {Black}} {
      set toMove Black
    } else {
      set toMove White
    }

    makeSetupFen
  } -width 10 -font font_Small

  button $sr.b.invert -text Invert -command {
    # reverse line order
    set tmp [lindex $setupBd 0] ; # sometimes setupBd has trailing side-to-move. Correct ???
    set setupBd {}
    set eight [string range $tmp 0 7]
    while {$eight != {}} {
      set setupBd "$eight$setupBd"
      set tmp [string range $tmp 8 end]
      set eight [string range $tmp 0 7]
    }
    setBoard .setup.l.bd $setupBd $setupboardSize
    makeSetupFen
  } -width 10 -font font_Small

  button $sr.b.transpose -text Transpose -command {
    # reverse each line
    set tmp [lindex $setupBd 0] ; # sometimes setupBd has trailing side-to-move. Correct ???
    set setupBd {}
    set eight [string range $tmp 0 7]
    while {$eight != {}} {
      set setupBd "$setupBd[string reverse $eight]"
      set tmp [string range $tmp 8 end]
      set eight [string range $tmp 0 7]
    }
    setBoard .setup.l.bd $setupBd $setupboardSize
    makeSetupFen
  } -width 10 -font font_Small

  pack $sr.b		-side top -pady 10
  pack $sr.b.clear	-side top -padx 5 -pady 2
  pack $sr.b.initial	-side top -padx 5 -pady 2
  pack $sr.b.flip	-side top -padx 5 -pady 2
  pack [frame $sr.b.space -height 10] -side top
  pack $sr.b.swap	-side top -padx 5 -pady 1
  pack $sr.b.invert	-side top -padx 5 -pady 1
  pack $sr.b.transpose	-side top -padx 5 -pady 1

  ### Buttons: Setup and Cancel.

  frame $sr.b2
  button $sr.b2.ok -text "OK" -width 7 -command exitSetupBoard
  button $sr.b2.cancel -textvar ::tr(Cancel) -width 7 -command cancelSetupBoard

  pack $sr.b2 -side bottom -pady 10 -anchor s
  pack $sr.b2.ok -side left -padx 5
  pack $sr.b2.cancel -side right -padx 5

  ### Fen combobox and buttons
  button .setup.paste -textvar ::tr(PasteFen) -command {
    if {[catch {set setupFen [selection get -selection PRIMARY]} ]} {
      catch {set setupFen [selection get -selection CLIPBOARD]}
      # PRIMARY is the X selection, unsure about CLIPBOARD
    }
  }
  button .setup.clear -textvar ::tr(ClearFen) -command {set setupFen ""}

  ttk::combobox .setup.fencombo -textvariable setupFen -height 10
  bind .setup.fencombo <<ComboboxSelected>> setSetupBoardToFen
  ::utils::history::SetCombobox setupFen .setup.fencombo
  ::utils::history::SetLimit setupFen 20

  update ; # necessary in case of quick-draw user interactions

  pack .setup.paste .setup.clear -in .setup.fenframe -side left
  pack .setup.fencombo -in .setup.fenframe -side right -expand yes -fill x -anchor w

  bind .setup <Escape> cancelSetupBoard
  bind .setup <Destroy> cancelSetupBoard

  makeSetupFen

  bind .setup <Configure> "recordWinSize .setup"
}

proc releaseSetupSquare {w x y} {

  global selectedSq bestSq setupBd pastePiece setupboardSize

  ::board::setDragSquare $w -1
  set square [::board::getSquare $w $x $y]
  if {$square < 0} {
    set selectedSq -1
    setBoard .setup.l.bd $setupBd $setupboardSize
    return
  }

  set prev [string index $setupBd $selectedSq]
  set this [string index $setupBd $square]

  if {$square == $selectedSq} {
    setupBoardPiece $square
  } else {
    if {$selectedSq == -1} {
      setBoard .setup.l.bd $setupBd $setupboardSize
      return
    }

    # only process drag and drop if drag square wasn't empty ("."), and pawn placement is legal

    if {($prev == "P"  ||  $prev == "p") && ($square < 8  ||  $square >= 56)} {
      setBoard .setup.l.bd $setupBd $setupboardSize
      return
    }
    setupBoardPiece $selectedSq 1 ; # ???
    if {$prev != "."} {
      setupBoardPiece $square
    }
  }
}

#   Resets the squares of the board according to the board string
#   "boardStr" and the piece bitmap size "psize".

proc setBoard {w boardStr psize} {
  for {set i 0} { $i < 64 } { incr i } {

    set c [$w.bd coords sq$i]

    set x [expr {[lindex $c 0] + $psize/2} ]
    set y [expr {[lindex $c 1] + $psize/2} ]

    set piece [string index $boardStr $i]
    $w.bd delete p$i
    $w.bd create image $x $y -image $::board::letterToPiece($piece)$psize -tag p$i
  }
}

### end of edit.tcl
