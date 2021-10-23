### tools/tablebase.tcl:
###   Tablebase display routines for Scid.

set tbTraining 0
set tbBoard 0
set tbStatus ""

set ::tb::online_available [expr ! [catch {
  package require http
  ::splash::add "tls package [package require tls] found"
} ] ]

namespace eval ::tb {
  set url {}

  # Proxy unused for new lichess tablebases
  # # proxy configuration
  # set proxyhost "127.0.0.1"
  # set proxyport 3128

  set token {}
  # caching results of queries
  set afterid(update) {}
  set afterid(connect) {}
  array set hash {}
  set history {}
  # helper for a flick-free display
  set noresult 0
}

set tbInfo(section) 21
set tbInfo(material) "kpk"
set tbInfo(sections) [list 21 22 31 32 41]
foreach i $tbInfo(sections) { set tbInfo($i) [list] }

set tbInfo(21) [list kqk krk kbk knk kpk]

set tbInfo(22) [list \
    kqkq kqkr kqkb kqkn kqkp \
    -    krkr krkb krkn krkp \
    -    -    kbkb kbkn kbkp \
    -    -    -    knkn knkp \
    -    -    -    -    kpkp ]

set tbInfo(31) [list \
    kqqk kqrk kqbk kqnk kqpk \
    -    krrk krbk krnk krpk \
    -    -    kbbk kbnk kbpk \
    -    -    -    knnk knpk \
    -    -    -    -    kppk ]

set tbInfo(32) [list \
    kqqkq kqqkr kqqkb kqqkn kqqkp \
    kqrkq kqrkr kqrkb kqrkn kqrkp \
    kqbkq kqbkr kqbkb kqbkn kqbkp \
    kqnkq kqnkr kqnkb kqnkn kqnkp \
    kqpkq kqpkr kqpkb kqpkn kqpkp \
    -     -     -     -     -     \
    krrkq krrkr krrkb krrkn krrkp \
    krbkq krbkr krbkb krbkn krbkp \
    krnkq krnkr krnkb krnkn krnkp \
    krpkq krpkr krpkb krpkn krpkp \
    -     -     -     -     -     \
    kbbkq kbbkr kbbkb kbbkn kbbkp \
    kbnkq kbnkr kbnkb kbnkn kbnkp \
    kbpkq kbpkr kbpkb kbpkn kbpkp \
    -     -     -     -     -     \
    knnkq knnkr knnkb knnkn knnkp \
    knpkq knpkr knpkb knpkn knpkp \
    kppkq kppkr kppkb kppkn kppkp ]

set tbInfo(41) [list \
    kqqqk kqqrk kqqbk kqqnk kqqpk \
    -     kqrrk kqrbk kqrnk kqrpk \
    -     -     kqbbk kqbnk kqbpk \
    -     -     -     kqnnk kqnpk \
    -     -     -     -     kqppk \
    -     krrrk krrbk krrnk krrpk \
    -     -     krbbk krbnk krbpk \
    -     -     -     krnnk krnpk \
    -     -     -     -     krppk \
    -     -     kbbbk kbbnk kbbpk \
    -     -     -     kbnnk kbnpk \
    -     -     -     -     kbppk \
    -     -     -     knnnk knnpk \
    -     -     -     -     knppk \
    -     -     -     -     kpppk ]

set tbInfo(42) [list \
    kqqqkq kqqqkr kqqqkb kqqqkn kqqqkp \
    kqqrkq kqqrkr kqqrkb kqqrkn kqqrkp \
    kqqbkq kqqbkr kqqbkb kqqbkn kqqbkp \
    kqqnkq kqqnkr kqqnkb kqqnkn kqqnkp \
    kqqpkq kqqpkr kqqpkb kqqpkn kqqpkp \
    kqrrkq kqrrkr kqrrkb kqrrkn kqrrkp \
    kqrbkq kqrbkr kqrbkb kqrbkn kqrbkp \
    kqrnkq kqrnkr kqrnkb kqrnkn kqrnkp \
    kqrpkq kqrpkr kqrpkb kqrpkn kqrpkp \
    kqbbkq kqbbkr kqbbkb kqbbkn kqbbkp \
    kqbnkq kqbnkr kqbnkb kqbnkn kqbnkp \
    kqbpkq kqbpkr kqbpkb kqbpkn kqbpkp \
    kqnnkq kqnnkr kqnnkb kqnnkn kqnnkp \
    kqnpkq kqnpkr kqnpkb kqnpkn kqnpkp \
    kqppkq kqppkr kqppkb kqppkn kqppkp \
    krrrkq krrrkr krrrkb krrrkn krrrkp \
    krrbkq krrbkr krrbkb krrbkn krrbkp \
    krrnkq krrnkr krrnkb krrnkn krrnkp \
    krrpkq krrpkr krrpkb krrpkn krrpkp \
    krbbkq krbbkr krbbkb krbbkn krbbkp \
    krbnkq krbnkr krbnkb krbnkn krbnkp \
    krbpkq krbpkr krbpkb krbpkn krbpkp \
    krnnkq krnnkr krnnkb krnnkn krnnkp \
    krnpkq krnpkr krnpkb krnpkn krnpkp \
    krppkq krppkr krppkb krppkn krppkp \
    kbbbkq kbbbkr kbbbkb kbbbkn kbbbkp \
    kbbnkq kbbnkr kbbnkb kbbnkn kbbnkp \
    kbbpkq kbbpkr kbbpkb kbbpkn kbbpkp \
    kbnnkq kbnnkr kbnnkb kbnnkn kbnnkp \
    kbnpkq kbnpkr kbnpkb kbnpkn kbnpkp \
    kbppkq kbppkr kbppkb kbppkn kbppkp \
    knnnkq knnnkr knnnkb knnnkn knnnkp \
    knnpkq knnpkr knnpkb knnpkn knnpkp \
    knppkq knppkr knppkb knppkn knppkp \
    kpppkq kpppkr kpppkb kpppkn kpppkp ]

proc ::tb::isopen {} {
  return [winfo exists .tbWin]
}

proc ::tb::Open {} {
  global tbInfo tbOnline

  set w .tbWin
  if {[winfo exists $w]} {
    raiseWin $w
    return
  }
  ::createToplevel $w
  catch {wm state $w withdrawn}
  setWinLocation $w
  setWinSize $w
  setTitle $w "[tr WindowsTB]"

  pack [frame $w.b] -side bottom -fill x ;# buttons
  pack [frame $w.info] -side left -fill y ;# summary
  pack [frame $w.pos] -side left -fill both -expand yes -padx 5 -pady 3 ;# results

  ### Tablebase browser and Summary

  set f $w.info
  pack [frame $f.sec] -side top -pady 3

  label $f.sec.label -text Summary
  menubutton $f.sec.menu -text {2-1} -menu $f.sec.menu.m -relief raised -indicatoron 1 
  menu $f.sec.menu.m -tearoff 0
  foreach i $tbInfo(sections) {
    set name "[string index $i 0]-[string index $i 1]"
    $f.sec.menu.m add command -label $name -command "
      $f.sec.menu configure -text $name
      set tbInfo(section) $i
      ::tb::section $i
    "
  }
  pack $f.sec.label $f.sec.menu -side left -pady 1 -padx 10

  autoscrollframe $f.list text $f.list.text \
      -width 35 -height 7 -font font_Fixed -wrap none -cursor top_left_arrow
  $f.list configure -relief flat
  pack $f.list -side top

  # pack [frame $f.separator -height 2]
  # addHorizontalRule $f

  autoscrollframe $f.data text $f.data.text \
      -width 35 -height 0 -font font_Fixed -wrap none -cursor top_left_arrow
  $f.data configure -relief flat
  pack $f.data -side top -fill y -expand yes

  $f.list.text tag configure avail -foreground blue
  $f.list.text tag configure unavail -foreground gray40
  $f.data.text tag configure fen -foreground blue

  ### Results for current position

  set f $w.pos

  pack [frame $f.results] -side top -pady 3

  pack [label $f.results.label -text Results] -side left -padx 30

  if { $::tb::online_available } {
    menubutton $f.results.online -text $tbOnline -menu $f.results.online.menu -relief raised -indicatoron 1
    menu $f.results.online.menu -tearoff 0
    foreach i {Nalimov Shredder Lichess} {
      $f.results.online.menu add command -label $i -command "
        $f.results.online configure -text $i
        set tbOnline $i 
        update_tbWidgets $w $i
        ::tb::results
      "
    }


    # ttk::combobox $f.results.online -textvariable tbOnline -values {Nalimov Lichess Shredder} -width 10 -state readonly -takefocus 0
    # bind $f.results.online <<ComboboxSelected>> ::tb::results
    pack $f.results.online -side right -padx 30 -pady 2
  }
  
  text $f.text -font font_Fixed -relief flat -wrap word
  pack $f.text -side top -fill both -expand yes

  $f.text tag configure indent -lmargin2 [font measure font_Fixed  "        "]
  $f.text tag configure title -font font_Regular -justify center

  ### Board

  ::board::new $f.board 25
  $f.board configure -relief solid -borderwidth 1
  if {$::tbBoard} {
    pack $f.board -side bottom -before $f.text -pady 3
  }

  for {set i 0} {$i < 64} {incr i} {
    ::board::bind $f.board $i <Button-1> [list ::tb::resultsBoard $i]
  }

  ### Buttons

  checkbutton $w.b.training -text $::tr(Training) -variable tbTraining -command ::tb::training -relief raised -padx 4 -pady 5
  button $w.b.random -text "Random" -command ::tb::random
  button $w.b.showboard -image tb_coords -command ::tb::showBoard
  button $w.b.help -text $::tr(Help) -command { helpWindow TB }
  button $w.b.close -text $::tr(Close) -command "destroy $w"
  label $w.b.status -width 1 -textvar tbStatus -font font_Small \
      -relief flat -anchor w -height 0
  packbuttons right $w.b.close $w.b.help $w.b.showboard 
  pack $w.b.training $w.b.random -side left -padx 8 -pady 2
  pack $w.b.status -side left -fill x -expand yes
  bind $w <Destroy> {set tbTraining 0}
  bind $w <Escape> "destroy $w"
  bind $w <F1> {helpWindow TB}
  bind $w <Configure> "recordWinSize $w"
  wm minsize $w 15 15
  set ::tbTraining 0
  ::tb::section
  ::tb::summary
  ::tb::results
  if {$::tb::online_available} {
    update_tbWidgets $w $tbOnline
  } else {
    set tbOnline Nalimov
  }
  update
  catch {wm state $w normal}
  ::createToplevelFinalize $w
}

proc update_tbWidgets {w tb} {
  if {$tb != "Nalimov"} {
    pack forget $w.info
    pack forget $w.b.random
  } else {
    pack $w.info -side left -fill y -before $w.pos
    pack $w.b.random -side left -padx 8 -pady 2 -after $w.b.training
  }
}

###  Toggle the results board.

proc ::tb::showBoard {} {
  global tbBoard
  set f .tbWin.pos
  if {$tbBoard} {
    set tbBoard 0
    pack forget $f.board
  } else {
    set tbBoard 1
    pack $f.board -side bottom -before $f.text -pady 3
  }
}

### Updates the resultsBoard board for a particular square.

proc ::tb::resultsBoard {sq} {
  set f .tbWin.pos
  set board [sc_pos board]
  # If selected square is empty, take no action:
  if {[string index $board $sq] == "."} { return }
  # Clear any previous results:
  ::board::clearText $f.board
  # Highlight the selected square:
  ::board::colorSquare $f.board $sq $::highcolor
  # Retrieve tablebase scores:
  busyCursor .
  set scores [sc_pos probe board $sq]
  set text(X) X; set color(X) red4; set shadow(X) grey
  set text(=) =; set color(=) blue; set shadow(=) grey
  set text(?) "?"; set color(?) red4; set shadow(?) grey
  set text(+) "#"; set text(-) "#"
  if {[sc_pos side] == "white"} {
    set color(+) white; set color(-) black
    set shadow(+) black; set shadow(-) white
  } else {
    set color(+) black; set color(-) white
    set shadow(+) white; set shadow(-) black
  }
  for {set i 0} {$i < 64} {incr i} {
    # Skip squares that have a piece.
    if {[string index $board $i] != "."} { continue }
    # Draw the score on this square:
    set score [string index $scores $i]
    catch {::board::drawText $f.board $i $text($score) $color($score) 0 $shadow($score)}
  }
  unbusyCursor .
}

### Converts a material string like "kqkr" or "KQKR" to "KQ-KR".

proc ::tb::name {s} {
  set s [string toupper $s]
  set idx [string last "K" $s]
  set new [string range $s 0 [expr $idx - 1]]
  append new "-"
  append new [string range $s $idx end]
  return $new
}

### Clear the text widget.

proc ::tb::clearText {t} {
  $t configure -state normal
  $t delete 1.0 end
  $t configure -state disabled
  set ::tb::noresult 0
}

### Updates the tablebase list for the specified section.

