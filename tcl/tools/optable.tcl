### optable.tcl: Opening report and theory table generation.
### Part of Scid. Copyright 2001-2003 Shane Hudson.

namespace eval ::optable {}
array set ::optable::_data {}

set ::optable::_data(exclude) none
set ::optable::_docStart(text) {}
set ::optable::_docEnd(text) {}
set ::optable::_docStart(ctext) {}
set ::optable::_docEnd(ctext) {}
set ::optable::_flip 0
set ::optable::OPTABLE_MAX_LINES       100000 ; # also redefined in optable.h
set ::optable::OPTABLE_MAX_TABLE_LINES 10000  ; # also redefined in optable.h

set ::optable::_docStart(html) {<html>
  <head>
  <title>Opening Report</title>
  <style type="text/css">
  <!--
  h1 { color:#990000 }
  h2 { color:#990000 }
  h3 { color:#990000 }
  .player {
    color:darkblue
  }
  .elo {
    color:green
    font-style:italic
  }
  sup {
    color:red
  }
  -->
  </style>
  </head>
  <body bgcolor="#ffffff">
}
set ::optable::_docEnd(html) {</body>
  </html>
}

set ::optable::_docStart(latex) $::exportStartFile(Latex)
set ::optable::_docEnd(latex) $::exportEndFile(Latex)

proc ::optable::ConfigMenus {{lang ""}} {
  if {! [winfo exists .oprepWin]} { return }
  if {$lang == ""} { set lang $::language }
  set m .oprepWin.menu
  foreach idx {0 1 2} tag {File Favorites Help} {
    configMenuText $m $idx Oprep$tag $lang
  }
  foreach idx {0 1 2 4 6} tag {Text Html LaTeX Options Close} {
    configMenuText $m.file $idx OprepFile$tag $lang
  }
  foreach idx {0 1 2} tag {Add Edit Generate} {
    configMenuText $m.favorites $idx OprepFavorites$tag $lang
  }
  foreach idx {0 1} tag {Report Index} {
    configMenuText $m.help $idx OprepHelp$tag $lang
  }
}

proc ::optable::makeReportWin {{showProgress 1} {showDisplay 1}} {
  if {! [sc_base inUse]} { return }
  set ::optable::opReportBase [sc_base current]
  if {$showProgress} {
    set w .progress
    toplevel $w
    wm withdraw $w
    wm title $w "Generating Report"

    pack [frame $w.b] -side bottom -fill x
    set ::optable::_interrupt 0
    button $w.b.cancel -text $::tr(Cancel) -command {
      set ::optable::_interrupt 1
      sc_progressBar
    }
    pack $w.b.cancel -side right -pady 5 -padx 2

    foreach i {1 2} name { "Searching database for report games" "Generating report information" } {
      label $w.text$i -text "$i. $name"
      pack $w.text$i -side top
      canvas $w.c$i -width 400 -height 20  -relief solid -border 1
      $w.c$i create rectangle 0 0 0 0 -fill $::progcolor -outline $::progcolor -tags bar
      $w.c$i create text 395 10 -anchor e -font font_Regular -tags time \
          -fill black -text "0:00 / 0:00"
      pack $w.c$i -side top -pady 10
    }
    placeWinCenter $w
    wm deiconify $w
    grab $w.b.cancel
    sc_progressBar $w.c1 bar 401 21 time
    busyCursor .
  }
  sc_search board RESET Exact false 0
  set newTreeData [sc_tree search -time 0 -epd 0 -adjust 1]
  if {$showProgress} {
    # This is ugly.. Handle the (two possible) interrupts, but continue on to main window regardless
    # as we may wish to process a favourite, or change an option, and there is no other way to do so. &%$#@ S.A.
    if {$::optable::_interrupt} {
      unbusyCursor .
      grab release $w.b.cancel
    } else {
      sc_progressBar $w.c2 bar 401 21 time
    }
  }
  if {!$::optable::_interrupt} {
    sc_report opening create $::optable(ExtraMoves) $::optable(MaxTableGames) $::optable(MaxGames) $::optable::_data(exclude)
  }

  if {$showProgress} {
    grab release $w.b.cancel
    destroy $w
    if {$::optable::_interrupt} {
      # unbusyCursor .
      # return
    }
  }
  set ::optable::_data(tree) $newTreeData
  # We latexifyTree here and not when we request "View Latex" to assure the report tree and latex tree are the same.
  ::optable::latexifyTree
  set ::optable::_data(bdLaTeX) [sc_pos tex]
  set ::optable::_data(bdHTML) [sc_pos html]
  set ::optable::_data(bdLaTeX_flip) [sc_pos tex flip]
  set ::optable::_data(bdHTML_flip) [sc_pos html -flip 1]
  ::optable::setupRatios
  if {$::optable::_interrupt} {
    # Don't generate report if interrupted... bugs ?
    set report [tr ErrSearchInterrupted]
  } else {
    set report [::optable::report ctext 1]
  }
  if {!$showDisplay} {
    return
  }

  set w .oprepWin
  if {[winfo exists $w]} {
    raiseWin $w
  } else {
    toplevel $w
    wm withdraw $w
    wm title $w "[tr ToolsOpReport]"
    menu $w.menu
    $w configure -menu $w.menu

    $w.menu add cascade -label OprepFile -menu $w.menu.file
    $w.menu add cascade -label OprepFavorites -menu $w.menu.favorites
    $w.menu add cascade -label OprepHelp -menu $w.menu.help
    foreach i {file favorites help} {
      menu $w.menu.$i -tearoff 0
    }
    $w.menu.file add command -label OprepFileText -command {::optable::saveReport text}
    $w.menu.file add command -label OprepFileHtml -command {::optable::saveReport html}
    $w.menu.file add command -label OprepFileLaTeX -command {::optable::saveReport latex}
    $w.menu.file add separator
    $w.menu.file add command -label OprepFileOptions -command ::optable::setOptions
    $w.menu.file add separator
    $w.menu.file add command -label OprepFileClose -command "$w.b.close invoke"
    $w.menu.favorites add command -label OprepFavoritesAdd -command ::optable::addFavoriteDlg
    $w.menu.favorites add command -label OprepFavoritesEdit -command ::optable::editFavoritesDlg
    $w.menu.favorites add command -label OprepFavoritesGenerate -command ::optable::generateFavoriteReports
    $w.menu.favorites add separator
    $w.menu.help add command -label OprepHelpReport \
        -accelerator F1 -command {helpWindow Reports Opening}
    $w.menu.help add command -label OprepHelpIndex -command {helpWindow Index}
    ::optable::updateFavoritesMenu

    bind $w <F1> {helpWindow Reports Opening}
    bind $w <Escape> "$w.b.close invoke"
    bind $w <Up> "$w.text yview scroll -1 units"
    bind $w <Down> "$w.text yview scroll 1 units"
    bind $w <Prior> "$w.text yview scroll -1 pages"
    bind $w <Next> "$w.text yview scroll 1 pages"
    bind $w <Key-Home> "$w.text yview moveto 0"
    bind $w <Key-End> "$w.text yview moveto 0.99"
    bindMouseWheel $w $w.text

    text $w.text -setgrid 1 -wrap word  -yscrollcommand "$w.ybar set" -cursor top_left_arrow
    ::htext::init $w.text
    scrollbar $w.ybar -command "$w.text yview"
    frame $w.b
    button $w.b.previewLatex -textvar ::tr(OprepViewLaTeX) \
        -command {previewLatex Scid-Opening-Report {::optable::report latex 1 $::optable::_flip} .oprepWin}
    button $w.b.previewHTML -textvar ::tr(OprepViewHTML) \
        -command ::optable::previewHTML
    button $w.b.opts -text [tr OprepFileOptions] -command ::optable::setOptions
    label $w.b.lexclude -textvar ::tr(Exclude)
    menubutton $w.b.exclude -textvar ::optable::_data(exclude) \
        -indicatoron 1 -relief raised -bd 2 -menu $w.b.exclude.m -width 5
    menu $w.b.exclude.m -tearoff 0
    button $w.b.update -textvar ::tr(Update) -command {
      set ::optable::_data(yview) [lindex [.oprepWin.text yview] 0]
      ::optable::makeReportWin
      .oprepWin.text yview moveto $::optable::_data(yview)
    }
    button $w.b.mergeGames -textvar ::tr(MergeGames) -command ::optable::mergeGames
    button $w.b.help -textvar ::tr(Help) -command {helpWindow Reports Opening}
    button $w.b.close -textvar ::tr(Close) -command "destroy $w"
    bind $w <Destroy> {
      if {"%W" == ".oprepWin"} {
	set ::optable::_data(exclude) ---
	focus .main
	sc_tree clean $::optable::opReportBase
      }
    }

    bindWheeltoFont $w

    entry $w.b.find -width 10 -textvariable ::oreport(find) -highlightthickness 0
    configFindEntryBox $w.b.find ::oreport .oprepWin.text

    pack $w.b -side bottom -fill x -pady 3
    pack $w.ybar -side right -fill y
    pack $w.text -side left -fill both -expand yes
    pack $w.b.close $w.b.find $w.b.update -side right -padx 2
    pack $w.b.previewLatex -side left -padx 2
    pack $w.b.previewHTML -side left -padx 2
    pack $w.b.opts $w.b.lexclude $w.b.exclude $w.b.mergeGames -side left -padx 2
    ::optable::ConfigMenus
    placeWinCenter $w
    update
    wm deiconify $w
  }

  catch {destroy $w.text.bd}
  ::board::new $w.text.bd 40 0
  if {$::optable::_flip} { ::board::flip $w.text.bd }
  $w.text.bd configure -relief solid -borderwidth 1
  for {set i 0} {$i < 63} {incr i} {
    ::board::bind $w.text.bd $i <ButtonPress-1> ::optable::flipBoard
    ::board::bind $w.text.bd $i <ButtonPress-3> ::optable::resizeBoard
  }
  ::board::update $w.text.bd [sc_pos board]
  $w.b.exclude.m delete 0 end
  $w.b.exclude.m add radiobutton -label "none" \
      -variable ::optable::_data(exclude) -command "$w.b.update invoke"
  foreach move $::optable::_data(moves) {
    $w.b.exclude.m add radiobutton -label $move \
        -variable ::optable::_data(exclude) -command "$w.b.update invoke"
  }
  if {[lsearch $::optable::_data(moves) $::optable::_data(exclude)] < 0} {
    set ::optable::_data(exclude) "none"
  }
  busyCursor .
  update idletasks
  $w.text configure -state normal
  $w.text delete 1.0 end
  regsub -all "\n" $report "<br>" report
  ::htext::display $w.text $report
  unbusyCursor .
  ::windows::stats::Refresh
}

### Merge the N best games up to P plies to current game

proc ::optable::mergeGames { {game_count 50} {ply 999} } {
  set base  $::optable::opReportBase
  set current [sc_game number]
  sc_game undoPoint
  set games [split [sc_report opening best a $game_count] "\n"]
  foreach g $games {
    if {$g == "" } {continue}
    set res [scan $g "%d:  <g_%d>" d1 game_number]
    if {$res != 2} {
      set message "Error merging game"
      if {[info exists game_number]} {
        set message "$message $game_number"
      }
      tk_messageBox -title "Error writing report" -type ok -icon warning -message $message
      break
    }
    # dont merge current game with itself
    if {$game_number == $current} {continue}
    set err [ catch { sc_game merge $base $game_number $ply } result ]
    if {$err} {
      tk_messageBox -title Scid -type ok -icon info -message "Unable to merge the selected game:\n$result"
      break
    }
  }
  tk_messageBox -title "Merge Complete" -type ok -icon info -message "Merge Complete."
  updateBoard -pgn
}

proc ::optable::flipBoard {} {
  ::board::flip .oprepWin.text.bd
  set ::optable::_flip [::board::isFlipped .oprepWin.text.bd]
}

proc ::optable::resizeBoard {} {
  set bd .oprepWin.text.bd
  set size [::board::size $bd]
  if {$size >= 60} { set size 30 } else { incr size 5 }
  ::board::resize $bd $size
}

proc ::optable::setOptions {} {
  set w .oprepOptions

  if {[winfo exists $w]} {
    raiseWin $w
    return
  }
  toplevel $w
  wm withdraw $w
  wm title $w  "[tr ToolsOpReport] [tr OprepFileOptions]"

  pack [frame $w.f] -side top -fill x -padx 5 -pady 5
  set row 0
  foreach i {Stats Popular AvgPerf Results MovesFrom Themes Endgames} {
    set yesno($i) 1
  }
  set left 0
  set right 1
  set from 0
  set to 10
  foreach i {OprepStatsHist   MaxGames Stats Oldest Newest Popular MostFrequent sep \
             OprepRatingsPerf AvgPerf HighRating sep \
             OprepTrends      Results Shortest sep \
             OprepMovesThemes MoveOrders MovesFrom Themes Endgames sep \
             OprepTheoryTable MaxTableGames ExtraMoves} {
    if {$i == "col"} {
      # Used to signfify a second column, but now unused - S.A.
      incr left 4
      frame $w.f.colsep$left -width 8
      grid $w.f.colsep$left -row 0 -column $left
      incr left
      set right [expr {$left + 1}]
      set row 0
    } elseif {$i == "gap"} {
      # nothing
    } elseif {$i == "sep"} {
      frame $w.f.fsep$row$left -height 2 -borderwidth 2 -relief sunken
      # frame $w.f.tsep$row$left -height 2 -borderwidth 2 -relief sunken
      grid $w.f.fsep$row$left -row $row -column $left -sticky we -columnspan 4 -pady 2
    } elseif {[info exists yesno($i)]} {
      checkbutton $w.f.f$i -variable ::optable($i) -onvalue 1 -offvalue 0
      label $w.f.t$i -textvar ::tr(Oprep$i)
      grid $w.f.f$i -row $row -column $left -sticky n
      grid $w.f.t$i -row $row -column $right -sticky w -columnspan 3
    } elseif {[string match Oprep* $i]} {
      # section heading
      label $w.f.f$i -textvar ::tr($i) -font font_Bold
      grid $w.f.f$i -row $row -column $left -columnspan 4 ;# -sticky e
    } else {
      if {$i == "MaxGames" } {
        spinbox $w.f.s$i -textvariable ::optable($i) -from 0 -to $::optable::OPTABLE_MAX_LINES -increment 500 -width 6 -justify right
      } elseif {$i == "MaxTableGames"} {
        spinbox $w.f.s$i -textvariable ::optable($i) -from 0 -to $::optable::OPTABLE_MAX_TABLE_LINES -increment 500 -width 6 -justify right
      } else {
        spinbox $w.f.s$i -textvariable ::optable($i) -width 3 -from $from -to $to -justify right
      }
      label $w.f.t$i -textvar ::tr(Oprep$i)
      grid $w.f.s$i -row $row -column $left ;# -sticky e
      if {$i == "MostFrequent"  ||  $i == "Shortest"} {
        checkbutton $w.f.w$i -text $::tr(White) \
            -variable ::optable(${i}White)
        checkbutton $w.f.b$i -text $::tr(Black) \
            -variable ::optable(${i}Black)
        grid $w.f.t$i -row $row -column $right -sticky w
        grid $w.f.w$i -row $row -column 2
        grid $w.f.b$i -row $row -column 3
      } else {
        grid $w.f.t$i -row $row -column $right -sticky w -columnspan 3
      }
    }
    grid rowconfigure $w.f $row -pad 2
    if {$i != "col"} { incr row }
  }
  addHorizontalRule $w
  pack [frame $w.b] -side bottom -fill x
  dialogbutton $w.b.defaults -textvar ::tr(Defaults) -command {
    array set ::optable [array get ::optableDefaults]
  }
  dialogbutton $w.b.ok -text "OK" -command {
    destroy .oprepOptions
    catch {set ::optable::_data(yview) [lindex [.oprepWin.text yview] 0]}
    ::optable::makeReportWin
    catch {.oprepWin.text yview moveto $::optable::_data(yview)}
  }
  dialogbutton $w.b.cancel -textvar ::tr(Cancel) -command {
    array set ::optable [array get ::optable::backup]
    destroy .oprepOptions
  }
  packbuttons left $w.b.defaults
  packbuttons right $w.b.cancel $w.b.ok
  array set ::optable::backup [array get ::optable]
  bind $w <Escape> "$w.b.cancel invoke"
  bind $w <F1> {helpWindow Reports Opening}

  update
  placeWinOverParent $w .oprepWin
  wm deiconify $w
}

### Save the report to a temporary file, and invoke the user's web browser to display it.

proc ::optable::previewHTML {} {
  busyCursor .
  set tmpdir $::scidLogDir
  set tmpfile TempOpeningReport.html
  set fname [file nativename [file join $tmpdir $tmpfile]]
  if {[catch {set tempfile [open $fname w]}]} {
    tk_messageBox -title "Scid: Error writing report" -type ok -icon warning \
        -message "Unable to write the file: $fname"
  }
  puts $tempfile [::optable::report html 1 $::optable::_flip]
  close $tempfile
  set sourcedir [file nativename $::scidShareDir/bitmaps/]
  catch { file copy -force $sourcedir $tmpdir }
  openURL $fname
  unbusyCursor .
}

###  Save the current opening report to a file.
#   "fmt" is the format: text, html or latex.
#   "type" is the report type: report, table, or both.

proc ::optable::saveReport {fmt} {
  set t [tk_dialog .oprepWin.dialog "Select report type" \
      "Select Report Type\n\nFull report (includes theory table), Compact report (no theory table) or theory table by itself." \
      question 0 "Full report" "Compact report" \
      "Theory table" "Cancel"]
  if {$t == 3} { return }
  set default ".txt"
  set ftype {
    { "Text files" {".txt"} }
    { "All files"  {"*"}    }
  }
  if {$fmt == "latex"} {
    set default ".tex"
    set ftype {
      { "LaTeX files" {".tex" ".ltx"} }
      { "All files"  {"*"}    }
    }
  } elseif {$fmt == "html"} {
    set default ".html"
    set ftype {
      { "HTML files" {".html" ".htm"} }
      { "All files"  {"*"}    }
    }
  }

  set fname [tk_getSaveFile -initialdir $::env(HOME) -filetypes $ftype -parent .oprepWin \
      -defaultextension $default -title "Save Opening Report"]
  if {$fname == ""} { return }
  if {$::macOS && ![string match *$default $fname] && ![string match *.* $fname]} {
      append fname $default
  }

  busyCursor .
  if {[catch {set tempfile [open $fname w]}]} {
    tk_messageBox -title "Scid: Error writing report" -type ok -icon warning \
        -message "Unable to write the file: $fname\n\n"
  } else {
    if {$t == 2} {
      set report [::optable::table $fmt]
    } elseif {$t == 1} {
      set report [::optable::report $fmt 0 $::optable::_flip]
    } else {
      set report [::optable::report $fmt 1 $::optable::_flip]
    }
    # Why are we trying to convert these reports to the default language's menu encoding ?? S.A.
    # if {$::hasEncoding  &&  $::langEncoding($::language) != ""} { catch {set report [encoding convertto $::langEncoding($::language) $report]} }

    puts $tempfile $report
    close $tempfile
  }
  unbusyCursor .
}

### Convert the plain text tree output used for text/html reports to a table for LaTeX output.

proc ::optable::latexifyTree {} {
  set ::optable::_data(moves) {}
  if {! [info exists ::optable::_data(tree)]} { return }
  set tree [split $::optable::_data(tree) "\n"]
  set ltree "\\begin{tabularx}{0.9\\textwidth}{rlrr@{:}rrrrrX}\n\\hline\n"
  append ltree " & Move"
  append ltree " & \\multicolumn{2}{c}{Frequency}"
  append ltree " & Score"
  append ltree " & \\% Draws"
  append ltree " & \$\\mu\$Elo"
  append ltree " & Perf"
  append ltree " & \$\\mu\$Year"
  append ltree " & ECO \\\\ \\hline\n"
  set len [llength $tree]
  set done 0
  for {set i 1} {$i < $len} {incr i} {
    set line [lindex $tree $i]
    if {[string index $line 0] == "_"} {
      append ltree "\\hline\n"
      continue
    }
    if {[string length $line] == 0} { continue }
    set num    [string range $line  0  1]
    set move   [string range $line  4  9]
    set freq   [string range $line 10 16]
    set fpct   [string range $line 18 22]
    set score  [string range $line 26 30]
    set pctDraw [string range $line 33 35]
    set avElo  [string range $line 39 42]
    set perf   [string range $line 45 48]
    set avYear [string range $line 51 54]
    set eco    [string range $line 56 61]
    set mv [string trim $move]
    regsub K $move {{\\K}} move
    regsub Q $move {{\\Q}} move
    regsub R $move {{\\R}} move
    regsub B $move {{\\B}} move
    regsub N $move {{\\N}} move
    if {[string index $line 0] == "T"} {
      append ltree "\\multicolumn{2}{l}{Total}"
    } else {
      append ltree " $num & $move "
      lappend ::optable::_data(moves) $mv
    }
    append ltree " & $freq & $fpct\\% & $score\\%"
    append ltree " & $pctDraw\\% & $avElo & $perf & $avYear & $eco \\\\ \n"
  }
  append ltree "\\hline\n"
  append ltree "\\end{tabularx}\n"
  set ::optable::_data(latexTree) $ltree
}

proc ::optable::setupRatios {} {
  set r [sc_filter freq date 0000.00.00]
  if {[lindex $r 0] == 0} {
    set ::optable::_data(ratioAll) 0
  } else {
    set ::optable::_data(ratioAll) \
        [expr {int(double([lindex $r 1]) / double([lindex $r 0]))} ]
  }
  foreach {start end} {1800 1899  1900 1949  1950 1969  1970 1979
    1980 1989 1990 1999 2000 2009 2010 2019 2020 2029} {
    set r [sc_filter freq date $start.00.00 $end.12.31]
    set filter [lindex $r 0]
    set all [lindex $r 1]
    if {$filter == 0} {
      set ::optable::_data(range$start) "none"
    } else {
      set ::optable::_data(range$start) [expr {int(double($all) / double($filter))} ]
    }
  }
  foreach y {1 5 10} {
    set year "[expr [::utils::date::today year]-$y]"
    append year ".[::utils::date::today month].[::utils::date::today day]"
    set r [sc_filter freq date $year]
    set filter [lindex $r 0]
    set all [lindex $r 1]
    if {$filter == 0} {
      set ::optable::_data(ratio$y) 0
    } else {
      set ::optable::_data(ratio$y) \
          [expr {int(double($all) / double($filter))} ]
    }
    if {$::optable::_data(ratio$y) == 0} {
      set r 1.0
    } else {
      set r [expr {double($::optable::_data(ratioAll))} ]
      set r [expr {$r / double($::optable::_data(ratio$y))} ]
    }
    set ::optable::_data(delta$y) [expr {int(($r - 1.0) * 100.0 + 0.5)} ]
  }
}

proc ::optable::_percent {x fmt} {
  set p "%"
  if {$fmt == "latex"} { set p "\\%" }
  return "[expr $x / 10][sc_info decimal][expr $x % 10]$p"
}

proc ::optable::results {reportType fmt} {
  set s {}
  set n "\n"; set next " "; set p "%"
  set white "1-0"; set draw "=-="; set black "0-1"

  if {$fmt == "latex"} {
    set next " & "; set n "\\\\\n"; set p "\\%"
    set white "\\win"; set draw "\\draw"; set black "\\loss"
    append s "\\begin{tabularx}{0.7\\textwidth}{Xccccccc}\n"
  }

  if {$fmt == "html"} { append s "<pre>\n" }
  if {$fmt == "ctext"} { append s "<tt>" }
  if {$fmt == "latex"} { append s "\\hline\n" }

  set lenReport [string length $::tr(OprepReportGames)]
  set lenAll [string length $::tr(OprepAllGames)]
  set len [expr {($lenReport > $lenAll) ? $lenReport : $lenAll} ]
  set score [::utils::string::Capital $::tr(score)]
  set slen [string length $score]
  if {$slen < 7} { set slen 7 }

  append s " [::utils::string::Pad {} $len] $next"
  append s "[::utils::string::PadRight $score $slen] $next"
  if {$fmt == "latex"} {
    append s "\\multicolumn{3}{c}{$::tr(OprepLength)} & "
    append s "\\multicolumn{3}{c}{$::tr(OprepFrequency)} $n "
  } else {
    append s "[::utils::string::PadCenter $::tr(OprepLength) 19] $next"
    append s "[::utils::string::PadCenter $::tr(OprepFrequency) 22] $n"
  }

  append s " [::utils::string::Pad {} $len] $next"
  append s "[::utils::string::PadRight {} $slen] $next"
  append s "[::utils::string::PadRight $white 5] $next"
  append s "[::utils::string::PadRight $draw  5] $next"
  append s "[::utils::string::PadRight $black 5] $next"
  append s "[::utils::string::PadRight $white 5]  $next"
  append s "[::utils::string::PadRight $draw  5]  $next"
  append s "[::utils::string::PadRight $black 5]  $n"
  if {$fmt == "latex"} { append s "\\hline\n" }

  set sc [sc_report $reportType score]
  set wlen [sc_report $reportType avgLength 1]
  set dlen [sc_report $reportType avgLength =]
  set blen [sc_report $reportType avgLength 0]
  set wf [sc_report $reportType freq 1]
  set df [sc_report $reportType freq =]
  set bf [sc_report $reportType freq 0]

  append s " [::utils::string::Pad $::tr(OprepReportGames) $len] $next"
  append s "[::utils::string::PadRight [::optable::_percent [lindex $sc 0] $fmt] $slen] $next"
  append s "[::utils::string::PadRight [lindex $wlen 0] 5] $next"
  append s "[::utils::string::PadRight [lindex $dlen 0] 5] $next"
  append s "[::utils::string::PadRight [lindex $blen 0] 5] $next"
  append s "[::utils::string::PadRight [::optable::_percent [lindex $wf 0] $fmt] 6] $next"
  append s "[::utils::string::PadRight [::optable::_percent [lindex $df 0] $fmt] 6] $next"
  append s "[::utils::string::PadRight [::optable::_percent [lindex $bf 0] $fmt] 6] $n"

  append s " [::utils::string::Pad $::tr(OprepAllGames) $len] $next"
  append s "[::utils::string::PadRight [::optable::_percent [lindex $sc 1] $fmt] $slen] $next"
  append s "[::utils::string::PadRight [lindex $wlen 1] 5] $next"
  append s "[::utils::string::PadRight [lindex $dlen 1] 5] $next"
  append s "[::utils::string::PadRight [lindex $blen 1] 5] $next"
  append s "[::utils::string::PadRight [::optable::_percent [lindex $wf 1] $fmt] 6] $next"
  append s "[::utils::string::PadRight [::optable::_percent [lindex $df 1] $fmt] 6] $next"
  append s "[::utils::string::PadRight [::optable::_percent [lindex $bf 1] $fmt] 6] $n"

  if {$fmt == "latex"} { append s "\\hline\n\\end{tabularx}\n" }
  if {$fmt == "html"} { append s "</pre>\n" }
  if {$fmt == "ctext"} { append s "</tt>" }

  return $s
}

proc ::optable::stats {fmt} {
  global stats
  set s {}
  set all $::tr(OprepStatAll)
  set both $::tr(OprepStatBoth)
  set since $::tr(OprepStatSince)
  set games [::utils::string::Capital $::tr(games)]
  set score [::utils::string::Capital $::tr(score)]

  set alen [string length $all]
  set blen [expr {[string length $both] + 6} ]
  set slen [expr {[string length $since] + 11} ]
  set len $alen
  if {$len < $blen} { set len $blen }
  if {$len < $slen} { set len $slen }

  set ratings 0
  set years 0
  set rlist [lsort -decreasing [array names stats r*]]
  set ylist [lsort [array names stats y*]]
  foreach i $rlist { if {$stats($i)} { set ratings 1 } }
  foreach i $ylist { if {$stats($i)} { set years 1 } }

  if {$fmt == "latex"} {
    append s "\\begin{tabularx}{0.7\\textwidth}{X r r r r r @{.} l}\n\\hline\n"
    append s "       & \\textbf{$games} & \\textbf{\\win} & \\textbf{\\draw} & \\textbf{\\loss} & "
    append s "\\multicolumn{2}{c}\\textbf{$score} \\\\ \\hline \n"
    scan [sc_filter stats all] "%u%u%u%u%u%\[.,\]%u" g w d l p c x
    append s "$all & $g & $w & $d & $l & $p&$x\\% \\\\\n"

    if {$ratings} {
      append s "\\hline\n"
      foreach i $rlist {
        if {$stats($i)} {
          set elo [string range $i 1 end]
          scan [sc_filter stats elo $elo] "%u%u%u%u%u%\[.,\]%u" g w d l p c x
          append s "$both $elo+ & $g & $w & $d & $l & $p&$x\\% \\\\\n"
        }
      }
    }
    if {$years} {
      append s "\\hline\n"
      foreach i $ylist {
        if {$stats($i)} {
          set year [string range $i 1 end]
          scan [sc_filter stats year $year] "%u%u%u%u%u%\[.,\]%u" g w d l p c x
          append s "$since $year.01.01 & $g & $w & $d & $l & $p&$x\\% \\\\\n"
        }
      }
    }
    append s "\\hline\n\\end{tabularx}\n"
    return $s
  }

  # For non-LaTeX format, just display in plain text:
  if {$fmt == "html"} { append s "<pre>\n" }
  if {$fmt == "ctext"} { append s "<tt>" }
  set stat ""
  append s " [::utils::string::Pad $stat [expr $len - 4]] [::utils::string::PadRight $games 10]"
  append s "     1-0     =-=     0-1 [::utils::string::PadRight $score 8]\n"
  append s "-----------------------------------------------------------"
  append s "\n [::utils::string::Pad $all $len]"     [sc_filter stats all]

  if {$ratings} {
    append s "\n"
    foreach i $rlist {
      if {$stats($i)} {
        set elo [string range $i 1 end]
        set stat "$both $elo+"
        append s "\n [::utils::string::Pad $stat $len]"   [sc_filter stats elo $elo]
      }
    }
  }
  if {$years} {
    append s "\n"
    foreach i $ylist {
      if {$stats($i)} {
        set year [string range $i 1 end]
        set stat "$since $year.01.01"
        append s "\n [::utils::string::Pad $stat $len]"   [sc_filter stats year $year]
      }
    }
  }
  append s "\n-----------------------------------------------------------\n"
  if {$fmt == "html"} { append s "</pre>\n" }
  if {$fmt == "ctext"} { append s "</tt>" }
  return $s
}

proc ::optable::_reset {} {
  set ::optable::_data(sec) 0
  set ::optable::_data(subsec) 0
}

proc ::optable::_title {title} {
  set fmt $::optable::_data(fmt)
  if {$fmt == "latex"} {
    return "\\begin{center}{\\LARGE \\color{blue}$title}\\end{center}\n\n"
  } elseif {$fmt == "html"} {
    return "<h2><center>$title</center></h2>\n\n"
  } elseif {$fmt == "ctext"} {
    return "<h2><center>$title</center></h2>\n\n"
  }
  set r    "--------------------------------------------------------------"
  append r "\n                        [string toupper $title]\n"
  append r "--------------------------------------------------------------"
  append r "\n\n"
  return $r
}

proc ::optable::_sec {text} {
  set fmt $::optable::_data(fmt)
  incr ::optable::_data(sec)
  set ::optable::_data(subsec) 0
  if {$fmt == "latex"} {
    return "\n\n\\section{$text}\n"
  } elseif {$fmt == "html"} {
    return "\n<h2>$::optable::_data(sec). $text</h2>\n"
  } elseif {$fmt == "ctext"} {
    return "<h4>$::optable::_data(sec). $text</h4>"
  }
  set line "$::optable::_data(sec). [string toupper $text]"
  set underline "-----------------------------------------------------"
  return "\n\n$line\n[string range $underline 1 [string length $line]]\n"
}

proc ::optable::_subsec {text} {
  set fmt $::optable::_data(fmt)
  incr ::optable::_data(subsec)
  if {$fmt == "latex"} {
    return "\n\\subsection{$text}\n\n"
  } elseif {$fmt == "html"} {
    return "\n<h3>$::optable::_data(sec).$::optable::_data(subsec) $text</h3>\n\n"
  } elseif {$fmt == "ctext"} {
    return "\n<maroon><b>$::optable::_data(sec).$::optable::_data(subsec) $text</b></maroon>\n\n"
  }
  return "\n$::optable::_data(sec).$::optable::_data(subsec)  $text\n\n"
}

### Produces a report in the appropriate format. If "withTable" is true, the theory table is also included.

proc ::optable::report {fmt withTable {flipPos 0}} {
  global tr
  sc_report opening format $fmt
  set fmt [string tolower $fmt]
  set ::optable::_data(fmt) $fmt
  ::optable::_reset

  # numRows: the number of rows to show in the theory table.
  # If it is zero, the number of rows if decided according to the
  # number of games in the report.
  set numRows 0

  # Specify whether a theory table is to be printed, so note numbers
  # can be generated and displayed if necessary:
  sc_report opening notes $withTable $numRows

  set n "\n"; set p "\n\n"; set preText ""; set postText ""
  set percent "%"; set bullet "  * "; set next ""
  if {$fmt == "latex"} {
    set n "\\\\\n"; set p "\n\n"
    set preText "\\begin{tabularx}{0.8\\textwidth}{rX} \n";
    set postText "\\end{tabularx}\n"
    set percent "\\%"; set bullet "\\hspace{0.5cm}\$\\bullet\$"
    set next " & "
    set multicolumnstart "\\multicolumn{2}{c}{"
    set multicolumnend "}"
  } elseif {$fmt == "html"} {
    set n "<br>\n"; set p "<p>\n\n"
    set preText "<pre>\n"; set postText "</pre>\n"
  } elseif {$fmt == "ctext"} {
    set preText "<tt>"; set postText "</tt>"
  }

  # Generate the report:
  set games $tr(games)
  set moves $tr(moves)
  set counts [sc_report opening count]
  set rgames [lindex $counts 0]
  set tgames [lindex $counts 1]

  set r {}
  append r $::optable::_docStart($fmt)
  set line [::trans [sc_report opening line]]
  if {$line == ""} {
    # No starting Line so Report title is just a title
    set line $::tr(OprepTitle)
    append r [::optable::_title $line]
    if {$fmt == "latex"} {
      append r "\\newchessgame%\n"
    }
  } else {
    # There is a starting Line
    if {$fmt == "latex"} {
      append r "\\newchessgame%\n"
      append r "\\hidemoves{$line}%\n"
      append r [::optable::_title "\\printchessgame"]
    } else {
      append r [::optable::_title $line]
    }
  }

  if {$fmt == "latex"} {
    append r "\\begin{tabularx}{1\\textwidth}{rX} \n"
  } else {
    append r "$preText"
  }
  if {$fmt == "latex"} {
    if {$flipPos} {
      append r "$multicolumnstart \\chessboard\[inverse\] $multicolumnend $n"
    } else {
      append r "$multicolumnstart \\chessboard $multicolumnend $n"
    }
  } elseif {$fmt == "html"} {
    if {$flipPos} {
      append r $::optable::_data(bdHTML_flip)
    } else {
      append r $::optable::_data(bdHTML)
    }
  } elseif {$fmt == "ctext"} {
    append r "\n<center><window .oprepWin.text.bd></center>\n"
  }

  set baseName [file tail [sc_base filename]]
  if {$fmt == "latex"} {
    # A latex f-me - underscores throw errors
    set baseName [string map {_ -} $baseName]
  }
  append r "$n$tr(Database):$next $baseName "

  append r "([::utils::thousands [sc_base numGames]] $games)$n"
  if {$fmt == "latex"} {
     append r "$tr(OprepReport):$next {\\printchessgame} ("
  } else {
     append r "$tr(OprepReport): $line ("
  }
  if {$fmt == "ctext"} {
    append r "<darkblue><run sc_report opening select all 0; ::windows::stats::Refresh>"
  }
  append r [::utils::thousands $rgames]
  if {$fmt == "ctext"} { append r "</run></darkblue>"; }
  append r " $games)$n"
  set eco [sc_report opening eco]
  if {$eco != ""} {
    append r "$tr(ECO):$next $eco$n"
  }

  append r "$::tr(OprepGenerated):$next $::scidName [sc_info version], [::utils::date::today]$n"

  append r "$postText"
  if {$rgames == 0} {
    append r $::optable::_docEnd($fmt)
    return $r
  }

  if {$::optable(Stats) > 0  ||
    $::optable(Oldest) > 0  ||
    $::optable(Newest) > 0  ||
    $::optable(Popular) > 0  ||
    ($::optable(MostFrequent) > 0 &&
    ($::optable(MostFrequentWhite) || $::optable(MostFrequentBlack)))} {
    append r [::optable::_sec $tr(OprepStatsHist)]
    if {$tgames > $::optable(MaxGames)} {
      append r "$n[format $tr(OprepTableComment) $::optable(MaxGames)]$n"
    } else {
      if {$tgames > $::optable::OPTABLE_MAX_LINES} {
	append r "$n[format $tr(OprepTableComment) $::optable::OPTABLE_MAX_LINES]$n"
      }
    }
  }
  if {$::optable(Stats)} {
    append r [::optable::_subsec $tr(OprepStats)]
    append r [::optable::stats $fmt]
  }
  if {$::optable(Oldest) > 0} {
    append r [::optable::_subsec $tr(OprepOldest)]
    append r "$preText[sc_report opening best o $::optable(Oldest)]$postText"
  }
  if {$::optable(Newest) > 0} {
    append r [::optable::_subsec $tr(OprepNewest)]
    append r "$preText[sc_report opening best n $::optable(Newest)]$postText"
  }

  if {$::optable(Popular) > 0} {
    append r [::optable::_subsec $tr(OprepPopular)]
    set next ""
    if {$fmt == "latex"} { set next " & " }

    # A table showing popularity by year ranges:
    if {$fmt == "latex"} {
      append r "\\begin{tabularx}{1\\textwidth}{Xcccccccc}\n\\hline\n"
    } else {
      append r $preText
    }

    set sYear $tr(Year)
    set sEvery [::utils::string::Capital $tr(OprepEvery)]
    regsub "%u" $sEvery X sEvery
    set len [string length $sYear]
    if {[string length $sEvery] > $len} { set len [string length $sEvery] }
    append r [::utils::string::Pad $tr(Year) $len]
    foreach range {1800-99 1900-49 1950-69 1970-79 1980-89 1990-99 2000-09 2010-19 2020-29} {
      append r $next
      append r [::utils::string::PadCenter $range 8]
    }

    append r $n
    append r [::utils::string::Pad $sEvery $len]
    foreach y {1800 1900 1950 1970 1980 1990 2000 2010 2020} {
      append r $next
      append r [::utils::string::PadCenter $::optable::_data(range$y) 8]
    }
    append r $n
    if {$fmt == "latex"} {
      append r "\\hline\n\\end{tabularx}\n"
    } else {
      append r $postText
    }

    append r "$n"

    # A table showing popularity in the last 1/5/10 years:
    if {$fmt == "latex"} {
      append r "\\begin{tabularx}{0.9\\textwidth}{rlX}\n"
    }

    foreach y {All 10 5 1} {
      if {$fmt == "ctext"} { append r "<tt>" }
      append r $tr(OprepFreq$y)
      if {$fmt == "ctext"} { append r "</tt>" }
      append r $next
      append r [format $tr(OprepEvery) $::optable::_data(ratio$y)]
      if {$y != "All"} {
        append r $next
        set d $::optable::_data(delta$y)
        if {$d > 0} {
          append r " ([format $tr(OprepUp) $d $percent])"
        } elseif {$d < 0} {
          append r " ([format $tr(OprepDown) [expr 0- $d] $percent])"
        } else {
          append r " ($tr(OprepSame))"
        }
      }
      append r "$n"
    }
    if {$fmt == "latex"} {
      append r "\\end{tabularx}\n"
    }
  }

  if {$::optable(MostFrequent) > 0  &&  $::optable(MostFrequentWhite)} {
    append r [::optable::_subsec "$tr(OprepMostFrequent) ($tr(White))"]
    append r [sc_report opening players white $::optable(MostFrequent)]
  }
  if {$::optable(MostFrequent) > 0  &&  $::optable(MostFrequentBlack)} {
    append r [::optable::_subsec "$tr(OprepMostFrequent) ($tr(Black))"]
    append r [sc_report opening players black $::optable(MostFrequent)]
  }

  if {$::optable(AvgPerf)  ||  $::optable(HighRating)} {
    append r [::optable::_sec $tr(OprepRatingsPerf)]
  }
  if {$::optable(AvgPerf)} {
    append r [::optable::_subsec $tr(OprepAvgPerf)]
    set e [sc_report opening elo white]
    set welo [lindex $e 0]; set wng [lindex $e 1]
    set bpct [lindex $e 2]; set bperf [lindex $e 3]
    set e [sc_report opening elo black]
    set belo [lindex $e 0]; set bng [lindex $e 1]
    set wpct [lindex $e 2]; set wperf [lindex $e 3]
    append r $preText
    append r "$tr(OprepWRating): $welo ($wng $games); $next"
    append r "$tr(OprepWPerf): $wperf ($wpct$percent vs $belo)$n"
    append r "$tr(OprepBRating): $belo ($bng $games); $next"
    append r "$tr(OprepBPerf): $bperf ($bpct$percent vs $welo)$n"
    append r $postText
  }

  if {$::optable(HighRating) > 0} {
    append r [::optable::_subsec $tr(OprepHighRating)]
    append r "$preText[sc_report opening best a $::optable(HighRating)]$postText"
  }

  if {$::optable(Results)  ||
    ($::optable(Shortest) > 0  &&
    ($::optable(ShortestBlack) || $::optable(ShortestBlack)))} {
    append r [::optable::_sec $tr(OprepTrends)]
  }

  if {$::optable(Results)} {
    append r [::optable::_subsec $::tr(OprepResults)]
    append r [::optable::results opening $fmt]
  }

  if {$::optable(Shortest) > 0  &&  $::optable(ShortestWhite)} {
    append r [::optable::_subsec "$tr(OprepShortest) ($tr(White))"]
    append r "$preText[sc_report opening best w $::optable(Shortest)]$postText"
  }
  if {$::optable(Shortest) > 0  &&  $::optable(ShortestBlack)} {
    append r [::optable::_subsec "$tr(OprepShortest) ($tr(Black))"]
    append r "$preText[sc_report opening best b $::optable(Shortest)]$postText"
  }

  if {$::optable(MoveOrders) > 0  ||
    $::optable(MovesFrom) > 0  ||
    $::optable(Themes) > 0  ||
    $::optable(Endgames) > 0} {
    append r [::optable::_sec $tr(OprepMovesThemes)]
  }
  if {$::optable(MoveOrders) > 0} {
    append r [::optable::_subsec $tr(OprepMoveOrders)]
    set nOrders [sc_report opening moveOrders 0]
    set maxOrders $::optable(MoveOrders)
    if {$fmt == "latex"} {
      append r "\\begin{center}\n"
    }
    if {$nOrders == 1} {
      append r $tr(OprepMoveOrdersOne)
    } elseif {$nOrders <= $maxOrders} {
      append r [format $tr(OprepMoveOrdersAll) $nOrders]
    } else {
      append r [format $tr(OprepMoveOrdersMany) $nOrders $maxOrders]
    }
    if {$fmt == "latex"} {
      append r "\n\\end{center}\n"
      append r "\\begin{tabularx}{0.9\\textwidth}{rlX} \\hline \n"
      append r [sc_report opening moveOrders $maxOrders]
      append r "\\end{tabularx}\n"
    } else {
      append r "\n"
      append r [::trans [sc_report opening moveOrders $maxOrders]]
    }
  }
  if {$::optable(MovesFrom)} {
    append r [::optable::_subsec $tr(OprepMovesFrom)]
    if {$fmt == "latex"} {
      append r $::optable::_data(latexTree)
    } else {
      append r $preText
      append r $::optable::_data(tree)
      append r $postText
    }
  }

  if {$::optable(Themes) > 0} {
    append r [::optable::_subsec $tr(OprepThemes)]
    append r [sc_report opening themes \
	$tr(OprepThemeDescription) \
        $tr(OprepThemeSameCastling) \
	$tr(OprepThemeOppCastling) \
	$tr(OprepThemeQueenswap) \
        $tr(OprepTheme1BishopPair) \
        $tr(OprepThemeKPawnStorm) \
        $tr(OprepThemeWIQP) \
	$tr(OprepThemeBIQP) \
        $tr(OprepThemeWP567) \
	$tr(OprepThemeBP234) \
        $tr(OprepThemeOpenCDE) ]
  }

  if {$::optable(Endgames) > 0} {
    append r [::optable::_subsec $tr(OprepEndgames)]
    if {$fmt == "latex"} {
      append r "\\begin{center}\n"
    }
    append r "$tr(OprepEndClass):$n"
    if {$fmt == "latex"} {
      append r "\\end{center}"
    }
    append r [sc_report opening endmat]
  }

  if {$withTable  &&  $::optable(MaxTableGames) > 0} {
    set sec [::optable::_sec $tr(OprepTheoryTable)]
    set comment ""
    if {$tgames > $::optable(MaxTableGames)} {
      set comment [format $tr(OprepTableComment) $::optable(MaxTableGames)]
    } else {
      if {$tgames > $::optable::OPTABLE_MAX_TABLE_LINES} {
        append r "$n[format $tr(OprepTableComment) $::optable::OPTABLE_MAX_TABLE_LINES]$n"
      }
    }
    append r [sc_report opening print $numRows $sec $comment]
  }
  append r $::optable::_docEnd($fmt)

  # Eszet (ss) characters seem to be mishandled by LaTeX, even with
  # the font encoding package, so convert them explicitly:
  if {$fmt == "latex"} { regsub -all ß $r {{\\ss}} r }

  return $r
}

###   Produces only the ECO table, not any other part of the report.

proc ::optable::table {fmt} {
  sc_report opening format $fmt
  set ::optable::_data(fmt) $fmt
  set r {}
  append r $::optable::_docStart($fmt)
  set r [string map [list "\[OprepTitle\]" $::tr(OprepTitle)] $r]
  append r [sc_report opening print]
  append r $::optable::_docEnd($fmt)
  return $r
}


set reportFavorites {}

#   Update the Favorites menu in the report window, adding a
#   command for each favorite report position.

proc ::optable::updateFavoritesMenu {} {
  set m .oprepWin.menu.favorites
  $m delete 3 end
  $m add separator
  foreach entry $::reportFavorites {
    set name [lindex $entry 0]
    set moves [lindex $entry 1]
    $m add command -label $name -command "
      importMoveList [list $moves]
      ::optable::makeReportWin
    "
  }
  if {[llength $::reportFavorites] == 0} {
    $m entryconfigure 1 -state disabled
    $m entryconfigure 2 -state disabled
  } else {
    $m entryconfigure 1 -state normal
    $m entryconfigure 2 -state normal
  }
}

### Return a list of the favorite report names.

proc ::optable::favoriteReportNames {} {
  set reportNames {}
  foreach entry $::reportFavorites {
    lappend reportNames [lindex $entry 0]
  }
  return $reportNames
}

### Add the current position to the opening report favorites list

proc ::optable::addFavoriteDlg {} {
  set w .addFavoriteDlg

  if {[winfo exists $w]} {
    raiseWin $w
    return
  }
  toplevel $w
  wm withdraw $w

  wm title $w "Add Opening Report Favorite"
  label $w.name -text "Enter a name for this position"
  pack $w.name -side top
  entry $w.e -width 40
  pack $w.e -side top -fill x -padx 2
  addHorizontalRule $w
  label $w.old -text "Existing favorite report names"
  pack $w.old -side top
  pack [frame $w.existing] -side top -fill x -padx 2
  text $w.existing.list -width 30 -height 10 -yscrollcommand [list $w.existing.ybar set]
  scrollbar $w.existing.ybar -command [list $w.existing.list yview]
  pack $w.existing.ybar -side right -fill y
  pack $w.existing.list -side left -fill both -expand yes
  foreach entry $::reportFavorites {
    $w.existing.list insert end "[lindex $entry 0]\n"
  }
  $w.existing.list configure -state disabled
  addHorizontalRule $w
  frame $w.b
  pack $w.b -side bottom -fill x
  dialogbutton $w.b.ok -text OK -command ::optable::addFavoriteOK
  dialogbutton $w.b.cancel -text $::tr(Cancel) -command "destroy $w"
  pack $w.b.cancel $w.b.ok -side right -padx 5 -pady 5
  focus $w.e

  bind $w <Escape> "$w.b.cancel invoke"
  bind $w <F1> {helpWindow Reports Favorites}
  update
  placeWinOverParent $w .oprepWin
  wm deiconify $w
}

proc ::optable::addFavoriteOK {} {
  global reportFavorites

  set w .addFavoriteDlg
  set reportName [$w.e get]
  set err ""
  if {$reportName == ""} {
    set err "The report name must not be empty."
  } elseif {[lsearch -exact [::optable::favoriteReportNames] $reportName] >= 0} {
    set err "That name is already used for another favorite report position."
  } else {
    lappend reportFavorites [list $reportName [sc_game moves]]
    ::optable::saveFavorites
    ::optable::updateFavoritesMenu
    grab release $w
    destroy $w
    return
  }
  tk_messageBox -title Scid -icon info -type ok -message $err
}

set reportFavoritesName ""

### Edit the list of opening report favorite positions

proc ::optable::editFavoritesDlg {} {
  global reportFavorites reportFavoritesTemp reportFavoritesName tr
  set w .editFavoritesDlg

  if {[winfo exists $w]} {
    raiseWin $w
    return
  }
  toplevel $w
  wm withdraw $w
  wm title $w "[tr OprepFavoritesEdit]"

  set ::reportFavoritesTemp $::reportFavorites
  # wm transient $w .
  entry $w.e -width 60 -textvariable reportFavoritesName -exportselection 0

  trace variable reportFavoritesName w ::optable::editFavoritesRefresh
  pack [frame $w.b] -side bottom -fill x
  pack $w.e -side bottom -fill x
  frame $w.f
  pack $w.f -side top -fill both -expand yes

  listbox $w.f.list -width 50 -height 10 -exportselection 0 \
    -setgrid 1 -yscrollcommand "$w.f.ybar set"
  scrollbar $w.f.ybar -takefocus 0 -command "$w.f.list yview"

  pack $w.f.list -side left -fill both -expand yes
  pack $w.f.ybar -side right -fill y

  bind $w.f.list <<ListboxSelect>>  ::optable::editFavoritesSelect
  foreach entry $::reportFavoritesTemp {
    set name [lindex $entry 0]
    set moves [lindex $entry 1]
    $w.f.list insert end "$name \[$moves\]"
  }
  button $w.b.delete -textvar tr(Delete) -command ::optable::editFavoritesDelete
  button $w.b.up -image bookmark_up -command {::optable::editFavoritesMove up}
  button $w.b.down -image bookmark_down -command {::optable::editFavoritesMove down}
  foreach i [list $w.b.up $w.b.down] {
    $i configure -padx 0 -pady 0 -borderwidth 1
  }
  button $w.b.ok -text "OK" -command ::optable::editFavoritesOK
  button $w.b.cancel -textvar tr(Cancel) -command "destroy $w"
  pack $w.b.delete $w.b.up $w.b.down -side left -padx 2 -pady 2
  pack $w.b.cancel $w.b.ok -side right -padx 2 -pady 2
  set editFavoritesName ""

  bind $w <Escape> "$w.b.cancel invoke"
  bind $w <F1> {helpWindow Reports Favorites}

  update
  placeWinOverParent $w .oprepWin
  wm deiconify $w
}

proc ::optable::editFavoritesRefresh {args} {
  global reportFavoritesTemp reportFavoritesName
  set list .editFavoritesDlg.f.list
  set sel [lindex [$list curselection] 0]
  if {$sel == ""} { return }
  set name $reportFavoritesName
  set e [lindex $reportFavoritesTemp $sel]
  set moves [lindex $e 1]
  set e [lreplace $e 0 0 $name]
  set reportFavoritesTemp [lreplace $reportFavoritesTemp $sel $sel $e]
  $list insert $sel "$name \[$moves\]"
  $list delete [expr $sel + 1]
  $list selection clear 0 end
  $list selection set $sel
}

proc ::optable::editFavoritesSelect {} {
  set list .editFavoritesDlg.f.list
  set sel [lindex [$list curselection] 0]
  if {$sel == ""} {
    set ::reportFavoritesName ""
    return
  }
  if {$sel >= [llength $::reportFavoritesTemp]} {
    $list selection clear 0 end
    set ::reportFavoritesName ""
    return
  }
  set e [lindex $::reportFavoritesTemp $sel]
  set ::reportFavoritesName [lindex $e 0]
}

proc ::optable::editFavoritesDelete {} {
  global reportFavoritesTemp

  set list .editFavoritesDlg.f.list
  set sel [lindex [$list curselection] 0]
  if {$sel == ""} { return }
  set reportFavoritesTemp [lreplace $reportFavoritesTemp $sel $sel]
  $list selection clear 0 end
  $list delete $sel
  set ::reportFavoritesName ""

}

proc ::optable::editFavoritesMove {dir} {
  global reportFavoritesTemp

  set list .editFavoritesDlg.f.list
  set sel [lindex [$list curselection] 0]
  if {$sel == ""} { return }
  set e [lindex $reportFavoritesTemp $sel]
  set name [lindex $e 0]
  set moves [lindex $e 1]
  set text "$name \[$moves\]"

  set newsel $sel
  if {$dir == "up"} {
    incr newsel -1
    if {$newsel < 0} { return }
  } else {
    incr newsel
    if {$newsel >= [$list index end]} { return }
  }
  set reportFavoritesTemp [lreplace $reportFavoritesTemp $sel $sel]
  set reportFavoritesTemp [linsert $reportFavoritesTemp $newsel $e]
  $list selection clear 0 end
  $list delete $sel
  $list insert $newsel $text
  $list selection set $newsel
}

proc ::optable::editFavoritesOK {} {
  destroy .editFavoritesDlg
  set ::reportFavorites $::reportFavoritesTemp
  ::optable::saveFavorites
  ::optable::updateFavoritesMenu
}

proc ::optable::favoritesFilename {} {
  return [scidConfigFile reports]
}

proc ::optable::saveFavorites {} {
  set fname [::optable::favoritesFilename]
  if {[catch {open $fname w} f]} {
    # tk_messageBox ...
    return
  }
  puts $f "# $::scidName opening report favorites file\n"
  puts $f "set reportFavorites [list $::reportFavorites]"
  close $f
}

proc ::optable::loadFavorites {} {
  global reportFavorites
  set fname [::optable::favoritesFilename]
  catch {source $fname}
}

::optable::loadFavorites

set reportFormat html
set reportType full

proc ::optable::generateFavoriteReports {} {
  global reportFavorites
  if {[llength $reportFavorites] == 0} {
    tk_messageBox -title Scid -type ok -icon info \
        -message "You have no favorite report positions."
    return
  }
  set ::reportDir $::initialDir(report)

  set w .reportFavoritesDlg
  if {[winfo exists $w]} {
    raiseWin $w
    return
  }
  toplevel $w
  wm withdraw $w
  wm title $w "Generate Reports..."

  pack [label $w.typelabel -text "Select the report type:" -font font_Bold] -side top
  pack [frame $w.type] -side top -padx 2
  radiobutton $w.type.full -text "Full" -variable reportType -value full
  radiobutton $w.type.compact -text "Compact (no theory table)" -variable reportType -value compact
  radiobutton $w.type.theory -text "Theory table only" -variable reportType -value theory
  pack $w.type.full $w.type.compact $w.type.theory -side left -padx 4
  addHorizontalRule $w
  pack [label $w.fmtlabel -text "Select the report file format:" -font font_Bold] -side top
  pack [frame $w.fmt] -side top -padx 2
  radiobutton $w.fmt.text -text "Text" -variable reportFormat -value text
  radiobutton $w.fmt.html -text "HTML" -variable reportFormat -value html
  radiobutton $w.fmt.latex -text "LaTeX" -variable reportFormat -value latex
  pack $w.fmt.text $w.fmt.html $w.fmt.latex -side left -padx 4
  addHorizontalRule $w
  pack [frame $w.dir] -side top -padx 2 -pady 2
  label $w.dir.label -text "Save reports in the folder" -font font_Bold
  entry $w.dir.entry  -textvariable ::reportDir
  dialogbutton $w.dir.choose -text $::tr(Browse) -command {
    set tmpdir [tk_chooseDirectory -parent .reportFavoritesDlg \
        -title "Scid: Choose Report Folder"]
    if {$tmpdir != ""} {
      set ::reportDir [file nativename $tmpdir]
    }
  }
  pack $w.dir.label -side left
  pack $w.dir.choose -side right
  pack $w.dir.entry -side left -fill x -padx 5
  addHorizontalRule $w
  pack [frame $w.b] -side bottom -fill x
  dialogbutton $w.b.ok -text OK -command "
    ::optable::reportFavoritesOK
    grab release $w
    destroy $w
    ::optable::makeReportWin
  "
  dialogbutton $w.b.cancel -text $::tr(Cancel) -command "grab release $w; destroy $w"
  pack $w.b.cancel $w.b.ok -side right -padx 5 -pady 5

  bind $w <Escape> "$w.b.cancel invoke"
  bind $w <F1> {helpWindow Reports Favorites}

  update
  placeWinOverParent $w .oprepWin
  wm deiconify $w
}

proc ::optable::reportFavoritesOK {} {
  global reportDir reportFormat reportType
  set ::initialDir(report) $reportDir
  set fmt $reportFormat
  switch $reportFormat {
    "html" { set suffix ".html" }
    "text" { set suffix ".txt" }
    "latex" { set suffix ".tex" }
  }

  set w .reportsProgress
  toplevel $w
  wm withdraw $w
  wm title $w "Generating Reports"
  pack [label $w.t -width 40 -text "Generating reports. Please wait..." -font font_Bold] -side top -pady 5
  pack [label $w.report] -side top -pady 5
  placeWinCenter $w
  wm deiconify $w
  grab $w
  update

  set count 0
  set total [llength $::reportFavorites]
  foreach entry $::reportFavorites {
    set name [lindex $entry 0]
    set moves [lindex $entry 1]
    set fname [file join $reportDir "$name$suffix"]
    if {[catch {open $fname w} f]} {
      tk_messageBox -title Scid -icon warning -type ok \
          -message "Unable to write file: $fname\n$f"
      grab release $w
      destroy $w
      return
    }
    incr count
    $w.report configure -text "$count / $total: $name$suffix"
    update
    sc_game push
    sc_move addSan $moves
    ::optable::makeReportWin 0 0
    if {$reportType == "theory"} {
      set report [::optable::table $fmt]
    } elseif {$reportType == "compact"} {
      set report [::optable::report $fmt 0 $::optable::_flip]
    } else {
      set report [::optable::report $fmt 1 $::optable::_flip]
    }
    # if {$::hasEncoding  &&  $::langEncoding($::language) != ""} { catch {set report [encoding convertto $::langEncoding($::language) $report]} }
    sc_game pop
    puts $f $report
    close $f
  }
  grab release $w
  destroy $w
}

# end of optable.tcl