proc ::tb::section {{sec 0}} {
  global tbInfo
  set w .tbWin
  if {! [winfo exists $w]} { return }
  if {$sec == 0} { set sec $tbInfo(section)}
  set tbInfo(section) $sec
  if {! [info exists tbInfo($sec)]} { return }
  set t $w.info.list.text
  ::tb::clearText $t
  set ::tb::tagonline 0
  $t configure -state normal
  $t configure -height 10
  set count 0
  set linecount 1
  foreach tb $tbInfo($sec) {
    if {$count == 5} { set count 0; incr linecount; $t insert end "\n" }
    if {$tb == "-"} {
      $t insert end [format "%-7s" ""]
    } else {
      # This doesn't test that *both* the white and black tb files are available
      set avail [sc_info tb available $tb]
      if {$avail} {
        set taglist [list avail $tb]
      } else {
        set taglist [list unavail $tb]
      }
      $t insert end [format "%-6s" [::tb::name $tb]] $taglist
      $t insert end " "
      # Bind tags for enter/leave/buttonpress on this tb:
      $t tag bind $tb <Any-Enter> \
          [list $t tag configure $tb -background grey]
      $t tag bind $tb <Any-Leave> \
          [list $t tag configure $tb -background {}]
      $t tag bind $tb <ButtonPress-1> [list ::tb::summary $tb]
    }
    incr count
  }

  if {$linecount > 10} { set linecount 10 }
  $t configure -height $linecount
  $t configure -state disabled
}

### Shows the tablebase information for the specified tablebase.

proc ::tb::summary {{material ""}} {
  global tbInfo tbs
  set w .tbWin
  if {! [winfo exists $w]} { return }

  if {$material == ""} { set material $tbInfo(material) }
  set tbInfo(material) $material
  set t $w.info.data.text
  ::tb::clearText $t
  $t configure -state normal
  $t insert end [format "%-6s" [::tb::name $material]] fen
  if {! [info exists tbs($material)]} {
    $t insert end "\nNo summary for this tablebase."
    $t configure -state disabled
    return
  }
  set data $tbs($material)

  $t insert end [format "    %5u games per million\n\n" [lindex $data 0]]

  # Longest-mate and result-percentage stats:

  $t insert end "Side    Longest    %     %     %\n"
  $t insert end "to move   mate    Win  Draw  Loss\n"
  $t insert end "---------------------------------\n"

  # Stats for White:
  $t insert end "White     "
  set len [lindex $data 1]
  set fen [lindex $data 2]
  if {$len == "0"} { set len "-" }
  if {[string length $fen] > 2} {
    append fen " w"
    $t insert end [format "%3s" $len] [list fen $fen]
    $t tag bind $fen <Any-Enter> \
        [list $t tag configure $fen -background grey]
    $t tag bind $fen <Any-Leave> \
        [list $t tag configure $fen -background {}]
    $t tag bind $fen <ButtonPress-1> [list ::tb::setFEN $fen]
  } else {
    $t insert end [format "%3s" $len]
  }
  $t insert end "  "
  $t insert end [format " %5s" [lindex $data 5]]
  $t insert end [format " %5s" [lindex $data 6]]
  $t insert end [format " %5s" [lindex $data 7]]
  $t insert end "\n"

  # Stats for Black:
  $t insert end "Black     "
  set len [lindex $data 3]
  set fen [lindex $data 4]
  if {$len == "0"} { set len "-" }
  if {[string length $fen] > 2} {
    append fen " b"
    $t insert end [format "%3s" $len] [list fen $fen]
    $t tag bind $fen <Any-Enter> \
        [list $t tag configure $fen -background grey]
    $t tag bind $fen <Any-Leave> \
        [list $t tag configure $fen -background {}]
    $t tag bind $fen <ButtonPress-1> [list ::tb::setFEN $fen]
  } else {
    $t insert end [format "%3s" $len]
  }
  $t insert end "  "
  $t insert end [format " %5s" [lindex $data 8]]
  $t insert end [format " %5s" [lindex $data 9]]
  $t insert end [format " %5s" [lindex $data 10]]
  $t insert end "\n\n"

  set mzugs [lindex $data 11]
  $t insert end "Mutual zugzwangs: "
  if {$mzugs >= 0} { $t insert end "$mzugs\n" } else { $t insert end "?\n" }
  if {$mzugs <= 0} {
    $t configure -state disabled
    return
  }

  # Extra Zugzwang info:
  set nBtmLoses [lindex $data 12]
  set nWtmLoses [lindex $data 14]
  set nBothLose [lindex $data 16]
  set zugnames [list " White draws, Black loses: " \
      " Black draws, White loses: " \
      " Whoever moves loses:      "]
  if {$nBtmLoses > 0} {
    $t insert end [lindex $zugnames 0]
    $t insert end [format "%5d\n" $nBtmLoses]
  }
  if {$nWtmLoses > 0} {
    $t insert end [lindex $zugnames 1]
    $t insert end [format "%5d\n" $nWtmLoses]
  }
  if {$nBothLose > 0} {
    $t insert end [lindex $zugnames 2]
    $t insert end [format "%5d\n" $nBothLose]
  }

  # Selected zugzwang positions:
  set btmFens [lindex $data 13]
  set wtmFens [lindex $data 15]
  set bothFens [lindex $data 17]
  set nBtmFens [llength $btmFens]
  set nWtmFens [llength $wtmFens]
  set nBothFens [llength $bothFens]
  set nTotalFens [expr $nBtmFens + $nWtmFens + $nBothFens]
  if {$nTotalFens == 0} {
    $t configure -state disabled
    return
  }

  # Print the lists of selected zugzwang positions:
  $t insert end "\nSelected zugzwang positions:"
  foreach n [list $nBtmFens $nWtmFens $nBothFens] \
      fenlist [list $btmFens $wtmFens $bothFens] \
      name $zugnames tomove [list b w w] {
        if {$n == 0} { continue }
        $t insert end "\n [string trim $name]"
        set count 0
        for {set count 0} {$count < $n} {incr count} {
          set fen [lindex $fenlist $count]
          if {[expr $count % 10] == 0} {
            $t insert end "\n  "
          }
          $t insert end " "
          append fen " $tomove"
          $t insert end [format "%2d" [expr $count + 1]] [list fen $fen]
          $t tag bind $fen <Any-Enter> \
          [list $t tag configure $fen -background grey]
          $t tag bind $fen <Any-Leave> \
          [list $t tag configure $fen -background {}]
          $t tag bind $fen <ButtonPress-1> [list ::tb::setFEN $fen]
        }
      }

  $t configure -state disabled
}

### Called when the main window board changes, to display tablebase
### results for all moves from the current position.

proc ::tb::results {} {
  global tbTraining tbOnline
  set f .tbWin.pos
  if {! [winfo exists $f]} { return }

  # Reset results board:
  ::board::clearText $f.board
  ::board::update $f.board [sc_pos board]

  set t $f.text

  # Update results panel:
  if {$tbTraining} {
    ::tb::clearText $t
    ::tb::insertText "\n (Training mode; results are hidden)"
  } else {
    if { $tbOnline == "Lichess" || $tbOnline == "Shredder"} {
      if {!$::tb::noresult} {
        ::tb::clearText $t
      }
      if { $::tb::online_available} {
        set cmd ::tb::updateOnline
      } else {
        set cmd {}
      }
    } else {
      ::tb::clearText $t
      set cmd [list ::tb::insertText [sc_pos probe report] indent]
    }
    if {[llength $cmd]} {
      variable afterid
      after cancel $afterid(update)
      set afterid(update) [after 100 $cmd]
    }
  }
}

proc ::tb::insertText {s {tag {}}} {
  set t .tbWin.pos.text
  $t configure -state normal
  $t insert end $s {*}$tag
  $t configure -state disabled
}

if { $::tb::online_available } {

  proc ::tb::zeroOnline {} {
    set t .tbWin.pos.text
    $t configure -state normal
    # delete previous online output
    while {1} {
      set del [$t tag nextrange tagonline 1.0]
      if {$del == ""} {break}
      catch {$t delete [lindex $del 0] [lindex $del 1]}
    }
    $t configure -state disabled
    set ::tb::noresult 0
  }

  proc ::tb::insertNoResult {{pieceCount {}}} {
    variable noresult

    # This proc will be called often, so don't
    # update text widget with same content,
    # otherwise the display is flickering.
    if {!$noresult} {
      set t .tbWin.pos.text
      ::tb::zeroOnline
      if {$pieceCount == {}} {
	::tb::insertText "Online: No result" tagonline
      } else {
			if {$::tbOnline == "Lichess"}  {   
			::tb::insertText "Online: No result\nMaximum piece count for Lichess is 7" tagonline
			}
			if {$::tbOnline == "Shredder"}  {   
			::tb::insertText "Online: No result\nMaximum piece count for Shredder is 6" tagonline
			}	
		}
      set noresult 1
    }
  }

  proc ::tb::updateOnline {} {
  
    set t .tbWin.pos.text
    $t configure -state normal
    global env
    variable token
    variable hash
    variable afterid

    set afterid(update) {}

    set w .tbWin
    if {! [winfo exists $w]} { return }

    set pieceCount [sc_pos pieceCount]

	if {$::tbOnline == "Lichess"}  {   
		if {$pieceCount <= 2 || $pieceCount > 7} {
      ::tb::insertNoResult $pieceCount
	  return
		}
	} 
	if {$::tbOnline == "Shredder"}  {   
		if {$pieceCount <= 2 || $pieceCount > 6} {
      ::tb::insertNoResult $pieceCount
      return
		}
	}
	
    set fen [sc_pos fen]
    if {![catch { set result $hash($fen) }]} {
      # show result from cache
      ::tb::showResult $fen {*}$result
      return
    }

    if {[llength $token]} {
      # reset current http request
      ::http::reset $token ignore
      ::http::cleanup $token
      set token {}
    }

    # Delay the contacting message a bit, this avoids flickering in most cases.
    after cancel $afterid(connect)
    set afterid(connect) [after 500 ::tb::showContactMsg]
	
	set fen_original $fen
    # replace spaces in FEN with underscores to meet Lichess & Shredder FEN format:
    set fen [regsub -all { } $fen "_"]

# $t insert end $::tbOnline

	if {$::tbOnline == "Shredder"}  {   
	set ::tb::url "https://www.shredderchess.com/online/playshredder/fetch.php?action=egtb&fen=$fen"
	}
	if {$::tbOnline == "Lichess"}  {   
	set ::tb::url "https://tablebase.lichess.ovh/standard?fen=$fen"
	}
	
    http::register https 443 tls::socket

    set cmd [list ::tb::httpCallback $fen]
    if {[catch {::http::geturl $::tb::url -timeout 5000 -command $cmd} ::tb::token]} {
      # Cancel contact message.
      after cancel $afterid(connect)
      set afterid(connect) {}
      set token {} ;# to be sure
      # Connection failed, flash old message before issuing "No connection"
      after 100
      ::tb::zeroOnline
      ::tb::insertText "No connection." tagonline
    }
  } ;# end of proc ::tb::updateOnline

  proc ::tb::showContactMsg {} {
    variable afterid
    set afterid(connect) {}
    ::tb::zeroOnline
    ::tb::insertText "Contacting server" tagonline
  }

  proc ::tb::httpCallback { fen token } {
    # Cancel contact message
    variable afterid
    after cancel $afterid(connect)
    set afterid(connect) {}

    if {[winfo exists .tbWin] && [::http::status $token] != "ignore"} {
      ::tb::showResult $fen {*}[::tb::getResult $fen $token]
    }

    ::http::cleanup $token
    set ::tb::token {}
  }

  proc ::tb::getResult { fen token } {
    set data [::http::data $token]
    set result ""
    set err ""
	set t .tbWin.pos.text
    $t configure -state normal


    if {[::http::status $token] != "ok"} {
      set err [::http::status $token]
    } else {
      set code [::http::ncode $token]
      switch $code {
        200     { # ok }
        400     { set err "400 - Bad request" }
        404     { set err "404 - Not found" }
        500     { set data "<Result></Result>" }
        default { set err "HTTP code $code received" }
      }
    }
   
    if {[string length $err] == 0} {
		if {$::tbOnline == "Lichess"}  {   
			set i [string first "category" $data]
			set k [string first "dtz" $data]
			set m [string first "dtm" $data]
		}	
		if {$::tbOnline == "Shredder"}  {   
			set i [string first "value" $data]
			set k [string first "NEXTCOLOR" $data]
			set m 1 ;# There are no other special characteristics in the Shredder data
		}	
      if {$i == -1 || $k == -1 || $m == -1} {
        set err "Bad return value"
      } else {
        variable hash
        variable history

		set result $data
		
        # cache the result, but not more than 500 queries
        if {[llength $history] > 500} {
          array unset hash [lindex $history 0]
          set history [lrange $history 1 end]
        }
        lappend history $fen
        set hash($fen) [list $err $result]
      }
    }

    return [list $err $result]
	
  } ;# end of procgetResult

  proc ::tb::showResult { fen err result } {
    ::tb::zeroOnline
    set t .tbWin.pos.text
    $t configure -state normal

    if {[string length $err]} {
      $t insert end "Online: $err" tagonline
    } else { 	
      set empty 1
							# bookmark 1
	
      foreach l [split $result "\n"] {
        if {![string match {*\?\?\?*} $l]} {
          if {$empty} {
          # $t insert end "All results; empty is $empty" tagonline
            set empty 0
          }
        }
      }

      if {$empty} {
        variable hash
        if {[info exists hash($fen)]} {
          set hash($fen) [list "No Result" ""]
        }
        $t insert end "Online: No result" tagonline
      }

####################################################################
# Process results (Michael Brown)

###############################################################
# Process the data in the Shredder answer:

if {$::tbOnline == "Shredder"}  {   

# NB Don't change the following because it splits the result on an invisible CRLF  
set result [split $result "\n"]

if {[string first "_w_" $fen 0] != -1} {
	set turn "w"
	} else {
	set turn "b"
	}
#
# Clip out data for colour without the move: 
if {$turn == "w"} {
set result_stat [string range $result 0 [expr {[string first "NEXTCOLOR" $result] - 2}] ]
set result [string range $result 0 [expr {[string first "NEXTCOLOR" $result] - 2}] ]
} else {
set result_stat [string range $result [expr {[string first "NEXTCOLOR" $result] + 10}] [string length $result] ]
set result [string range $result [expr {[string first "NEXTCOLOR" $result] + 10}] [expr {[string length $result] - 3}] ]
}			
 
if {[string first "Not found" $result] != -1} {
  set not_found 1
} else {
  set not_found 0
}
 
# Eliminate info only non-moves:
set count -1
set win_move ""
set draw_move ""
set loss_move ""
foreach move $result {
    switch -regexp $move {
    [W]     	{set win_move $move}
    [D]     	{set draw_move $move}
    [L]     	{set loss_move $move}
        }
set count [expr {$count + 1}]
if {[string first ":" $move] == -1} {
	set result [lreplace $result $count $count]
	}
}
  
# Produce list of men with their square number
#
# Reduce FEN to squares and men only:
set fen_3 [string range $fen 0 [string first "_" $fen] ]
# Get rid of forward slashes:
set fen_3 [regsub -all {[\/|\/]} $fen_3 ""]
# Insert spaces into FEN string:
set fen_4 ""
set my_char ""
for {set i 0} {$i < [string length $fen_3]} {incr i} {
set temp [string range $fen_3 $i $i]
set my_char [concat $temp " "]
set fen_4 [concat $fen_4 $my_char]
    }
#
# Produce list of men with their square number:
set white_count 0
set black_count 0
set square_start 56
set square_count 56
set square_counter 0
set square_man ""
set piece ""
set piece_list ""
set square_gap 0
#
foreach char $fen_4 {
	switch  -regexp $char	{
				[A-Z]	{set piece $char
						set white_count [expr {$white_count + 1}]
						}
				[a-z]		{set piece $char
						set black_count [expr {$black_count + 1}]
						}
				[0-9]		{set square_gap $char
						}
				}
if {$piece != ""} {
		if {$square_count < 10} {
			set square_man [concat 0$square_count:$piece]
			} else {
			set square_man [concat $square_count:$piece]
			}
	set piece_list [concat $piece_list $square_man ] 
	set square_counter [expr {$square_counter + 1}]
	set square_count [expr {$square_count + 1}]		
	}
set square_counter [expr {$square_counter + $square_gap}]
set square_count [expr {$square_count + $square_gap}]
if {$square_counter == 8 } {
	set square_start [expr {$square_start - 8}]
	set square_counter 0
	set square_count $square_start	
	}
set square_gap 0
set piece ""
set  square_man ""	
}

###########################################################################
### Statistics for won, drawn, loss
#

set move_summary ""
set move_header ""
set move_sign ""
set number_moves 0
set prev_number_moves 0

set win 0
set won_no 0
set win_count 0
set win_move ""
set temp_W ""
set w_won_no 0

set draw 0
set draw_no 0
set draw_count 0
set draw_move ""
set temp_D ""
set w_draw_no 0

set loss 0
set loss_no 0
set loss_count 0
set loss_move ""
set temp_L ""
set w_loss_no 0
	
set unknown_no 0

foreach move $result_stat {
		switch -regexp  $move {
			
		[W]		{set won_no [expr {$won_no + 1}]
				set temp_W $move
				set win_move $move
				}		
		[D]		{set draw_no [expr {$draw_no + 1}]
				set temp_D $move
				set draw_move $move
				}
		[L]		{set loss_no [expr {$loss_no + 1}] 
				set temp_L $move 
				set loss_move $move
				}		
} ;# end of switch
	
if {[string first "value" $win_move] != -1 || [string first ":" $win_move] == -1 && $win_move != ""} {
set won_no [expr {$won_no - 1}]
}
if {$won_no != 0 && [string first "value" $win_move] == -1} { 
	set win 1
	set w_won_no $won_no		
	set win_count [expr {$win_count + 1}]
	if { [string first ":" $temp_W] != -1 } {
		set number_moves [string range $temp_W [expr {[string last " " $temp_W] + 1}] [string length $temp_W] ]
		}
} ;# end of Win info
	
	if { $draw_no != 0 || [string first ":" $draw_move] == -1 && $draw_move != ""} {
	set draw 1
	set w_draw_no $draw_no
	if { [string first ":" $temp_D] == -1 } {
		set w_draw_no [expr {$w_draw_no - 1}]
		set draw_no [expr {$draw_no - 1}]
		set temp_D ""
		set draw_no 0
		}
	
	} ;# end of Draw info
	
	if { $loss_no != 0 || [string first ":" $loss_move] == -1 && $loss_move != ""} {
	set w_loss 1
	set w_loss_no $loss_no
	if { [string first ":" $temp_L] == -1 } {
		set w_loss_no [expr {$w_loss_no - 1}]
		set loss_no [expr {$loss_no - 1}]
		set temp_L ""
		set loss_no 0
	}
	if { [string first ":" $temp_L] != -1 } {
		set number_moves [string range $temp_L [expr {[string last " " $temp_L] + 1}] [string length $temp_L] ]		
	}
} ;# end of Loss info

		# end of Move info 

} ;# end of result_stat loop for statistics compilation

# Stastics Output:
if {$not_found == 0} {
if {$w_won_no > 0} {
	$t insert end "Won   $w_won_no\n"
	}
if {$w_draw_no > 0} {
	$t insert end "Drawn $w_draw_no\n"
	}
if {$w_loss_no > 0} {
	$t insert end "Loss  $w_loss_no\n"
	}
}
if {$not_found == 1} {
if {$white_count == 5 && $black_count ==1 || $white_count == 1 && $black_count ==5} {
	$t insert end "Online: Result not found\nInvalid FEN for Shredder\nWhite pieces $white_count Black pieces $black_count\n"
	} else {
	$t insert end "Online: Result not found\n"
	}
}
########## Move Processing #################
#

set move_summary ""
set move_header ""
set move_sign ""
set number_moves 0
set prev_number_moves 0

set win 0
set won_no 0
set win_count 0
set win_move ""
set temp_W ""

set draw 0
set draw_no 0
set draw_count 0
set draw_move ""
set temp_D ""

set loss 0
set loss_no 0
set loss_count 0
set loss_move ""
set temp_L ""

set unknown_no 0

foreach move $result {
		switch -regexp  $move {
			
			[W]	{set win_move $move
				}
			[D]	{set draw_move $move
				}
			[L]	{set loss_move $move
				}
		} ;# end of switch

# Win
if {[string first "value" $win_move] == -1 && [string first ":" $win_move] != -1} { 		
set win 1
set won_no [expr {$won_no + 1}]
set win_count [expr {$win_count + 1}]
set move_sign "+"
set move_header "Winning moves"
# Deal with pawn promotion moves in the unhelpful format with two dashes: NN-NN-P,
# where P is the piece promotion: 9 = Q, 8 = R, 10 = B, 11 = N. 	
	if {[string first "-" [string range $win_move 0 2]] != -1 && 
		[string first "-" [string range $win_move  3 99]] != -1 } { 
		set promotion_flag 1
		set move_numbers_W \
		[string range $win_move 0 [ expr {[string first "-" [string range $win_move 3 99]] + 2} ] ]
		} else {
		set move_numbers_W \
			[string range $win_move  \
			0 [expr {[string first ":" $win_move] - 1}] ] 	
			}			
set move_from \
	[string range $move_numbers_W  \
	0 [expr {[string first "-" $move_numbers_W] - 1}] ] 
set move_to \
	[string range $move_numbers_W  \
	[expr {[string first "-" $move_numbers_W] + 1}] \
	[expr {[string first "-" $move_numbers_W] + 2}] ] 
set move_current [concat $move_from $move_to]
set number_moves	\
	[string range $win_move  \
	[expr {[string first ":" $win_move] + 9}] \
	[expr {[string length $win_move] - 1} ] ]
			} ;# end of Win info

# Draw
if {[string first "value" $draw_move] == -1 && [string first ":" $draw_move] != -1} {
	set draw 1
	set draw_no [expr {$draw_no + 1}]
	set draw_count [expr {$draw_count + 1}]
	set move_sign "="
	set move_header "Drawing moves"
# Deal with pawn promotion moves in the unhelpful format with two dashes: NN-NN-P,
# where P is the piece promotion: 9 = Q, 8 = R, 10 = B, 11 = N. 	
	if {[string first "-" [string range $draw_move 0 2]] != -1 && 
		[string first "-" [string range $draw_move  3 99]] != -1 } { 
		set promotion_flag 1
		set move_numbers_D \
		[string range $draw_move 0 [ expr {[string first "-" [string range $draw_move 3 99]] + 2} ] ]
		} else {
		set move_numbers_D \
			[string range $draw_move  \
			0 [expr {[string first ":" $draw_move] - 1}] ] 	
				}	
	set move_from \
		[string range $move_numbers_D  \
		0 [expr {[string first "-" $move_numbers_D] - 1}] ] 
	set move_to \
		[string range $move_numbers_D  \
		[expr {[string first "-" $move_numbers_D] + 1}] \
		[expr {[string first "-" $move_numbers_D] + 2}] ] 
	set move_current [concat $move_from $move_to]
	set number_moves "?"

} ;# end of Draw info 

# Loss	
	if { [string first "value" $loss_move] == -1 && [string first ":" $loss_move] != -1} {
	set loss 1
	set loss_no [expr {$loss_no + 1}]
	set loss_count [expr {$loss_count + 1}]
	set move_sign "-"
	set move_header "Losing moves"	
	# Deal with pawn promotion moves in the unhelpful format with two dashes: NN-NN-P,
	# where P is the piece promotion: 9 = Q, 8 = R, 10 = B, 11 = N. 	
	if {[string first "-" [string range $loss_move 0 2]] != -1 && 
		[string first "-" [string range $loss_move  3 99]] != -1 } { 
		set promotion_flag 1
		set move_numbers_L \
		[string range $loss_move 0 [ expr {[string first "-" [string range $loss_move 3 99]] + 2} ] ]
	} else {
		set move_numbers_L \
		[string range $loss_move  \
		0 [expr {[string first ":" $loss_move] - 1}] ] 	
		}
	set move_from \
		[string range $move_numbers_L  \
		0 [expr {[string first "-" $move_numbers_L] - 1}] ] 
	set move_to \
	[string range $move_numbers_L  \
		[expr {[string first "-" $move_numbers_L] + 1}] \
		[expr {[string first "-" $move_numbers_L] + 2}] ] 
	set move_current [concat $move_from $move_to]
	set number_moves	\
		[string range $loss_move  \
		[expr {[string first ":" $loss_move] + 10}] \
		[expr {[string length $loss_move] - 1} ] ]
	} ;# end of Loss info


###############################
set san_1 ""
set san_2 ""

# Deal with pawn promotion moves, which are in the unhelpful format with two dashes: NN-NN-P,
# where P is the piece promotion: 8 = Q, 9 = R, 10 = B, 11 = N. 	
if {$turn == "w"} {
	set promos [list 8:Q 9:R 10:B 11:N]
	} else {
	set promos [list 8:q 9:r 10:b 11:n]
	}
if {[string first "-" [string range $move 0 2]] != -1 && 
	[string first "-" [string range $move  3 99]] != -1 } { 
	set promotion_flag 1
	set promo_code [string range $move [expr {[string last "-" $move] +1}] [expr {[string first ":" $move] - 1}] ]
	if {$promo_code < 10} {
			set promo_piece [string index $promos [expr {[string first $promo_code $promos] + 2}] ] 
			} else {
			set promo_piece [string index $promos [expr {[string first $promo_code $promos] + 3}] ] 
			}
	set move_numbers \
		"[string range $move 0 [expr {[string first "-" [string range $move 3 99]] + 2}]]=$promo_piece"  \
	} else {
		set move_numbers \
		[string range $move  \
		0 [expr {[string first ":" $move] - 1}] ] 
	}

# Separate square numbers into "from" and "to"
set move_from \
	[string range $move_numbers  \
	0 [expr {[string first "-" $move_numbers] -1}] ] 
if {[string first "=" $move_numbers] == -1} {
	set move_to \
		[string range $move_numbers \
		[expr {[string first "-" $move_numbers] + 1}] \
		[expr {[string first "-" $move_numbers] + 2}] ] 
	} else {
		set move_to \
		[string range $move_numbers \
		[expr {[string first "-" $move_numbers] + 1}] \
		[expr {[string first "=" $move_numbers] +1}] ] 
		}
			
### SAN Move Conversion from square numbers to SAN notation:

# Move from
if {$move_from < 10} {
set temp_1 "0$move_from:"
if {[string first $temp_1 $piece_list] != -1} {
		set san_1 [string range $piece_list \
			[expr {[string first $temp_1 $piece_list] + 3}] \
			[expr {[string first $temp_1 $piece_list] + 3}] ]
	}
} else {
set temp_2 "$move_from:"
if {[string first $temp_2 $piece_list] != -1} {
		set san_1 [string range $piece_list \
			[expr {[string first $temp_2 $piece_list] + 3}] \
			[expr {[string first $temp_2 $piece_list] + 3}]	]	
	}
} 

if {$san_1 == "p" | $san_1 == "P"} {
set san_1 ""
}

# Move to
set temp ""
set temp_1 ""
set temp_p ""

if {[string first "=" $move_to] == -1} {
    set san_2 [lindex $::board::squareIndex $move_to]
} else {
    # Promotion
    set temp [string range $move_to 0 [expr {[string first "=" $move_to] -1}] ]
    set temp_p [string range $move_to [string first "=" $move_to] [expr {[string first "=" $move_to] +1}] ]
    set san_2 "[lindex $::board::squareIndex $temp]$temp_p"
}

set san_move "$san_1$san_2"		

### end of SAN conversion ###

# Add the SAN to the move:
set move_san "$move  ,$san_move,"

set san [string range $move_san [expr {[string first "," $move_san] +1}] [expr {[string last "," $move_san] -1}] ]
# Get the move from square number:
set move_from [string range $move_san 0 [expr {[string first "-" $move_san] -1}] ] 
set move_from_square [lindex $::board::squareIndex $move_from]

## Next few lines are connected with making move on board to see if there is check
# Change normal SAN format for pawn move back to P or p
set piece [string index $move_san [expr {[string first "," $move_san] +1}] ]
if {[string length $san] == 2} {
set temp_move [ concat [string range $move_san [expr {[string first "," $move_san] +1}] [expr {[string last "," $move_san] -1}] ] ] 
	if {[string first $piece "a b c d e f g h"] != -1 } {
		if {$turn == "w"} {
		set piece "P"
		} else {
		set piece "p"
		}
	}
}
if {$move_from < 10} {
set square_man [concat 0$move_from:$piece]
} else {
set square_man [concat $move_from:$piece]
}
##

# Show captures by inserting the "x" in the SAN
# Captures excluding pawns:
set temp_01 ""
set move_to [string range $move_san [expr {[string first "-" $move_san] +1}]  [expr {[string first ":" $move_san] - 1}] ]
if {$move_to < 10} {
	set move_to "0$move_to"
	}

# Look to see if piece at move to square:
set temp_01 [string index $piece_list [expr {[string first $move_to $piece_list] + 3}] ]
# Make SAN with capture "x"
if {$turn == "w" && [string length $san] > 2  && $temp_01 != ""} {
	if {[string first $move_to $piece_list] != -1 &&  [string first $temp_01 "qrbnp"] != -1} {
		set san "[string index $san 0]x[string index $san 1][string index $san 2]"
		}
}
if {$turn == "b" && [string length $san] > 2 && $temp_01 != ""} {
	if {[string first $move_to $piece_list] != -1 &&  [string first $temp_01 "QRBNP"] != -1} {
		set san [concat [string index $san 0]x[string index $san 1][string index $san 2]]
		}
}
set temp_01 ""

#
# Captures with pawns (so file letter changes, e.g. exd6):
if {[string length $san] <= 2 && [string index $san 0] != [string index $move_from_square 0] } {
		set san [regsub -all { } "[string index $move_from_square 0] x $san" ""]
	}

# Change piece case to upper if not a pawn
if {[string first [string index $san 0] "KQRBNkqrbn"] != -1 && [string first "=" $san] == -1} {
set san [concat [string toupper [string index $san 0]][string range $san 1 [string length $san]]]
	}

# Put pieces, not pawns, in upper case:
# Pawn promotion
if {[string first "=" $san] != -1} {
	set san "[string range $san 0 2][string toupper [string index $san 3]]"
	}
# Non-pawn
if {[string first "=" $san] == -1 && [string length $san] > 2} {
	set san "[string toupper [string index $san 0]][string range $san 1 3]"
	}
# Pawn	 (not capture)
if {[string first "=" $san] == -1 && [string length $san] == 2} {
	set san "[string tolower [string index $san 0]][string range $san 1 1]"
	}	
# Pawn	 (capture)
if {[string first "=" $san] == -1 && [string length $san] > 3 && [string first "P" $square_man] != -1 || [string first "p" $square_man] != -1} {
	set san "[string tolower [string index $san 0]][string range $san 1 3]"
	}

# If ambiguity of move, put the move_from_square rank or file into the SAN:
# If there are more than two identical pieces moving to three different squares, the user should use Lichess!
# NB exclude pawn and king moves 

set temp ""
set found ""
set temp_piece_1 ""
set temp_piece_2 ""
set san_man ""
set san_square_1 ""
set san_square_2 ""
set temp_san ""
set temp_rank ""
set temp_file ""
set temp_file_1 "" 
set temp_rank_1 ""
set temp_san_1 ""
set temp_file_2 "" 
set temp_rank_2 ""
set temp_san_2 ""

# Loop the result to look for moves with the same move to square as the current move
foreach move_a $result {
	switch -regexp $move_a {
		[:] 	{
			set piece_2_square [string range $move_a 0 [expr {[string first "-" $move_a] - 1}] ]
			set piece_2_move_to [string range $move_a [expr {[string first "-" $move_a] + 1}] [expr {[string first ":" $move_a] - 1}] ]
			}
		}

if {$piece_2_square < 10} {
set piece_2_square_a "0$piece_2_square"
} else {
set piece_2_square_a "$piece_2_square"
}
set piece_2 [string index $piece_list [expr {[string first $piece_2_square_a $piece_list] + 3}] ]
#
if {$turn == "b"} {
	set san_man [string tolower [string index $san 0]]
	} else {
	set san_man [string index $san 0]
	}

set piece_2_square_man \
[string range $piece_list [string first $piece_2_square_a $piece_list] [expr {[string first $piece_2_square_a $piece_list] +3}] ]

# If the same piece type from the piece list loop, not a pawn and not the piece of the current move:
if {[string length $san] > 2 && $san_man == $piece_2 \
	&& [string first [string index $san 0] "a b c d e f g h"] == -1  && $square_man != $piece_2_square_man \
	&& $san_man != "k" && $san_man != "K" && $piece_2 != "k" && $piece_2 != "K"} {
# 
# Current move piece - move from square:
set san_square_1 $move_from_square
#For loop piece, set move from square:
set san_square_2 [lindex $::board::squareIndex $piece_2_square]
#
set temp_file_1 [string index $san_square_1 0] 
set temp_rank_1 [string index $san_square_1 1]
set temp_san_1 "$san_man$san_square_1"
set temp_file_2 [string index $san_square_2 0]  
set temp_rank_2 [string index $san_square_2 1]
set temp_san_2 "$piece_2$san_square_2"
#
# Compare rank/file of the move from square of the two pieces and 
# insert rank or file into SAN of current move, if move to square is identical:

if {$move_to == $piece_2_move_to} {

# 	Pieces on the same file:	
if {$temp_file_1 == $temp_file_2 } {
	set san "[string index $san 0]$temp_rank_1[string index $san 1][string index $san 2][string index $san 3]" 
	} 		

# 	Pieces on the same rank:		
if {$temp_rank_1 == $temp_rank_2} {
	set san "[string index $san 0]$temp_file_1[string index $san 1][string index $san 2][string index $san 3]" 
	}
# 	Pieces on dissimilar file and rank:	
if {$temp_file_1 != $temp_file_2 && $temp_rank_1 != $temp_rank_2 } {
	set san "[string index $san 0]$temp_file_1[string index $san 1][string index $san 2][string index $san 3]" 
	}
	
} ;# end of set san with file letter or rank number		

} ;# end of if same piece type etc 	

} ;# end of foreach loop of result list

# end of SAN revision

## Find out if move checks the opponent
# Produce Board list in the same order as a FEN
# and then update it with the move
set flag 0
set board ""
set square_count 0
set board_nos {56 57 58 59 60 61 62 63 48 49 50 51 52 53 54 55 40 41 42 43 44 45 46 47 32 33 34 35 36 37 38 39 24 25 26 27 28 29 30 31 16 17 18 19 20 21 22 23 08 09 10 11 12 13 14 15 00 01 02 03 04 05 06 07}
set board_nos [regsub -all { } $board_nos ","] 
set board_nos [split $board_nos {,}]

foreach item $board_nos {
	switch -regexp $item {
		{[0-9,0-9]} 	{set square $item}
	}
	foreach ele $piece_list {
		switch -glob $ele {
			{[0-9,0-9]*} 	{set square_1 $ele}
			}
	if {[string range $square_1 0 1] == $square} {
		set board [concat $board $square_1]
		set flag 1
		}
	} ;# end of foreach piece list
if {$flag == 0} {
	set board [concat $board $item:x]
	}
set flag 0	
} ;# end of foreach board nos

# Update the board with the move and convert board back to FEN (3 stages)
# 			use sc_pos function in the scid.gui (not available here)
# Make move on board:
# First, make exit square empty
set fen_6 ""
set counter -1
foreach square_on $board {
	switch -regexp $square_on {
		{[00-99]*}			{set board_sq $square_on
						set counter [expr { $counter + 1}]
						set square_no [string range $square_on 0 1]
						}
	}

if {$square_man == $square_on } {
		lset board $counter [concat $square_no:x] 
	}
 } ;# end of foreach loop

# Secondly, put piece on destination square
set counter -1
set temp_1 ""
set temp_2 ""
set square_man ""
foreach square_on $board {
	switch -regexp $square_on {
			{[00-99]*}			{set board_sq $square_on
							set counter [expr { $counter + 1}]
							set square_no [string range $square_on 0 1]
							}
			}

	set temp_1 [string range $move_to 0 1]

	# if promotion found, get the piece and promotion square:
	if {[string first "=" $move_san] != -1} {
		set piece [string index $move_san [expr {[string first "=" $move_san] + 1}] ]
		set temp_1 [string range $move_to 0 [expr {[string first "-" $move_to] - 1}] ]
		if {$temp_1 < 10} {
			set $temp_1 "0$temp_1"
			}
		}

	if {$temp_1 == $square_no } {
		lset board $counter [concat $square_no:$piece] 
		}

set temp_1 ""
set temp_2 ""
	
} ;# end of foreach loop

# Thirdly, convert board back to a FEN
set fen_6 ""
set gap_counter 0
set rank_count 0
set row_counter 0
foreach square_piece $board {
		switch -regexp {
					{[00-99]*}  {set square_1 $square_piece}
		}
	if {[string index $square_piece 3] == "x"} {
		set gap_counter [expr { $gap_counter + 1}]
		set row_counter [expr { $row_counter + 1}]
		}
	if 	{[string index $square_piece 3] != "x"} {
		if {$gap_counter > 0} { 
			set fen_6 [concat $fen_6$gap_counter[string index $square_piece 3] ]
			} else {
			set fen_6 [concat $fen_6[string index $square_piece 3] ]
			}
		set row_counter [expr { $row_counter + 1}]
		set gap_counter 0
		}
	if {$row_counter == 8 } {
		if {$gap_counter > 0} {
			set fen_6 [concat $fen_6$gap_counter/ ]
			} else {
			set fen_6 [concat $fen_6/]
			}
		set gap_counter 0
		set rank_count [expr { $rank_count + 1}]
		set row_counter 0
		}	
	if {$rank_count == 8 && [string range $square_piece 0 1] != "07"} {
		set fen_6 [concat $fen_6$gap_counter ]
		set gap_counter 0
		set rank_count 0
		set row_counter 0
		}
	
	} ;# end of board loop

if {[string first ":" $move] == -1 } {
set fen_6 ""
}

if {$turn == "w"} {
	set turn_next "b"
	} else {
	set turn_next "w"
	}
	
if {$fen_6 != ""} {
set fen_6 [concat [string range $fen_6 0 [expr {[string last  "/" $fen_6] - 1}] ] $turn_next " - -"] 
}

# Verifying the in check status; this code is based on Steve Austin's (!!! - S.A) suggestion:
set isCheck 0
if {$fen_6 != ""} { 
sc_game push
sc_game startBoard $fen_6
if {[sc_pos isCheck]} {
	if {[sc_pos moves] == {}} {
	set san "$san#"
	} else {
	set san "$san+"
	}
}
sc_game pop
}
##################################################################################

### Processing output for win/loss with 6 men and fewer:
#
if {$number_moves != "?" && $number_moves < 10} {
set number_moves_display " $number_moves"
} else {
set number_moves_display "$number_moves"
}

# Win
if {$win == 1 && $number_moves != "?" && $number_moves > 0 } {

if { $prev_number_moves == 0 } {
	$t insert end "\n$move_header\n"
	set move_summary " $move_sign  $number_moves_display  $san"
	}
if { $prev_number_moves != 0} {
	if {$number_moves == $prev_number_moves} {
		set move_summary "$move_summary $san"
		}
	if {$number_moves != $prev_number_moves} {
		$t insert end "$move_summary\n" indent
		set move_summary " $move_sign  $number_moves_display  $san"
		}
	}
set prev_number_moves $number_moves
set number_moves 0
}
		
## Draw always has DTM = 0, so treat as a special case :
# Draw output - White to move:
		if {$draw == 1 && $draw_count == 1 && $number_moves == "?"} {
	$t insert end "\n$move_header\n"
	set move_summary " $move_sign   $number_moves_display "
	}
if {$draw == 1 && $number_moves == "?"} {
	set move_summary "$move_summary $san"
	}
if {$draw == 1 && $draw_count == $w_draw_no} {
	$t insert end "$move_summary" indent
	set move_summary ""
	set prev_number_moves 0 
	set draw 0
	}

# Loss output:
if {$loss == 1 && $number_moves != "?" && $number_moves > 0 } {

if { $prev_number_moves == 0 } {
	$t insert end "\n$move_header\n" indent
	set move_summary " $move_sign  $number_moves_display  $san"
	}
if { $prev_number_moves != 0} {
	if {$number_moves == $prev_number_moves} {
		set move_summary "$move_summary $san"
		}
	if {$number_moves != $prev_number_moves} {
		$t insert end "$move_summary\n" indent 
		set move_summary " $move_sign  $number_moves_display  $san"
		}
	}
set prev_number_moves $number_moves
set number_moves 0		
}
		
## Display final move where moves exceed 1 :

if {$win == 1 && $win_count == $w_won_no && $win_count > 0
			&& $move_summary != "" && $number_moves != "?" } {
	$t insert end "$move_summary" indent
	set prev_number_moves 0
	}
if {$loss == 1 && $loss_count == $w_loss_no && $loss_count > 0
			&& $move_summary != "" && $number_moves != "?" } {
	$t insert end "$move_summary" indent
	set prev_number_moves 0
	break
	}
	
# end of move update

### Clean up after move processing ###

set win 0
set draw 0
set loss 0
set move_current ""
set move_sign ""
set move_header ""

} ;# end of move loop for result

} ;# end of Shredder Result

##############################################################################
# Process the data in the Lichess answer:

if {[string first "Lichess" $::tbOnline] != -1} {

## Construct a usable moves data list from "result":
# Get rid of square brackets and quotation marks from data returned by Lichess:
regsub  -all {[\[|\]]} $result "" answer_2
regsub  -all {[\"|\"]} $answer_2 "" answer_2

# Piece count is used in connection with results window:
set pieceCount [sc_pos pieceCount]

# Get string excluding best move data:
set x [string first "uci" $answer_2 1]
set moves_2 [string range $answer_2 [expr {$x - 1}] [expr {[string length $answer_2] - 2}]]
#
set moves_2 [string map {, { }} $moves_2]

#################################################	
# Count won, cursed win, drawn, blessed loss, loss, :
#
# NB Lichess result is shown from the view of the opposing colour, so this has been inverted in order
# to conform to the ScidvsPC convention of showing the result from the perspective of the colour to move.
#
set won_no 0
set drawn 0
set cursed_win_no 0
set blessed_loss_no 0
set loss_no 0
foreach move $moves_2 {
	foreach move_element $move {
		switch -glob $move_element {
			{category:loss} 		{set won_no [expr {$won_no + 1}]}
			{category:blessed-loss} 	{set cursed_win_no [expr {$cursed_win_no + 1}]}
			{category:draw}		{set drawn  [expr {$drawn + 1}]}
			{category:cursed-win}  	{set blessed_loss_no [expr {$blessed_loss_no + 1}]}
			{category:win} 		{set loss_no  [expr {$loss_no + 1}]}
		}
	}
}
# $t insert end "Lichess\n"
if {$won_no > 0} {
	$t insert end "Won   $won_no\n"
	}
if {$cursed_win_no > 0} {
	$t insert end "Cursed Win $cursed_win_no\n"
	}
if {$drawn > 0} {
	$t insert end "Drawn $drawn\n"
	}
if {$blessed_loss_no > 0} {
	$t insert end "Blessed Loss $blessed_loss_no\n"
	}
if {$loss_no > 0} {
	$t insert end "Loss  $loss_no\n"
	}

if {$pieceCount > 5} {
$t insert end "Syzygy DTZ: 6/7 men\n"
} else {
$t insert end "DTM: < 6 men\n"
}	
###############################################################
#
# All Moves
#
namespace import ::tcl::mathfunc::*
#
set distance_to_zero 0
set dtz_temp 0
set number_moves 0
set number_moves_z 0
set prev_number_moves 0
set prev_distance_to_zero 0
set zero ""
set win 0
set win_count 0
set c_win 0
set c_win_count 0
set draw 0
set draw_count 0
set loss 0
set loss_count 0
set b_loss 0
set b_loss_count 0
set move_current ""
set move_summary ""
set move_header ""
set move_sign ""

# NB Lichess result is shown from the view of the opposing colour, so this has been inverted in order
# to conform to the ScidvsPC convention of showing the result from the perspective of the colour to move.

foreach move $moves_2 {
	foreach move_element $move {
	switch -glob $move_element {
	{category:loss}		{set win 1 ; set win_count [expr {$win_count + 1}] ;
						set move_header "Winning moves"
						set move_sign "+"}
	{category:blessed-loss}	{set c_win 1 ; set c_win_count [expr {$c_win_count + 1}] ;
						set move_header "Cursed Win moves"
						set move_sign "="}
	{category:draw}		{set draw 1 ; set draw_count [expr {$draw_count + 1}] ;
						set move_header "Drawing moves"
						set move_sign "="}
	{category:win}			{set loss 1 ; set loss_count [expr {$loss_count + 1}]
						set move_header "Losing moves"
						set move_sign "-"}
	{category:cursed-win}	{set b_loss 1 ; set b_loss_count [expr {$b_loss_count + 1}] ;
						set move_header "Blessed Loss moves"
						set move_sign "="}	
	{dtm:*}				{set number_moves \
						[string range $move_element [expr {[string first ":" $move_element] + 1}] \
						[string length $move_element] ] \
						}	
	{san:*} 				{set move_current \
						[string range $move_element [expr {[string first ":" $move_element] + 1}] \
						[string length $move_element] ] \
						}
	{dtz:*} 				{set distance_to_zero \
						[ expr { round ( double ( \
						[string range $move_element [expr {[string first ":" $move_element] + 1}] \
						[string length $move_element] ] ) / 2) + 0} ]
						}
	{zeroing:*} 			{set zero \
						[string range $move_element [expr {[string first ":" $move_element] + 1}] \
						[string length $move_element] ] \
						}
	{checkmate:*} 		{set mate \
						[string range $move_element [expr {[string first ":" $move_element] + 1}] \
						[string length $move_element] ] \
}
} ;# end of switch

	} ;# end of move element loop

### General data processing ###	

if {$mate == "true"} {
	set distance_to_zero 0
	}
	
# The Lichess move count does not match Nalimov and Shredder, 
# hence this fix - just round odd numbers/2 but add 1 to even numbers/2:
if {$number_moves != 0 && $number_moves != "null" } {
	if {[expr {[double [abs $number_moves]]/2}] > [expr {[abs $number_moves]/2}] } {
		set number_moves [expr {round([double [abs $number_moves]]/2)}]
		} else {
	set number_moves [expr {[abs $number_moves]/2 + 1}]
	}
}
	
if {$pieceCount > 5} {
	if {$number_moves != 0 && $number_moves != "null" } {
 	set number_moves_z	$number_moves
	}
	set number_moves "?"
	set distance_to_zero [expr {[abs $distance_to_zero]}]
	}
if {$distance_to_zero < 0} {
	set distance_to_zero [expr {$distance_to_zero / -1}]
	}
if {$number_moves == "null"} {
	set number_moves "?"
	}
if {$draw == 1} { set number_moves "?"
	}
if { $number_moves != "?" && $number_moves < 0} {
set number_moves [expr {$number_moves * -1 }]
	}

### Section 1 - Processing for 5 men and fewer where DTM is not "null" and not a draw:
	
if {$number_moves < 10} {
	set number_moves_display  " $number_moves"
	} else {
	set number_moves_display  "$number_moves"
	}
	
if {$draw != 1 && $number_moves != "?"} {

if { $prev_number_moves == 0 } {
	$t insert end "\n$move_header"
	set move_summary " $move_sign  $number_moves_display  $move_current "
	}
if { $prev_number_moves != 0} {
	if {$number_moves == $prev_number_moves} {
		set move_summary "$move_summary $move_current"
		}
	if {$number_moves != $prev_number_moves} {
		$t insert end "\n$move_summary" indent
		set move_summary " $move_sign  $number_moves_display  $move_current"
		}
	}
set prev_number_moves $number_moves
set number_moves 0		
}
		
# Display final move if moves exceed 1 :
		
if {$win == 1 && $win_count == $won_no 
		&& $move_summary != "" && $number_moves != "?" } {
	$t insert end "\n$move_summary" indent
	set prev_number_moves 0
	}
if {$c_win == 1 && $c_win_count  == $cursed_win_no
	&& $move_summary != "" && $number_moves != "?" } {
	$t insert end "\n$move_summary" indent
	set prev_number_moves 0
	}
if {$b_loss == 1 && $b_loss_count  == $blessed_loss_no
	&& $move_summary != "" && $number_moves != "?" } {
	$t insert end "\n$move_summary" indent
	set prev_number_moves 0
	}
if {$loss == 1 && $loss_count == $loss_no 
	&& $move_summary != "" && $number_moves != "?" } {
	$t insert end "\n$move_summary" indent
	set prev_number_moves 0
	}	

### Section 2 - Processing for 6 or 7 men

if {$distance_to_zero < 10} {
	set DTZ_display  " $distance_to_zero"
	} else {
	set DTZ_display  "$distance_to_zero"
	}

#This variable is necessary because the following process will not work 
#if a move has "zeroing" and so $distance_to_zero is 0.
set dtz_temp [expr {$distance_to_zero + 1}]

#If depth to mate is given, show it in braces:
if {$number_moves_z != 0} {
	set move_current "$move_current{M$number_moves_z}"
	}

if {$draw != 1 && $number_moves == "?"} {

if { $prev_distance_to_zero == 0 } {
	$t insert end "\n$move_header\n" indent
	set move_summary " $move_sign  $DTZ_display  $move_current"
	}
if { $prev_distance_to_zero != 0 } {
	if {$dtz_temp == $prev_distance_to_zero} {
		set move_summary "$move_summary $move_current"
		}
	if {$dtz_temp != $prev_distance_to_zero} {
		$t insert end "$move_summary\n" indent
		set move_summary " $move_sign  $DTZ_display  $move_current"
		}
	}

set prev_distance_to_zero $dtz_temp
set distance_to_zero 0
set dtz_temp 0
set number_moves_z 0
}
		
# Display final move if moves exceed 1 :
		
if {$win == 1 && $win_count == $won_no 
	&& $move_summary != "" && $number_moves == "?" } {
	$t insert end "$move_summary" indent
	set prev_distance_to_zero 0
	}
if {$c_win == 1 && $c_win_count == $cursed_win_no 
	&& $move_summary != "" && $number_moves == "?" } {
	$t insert end "$move_summary" indent
	set prev_distance_to_zero 0
	}
if {$b_loss == 1 && $b_loss_count == $blessed_loss_no 
	&& $move_summary != "" && $number_moves == "?" } {
	$t insert end "$move_summary" indent
	set prev_distance_to_zero 0
	}
if {$loss == 1 && $loss_count == $loss_no 
	&& $move_summary != "" && $number_moves == "?" } {
	$t insert end "$move_summary" indent
	set prev_distance_to_zero 0
	}

## Draw always has DTM = 0 and DTZ = 0, so treat as a special case : 

if {$draw == 1 && $draw_count == 1 } {
	$t insert end "\n$move_header\n"
	set move_summary " $move_sign   $number_moves  $move_current"
	}
if {$draw == 1 && $draw_count > 1} {
	set move_summary "$move_summary $move_current"
	}
if {$draw == 1 && $draw_count == $drawn} {
	$t insert end "$move_summary" indent
	set move_summary ""
	set number_moves 0
	set prev_number_moves 0
	}

# end of move update for the move element

# Clean up after move processing

set win 0
set c_win 0
set draw 0
set b_loss 0
set loss 0
set distance_to_zero 0
set move_current ""
set move_sign ""
set move_header ""
} ;# end of move loop

} ;# end of Lichess Result

#########################################################
	
    } ;# end of bookmark 1 section showing results in TB window

    $t configure -state disabled
	
  } ;# end of proc ::tb::showResult
  
} ;# end of if ::tb::online_available

############################################################################

# ::tb::random
#   Sets up a random position with the material of the tablebase
#   currently displayed in the info frame.
#
proc ::tb::random {} {
  global tbInfo
  if {[catch {sc_game startBoard "random:$tbInfo(material)"} err]} {
    tk_messageBox -title Scid -icon warning -type ok -message $err
    return
  }
  # The material is valid, so clear the game and regenerate a
  # random starting position:
  sc_game new
  sc_game startBoard "random:$tbInfo(material)"
  updateBoard -pgn
}

# ::tb::setFEN
#   Called when an item in the Tablebase info browser with an
#   associated FEN position is selected with the left mouse button,
#   causing the position to be set in the main window.

proc ::tb::setFEN {fen} {
  if {[catch {sc_game startBoard $fen} err]} {
    tk_messageBox -title Scid -icon info -type ok -message $err
    return
  }
  # The FEN is valid, so clear the game and reset the FEN:
  sc_game new
  sc_game startBoard $fen
  updateBoard -pgn
}

# ::tb::training
#   Toggle tablebase training mode.
#
proc ::tb::training {} {
  global tbTraining tbStatus gameInfo
  set w .tbWin
  set tbStatus ""
  if {$tbTraining} {
    set gameInfo(showTB_old) $gameInfo(showTB)
    set gameInfo(showTB) 0
  } else {
    if {$gameInfo(showTB) == 0} { set gameInfo(showTB) $gameInfo(showTB_old) }
  }
  updateBoard -pgn
  ::tb::results
}

# ::tb::move
#   Finds and executes the best move in the current position,
#   if one can be determined from the tablebases.

proc ::tb::move {} {
  global tbTraining tbStatus
  if {! $tbTraining} { return }
  set moves [split [sc_pos probe optimal]]
  set len [llength $moves]
  if {$len == 0} {
    set tbStatus "No optimal move was found."
    return
  }
  set i [expr int(rand() * $len)]
  set move [lindex $moves $i]
  if {[catch {sc_move addSan $move}]} {
    set tbStatus "Error playing $move."
  } else {
    set tbStatus "Played $move."
  }
  updateBoard -pgn
}


# tbs:
#   Summary data about tablebases.
#   Each list has the following elements:
#     (0) Frequency (per million games),
#     (1) Longest-wtm-mate length, (2) Longest-wtm-mate FEN,
#     (3) Longest-btm-mate length, (4) Longest-btm-mate FEN,
#     (5) wtm-win-%, (6) wtm-draw-%, (7) wtm-loss-%,
#     (8) btm-win-%, (9) btm-draw-%, (10) btm-loss-%,
#     (11) number of mutual zugzwangs (-1 if unknown).
#  The longest-mate FENs have a board field only; no side to move, etc.
#
#   There are three types of mutual zugzwang:
#     wtm draws / btm loses, wtm loses / btm draws, wtm loses / btm loses.
#   The first two are "half-point" zugzwangs, the last is "full-point".
#
#   If the number of mutual zugzwangs is known and nonzero,
#   six more items should follow in the list:
#     (12) number of wtm-draws-btm-loses zugzwangs,
#     (13) list of selected wtm-draws-btm-loses zugzwang FENs,
#     (14) number of wtm-loses-btm-draws zugzwangs,
#     (15) list of selected wtm-loses-btm-draws zugzwang FENs,
#     (16) number of whoever-moves-loses (full-point) zugzwangs,
#     (17) list of selected whoever-moves-loses zugzwang FENs.
#   These zugzwang FENs board field only; no side to move, etc.

set tbs(kqk) {
  257 10 {7K/6Q1/8/8/2k5/8/8/8} 0 -
  100.0 0.0 0.0 0.0 10.3 89.7
  0
}

set tbs(krk) {
  542 16 {8/8/2R5/3k4/8/8/8/1K6} 0 -
  100.0 0.0 0.0 0.0 9.9 90.1
  0
}

set tbs(kbk) {
  194 0 - 0 -
  0.0 100.0 0.0 0.0 100.0 0.0
  0
}

set tbs(knk) {
  224 0 - 0 -
  0.0 100.0 0.0 0.0 100.0 0.0
  0
}

set tbs(kpk) {
  2352 28 {8/8/8/1k6/8/8/K5P1/8} 0 -
  76.5 23.5 0.0 0.0 41.9 58.1
  80 80 {} 0 {} 0 {}
}

set tbs(kqkq) {
  222 13 {8/8/8/8/8/8/8/qk1K2Q1} 13 {8/8/8/8/8/8/8/QK1k2q1}
  41.7 57.8 0.5 41.7 57.8 0.5
  0
}

set tbs(kqkr) {
  400 35 {K3r3/8/5k2/Q7/8/8/8/8} 19 {k7/5r2/K7/8/8/8/1Q6/8}
  99.0 0.8 0.2 28.7 5.8 65.5
  0
}

set tbs(kqkb) {
  25 17 {K7/8/8/3k4/4b3/8/8/7Q} 0 -
  99.7 0.3 0.0 0.0 23.1 76.9
  0
}

set tbs(kqkn) {
  74 21 {8/KQ6/2n5/2k5/8/8/8/8} 0 -
  99.3 0.7 0.0 0.0 19.5 80.5
  0
}

set tbs(kqkp) {
  937 28 {3KQ3/8/8/8/8/8/3kp3/8} 29 {8/1p4k1/7Q/8/7K/8/8/8}
  99.4 0.6 0.0 7.7 12.1 80.2
  0
}

set tbs(krkr) {
  423 19 {8/3R4/8/8/5k2/6r1/7K/8} 19 {1k6/2R5/3K4/8/8/8/6r1/8}
  29.1 70.2 0.7 29.1 70.2 0.7
  0
}

set tbs(krkb) {
  322 29 {k7/8/b7/8/K7/R7/8/8} 0 -
  35.2 64.8 0.0 0.0 96.8 3.2
  5  5 {
    4R3/8/8/8/8/b1K5/8/3k4 8/5R2/7b/8/8/2K5/8/1k6 8/8/1b6/5R2/8/3K4/8/2k5
    8/8/8/8/8/1k6/b7/R1K5 8/8/8/8/8/2K5/4k3/R2b4
  } 0 {} 0 {}
}

set tbs(krkn) {
  397 40 {8/8/6R1/2K5/n7/8/8/3k4} 1 {8/8/8/8/1n6/k7/8/KR6}
  48.4 51.6 0.0 0.0 89.0 11.0
  18 18 {
    8/2n5/8/4R3/3K1k2/8/8/8 8/8/5k2/4R3/3K4/2n5/8/8 8/8/8/1k6/2R5/3K4/4n3/8
    8/8/8/2n5/3K4/4R3/5k2/8 8/8/8/3k4/2R5/3K4/n7/8 8/8/8/3k4/4R3/3K4/6n1/8
    8/8/8/4k3/3R4/2K5/1n6/8 8/8/8/5k2/4R3/3K4/2n5/8 8/8/8/6n1/3K4/4R3/3k4/8
    8/8/8/8/2R5/1k1K4/4n3/8 8/8/8/8/3K1k2/4R3/8/2n5 8/8/8/8/3R4/2K1k3/1n6/8
    8/8/8/8/4R3/3K1k2/2n5/8 8/8/8/8/6n1/3K4/4R3/3k4 8/8/8/8/8/2KR4/8/2k2n2
    8/8/8/8/8/2RK4/8/n2k4 8/8/8/8/8/3KR3/8/3k2n1 8/8/8/n7/3K4/2R5/3k4/8
  } 0 {} 0 {}
}

set tbs(krkp) {
  2146 26 {2K5/8/7p/6k1/8/8/R7/8} 43 {8/8/8/8/5R2/2pk4/5K2/8}
  91.4 8.4 0.2 16.4 17.5 66.1
  12 12 {
    8/8/8/8/8/1k6/p7/R1K5   8/8/8/8/8/2k5/1p6/1R1K4 8/8/8/8/8/4k3/5p2/3K1R2
    8/3K4/8/3k4/3p4/8/8/3R4 8/1K6/8/1k6/1p6/8/8/1R6 8/2K5/8/2k5/2p5/8/8/2R5
    8/2K5/8/2k5/3p4/8/8/3R4 8/3K4/8/3k4/4p3/8/8/4R3 8/1K6/8/1k6/2p5/8/8/2R5
    8/2K5/8/2k5/1p6/8/8/1R6 8/3K4/8/3k4/2p5/8/8/2R5 8/K7/8/k7/1p6/8/8/1R6
  } 0 {} 0 {}
}

set tbs(kbkb) {
  49 1 {8/8/8/8/8/K7/7B/kb6} 1 {6BK/8/6k1/8/8/b7/8/8}
  0.0 100.0 0.0 0.0 100.0 0.0
  0
}

set tbs(kbkn) {
  87 1 {knB5/8/1K6/8/8/8/8/8} 1 {K1k1n3/B7/8/8/8/8/8/8}
  0.0 100.0 0.0 0.0 100.0 0.0
  0
}

set tbs(kbkp) {
  387 1 {7k/7p/5K2/8/8/8/1B6/8} 29 {8/1p4k1/7B/8/8/7K/8/8}
  0.0 94.8 5.2 23.6 76.4 0.0
  1 0 {} 1 {8/8/8/8/8/8/1pK5/kB6} 0 {}
}

set tbs(knkn) {
  68 1 {k7/n1K5/8/3N4/8/8/8/8} 1 {8/8/8/8/1n6/1k6/8/KN6}
  0.0 100.0 0.0 0.0 100.0 0.0
  0
}

set tbs(knkp) {
  497 7 {8/8/8/8/pN6/8/2K5/k7} 29 {8/1p6/6kN/8/8/7K/8/8}
  0.0 87.1 12.9 32.6 67.4 0.0
  29 22 {} 7 {} 0 {}
}

set tbs(kpkp) {
  2810 33 {2K5/k7/7p/8/8/8/6P1/8} 33 {8/2p1K3/8/8/8/4P3/8/3k4}
  43.4 33.3 23.2 43.4 33.3 23.2
  121 106 {} 106 {} 15 {
    8/8/8/1Kp5/2Pk4/8/8/8 8/8/8/2Kp4/3Pk3/8/8/8 8/8/8/8/1Kp5/2Pk4/8/8
    8/8/8/8/1pK5/kP6/8/8  8/8/8/8/2Kp4/3Pk3/8/8 8/8/8/8/2pK4/1kP5/8/8
    8/8/8/8/3Kp3/4Pk2/8/8 8/8/8/8/8/1Kp5/2Pk4/8 8/8/8/8/8/1pK5/kP6/8
    8/8/8/8/8/2Kp4/3Pk3/8 8/8/8/8/8/2pK4/1kP5/8 8/8/8/8/8/3Kp3/4Pk2/8
    8/8/8/8/8/Kp6/1Pk5/8  8/8/8/8/Kp6/1Pk5/8/8  8/8/8/Kp6/1Pk5/8/8/8
  }
}

set tbs(kqqk) {
  13 4 {8/8/8/4k3/8/8/1K6/QQ6} 0 -
  100.0 0.0 0.0 0.0 2.1 97.9
  0
}

set tbs(kqrk) {
  18 6 {7Q/8/8/8/4k3/8/8/1R5K} 0 -
  100.0 0.0 0.0 0.0 1.1 98.9
  0
}

set tbs(kqbk) {
  36 8 {8/Q4B2/5k2/8/8/8/8/K7} 0 -
  100.0 0.0 0.0 0.0 9.4 90.6
  0
}

set tbs(kqnk) {
  41 9 {K7/N7/8/8/8/5k2/Q7/8} 0 -
  100.0 0.0 0.0 0.0 9.7 90.3
  0
}

set tbs(kqpk) {
  156 10 {8/8/8/2k5/8/8/4P1Q1/7K} 0 -
  100.0 0.0 0.0 0.0 2.8 97.2
  0
}

set tbs(krrk) {
  8 7 {4R3/3k4/8/8/5R1K/8/8/8} 0 -
  100.0 0.0 0.0 0.0 0.3 99.7
  0
}

set tbs(krbk) {
  46 16 {8/8/3R4/4k3/4B3/8/8/K7} 0 -
  100.0 0.0 0.0 0.0 8.8 91.2
  0
}

set tbs(krnk) {
  15 16 {K7/2R5/3k4/3N4/8/8/8/8} 0 -
  100.0 0.0 0.0 0.0 9.2 90.8
  0
}

set tbs(krpk) {
  333 16 {K7/8/3R4/4kP2/8/8/8/8} 0 -
  100.0 0.0 0.0 0.0 2.5 97.5
  0
}

set tbs(kbbk) {
  31 19 {K7/8/3B4/3k4/8/8/4B3/8} 0 -
  49.3 50.7 0.0 0.0 58.8 41.2
  0
}

set tbs(kbnk) {
  206 33 {7K/4B3/4k3/8/8/8/8/2N5} 0 -
  99.5 0.5 0.0 0.0 18.1 81.9
  0
}

set tbs(kbpk) {
  453 31 {8/3P4/KBk5/8/8/8/8/8} 0 -
  96.0 4.0 0.0 0.0 16.8 83.2
  6 6 {
    1B1K4/8/8/k7/8/P7/8/8 1B6/3K4/8/1k6/8/P7/8/8 1BK5/8/1k6/8/8/P7/8/8
    8/B1k5/K7/P7/8/8/8/8 kB6/8/1PK5/8/8/8/8/8 kB6/8/KP6/8/8/8/8/8
  } 0 {} 0 {}
}

set tbs(knnk) {
  20 1 {k7/3N4/K1N5/8/8/8/8/8} 0 -
  0.0 100.0 0.0 0.0 100.0 0.0
  0
}

set tbs(knpk) {
  426 27 {1N6/8/8/8/8/2k3P1/8/2K5} 0 -
  96.3 3.7 0.0 0.0 18.5 81.5
  75 75 {} 0 {} 0 {}
}

set tbs(kppk) {
  563 32 {8/8/8/8/2k5/6P1/K5P1/8} 0 -
  98.4 1.6 0.0 0.0 7.9 92.1
  43 43 {} 0 {} 0 {}
}

set tbs(kqqkq) {
  51 30 {2K5/8/1k6/5q2/8/8/6Q1/7Q} 13 {7Q/7K/8/6Qk/8/8/7q/8}
  99.1 0.8 0.1 0.6 32.8 66.6
  0
}

set tbs(kqqkr) {
  0 35 {Kr6/8/8/8/8/3Q4/4Q3/2k5} 19 {6Q1/8/8/8/8/7K/2r4Q/7k}
  100.0 0.0 0.0 0.1 0.2 99.7
  0
}

set tbs(kqqkb) {
  0 15 {8/8/7Q/5k1K/7Q/5b2/8/8} 0 -
  100.0 0.0 0.0 0.0 0.1 99.9
  0
}

set tbs(kqqkn) {
  0 19 {5K2/3n4/4k3/2Q5/8/8/8/1Q6} 0 -
  100.0 0.0 0.0 0.0 0.1 99.9
  0
}

set tbs(kqqkp) {
  7 22 {8/8/8/3Q4/7Q/2k5/1p6/K7} 13 ?
  100.0 0.0 0.0 0.0 0.7 99.3
  0
}

set tbs(kqrkq) {
  36 67 {8/8/8/8/q7/6k1/8/KR5Q} 38 {8/8/q7/8/8/6R1/2K4Q/k7}
  97.0 2.8 0.2 24.4 21.2 54.4
  1 1 {8/8/8/8/1R6/k4q2/8/1K2Q3} 0 {} 0 {}
}

set tbs(kqrkr) {
  132 34 {1K2Q3/8/3k4/1r2R3/8/8/8/8} 20 {6rQ/8/8/8/8/7K/5R2/6k1}
  99.8 0.1 0.0 0.3 17.1 82.1
  0
}

set tbs(kqrkb) {
  12 29 {2k5/5b2/8/8/2K5/8/Q7/6R1} 0 -
  100.0 0.0 0.0 0.0 11.6 88.4
  0
}

set tbs(kqrkn) {
  2 40 ? 1 {8/8/8/8/1n6/k7/8/KR5Q}
  99.9 0.1 0.0 0.0 7.7 92.3
  0
}

set tbs(kqrkp) {
  25 40 ? 43 ?
  100.0 0.0 0.0 0.3 1.4 98.3
  0
}

set tbs(kqbkq) {
  28 33 {5q2/8/8/5B2/k1K4Q/8/8/8} 24 {6KQ/8/1B6/6k1/8/6q1/8/8}
  55.7 44.0 0.3 30.5 62.3 7.2
  25 25 {} 0 {} 0 {}
}

set tbs(kqbkr) {
  21 40 ? 30 ?
  99.3 0.6 0.0 0.7 27.5 71.8
  0
}

set tbs(kqbkb) {
  2 17 ? 2 ?
  99.7 0.3 0.0 0.0 19.8 80.2
  0
}

set tbs(kqbkn) {
  2 21 ? 1 ?
  99.5 0.5 0.0 0.0 16.7 83.3
  0
}

set tbs(kqbkp) {
  25 32 ? 24 ?
  100.0 0.0 0.0 1.0 14.1 84.9
  0
}

set tbs(kqnkq) {
  74 41 {8/7q/8/k7/2K5/2N5/8/4Q3} 24 {7K/8/1N6/Q5k1/8/8/6q1/8}
  50.1 49.6 0.3 33.5 62.2 4.3
  38 38 {} 0 {} 0 {}
}

set tbs(kqnkr) {
  12 38 ? 41 ?
  99.2 0.7 0.0 3.0 27.2 69.8
  0
}

set tbs(kqnkb) {
  7 17 ? 1 ?
  99.8 0.2 0.0 0.0 20.9 79.1
  0
}

set tbs(kqnkn) {
  13 21 ? 1 ?
  99.4 0.6 0.0 0.0 17.8 82.2
  0
}

set tbs(kqnkp) {
  46 30 ? 29 ?
  99.9 0.1 0.0 1.9 15.0 83.1
  0
}

set tbs(kqpkq) {
  1179 124 {4q3/K7/8/8/8/4P3/6Q1/k7} 29 {8/7q/3PK3/8/8/8/Q7/3k4}
  68.4 31.2 0.4 35.2 51.2 13.6
  640 640 {} 0 {} 0 {}
}

set tbs(kqpkr) {
  216 38 ? 33 ?
  99.6 0.3 0.1 19.7 6.1 74.1
  1 1 {k7/8/KQ1r4/P7/8/8/8/8} 0 {} 0 {}
}

set tbs(kqpkb) {
  16 28 ? 2 ?
  99.9 0.1 0.0 0.0 16.7 83.3
  0
}

set tbs(kqpkn) {
  41 30 ? 8 ?
  99.7 0.3 0.0 0.0 12.5 87.5
  0
}

set tbs(kqpkp) {
  622 105 {8/8/8/8/3P2Q1/8/1p6/K1k5} 34 ?
  100.0 0.0 0.0 3.3 7.3 89.4
  0
}

set tbs(krrkq) {
  8 29 {3R4/1R6/8/8/q7/7K/8/k7} 49 {7R/1q6/3K4/8/k7/8/2R5/8}
  58.2 36.8 5.1 52.0 37.0 11.0
  10 10 {
    6R1/8/8/8/6R1/7q/1K5k/8 6R1/8/8/8/8/6R1/7q/K6k  8/6R1/8/8/8/3K2R1/7q/7k
    8/6R1/8/8/8/6R1/7q/1K5k 8/8/1R6/8/8/1R1K4/q7/k7 8/8/6R1/8/8/6R1/7q/2K4k
    8/8/8/3R4/8/k7/2KR4/4q3 8/8/8/6R1/8/6R1/7q/3K3k 8/8/8/8/1R6/1R6/q7/k2K4
    8/8/8/8/8/2K5/2R1R3/kq6
  } 0 {} 0 {}
}

set tbs(krrkr) {
  38 31 {8/1R6/8/8/8/5r1K/4R3/k7} 20 {1k6/2R5/7r/3K3R/8/8/8/8}
  99.2 0.7 0.0 0.4 33.4 66.2
  0
}

set tbs(krrkb) {
  8 29 {8/8/8/2b5/8/4KR2/1k6/6R1} 0 -
  99.3 0.7 0.0 0.0 22.4 77.6
  1 1 {8/8/8/8/8/b1k5/1R6/1RK5} 0 {} 0 {}
}

set tbs(krrkn) {
  8 40 {4k3/6R1/8/7n/5K2/1R6/8/8} 1 {8/8/8/8/1n6/k7/8/KR1R4}
  99.7 0.3 0.0 0.0 15.0 85.0
  0
}

set tbs(krrkp) {
  3 33 ? 50 ?
  100.0 0.0 0.0 1.0 5.7 93.3
  0
}

set tbs(krbkq) {
  23 21 ? 70 ?
  38.7 48.0 13.4 71.2 25.6 3.2
  372 0 {} 372 {3Kn3/8/8/8/8/4r3/7Q/3k4} 0 {}
}

set tbs(krbkr) {
  649 65 {k7/7r/3K4/8/6B1/8/4R3/8} 30 {8/4R2K/8/5k2/8/8/7B/4r3}
  41.3 58.7 0.0 0.8 94.1 5.1
  17 17 {
    8/8/8/8/8/1R1K4/2B5/r1k5 8/8/8/8/8/2KB4/2R5/kr6   8/8/8/8/7B/4r3/5R2/2K1k3
    8/8/8/8/rB6/8/1R6/1K1k4  7k/6R1/7r/8/8/8/1B6/1K6  8/8/8/8/8/2K4B/6R1/3r1k2
    8/8/8/8/4R3/k7/2K1B3/4r3 8/8/8/8/8/2R3Br/k1K5/8   8/8/8/3B1r2/3K4/8/6R1/3k4
    8/8/8/8/8/R2K4/5B2/1r1k4 8/8/8/8/8/3K2R1/8/4k1Br  8/8/8/2B5/8/6r1/k1K4R/8
    8/5r2/8/8/1R4B1/8/3K4/k7 8/8/3B4/1r6/8/2K5/4R3/1k6 8/8/8/8/3KB2r/8/5R2/k7
    8/8/3B4/8/8/5r2/k1K1R3/8 5R2/8/8/8/8/3K4/5Br1/2k5
  } 0 {} 0 {}
}

set tbs(krbkb) {
  20 30 ? 2 ?
  98.2 1.8 0.0 0.0 31.1 68.9
  0
}

set tbs(krbkn) {
  5 40 ? 1 ?
  98.9 1.1 0.0 0.0 24.0 76.0
  0
}

set tbs(krbkp) {
  33 28 ? 70 ?
  99.1 0.9 0.0 2.4 17.1 80.5
  1 1 {1k1K4/7R/8/8/8/8/6p1/7B} 0 {} 0 {}
}

set tbs(krnkq) {
  15 20 ? 69 ?
  35.4 41.1 23.4 78.2 19.7 2.1
  455 0 {} 455 {} 0 {}
}

set tbs(krnkr) {
  430 37 {2k1r3/8/R7/N2K4/8/8/8/8} 41 {4K3/8/1r6/8/5k2/1R4N1/8/8}
  36.7 63.3 0.1 3.2 93.6 3.2
  10 10 {
    2R5/8/8/8/8/k2K4/8/r1N5  8/8/8/8/3N4/1R1K4/8/r1k5 8/8/8/8/3N4/2KR4/8/2k1r3
    8/8/8/8/4N3/7R/k1K5/5r2  8/8/8/8/8/2KRN3/8/2k1r3  8/8/8/8/8/3KN3/3R4/2k1r3
    8/8/8/8/8/5RN1/8/2K1k1r1 8/8/8/8/8/6RN/8/3K1k1r   8/8/8/8/8/NR1K4/8/r1k5
    8/8/8/8/r1N5/2R5/k1K5/8
  } 0 {} 0 {}
}

set tbs(krnkb) {
  7 31 ? 1 ?
  97.7 2.3 0.0 0.0 32.4 67.6
  0
}

set tbs(krnkn) {
  12 37 ? 1 ?
  99.0 1.0 0.0 0.0 24.6 75.4
  3 3 {
    8/8/8/8/4n3/1k6/N7/R1K5 8/8/8/8/8/3n4/N2k4/RK6 8/8/8/8/8/n7/1k6/N1RK4
  } 0 {} 0 {}
}

set tbs(krnkp) {
  32 29 ? 68 ?
  98.5 1.5 0.0 4.5 17.1 78.4
  0
}

set tbs(krpkq) {
  367 68 ? 104 ?
  37.7 11.8 50.5 91.0 7.1 1.8
  243 2 {} 241 {} 0 {}
}

set tbs(krpkr) {
  9184 74 {8/1k6/4R3/8/8/8/6Pr/4K3} 33 {8/1P6/2k5/8/K7/8/8/1r5R}
  66.6 33.0 0.4 20.1 54.4 25.5
  209 209 {} 0 {} 0 {}
}

set tbs(krpkb) {
  626 73 ? 2 ?
  96.4 3.6 0.0 0.0 32.6 67.4
  225 225 {} 0 {} 0 {}
}

set tbs(krpkn) {
  397 54 ? 8 ?
  97.5 2.5 0.0 0.0 24.7 75.3
  413 413 {} 0 {} 0 {} 0 {}
}

set tbs(krpkp) {
  1092 56 ? 103 ?
  99.4 0.4 0.3 10.0 6.6 83.5
  3 0 {} 2 {
    8/8/8/8/8/1p6/kP6/1RK5 8/8/8/8/8/k7/Pp6/RK6
  } 1 {8/8/8/8/8/2p5/1kP5/2RK4}
}

set tbs(kbbkq) {
  3 21 ? 81 ?
  15.3 20.2 64.5 96.5 2.9 0.6
  1 0 {} 1 {8/8/8/8/q7/2BB4/1K6/3k4} 0 {}
}

set tbs(kbbkr) {
  13 23 {4r3/8/8/8/8/4B3/8/k1K4B} 31 {1K4B1/8/3k4/8/B5r1/8/8/8}
  16.5 83.4 0.1 1.3 97.2 1.5
  3 3 {
    8/8/8/8/8/3K1k2/6r1/4B2B 8/8/8/8/8/5k2/6r1/3KB2B 8/8/8/B7/8/3k4/2r5/KB6
  } 0 {} 0 {}
}

set tbs(kbbkb) {
  35 22 {6B1/8/7B/8/b7/2K5/8/k7} 2 {1B5K/5k1B/8/8/8/4b3/8/8}
  15.6 84.3 0.0 0.0 98.6 1.4
  0
}

set tbs(kbbkn) {
  28 78 {8/K7/8/8/8/5k2/6n1/2B4B} 1 ?
  48.2 51.8 0.0 0.0 66.1 33.9
  1 1 {8/8/8/8/8/6n1/2K4B/kB6} 0 {} 0 {}
}

set tbs(kbbkp) {
  23 74 ? 83 ?
  48.0 50.2 1.8 11.4 54.1 34.5
  1 1 {B1k5/1pB5/3K4/8/8/8/8/8} 0 {} 0 {}
}

set tbs(kbnkq) {
  13 36 ? 53 ?
  25.0 6.4 68.6 97.6 1.7 0.7
  1 0 {} 1 {8/8/q7/8/3K4/2N5/8/k1B5} 0 {}
}

set tbs(kbnkr) {
  64 36 {8/8/8/2N5/8/8/B6K/5kr1} 41 {8/8/1B4N1/5k2/8/1r6/8/4K3}
  26.0 73.8 0.2 3.8 94.6 1.6
  8 6 {
    3r4/8/2B5/8/1N6/8/8/k1K5 8/8/8/8/8/2k5/1r6/B1NK4  8/8/8/8/8/2k5/3r4/1KN1B3
    8/8/8/8/8/3k4/4r3/2KN1B2 8/8/8/8/8/4k3/5r2/3KN1B1 8/8/8/8/B7/1r6/N1k5/K7
  } 2 {8/8/8/8/8/1k3r2/8/1KB4N 8/r7/8/B7/8/8/N1k5/K7} 0 {}
}

set tbs(kbnkb) {
  54 39 {8/7B/8/8/6N1/8/3k4/1Kb5} 2 {KB6/8/k4N2/8/6b1/8/8/8}
  25.5 74.5 0.0 0.0 98.8 1.2
  45 45 {} 0 {} 0 {}
}

set tbs(kbnkn) {
  36 107 {6Bk/8/8/7N/8/7K/6n1/8} 1 {8/8/3N4/8/3n4/8/B7/K1k5}
  32.2 67.8 0.0 0.0 96.1 3.9
  922 922 {} 0 {} 0 {}
}

set tbs(kbnkp) {
  165 104 ? 55 ?
  91.4 5.5 3.2 14.7 23.0 62.4
  62 61 {} 1 {8/8/8/1N6/3K4/B7/5p2/k7} 0 {}
}

set tbs(kbpkq) {
  117 35 ? 50 ?
  21.3 11.5 67.2 96.8 2.8 0.4
  16 0 {} 16 {
    3K4/2P5/B3qk2/8/8/8/8/8   8/1KP1q3/1B1k4/8/8/8/8/8 8/qPK5/8/3k4/1B6/8/8/8
    2q5/2B2P2/3K4/1k6/8/8/8/8 8/2P5/4q3/KB6/8/k7/8/8   8/3P4/5q2/1KB5/8/1k6/8/8
    8/1KP1q3/4k3/B7/8/8/8/8   3K4/q1P5/B4k2/8/8/8/8/8  8/5P2/3K4/8/4k2B/7q/8/8
    8/4P3/6q1/k1K5/2B5/8/8/8  3k4/KP1q4/3B4/8/8/8/8/8  8/3K1P2/1k2Bq2/8/8/8/8/8
    3K4/2P5/2B2k2/8/1q6/8/8/8 8/1P1K4/1qB2k2/8/8/8/8/8 1k6/3K1P2/4Bq2/8/8/8/8/8
    5k2/1P1K4/1qB5/8/8/8/8/8
  } 0 {}
}

set tbs(kbpkr) {
  451 45 ? 39 ?
  30.9 67.3 1.8 23.4 73.1 3.5
  306 4 {} 302 {} 0 {}
}

set tbs(kbpkb) {
  570 51 ? 3 ?
  41.3 58.7 0.0 0.0 86.9 13.1
  160 160 {} 0 {} 0 {}
}

set tbs(kbpkn) {
  497 105 ? 8 ?
  53.7 46.3 0.0 0.0 76.4 23.6
  2125 2112 {} 13 {} 0 {}
}

set tbs(kbpkp) {
  1443 67 ? 51 ?
  86.4 9.5 4.1 16.7 24.1 59.2
  406 403 {} 2 {} 1 {8/8/8/8/8/k1p5/2P5/1BK5}
}

set tbs(knnkq) {
  5 1 {k1N5/2K5/8/3N4/8/5q2/8/8} 72 ?
  0.0 42.8 57.1 94.0 6.0 0.0
  229 0 {} 229 {} 0 {}
}

set tbs(knnkr) {
  15 3 {5r1k/8/7K/4N3/5N2/8/8/8} 41 {8/8/1r4N1/4kN2/8/8/8/4K3}
  0.0 99.6 0.4 6.3 93.7 0.0
  25 0 {} 25 {} 0 {}
}

set tbs(knnkb) {
  2 4 {7k/5K2/8/8/5NN1/8/8/2b5} 1 {8/8/8/8/8/8/N1k5/K1b4N}
  0.0 100.0 0.0 0.0 100.0 0.0
  0
}

set tbs(knnkn) {
  8 7 {7n/8/8/8/1N1KN3/8/8/k7} 1 {K7/N1k5/8/3n4/3N4/8/8/8}
  0.1 99.9 0.0 0.0 100.0 0.0
  362 362 {} 0 {} 0 {}
}

set tbs(knnkp) {
  71 115 ? 74 ?
  31.3 66.4 2.3 12.8 73.6 13.6
  3143 3124 {} 19 {} 0 {}
}

set tbs(knpkq) {
  130 41 ? 55 ?
  17.9 11.9 70.2 97.2 2.3 0.5
  52 0 {} 52 {} 0 {}
}

set tbs(knpkr) {
  433 44 ? 67 ?
  26.7 69.3 4.0 29.3 68.5 2.2
  1181 23 {} 1158 {} 0 {}
}

set tbs(knpkb) {
  728 43 ? 9 ?
  38.8 61.2 0.0 0.0 88.1 11.9
  642 640 {} 2 {} 0 {}
}

set tbs(knpkn) {
  781 97 ? 7 ?
  49.2 50.8 0.0 0.0 77.2 22.8
  4191 4128 {} 63 {} 0 {}
}

set tbs(knpkp) {
  1410 57 ? 58 ?
  78.3 13.6 8.1 21.8 27.6 50.6
  2303 2281 {} 14 {} 8 {
    8/8/8/8/3K4/NkpP4/8/8   8/8/8/8/3K4/3PpkN1/8/8 8/8/8/8/8/1k2p3/4P3/KN6
    8/8/8/8/8/2K5/2PpkN2/8  8/8/8/8/8/3K4/3PpkN1/8 8/8/8/8/8/3K4/NkpP4/8
    8/8/8/8/1p6/1P6/K7/N1k5 8/8/8/8/8/1K6/1PpkN3/8
  }
}

set tbs(kppkq) {
  726 124 {8/5P2/8/8/3K4/3P3q/7k/8} 41 {8/2KP2q1/8/2P5/5k2/8/8/8}
  16.0 12.6 71.4 98.4 1.5 0.1
  2 0 {} 2 {8/2KP3q/2P2k2/8/8/8/8/8 8/2KP3q/8/2P3k1/8/8/8/8} 0 {}
}

set tbs(kppkr) {
  1652 54 {3K4/8/8/4P3/8/2r5/5P2/2k5} 40 {8/8/8/7K/5P2/3Pr3/8/2k5}
  35.4 20.1 44.5 75.2 18.2 6.6
  119 18 {} 99 {} 2 {1r1k4/1P6/1PK5/8/8/8/8/8 8/8/8/8/k7/r1P5/1KP5/8}
}

set tbs(kppkb) {
  519 43 {8/6P1/7k/8/6P1/1K6/8/1b6} 4 {K5b1/P7/1k6/8/8/8/2P5/8}
  54.4 45.6 0.0 0.0 75.4 24.6
  212 211 {} 1 {8/8/8/8/8/b2k4/P2P4/1K6} 0 {}
}

set tbs(kppkn) {
  705 50 {3n4/5P2/8/8/3K2P1/8/k7/8} 17 {7K/8/4k2P/8/8/8/5P2/5n2}
  64.7 35.3 0.0 0.0 62.4 37.6
  1077 920 {} 157 {} 0 {}
}

set tbs(kppkp) {
  5080 127 {8/8/8/8/1p2P3/1k1KP3/8/8} 43 {7K/8/4P3/5P2/3k4/7p/8/8}
  77.1 10.3 12.6 27.7 19.1 53.2
  4237 4179 {} 52 {} 6 {
    8/8/8/8/2k5/K1p5/P3P3/8   8/8/8/8/3k4/1K1p4/1P3P2/8
    8/8/8/8/4k3/2K1p3/2P3P1/8 8/8/8/2k5/K1p5/P3P3/8/8
    8/8/8/8/5k2/3K1p2/3P3P/8  8/8/8/k7/p1K5/2P5/2P5/8
  }
}

set tbs(kqqqk) {
  0 3 ? 0 -
  100.0 0.0 0.0 0.0 4.0 96.0
  0
}

set tbs(kqqrk) {
  0 4 ? 0 -
  100.0 0.0 0.0 0.0 3.1 96.9
  0
}

set tbs(kqqbk) {
  3 4 ? 0 -
  100.0 0.0 0.0 0.0 2.7 97.3
  0
}

set tbs(kqqnk) {
  2 4 ? 0 -
  100.0 0.0 0.0 0.0 2.4 97.6
  0
}

set tbs(kqqpk) {
  12 4 ? 0 -
  100.0 0.0 0.0 0.0 2.1 97.9
  0
}

set tbs(kqrrk) {
  0 4 ? 0 -
  100.0 0.0 0.0 0.0 2.0 98.0
  0
}

set tbs(kqrbk) {
  3 5 ? 0 -
  100.0 0.0 0.0 0.0 1.7 98.3
  0
}

set tbs(kqrnk) {
  3 5 ? 0 -
  100.0 0.0 0.0 0.0 1.4 98.6
  0
}

set tbs(kqrpk) {
  26 7 ? 0 -
  100.0 0.0 0.0 0.0 1.1 98.9
  0
}

set tbs(kqbbk) {
  3 6 ? 0 -
  100.0 0.0 0.0 0.0 5.0 95.0
  0
}

set tbs(kqbnk) {
  5 7 ? 0 -
  100.0 0.0 0.0 0.0 1.1 98.9
  0
}

set tbs(kqbpk) {
  31 9 ? 0 -
  100.0 0.0 0.0 0.0 1.2 98.8
  0
}

set tbs(kqnnk) {
  0 8 ? 0 -
  100.0 0.0 0.0 0.0 9.1 90.9
  0
}

set tbs(kqnpk) {
  10 9 ? 0 -
  100.0 0.0 0.0 0.0 1.0 99.0
  0
}

set tbs(kqppk) {
  64 9 ? 0 -
  100.0 0.0 0.0 0.0 0.7 99.3
}

set tbs(krrrk) {
  2 5 ? 0 -
  100.0 0.0 0.0 0.0 0.9 99.1
  0
}

set tbs(krrbk) {
  0 10 ? 0 -
  100.0 0.0 0.0 0.0 0.8 99.2
  0
}

set tbs(krrnk) {
  0 10 ? 0 -
  100.0 0.0 0.0 0.0 0.6 99.4
  0
}

set tbs(krrpk) {
  7 14 ? 0 -
  100.0 0.0 0.0 0.0 0.3 99.7
  0
}

set tbs(krbbk) {
  0 12 ? 0 -
  100.0 0.0 0.0 0.0 4.3 95.7
  0
}

set tbs(krbnk) {
  3 29 ? 0 -
  100.0 0.0 0.0 0.0 0.5 99.5
  0
}

set tbs(krbpk) {
  23 16 ? 0 -
  100.0 0.0 0.0 0.0 0.6 99.4
  0
}

set tbs(krnnk) {
  0 15 ? 0 -
  100.0 0.0 0.0 0.0 8.5 91.5
  0
}

set tbs(krnpk) {
  16 17 ? 0 -
  100.0 0.0 0.0 0.0 0.5 99.5
  0
}

set tbs(krppk) {
  119 15 {8/8/4k3/8/8/3P4/3P4/5R1K} 0 -
  100.0 0.0 0.0 0.0 0.2 98.8
  0
}

set tbs(kbbbk) {
  0 16 ? 0 -
  74.0 26.0 0.0 0.0 31.6 68.4
  0
}

set tbs(kbbnk) {
  3 33 ? 0 -
  100.0 0.0 0.0 0.0 4.1 95.9
  0
}

set tbs(kbbpk) {
  5 30 ? 0 -
  98.3 1.7 0.0 0.0 6.8 93.2
  0
}

set tbs(kbnnk) {
  0 34 ? 0 -
  100.0 0.0 0.0 0.0 8.4 91.6
  0
}

set tbs(kbnpk) {
  26 33 ? 0 -
  100.0 0.0 0.0 0.0 0.8 99.2
  0
}

set tbs(kbppk) {
  100 25 ? 0 -
  99.8 0.2 0.0 0.0 1.3 98.7
  6 6 {
    8/B1k5/K7/P7/P7/8/8/8 K7/8/1k6/1P6/BP6/8/8/8 K7/8/Bk6/1P6/1P6/8/8/8
    KBk5/P1P5/8/8/8/8/8/8 kB6/8/1PK5/1P6/8/8/8/8 kB6/8/KP6/1P6/8/8/8/8
  } 0 {} 0 {}
}

set tbs(knnnk) {
  0 21 ? 0 -
  98.7 1.3 0.0 0.0 25.0 75.0
  0
}

set tbs(knnpk) {
  7 28 ? 0 -
  98.4 1.6 0.0 0.0 12.0 88.0
  0
}

set tbs(knppk) {
  96 32 ? 0 -
  100.0 0.0 0.0 0.0 1.0 99.0
  93 93 {} 0 {} 0 {}
}

set tbs(kpppk) {
  97 33 {7K/5k2/8/8/1P6/1P6/1P6/8} 0 -
  99.9 0.1 0.0 0.0 0.6 99.4
  11 11 {
    1k6/1P6/K7/P7/P7/8/8/8  1k6/1P6/K7/PP6/8/8/8/8  2k5/2P5/3K4/P7/P7/8/8/8
    8/1k6/1P6/KP6/1P6/8/8/8 8/8/1k6/1P6/KP6/1P6/8/8 8/8/8/1k1P4/8/PK6/P7/8
    8/8/8/1k6/1P6/KP6/1P6/8 8/K1k5/P1P5/P7/8/8/8/8  K1k5/2P5/P1P5/8/8/8/8/8
    K1k5/8/P1P5/P7/8/8/8/8  k7/8/KP6/PP6/8/8/8/8
  } 0 {} 0 {}
}

# End of file: tb.tcl

